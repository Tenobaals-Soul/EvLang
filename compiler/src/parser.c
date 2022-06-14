#include<parser.h>
#include<stdlib.h>
#include<stdarg.h>
#include<stack.h>
#include<stdio.h>
#include<compiler.h>

struct parse_args {
    bool has_errors;
    TokenList tokens;
    Stack access_dicts;
    unsigned int index;
};

typedef struct state_data {
    unsigned int index;
    unsigned int flags;
    const char* on_eof;
    unsigned int paranthesis_layer;
    Stack value_stack;
    Stack operator_stack;
    char* accessor;
    TokenList tokens;
    Stack access_dicts;
    unsigned int accessor_start;
    StackedData* st_data;
    Expression* pending_func_call;
    unsigned int end_on_index;
    bool throw;
} state_data;

typedef struct state {
    struct state (*func)(state_data*);
} state;

#define END_FINE (1 << 1)
#define NO_ADVANCE (1 << 2)
#define END_ERROR (1 << 3)

#define transition1(new_state_f) { data->flags = 0; data->on_eof = NULL; return (state) { new_state_f }; }
#define transition2(new_state_f, status) { data->flags = status; data->on_eof = NULL; return (state) { new_state_f }; }
#define transition3(new_state_f, status, on_eof) { data->flags = status; data->on_eof = on_eof; return (state) { new_state_f }; }
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define transition(...) GET_MACRO(__VA_ARGS__, transition3, transition2, transition1)(__VA_ARGS__)

static inline void parse_with_context(StringDict* dict_to_parse, struct parse_args* args);

void free_expression(Expression* exp) {
    switch (exp->expression_type) {
    default:
        break;
    }
    free(exp);
}

static void throw_raw_expected(Token* token_with_error, const char* expected, ...) {
    va_list arglist;

    va_start( arglist, expected );
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "%s", expected, arglist);
    va_end( arglist );
}

static void throw_unexpected_end(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line + token_with_error->text_len - 1, 1,
        "unexpected EOF");
}

static inline Token* get_token(state_data* state) {
    return state->index < state->tokens.cursor ? &state->tokens.tokens[state->index] : NULL;
}

static const int op_prio[] = {
    [ADD_OPERATOR] = 0,
    [SUBTRACT_OPERATOR] = 0,
    [MULTIPLY_OPERATOR] = 1,
    [DIVIDE_OPERATOR] = 1,
    [RIGHT_SHIFT_OPERATOR] = 2,
    [LEFT_SHIFT_OPERATOR] = 2,
    [EQUALS_OPERATOR] = -2,
    [SMALLER_THAN_OPERATOR] = -1,
    [SMALLER_EQUAL_OPERATOR] = -1,
    [GREATER_THAN_OPERATOR] = -1,
    [GREATER_EQUAL_OPERATOR] = -1,
    [NOT_EQUAL_OPERATOR] = -2
};

state parse_exp_start(state_data* data);
state parse_exp_found_open_paranthesis(state_data* data);
state parse_exp_found_value(state_data* data);
state parse_exp_found_ident(state_data* data);
state parse_exp_found_ident_dot(state_data* data);
state parse_exp_arg_ended(state_data* data);

StackedData* find_by_accessor(Stack* access_dicts, const char* accessor, TokenList* tokens, unsigned int token_index) {
    for (int i = access_dicts->count; i; i--) {
        StackedData* found = get_from_ident_dot_seq(access_dicts->data[i - 1], accessor, tokens, token_index, i <= 1);
        if (found) return found;
    }
    return NULL;
}

state on_find_ident(Token* token, state_data* data) {
    if (data->accessor != NULL) {
        if (find_by_accessor(&data->access_dicts, data->accessor, &data->tokens, data->accessor_start) == NULL) {
            while (data->tokens.tokens[data->index].type == DOT_TOKEN ||
                data->tokens.tokens[data->index].type == IDENTIFIER_TOKEN) {
                if (data->index >= data->end_on_index) {
                    transition(NULL, NO_ADVANCE | END_ERROR);
                }
                data->index++;
            }
            Expression* exp = calloc(sizeof(Expression), 1);
            exp->expression_type = EXPRESSION_VAR;
            push(&data->value_stack, exp);
            free(data->accessor);
            data->accessor = NULL;
            transition(parse_exp_found_ident, END_ERROR | NO_ADVANCE);
        }
    }
    if (data->accessor == NULL) data->accessor_start = data->index;
    data->accessor = append_accessor_str(data->accessor, token->identifier);
    transition(parse_exp_found_ident, END_FINE);
}

state parse_exp_start(state_data* data) {
    Token* token = get_token(data);
    Expression* exp;
    switch (token->type) {
    case FIXED_VALUE_TOKEN:
        exp = calloc(sizeof(Expression), 1);
        exp->expression_type = EXPRESSION_FIXED_VALUE;
        exp->fixed_value = token;
        push(&data->value_stack, exp);
        transition(parse_exp_found_value, END_FINE);
    case IDENTIFIER_TOKEN:
        return on_find_ident(token, data);
    case OPEN_PARANTHESIS_TOKEN:
        data->paranthesis_layer++;
        exp = calloc(sizeof(Expression), 1);
        exp->expression_type = EXPRESSION_OPEN_PARANTHESIS_GUARD;
        push(&data->operator_stack, exp);
        transition(parse_exp_start);
    default:
        if (data->throw) {
            throw_raw_expected(token, "expecten an expression");
            transition(NULL, END_ERROR);
        }
        else {
            transition(NULL);
        }
    }
}

state on_found_operator_token(state_data* data, Token* token) {
    Expression* top = NULL;
    while ((top = pop(&data->operator_stack)) && top->expression_type != EXPRESSION_OPEN_PARANTHESIS_GUARD
            && op_prio[top->expression_operator.operator] >= op_prio[token->operator_type]) {
        Expression* val2 = pop(&data->value_stack);
        Expression* val1 = pop(&data->value_stack);
        if (val1 == NULL || val2 == NULL) {
            printf("fatal internal error - parse - %d", __LINE__);
            exit(1);
        }
        top->expression_operator.left = val1;
        top->expression_operator.right = val2;
        push(&data->value_stack, top);
    }
    Expression* operator = calloc(sizeof(Expression), 1);
    operator->expression_type = EXPRESSION_OPERATOR;
    operator->expression_operator.operator = token->operator_type;
    push(&data->operator_stack, operator);
    transition(parse_exp_start);
}

state on_found_closing_paranthesis(state_data* data, Token* token) {
    if (token && data->paranthesis_layer == 0) {
        if (data->throw) {
            throw_raw_expected(token, "no matching opening paranthesis found");
            transition(NULL, END_ERROR);
        }
        else {
            transition(NULL);
        }
    }
    Expression* top = NULL;
    while ((top = pop(&data->operator_stack)) && top->expression_type != EXPRESSION_OPEN_PARANTHESIS_GUARD) {
        Expression* val2 = pop(&data->value_stack);
        Expression* val1 = pop(&data->value_stack);
        if (val1 == NULL || val2 == NULL) {
            printf("fatal internal error - parse - %d", __LINE__);
            exit(1);
        }
        top->expression_operator.left = val1;
        top->expression_operator.right = val2;
        push(&data->value_stack, top);
    }
    if (token) free(top);
    transition(parse_exp_found_value, END_FINE);
}

Expression* parse_expression(StackedData* method_or_var, struct parse_args* args, bool throw);

state on_function_exec(state_data* data) {
    if (data->st_data == NULL) {
        throw_raw_expected(&data->tokens.tokens[data->index],
            "unexpected token");
        transition(NULL, END_ERROR);
    }
    Expression* call = calloc(sizeof(Expression), 1);
    call->expression_type = EXPRESSION_CALL;
    call->expression_call.call = find_by_accessor(&data->access_dicts, data->accessor, &data->tokens, data->accessor_start);
    call->expression_call.args = malloc(sizeof(Expression*));
    call->expression_call.arg_count = 0;
    call->expression_call.args[0] = NULL;
    data->pending_func_call = call;
    data->index++;
    struct parse_args new_args = {
        .has_errors = false,
        .tokens = data->tokens,
        .access_dicts = data->access_dicts,
        .index = data->index
    };
    Expression* argument = parse_expression(data->st_data, &new_args, false);
    if (data->tokens.tokens[data->index].type == CLOSE_PARANTHESIS_TOKEN) {
        push(&data->value_stack, data->pending_func_call);
        transition(parse_exp_found_value);
    }
    if (argument == NULL) {
        while (data->index < data->end_on_index) {
            data->index++;
            for (int i = 0; data->pending_func_call->expression_call.args[i]; i++) {
                free(data->pending_func_call->expression_call.args[i]);
            }
            free(data->pending_func_call->expression_call.args);
            free(data->pending_func_call);
            data->pending_func_call = NULL;
            if (data->tokens.tokens[data->index].type == CLOSE_PARANTHESIS_TOKEN) {
                push(&data->value_stack, data->pending_func_call);
                data->pending_func_call = NULL;
                transition(parse_exp_found_value, END_ERROR | END_FINE);
            }
            if (data->tokens.tokens[data->index].type == END_TOKEN) {
                transition(NULL, END_ERROR);
            }
            if (data->tokens.tokens[data->index].type == SEPERATOR_TOKEN) {
                transition(parse_exp_arg_ended, NO_ADVANCE);
            }
        }
        transition(NULL, END_ERROR);
    }
    else if (call->expression_call.args) {
        call->expression_call.args[call->expression_call.arg_count] = argument;
        call->expression_call.args = realloc(call->expression_call.args, call->expression_call.arg_count++);
        call->expression_call.args[call->expression_call.arg_count] = NULL;
    }
    transition(parse_exp_arg_ended);
}

state parse_exp_found_ident(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case OPERATOR_TOKEN:
        Expression* exp = calloc(sizeof(Expression), 1);
        exp->expression_type = EXPRESSION_VAR;
        exp->expression_variable = find_by_accessor(&data->access_dicts, data->accessor, &data->tokens, data->accessor_start);
        push(&data->value_stack, exp);
        free(data->accessor);
        return on_found_operator_token(data, token);
    case CLOSE_PARANTHESIS_TOKEN:
        return on_found_closing_paranthesis(data, token);
    case DOT_TOKEN:
        transition(parse_exp_found_ident_dot);
    case OPEN_PARANTHESIS_TOKEN:
        return on_function_exec(data);
    default:
        if (data->throw) {
            throw_raw_expected(token, "unexpected token");
            transition(NULL, END_ERROR);
        }
        else {
            transition(NULL);
        }
    }
}

state parse_exp_found_ident_dot(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        return on_find_ident(token, data);
    default:
        if (data->throw) {
            throw_raw_expected(token, "expected an identifier");
            transition(NULL, END_ERROR);
        }
        else {
            transition(NULL);
        }
    }
}

state parse_exp_found_value(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case OPERATOR_TOKEN:;
        return on_found_operator_token(data, token);
    case CLOSE_PARANTHESIS_TOKEN:;
        return on_found_closing_paranthesis(data, token);
    default:
        if (data->throw) {
            throw_raw_expected(token, "unexpected token");
            transition(NULL, END_ERROR);
        }
        else {
            transition(NULL);
        }
    }
}

state parse_exp_arg_ended(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case OPERATOR_TOKEN:;
        return on_found_operator_token(data, token);
    case CLOSE_PARANTHESIS_TOKEN:;
        return on_found_closing_paranthesis(data, token);
    default:
        if (data->throw) {
            throw_raw_expected(token, "unexpected token");
            transition(NULL, END_ERROR);
        }
        else {
            transition(NULL);
        }
    }
}

Expression* parse_expression(StackedData* method_or_var, struct parse_args* args, bool throw) {
    state next_state = { parse_exp_start };
    bool error = false;
    state_data data = {
        .throw = throw,
        .access_dicts = args->access_dicts,
        .index = args->index,
        .tokens = method_or_var->env_token,
        .st_data = method_or_var,
        .end_on_index = method_or_var->text_end
    };
    while (next_state.func) {
        if (data.index >= method_or_var->text_end || get_token(&data)->type == END_TOKEN ||
                (get_token(&data)->type == SEPERATOR_TOKEN && data.paranthesis_layer == 0)) {
            if (data.flags & END_FINE) {
                on_found_closing_paranthesis(&data, NULL);
                Expression* to_return = pop(&data.value_stack);
                stack_destroy(&data.value_stack);
                stack_destroy(&data.operator_stack);
                args->index = data.index;
                return to_return;
            }
            else {
                throw_unexpected_end(&args->tokens.tokens[data.index - 1]);
                error = true;
                break;
            }
        }
        next_state = next_state.func(&data);
        if (data.flags & END_ERROR) {
            error = true;
        }
        if (!(data.flags & NO_ADVANCE)) {
            data.index++;
        }
    }
    if (error) {
        data.index--;
        return NULL;
    }
    Expression* to_return = pop(&data.value_stack);
    stack_destroy(&data.value_stack);
    stack_destroy(&data.operator_stack);
    args->index = data.index;
    return to_return;
}

void parse_method(StackedData* method, struct parse_args* args) {
    (void) method;
    (void) args;
    /*
    Token* token;
    while ((token = get_token(data, peek(&data->data_env))) && token->type != END_TOKEN) {
        data->index++;
    }
    */
}

void parse_dict_item(void* env, const char* key, void* val) {
    (void) key;
    struct parse_args* args = env;
    StackedData* st_data = val;
    switch (st_data->type) {
    case ENTRY_CLASS:
        struct parse_args new_args = {
            .has_errors = false,
            .tokens = st_data->env_token
        };
        stack_init(&new_args.access_dicts);
        for (unsigned int i = 0; i < args->access_dicts.count; i++) {
            push(&new_args.access_dicts, args->access_dicts.data[i]);
        }
        push(&new_args.access_dicts, st_data->class.class_content);
        parse_with_context(st_data->class.class_content, &new_args);
        stack_destroy(&new_args.access_dicts);
        args->has_errors |= new_args.has_errors;
        break;
    case ENTRY_ERROR:
        break;
    case ENTRY_METHOD:
        parse_method(st_data, args);
        break;
    case ENTRY_METHOD_TABLE:
        for (unsigned int i = 0; i < st_data->method_table.len; i++) {
            parse_method(st_data->method_table.methods[i], args);
        }
        break;
    case ENTRY_VARIABLE:
        if (st_data->text_start && st_data->text_end) {
            args->index = st_data->text_start;
            st_data->var.exec_text = parse_expression(st_data, args, true);
        }
        else {
            st_data->var.exec_text = NULL;
        }
        break;
    case ENTRY_MODULE:
    case ERROR_TYPE:
        printf("fatal internal error detected - %d", __LINE__);
        break;
    }
}

static void parse_with_context_wrapper(void* env, const char* key, void* val) {
    (void) key;
    StackedData* module = val;
    parse_with_context(module->class.class_content, env);
}

static inline void parse_with_context(StringDict* dict_to_parse, struct parse_args* args) {
    push(&args->access_dicts, dict_to_parse);
    string_dict_complex_foreach(dict_to_parse, parse_dict_item, args);
    pop(&args->access_dicts);
}

bool parse(StringDict* global_access_dict) {
    struct parse_args args = {
        .has_errors = false,
        .tokens = {0},
        .access_dicts = {0}
    };
    stack_init(&args.access_dicts);
    push(&args.access_dicts, global_access_dict);
    string_dict_complex_foreach(global_access_dict, parse_with_context_wrapper, &args);
    stack_destroy(&args.access_dicts);
    return args.has_errors;
}