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

struct state parse_internal(TokenList tokens, struct state start_state,
                            int no_throw_tokens);

#define SET(x) (1 << (x))

struct state_args {
    TokenList tokens;
    int ignore_throw_mask;
};

struct state {
    struct state (*call)(struct state_args, struct state);
    Stack scope_stack;
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

bool is_error(struct state_args args, struct state state) {
    return !(args.ignore_throw_mask | SET(args.tokens.tokens[state.token_index].type)) || !state.error;
}

struct state scan_start(struct state_args args, struct state state);
struct state scan_found_iden(struct state_args args, struct state state);
struct state scan_expect_init_or_method(struct state_args args, struct state state);

struct state basic_recover(struct state_args args, struct state state) {
    if (args.tokens.tokens[state.token_index].type == END_TOKEN) {
        state.call = scan_start;
    }
    state.error = true;
    state.token_index++;
    return state;
}

struct state expect_function_arg(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    switch (token->type) {
    default:
        if (is_error(args, state)) {
            throw(token, "expected an argument or pattern");
        }
        state.token_index++;
        state.error = true;
        break;
    }
    return state;
}

struct state scan_expect_init_or_method(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    char* name = state.positional_identifier[1];
    switch (token->type) {
    case OPEN_PARANTHESIS_TOKEN:;
        StringDict* scope = peek_ptr(&state.scope_stack);
        Method method = calloc(sizeof(struct Method), 1);
        method->name = strmcpy(name);
        method->return_type.unresolved = state.positional_identifier[0];
        UnresolvedEntry entry = string_dict_get(scope, method->name);
        if (entry == NULL) {
            MethodTable methods = calloc(sizeof(struct MethodTable), 1);
            methods->meta.entry_type = ENTRY_METHODS;
            methods->meta.line = args.tokens.tokens[state.token_index - 1].line_in_file;
            methods->meta.name = strmcpy(method->name);
            string_dict_put(scope, methods->meta.name, methods);
            entry =  &methods->meta;
        }
        else if (entry->entry_type != ENTRY_METHODS) {
            throw(token, "redefinition of \"%s\" in line %u", entry->name, entry->line);
            state.error = true;
            break;
        }
        MethodTable methods = (MethodTable) entry;
        methods->len++;
        methods->value = realloc(methods->value, sizeof(*methods->value) * methods->len);
        methods->value[methods->len - 1] = method;
        state.call = expect_function_arg;
        state.token_index++;
        break;
    default:
        if (is_error(args, state)) {
            throw(token, "expected an initialisation of %s", name);
            state.error = true;
            state.call = basic_recover;
        }
        else {
            state.call = scan_start;
        }
        break;
    }
    return state;
}

struct state scan_found_iden(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        state.positional_identifier[1] = token->identifier;
        state.token_index++;
        state.call = scan_expect_init_or_method;
        break;
    default:
        if (is_error(args, state)) {
            throw(token, "expected an assignment or a variable/function definition");
        }
        state.token_index++;
        state.error = true;
        break;
    }
    return state;
}

struct state scan_start(struct state_args args, struct state state) {
    Token* token = &args.tokens.tokens[state.token_index];
    state.error = false;
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        state.positional_identifier[0] = token->identifier;
        state.token_index++;
        state.call = scan_found_iden;
        break;
    default:
        if (is_error(args, state)) {
            throw(token, "unexpected token");
        }
        state.token_index++;
        state.error = true;
        break;
    }
    return state;
}

struct state parse_internal(TokenList tokens, struct state start_state,
                            int no_throw_tokens) {
    struct state state = start_state;
    struct state_args state_args = {
        .ignore_throw_mask = no_throw_tokens,
        .tokens = tokens
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
    struct state default_state = {
        .call = scan_start
    };
    init_stack(&default_state.scope_stack);
    push_ptr(&default_state.scope_stack, &module->module_content);
    struct state state = parse_internal(tokens, default_state, 0);
    if (state.had_error) {
        free_ast(NULL, module);
        return NULL;
    }
    else {
        return module;
    }
}