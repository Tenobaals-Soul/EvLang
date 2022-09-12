#include<parser.h>
#include<stdlib.h>
#include<stdarg.h>
#include<stack.h>
#include<stdio.h>
#include<compiler.h>
#include<tokenizer.h>
#include<string.h>
#include<stdarg.h>
#include<stack.h>

#define ALLOW_NONE                  (0)
#define ALLOW_ALL                   (~0)
#define ALLOW_VARIALBE_DECLARATION  (1 << 1)
#define ALLOW_FUNCTION_DEFINITION   (1 << 2)
#define ALLOW_CLASS_DEFINITION      (1 << 3)
#define ALLOW_SEMICOLON_EXPRESSION  (1 << 4)
#define ALLOW_STATEMENTS            (1 << 5)
#define ALLOW_CONTROL_STRUCTURES    (1 << 6)

#define SET(x) (1 << (x))

#define IS_POSTFIX_UNARY(operator) (!!((1 << (operator)) & (\
    SET(ADD_OPERATOR) | SET(SUBTRACT_OPERATOR) |\
    SET(BINARY_NOT_OPERATOR) | SET(BOOL_NOT_OPERATOR)\
)))

#define IS_PREFIX_UNARY(operator) (!!((1 << (operator)) & (\
    SET(INCREMENT_OPERATOR) | SET(DECREMENT_OPERATOR)\
)))

struct state parse_internal(TokenList tokens, struct state start_state,
                            int no_throw_tokens, int allowed_states);

struct state_args {
    TokenList tokens;
    int ignore_throw_mask;
    int allowed_states;
};

struct state {
    struct state (*call)(struct state_args, struct state);
    StringDict* scope;
    StructData* struct_data;
    unsigned int token_index;
    Stack paranthesis;
    char* positional_identifier[4];
    union {
        UnresolvedEntry current_entry;
        Method current_method;
        Field current_field;
    };
    bool error;
    bool had_error;
};

void throw(Token* token, const char* format, ...) {
    va_list l;
    va_start(l, format);
    vmake_error(token->line_content, token->line_in_file, token->char_in_line,
                token->text_len, format, l);
    va_end(l);
}

void debug_log_throw(Token* token, const char* format, ...) {
    va_list l;
    va_start(l, format);
    vmake_debug_message(token->line_content, token->line_in_file, token->char_in_line,
                        token->text_len, format, l);
    va_end(l);
}

bool avoid_throw(struct state_args args, struct state state) {
    return !!(args.ignore_throw_mask & SET(args.tokens.tokens[state.token_index].type));
}

struct state scan_start(struct state_args args, struct state state);
struct state scan_found_first_identifier(struct state_args args, struct state state);
struct state scan_found_second_identifier(struct state_args args, struct state state);
struct state scan_expect_operator(struct state_args args, struct state state);
struct state scan_expect_value(struct state_args args, struct state state);

struct state basic_recover(struct state_args args, struct state state) {
    if (debug_run) debug_log_throw(&args.tokens.tokens[state.token_index], "%s", __func__);
    state.error = true;
    if (args.tokens.tokens[state.token_index].type == END_TOKEN ||
        args.tokens.tokens[state.token_index].type == CLOSE_BLOCK_TOKEN) {
        state.call = NULL;
    }
    else {
        state.token_index++;
    }
    return state;
}

static struct state insert_function(struct state_args args, struct state state, Token* token) {
    Method method = mcalloc(sizeof(struct Method));
    method->name = strmcpy(state.positional_identifier[1]);
    method->return_type.unresolved = strmcpy(state.positional_identifier[0]);
    UnresolvedEntry entry = string_dict_get(state.scope, method->name);
    if (entry == NULL) {
        MethodTable methods = mcalloc(sizeof(struct MethodTable));
        methods->meta.entry_type = ENTRY_METHODS;
        methods->meta.line = args.tokens.tokens[state.token_index - 1].line_in_file;
        methods->meta.name = strmcpy(method->name);
        string_dict_put(state.scope, methods->meta.name, methods);
        entry =  &methods->meta;
    }
    else if (entry->entry_type != ENTRY_METHODS) {
        throw(token, "redefinition of \"%s\" in line %u", entry->name, entry->line);
        state.error = true;
        return state;
    }
    MethodTable methods = (MethodTable) entry;
    methods->len++;
    methods->value = mrealloc(methods->value, sizeof(*methods->value) * methods->len);
    methods->value[methods->len - 1] = method;
    return state;
}

struct state scan_expect_operator(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case OPERATOR_TOKEN:
        state.token_index++;
        state.call = IS_PREFIX_UNARY(token->operator_type)
            ? scan_expect_operator
            : scan_expect_value;
        break;
    case CLOSE_PARANTHESIS_TOKEN:
        if (peek_chr(&state.paranthesis) != '(') {
            goto error;
        }
        else {
            pop_chr(&state.paranthesis);
        }
        state.token_index++;
        break;
    case DOT_TOKEN:
        state.call = scan_expect_value;
        state.token_index++;
        break;
    case END_TOKEN:
        if (args.allowed_states & ALLOW_SEMICOLON_EXPRESSION) {
            state.call = scan_start;
            state.token_index++;
        }
        else {
            goto error;
        }
        break;
    case ASSIGN_TOKEN:
        state.token_index++;
        state.call = scan_expect_value;
        break;
    default:
    error:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected an operator");
            state.error = true;
            state.call = basic_recover;
        }
        break;
    }
    return state;
}

struct state scan_found_function_argument(struct state_args args, struct state state);

struct state get_function_arg(struct state_args args, struct state state) {
    struct state n_state = state;
    n_state.call = scan_expect_value;
    n_state.token_index++;
    init_stack(&n_state.paranthesis);
    n_state = parse_internal(args.tokens, n_state, SET(SEPERATOR_TOKEN) |
                                SET(CLOSE_PARANTHESIS_TOKEN), ALLOW_NONE);
    state.token_index = n_state.token_index;
    state.error = n_state.error;
    state.had_error = n_state.had_error;
    state.call = scan_found_function_argument;
    if (state.error) state.call = NULL;
    return state;
}

struct state scan_expect_operator_allow_call(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case OPEN_PARANTHESIS_TOKEN:
        state = get_function_arg(args, state);
        break;
    default:
        state.call = scan_expect_operator;
    }
    return state;
}

struct state scan_found_function_argument(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case SEPERATOR_TOKEN:
        state = get_function_arg(args, state);
        break;
    case CLOSE_PARANTHESIS_TOKEN:
        state.token_index++;
        state.call = scan_expect_operator;
        break;
    default:
        if (avoid_throw(args, state)){
            state.call = NULL;
        }
        else {
            throw(token, "expected an operator");
            state.error = true;
            state.call = basic_recover;
        }
        break;
    }
    return state;
}

struct state scan_found_function_declaration(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case OPEN_BLOCK_TOKEN:
        state.token_index++;
        state.call = scan_start;
        push_chr(&state.paranthesis, '{');
        break;
    case ASSIGN_TOKEN:
        state.token_index++;
        state.call = scan_expect_value;
        break;
    case END_TOKEN:
        state.token_index++;
        state.call = scan_start;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected =, ; of {");
            state.call = basic_recover;
            state.error = NULL;
        }
        break;
    }
    return state;
}

struct state found_function_argument_definition(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    struct state n_state;
    switch (token->type) {
    case SEPERATOR_TOKEN:
        n_state = state;
        n_state.call = scan_start;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(SEPERATOR_TOKEN) | SET(CLOSE_PARANTHESIS_TOKEN),
                                 ALLOW_SEMICOLON_EXPRESSION | ALLOW_VARIALBE_DECLARATION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        state.call = found_function_argument_definition;
        break;
    case CLOSE_PARANTHESIS_TOKEN:
        state.token_index++;
        state.call = scan_found_function_declaration;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "unexpected token");
            state.call = basic_recover;
            state.error = NULL;
        }
        break;
    }
    return state;
}

struct state scan_found_second_identifier(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    struct state n_state;
    switch (token->type) {
    case ASSIGN_TOKEN:
        state.call = scan_expect_value;
        state.token_index++;
        break;
    case END_TOKEN:
        state.call = scan_start;
        state.token_index++;
        break;
    case OPEN_PARANTHESIS_TOKEN:
        n_state = state;
        n_state.call = scan_start;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(SEPERATOR_TOKEN) | SET(CLOSE_PARANTHESIS_TOKEN),
                                 ALLOW_SEMICOLON_EXPRESSION | ALLOW_VARIALBE_DECLARATION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        state.call = found_function_argument_definition;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected an assignment or end of statement");
            state.call = basic_recover;
            state.error = NULL;
        }
        break;
    }
    return state;
}

struct state scan_found_first_identifier(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    if (token->type == IDENTIFIER_TOKEN && (args.allowed_states & ALLOW_VARIALBE_DECLARATION ||
        args.allowed_states & ALLOW_FUNCTION_DEFINITION)) {
        state.positional_identifier[1] = token->identifier;
        state.token_index++;
        state.call = scan_found_second_identifier;
    }
    else {
        state.call = scan_expect_operator_allow_call;
    }
    return state;
}

struct state found_scalar_initialisation_expression(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    struct state n_state;
    switch (token->type) {
    case SEPERATOR_TOKEN:
        n_state = state;
        n_state.call = scan_start;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(SEPERATOR_TOKEN) | SET(CLOSE_BLOCK_TOKEN),
                                 ALLOW_SEMICOLON_EXPRESSION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        state.call = found_scalar_initialisation_expression;
        break;
    case CLOSE_BLOCK_TOKEN:
        state.token_index++;
        state.call = scan_expect_operator;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected , or }");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_expect_value(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    struct state n_state;
    switch (token->type) {
    case OPERATOR_TOKEN:
        state.token_index++;
        if (IS_POSTFIX_UNARY(token->operator_type)) {
            state.call = scan_expect_value;
        }
        else {
            goto error;
        }
        break;
    case FIXED_VALUE_TOKEN:
        state.token_index++;
        state.call = scan_expect_operator;
        break;
    case OPEN_PARANTHESIS_TOKEN:
        state.token_index++;
        push_chr(&state.paranthesis, '(');
        break;
    case IDENTIFIER_TOKEN:
        state.token_index++;
        state.call = scan_expect_operator_allow_call;
        break;
    case OPEN_BLOCK_TOKEN:
        n_state = state;
        n_state.call = scan_start;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(SEPERATOR_TOKEN) | SET(CLOSE_BLOCK_TOKEN),
                                 ALLOW_SEMICOLON_EXPRESSION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        state.call = found_scalar_initialisation_expression;
        break;
    case SEPERATOR_TOKEN:
        // this needs to be done for function arguments
        throw(token, "unexpected token");
        state.call = basic_recover;
        state.error = true;
        break;
    default:
error:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "unexpected token");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}



struct state scan_expect_namespace_begin(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case OPEN_BLOCK_TOKEN:
        struct state n_state = state;
        n_state.call = scan_start;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(CLOSE_BLOCK_TOKEN), ALLOW_ALL);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        state.call = scan_start;
        if (state.error) state.call = NULL;
        else state.token_index++;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected \"{\"");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_expect_namespace_name(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        state.call = scan_expect_namespace_begin;
        state.token_index++;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected a name");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_expect_class_begin(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case OPEN_BLOCK_TOKEN:
        struct state n_state = state;
        n_state.call = scan_start;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(CLOSE_BLOCK_TOKEN), ALLOW_ALL);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        state.call = scan_start;
        if (state.error) state.call = NULL;
        else state.token_index++;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected \"{\"");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_expect_class_name(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        state.call = scan_expect_class_begin;
        state.token_index++;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected a name");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_expect_struct_begin(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case OPEN_BLOCK_TOKEN:
        struct state n_state = state;
        n_state.call = scan_start;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(CLOSE_BLOCK_TOKEN),
                                 ALLOW_VARIALBE_DECLARATION | ALLOW_SEMICOLON_EXPRESSION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        state.call = scan_start;
        if (state.error) state.call = NULL;
        else state.token_index++;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected \"{\"");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_expect_struct_name(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        state.call = scan_expect_struct_begin;
        state.token_index++;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected a name");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}



struct state scan_expect_union_begin(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case OPEN_BLOCK_TOKEN:
        struct state n_state = state;
        n_state.call = scan_start;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(CLOSE_BLOCK_TOKEN),
                                 ALLOW_VARIALBE_DECLARATION | ALLOW_SEMICOLON_EXPRESSION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        state.call = scan_start;
        if (state.error) state.call = NULL;
        else state.token_index++;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected \"{\"");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_expect_union_name(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        state.call = scan_expect_union_begin;
        state.token_index++;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected a name");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_expect_definition_name(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        state.token_index++;
        state.call = scan_found_second_identifier;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expecten a name");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_expect_definition(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case K_NAMESPACE_TOKEN:
        state.call = scan_expect_namespace_name;
        state.token_index++;
        break;
    case K_STRUCT_TOKEN:
        state.call = scan_expect_struct_name;
        state.token_index++;
        break;
    case K_UNION_TOKEN:
        state.call = scan_expect_union_name;
        state.token_index++;
        break;
    case IDENTIFIER_TOKEN:
        state.token_index++;
        state.call = scan_expect_definition_name;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expecten an , or ; after an expression");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_expect_value_or_definition(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    switch (token->type) {
    case K_PRIVATE_TOKEN:
    case K_PROTECTED_TOKEN:
    case K_PUBLIC_TOKEN:
        state.call = scan_expect_definition;
        state.token_index++;
        break;
    default:
        state.call = scan_expect_value; 
    }
    return state;
}

struct state scan_found_second_for_statement(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    struct state n_state;
    switch (token->type) {
    case END_TOKEN:
        n_state = state;
        n_state.call = scan_expect_value;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(OPEN_BLOCK_TOKEN),
                                 ALLOW_VARIALBE_DECLARATION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        if (!state.error) state.call = scan_start;
        break;
    case SEPERATOR_TOKEN:
        n_state = state;
        n_state.call = scan_expect_value;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(END_TOKEN) | SET(SEPERATOR_TOKEN),
                                 ALLOW_VARIALBE_DECLARATION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        if (!state.error) state.call = scan_found_second_for_statement;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expecten an , or ; after an expression");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
        break;
    }
    return state;
}

struct state scan_found_first_for_statement(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    struct state n_state;
    switch (token->type) {
    case END_TOKEN:
        n_state = state;
        n_state.call = scan_expect_value;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(END_TOKEN) | SET(SEPERATOR_TOKEN),
                                 ALLOW_VARIALBE_DECLARATION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        if (!state.error) state.call = scan_found_second_for_statement;
        break;
    case SEPERATOR_TOKEN:
        n_state = state;
        n_state.call = scan_expect_value;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(END_TOKEN) | SET(SEPERATOR_TOKEN),
                                 ALLOW_VARIALBE_DECLARATION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        if (!state.error) state.call = scan_found_first_for_statement;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expecten an , or ; after an expression");
            state.error = true;
            state.call = basic_recover;
            state.token_index++;
        }
    }
    return state;
}

struct state scan_start(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    if (debug_run) debug_log_throw(token, "%s", __func__);
    struct state n_state;
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        state.positional_identifier[0] = token->identifier;
        state.token_index++;
        state.call = scan_found_first_identifier;
        break;
    case OPEN_BLOCK_TOKEN:
        push_chr(&state.paranthesis, '{');
        state.token_index++;
        break;
    case CLOSE_BLOCK_TOKEN:
        if (peek_chr(&state.paranthesis) == '{') {
            pop_chr(&state.paranthesis);
            state.token_index++;
        }
        else goto error;
        break;
    case C_IF_TOKEN:
        if (!(args.allowed_states & ALLOW_CONTROL_STRUCTURES)) goto error;
        n_state = state;
        n_state.call = scan_expect_value;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state,
            SET(OPEN_BLOCK_TOKEN) | SET(END_TOKEN) | SET(IDENTIFIER_TOKEN) | SET(C_IF_TOKEN) |
            SET(C_RETURN_TOKEN) | SET(C_FOR_TOKEN) | SET(C_WHILE_TOKEN), ALLOW_NONE);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        if (!state.error) state.call = scan_start;
        break;
    case C_FOR_TOKEN:
        if (!(args.allowed_states & ALLOW_CONTROL_STRUCTURES)) goto error;
        n_state = state;
        n_state.call = scan_start;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, SET(END_TOKEN) | SET(SEPERATOR_TOKEN),
                                 ALLOW_VARIALBE_DECLARATION);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        if (!state.error) state.call = scan_found_first_for_statement;
        break;
    case C_WHILE_TOKEN:
        if (!(args.allowed_states & ALLOW_CONTROL_STRUCTURES)) goto error;
        n_state = state;
        n_state.call = scan_start;
        n_state.token_index++;
        init_stack(&n_state.paranthesis);
        n_state = parse_internal(args.tokens, n_state, 
            SET(OPEN_BLOCK_TOKEN) | SET(END_TOKEN) | SET(IDENTIFIER_TOKEN) | SET(C_IF_TOKEN) |
            SET(C_RETURN_TOKEN) | SET(C_FOR_TOKEN) | SET(C_WHILE_TOKEN), ALLOW_NONE);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        if (!state.error) state.call = scan_start;
        break;
    case END_TOKEN:
        state.token_index++;
        break;
    case C_RETURN_TOKEN:
        state.token_index++;
        state.call = scan_expect_value;
        break;
    default:
        error:
        state.call = scan_expect_value_or_definition;
    }
    return state;
}

struct state parse_internal(TokenList tokens, struct state start_state,
                            int no_throw_tokens, int allowed_states) {
    struct state state = start_state;
    struct state_args state_args = {
        .ignore_throw_mask = no_throw_tokens,
        .tokens = tokens,
        .allowed_states = allowed_states
    };
    for (;;) {
        if (state.token_index >= tokens.cursor) break;
        if (state.call) state = state.call(state_args, state);
        else return state;
        state.had_error |= state.error;
    }
    return state;
}

Module parse(TokenList tokens) {
    Module module = mcalloc(sizeof(struct Module));
    string_dict_init(&module->module_content);
    struct state state = {
        .call = scan_start,
        .struct_data = &module->globals,
        .scope = &module->module_content
    };
    init_stack(&state.paranthesis);
    while (state.token_index < tokens.cursor) {
        state = parse_internal(tokens, state, ALLOW_NONE, ALLOW_ALL);
        state.call = scan_start;
    }
    if (state.had_error) {
        free_ast(NULL, module);
        return NULL;
    }
    else {
        return module;
    }
}