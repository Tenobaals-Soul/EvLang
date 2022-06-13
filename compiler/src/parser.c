#include<parser.h>
#include<stdlib.h>
#include<stdarg.h>
#include<stack.h>
#include<stdio.h>

struct parse_args {
    bool has_errors;
    StackedData* context;
    TokenList tokens;
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
} state_data;

typedef struct state {
    struct state (*func)(state_data*);
} state;

#define END_FINE (1 << 2)
#define NO_ADVANCE (1 << 2)
#define END_ERROR (1 << 3)

#define transition1(new_state_f) { data->flags = 0; data->on_eof = NULL; return (state) { new_state_f }; }
#define transition2(new_state_f, status) { data->flags = status; data->on_eof = NULL; return (state) { new_state_f }; }
#define transition3(new_state_f, status, on_eof) { data->flags = status; data->on_eof = on_eof; return (state) { new_state_f }; }
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define transition(...) GET_MACRO(__VA_ARGS__, transition3, transition2, transition1)(__VA_ARGS__)

static inline bool parse_with_context(StringDict* dict_to_parse, struct parse_args* args);

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
        "expected %s", expected, arglist);
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
        exp = calloc(sizeof(Expression), 1);
        exp->expression_type = EXPRESSION_VAR;
        push(&data->value_stack, exp);
        data->accessor = append_accessor_str(data->accessor, token->identifier);
        transition(parse_exp_found_ident, END_FINE);
    case OPEN_PARANTHESIS_TOKEN:
        data->paranthesis_layer++;
        exp = calloc(sizeof(Expression), 1);
        exp->expression_type = EXPRESSION_OPEN_PARANTHESIS_GUARD;
        push(&data->operator_stack, exp);
        transition(parse_exp_start);
    default:
        throw_raw_expected(token, "expecten an expression");
        transition(NULL, END_ERROR);
    }
}

state on_found_operator_token(state_data* data, Token* token) {
    Expression* top = NULL;
    while ((top = peek(&data->operator_stack)) && top->expression_type != EXPRESSION_OPEN_PARANTHESIS_GUARD
            && op_prio[top->expression_operator.operator] >= op_prio[token->operator_type]) {
        pop(&data->operator_stack);
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
        throw_raw_expected(token, "no matching opening paranthesis found");
        transition(NULL, END_ERROR);
    }
    Expression* top = NULL;
    while ((top = peek(&data->operator_stack)) && top->expression_type != EXPRESSION_OPEN_PARANTHESIS_GUARD) {
        pop(&data->operator_stack);
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
    if (token) free(pop(&data->operator_stack));
    transition(parse_exp_found_value, END_FINE);
}

state parse_exp_found_ident(state_data* data) {
    Token* token = get_token(data);
    Expression* exp;
    switch (token->type) {
    case OPERATOR_TOKEN:
    case IDENTIFIER_TOKEN:
        data->accessor = append_accessor_str(data->accessor, token->identifier);
        transition(parse_exp_found_ident, END_FINE);
    case OPEN_PARANTHESIS_TOKEN:
        data->paranthesis_layer++;
        push(&data->operator_stack, &token);
        transition(parse_exp_start);
    default:
        throw_raw_expected(token, "unexpected token");
        transition(NULL, END_ERROR);
    }
}

state parse_exp_found_value(state_data* data) {
    Token* token = get_token(data);
    Expression* val2;
    switch (token->type) {
    case OPERATOR_TOKEN:;
        return on_found_operator_token(data, token);
    case CLOSE_PARANTHESIS_TOKEN:;
        return on_found_closing_paranthesis(data, token);
    default:
        throw_raw_expected(token, "unexpected token");
        transition(NULL, END_ERROR);
    }
}

Expression* parse_expression(StackedData* method_or_var, struct parse_args* args) {
    state next_state = { parse_exp_start };
    bool error = false;
    state_data data = {0};
    data.index = method_or_var->text_start;
    data.tokens = method_or_var->env_token;
    while (next_state.func) {
        if (data.index >= method_or_var->text_end || get_token(&data)->type == END_TOKEN ||
                (get_token(&data)->type == SEPERATOR_TOKEN && data.paranthesis_layer == 0)) {
            if (data.flags & END_FINE) {
                on_found_closing_paranthesis(&data, NULL);
                Expression* to_return = pop(&data.value_stack);
                stack_destroy(&data.value_stack);
                stack_destroy(&data.operator_stack);
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
        data.index++;
    }
    if (error) {
        data.index--;
        return NULL;
    }
    Expression* to_return = pop(&data.value_stack);
    stack_destroy(&data.value_stack);
    stack_destroy(&data.operator_stack);
    return to_return;
}

void parse_method(StackedData* method, struct parse_args* args) {
    /*
    Token* token;
    while ((token = get_token(data, peek(&data->data_env))) && token->type != END_TOKEN) {
        data->index++;
    }
    */
}

void parse_dict_item(void* env, const char* key, void* val) {
    struct parse_args* args = env;
    StackedData* st_data = val;
    switch (st_data->type) {
    case ENTRY_CLASS:
        struct parse_args new_args = {
            st_data,
            false
        };
        parse_with_context(st_data->class.class_content, &new_args);
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
            st_data->var.exec_text = parse_expression(st_data, args);
        }
        else {
            st_data->var.exec_text = NULL;
        }
        break;
    }
}

static void parse_with_context_wrapper(void* env, const char* key, void* val) {
    (void) key;
    parse_with_context(val, env);
}

static inline bool parse_with_context(StringDict* dict_to_parse, struct parse_args* args) {
    string_dict_complex_foreach(dict_to_parse, parse_dict_item, args);
}

bool parse(StringDict* dict_to_parse) {
    struct parse_args args = {
        false,
        NULL
    };
    string_dict_complex_foreach(dict_to_parse, parse_with_context_wrapper, &args);
    return args.has_errors;
}