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

char* strmcpy(const char* src) {
    char* new_str = malloc(strlen(src));
    strcpy(new_str, src);
    return new_str;
}

#define ALLOW_NONE                  (0)
#define ALLOW_ALL                   (~0)
#define ALLOW_VARIALBE_DECLARATION  (1 << 1)
#define ALLOW_FUNCTION_DEFINITION   (1 << 2)
#define ALLOW_CLASS_DEFINITION      (1 << 3)

#define SET(x) (1 << (x))

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

bool avoid_throw(struct state_args args, struct state state) {
    return !!(args.ignore_throw_mask & SET(args.tokens.tokens[state.token_index].type));
}

struct state scan_start(struct state_args args, struct state state);
struct state scan_found_iden(struct state_args args, struct state state);
struct state scan_expect_init_or_method(struct state_args args, struct state state);
struct state scan_expression_expect_operator(struct state_args args, struct state state);
struct state scan_expression_expect_value(struct state_args args, struct state state);

struct state basic_recover(struct state_args args, struct state state) {
    printf("%s\n", __func__);
    state.error = true;
    if (args.tokens.tokens[state.token_index].type == END_TOKEN) {
        state.call = NULL;
    }
    state.token_index++;
    return state;
}

struct state recover_to_expected(struct state_args args, struct state state) {
    printf("%s\n", __func__);
    state.error = true;
    if (args.ignore_throw_mask & SET(args.tokens.tokens[state.token_index].type)) {
        state.call = NULL;
    }
    state.token_index++;
    return state;
}

struct state expect_function_arg(struct state_args args, struct state state) {
    printf("%s\n", __func__);
    Token* token = &args.tokens.tokens[state.token_index];
    switch (token->type) {
    default:
        if (avoid_throw(args, state)) {
            throw(token, "expected an argument or pattern");
        }
        state.token_index++;
        state.error = true;
        break;
    }
    return state;
}

static struct state insert_function(struct state_args args, struct state state, Token* token) {
    Method method = calloc(sizeof(struct Method), 1);
    method->name = strmcpy(state.positional_identifier[1]);
    method->return_type.unresolved = strmcpy(state.positional_identifier[0]);
    UnresolvedEntry entry = string_dict_get(state.scope, method->name);
    if (entry == NULL) {
        MethodTable methods = calloc(sizeof(struct MethodTable), 1);
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
    methods->value = realloc(methods->value, sizeof(*methods->value) * methods->len);
    methods->value[methods->len - 1] = method;
    return state;
}

struct state scan_expression_expect_operator(struct state_args args, struct state state) {
    printf("%s\n", __func__);
    Token* token = &args.tokens.tokens[state.token_index];
    switch (token->type) {
    case OPERATOR_TOKEN:
        state.token_index++;
        state.call = scan_expression_expect_value;
        break;
    default:
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

struct state scan_expression_expect_value(struct state_args args, struct state state) {
    printf("%s\n", __func__);
    Token* token = &args.tokens.tokens[state.token_index];
    switch (token->type) {
    case FIXED_VALUE_TOKEN:
        state.token_index++;
        state.call = scan_expression_expect_operator;
        break;
    case IDENTIFIER_TOKEN:
        state.token_index++;
        state.call = scan_expression_expect_operator;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected an value");
            state.error = true;
            state.call = basic_recover;
        }
        break;
    }
    return state;
}

struct state scan_expression(struct state_args args, struct state state) {
    printf("%s\n", __func__);
    (void) args; (void) state;
    state.call = scan_expression_expect_value;
    return state;
}

struct state scan_expect_init_or_method(struct state_args args, struct state state) {
    printf("%s\n", __func__);
    Token* token = &args.tokens.tokens[state.token_index];
    switch (token->type) {
    case OPEN_PARANTHESIS_TOKEN:
        state = insert_function(args, state, token);
        state.call = expect_function_arg;
        state.token_index++;
        break;
    case END_TOKEN:
        state.call = scan_start;
        state.token_index++;
        break;
    case ASSIGN_TOKEN:;
        struct state n_state = state;
        n_state.call = scan_expression;
        n_state.token_index++;
        n_state = parse_internal(args.tokens, n_state, SET(END_TOKEN), ALLOW_NONE);
        state.token_index = n_state.token_index;
        state.error = n_state.error;
        state.had_error = n_state.had_error;
        state.call = scan_start;
        state.token_index++;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected an initialisation of %s", state.positional_identifier[1]);
            state.error = true;
            state.call = recover_to_expected;
        }
        break;
    }
    return state;
}

struct state scan_found_iden(struct state_args args, struct state state) {
    printf("%s\n", __func__);
    Token* token = &args.tokens.tokens[state.token_index];
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        state.positional_identifier[1] = token->identifier;
        state.token_index++;
        state.call = scan_expect_init_or_method;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "expected an assignment or a variable/function definition");
            state.error = true;
            state.call = basic_recover;
        }
        state.token_index++;
        break;
    }
    return state;
}

struct state scan_start(struct state_args args, struct state state) {
    printf("%s\n", __func__);
    Token* token = &args.tokens.tokens[state.token_index];
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        state.positional_identifier[0] = token->identifier;
        state.token_index++;
        state.call = scan_found_iden;
        break;
    default:
        if (avoid_throw(args, state)) {
            state.call = NULL;
        }
        else {
            throw(token, "unexpected token");
            state.error = true;
            state.call = basic_recover;
        }
        state.token_index++;
        break;
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
    while (1) {
        if (state.token_index >= tokens.cursor) break;
        if (state.call) state = state.call(state_args, state);
        else return state;
        state.had_error |= state.error;
    }
    return state;
}

Module parse(TokenList tokens) {
    Module module = calloc(sizeof(struct Module), 1);
    string_dict_init(&module->module_content);
    struct state state = {
        .call = scan_start,
        .struct_data = &module->globals,
        .scope = &module->module_content
    };
    while (state.token_index < tokens.cursor) {
        state = parse_internal(tokens, state, 0, ALLOW_ALL);
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