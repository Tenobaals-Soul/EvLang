#include<scanner.h>
#include<string_dict.h>
#include<compiler.h>
#include<stdlib.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdio.h>
#include<memory.h>
#include<stdarg.h>
#include<stack.h>

typedef struct state_data {
    StringDict* dest;
    StackedData* st_data;
    TokenList token_list;
    unsigned int index;
    unsigned int flags;
    unsigned int depth;
    const char* on_eof;
    bool on_lowest_level;
} state_data;

typedef struct state {
    struct state (*func)(state_data*);
} state;

static inline Token* get_token(state_data* state) {
    return &state->token_list.tokens[state->index];
}

char* strmcpy(const char* src) {
    int len = strlen(src) + 1;
    char* out = malloc(len + 1);
    strcpy(out, src);
    out[len] = 0;
    return out;
}

Accessability get_accessability(TokenList tokens, unsigned int i) {
    if (tokens.tokens[i].type == K_PRIVATE_TOKEN) {
        return PRIVATE;
    }
    else if (tokens.tokens[i].type == K_PUBLIC_TOKEN) {
        return PUBLIC;
    }
    else if (tokens.tokens[i].type == K_PROTECTED_TOKEN) {
        return PROTECTED;
    }
    else {
        printf("fatal internal error - non matching token types - %d", __LINE__);
        exit(1);
    }
}

StackedData* empty_base_entry(TokenList token_list) {
    StackedData* entry = calloc(sizeof(StackedData), 1);
    entry->env_token = token_list;
    entry->type = ENTRY_VARIABLE;
    entry->accessability = PROTECTED;
    entry->class.derives_from = NULL;
    return entry;
}

bool append_method(StringDict* class_content, Token* pos, StackedData* entry_found, char* name) {
    StackedData* found = string_dict_get(class_content, name);
    StackedData* method_table;
    if (found) {
        if (found->type != ENTRY_METHOD_TABLE) {
            make_error(pos->line_content, pos->line_in_file, 0, ~0,
                "\"%s\" is already defined in line %u.", name, found->line_no);
            return false;
        }
        method_table = found;
    }
    else {
        method_table = malloc(sizeof(StackedData));
        method_table->type = ENTRY_METHOD_TABLE;
        method_table->name = name;
        method_table->accessability = PUBLIC;
        method_table->method_table.len = 0;
        method_table->method_table.methods = NULL;
        string_dict_put(class_content, name, method_table);
    }
    method_table->method_table.len++;
    method_table->method_table.methods = realloc(method_table->method_table.methods, method_table->method_table.len * sizeof(StackedData*));
    method_table->method_table.methods[method_table->method_table.len - 1] = entry_found;
    return true;
}

static void throw_expected_def(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a definiton of a class, function or variable");
}

static void throw_expected_name(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a name here");
}

static void throw_unexpected_token(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "unexpected token");
}

static void throw_perhaps_missing_assign(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a \"=\", \"(\" or a \";\", you are propably missing a \"=\" between your variable and your value");
}

static void throw_unexpected_end(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line + token_with_error->text_len - 1, 1,
        "unexpected EOF");
}

static void throw_raw_expected(Token* token_with_error, const char* expected, ...) {
    va_list arglist;

    va_start( arglist, expected );
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected %s", expected, arglist);
    va_end( arglist );
}

static void throw_redefinition_error(Token* token_with_error, const char* name) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "redefinition of symbol", name);
}

bool flush_stacked_data(state_data* data) {
    if (string_dict_get(data->dest, data->st_data->name)) {
        throw_redefinition_error(data->st_data->causing, data->st_data->name);
        return false;
    }
    string_dict_put(data->dest, data->st_data->name, data->st_data);
    data->st_data = empty_base_entry(data->token_list);
    return true;
}

#define END_FINE (1 << 2)
#define NO_ADVANCE (1 << 2)
#define END_ERROR (1 << 3)

#define transition1(new_state_f) { data->flags = 0; data->on_eof = NULL; return (state) { new_state_f }; }
#define transition2(new_state_f, status) { data->flags = status; data->on_eof = NULL; return (state) { new_state_f }; }
#define transition3(new_state_f, status, on_eof) { data->flags = status; data->on_eof = on_eof; return (state) { new_state_f }; }
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define transition(...) GET_MACRO(__VA_ARGS__, transition3, transition2, transition1)(__VA_ARGS__)

state scan_class(state_data* data);
state scan_class_found_accessability(state_data* data);
state scan_class_found_static(state_data* data);
state scan_class_found_class_keyword(state_data* data);
state scan_class_found_class_name(state_data* data);
state scan_class_found_derive(state_data* data);
state scan_class_found_derive_name(state_data* data);
state scan_class_found_derive_name_dot(state_data* data);
state scan_class_found_implement(state_data* data);
state scan_class_found_implement_name(state_data* data);
state scan_class_found_implement_name_dot(state_data* data);
state scan_class_found_implement_seperator(state_data* data);
state scan_class_found_type(state_data* data);
state scan_class_found_name(state_data* data);
state scan_class_found_type_no_method(state_data* data);
state scan_class_found_name_no_method(state_data* data);
state scan_method_expect_arg_type(state_data* data);
state scan_method_expect_arg_name(state_data* data);
state scan_method_found_arg(state_data* data);
state scan_method_args_ended(state_data* data);
state scan_method_body(state_data* data);
state var_got_assigned(state_data* data);
state assert_end_of_class_token(state_data* data);

state scan_class(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case K_PRIVATE_TOKEN:
        data->st_data->accessability = PRIVATE;
        transition(scan_class_found_accessability);
    case K_PROTECTED_TOKEN:
        data->st_data->accessability = PROTECTED;
        transition(scan_class_found_accessability);
    case K_PUBLIC_TOKEN:
        data->st_data->accessability = PUBLIC;
        transition(scan_class_found_accessability);
    case K_STATIC_TOKEN:
        data->st_data->accessability = PROTECTED;
        data->st_data->is_static = true;
        transition(scan_class_found_static);
    case K_CLASS_TOKEN:
        data->st_data->accessability = PROTECTED;
        data->st_data->is_static = false;
        data->st_data->type = ENTRY_CLASS;
        transition(scan_class_found_class_keyword);
    case IDENTIFIER_TOKEN:
        data->st_data->accessability = PROTECTED;
        data->st_data->is_static = false;
        char* type = strmcpy(token->identifier);
        data->st_data->var.type = type;
        data->st_data->method.return_type = type;
        transition(scan_class_found_type);
    case CLOSE_BLOCK_TOKEN:
        transition(NULL, END_FINE);
    default:
        throw_expected_def(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_accessability(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case K_STATIC_TOKEN:
        data->st_data->is_static = true;
        transition(scan_class_found_static);
    case K_CLASS_TOKEN:
        data->st_data->is_static = false;
        data->st_data->type = ENTRY_CLASS;
        transition(scan_class_found_class_keyword);
    case IDENTIFIER_TOKEN:
        data->st_data->is_static = false;
        data->st_data->var.type = strmcpy(token->identifier);
        data->st_data->method.return_type = strmcpy(token->identifier);
        transition(scan_class_found_type);
    default:
        throw_unexpected_token(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_static(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case K_CLASS_TOKEN:
        data->st_data->type = ENTRY_CLASS;
        transition(scan_class_found_class_keyword);
    case IDENTIFIER_TOKEN:;
        char* type = strmcpy(token->identifier);
        data->st_data->var.type = type;
        data->st_data->method.return_type = type;
        transition(scan_class_found_type);
    default:
        throw_unexpected_token(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_class_keyword(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        data->st_data->is_static = true;
        data->st_data->name = strmcpy(token->identifier);
        data->st_data->causing = token;
        transition(scan_class_found_class_name);
    default:
        throw_expected_name(token);
        data->flags = END_ERROR;
        transition(NULL, END_ERROR);
    }
}

state scan_inner_class(state_data* data) {
    data->index++;
    StringDict* content = scan_content(data->token_list, &data->index, false);
    if (!content) transition(NULL, END_ERROR);
    data->st_data->class.class_content = content;
    flush_stacked_data(data);
    data->index--;
    transition(assert_end_of_class_token);
}

state assert_end_of_class_token(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case CLOSE_BLOCK_TOKEN:
        transition(scan_class, END_FINE);
    default:
        printf("fatal internal error - %d - runtime change of token type", __LINE__);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_class_name(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case K_DERIVES_TOKEN:
        transition(scan_class_found_derive);
    case K_IMPLEMENTS_TOKEN:
        data->st_data->class.derives_from = strmcpy("Object");
        transition(scan_class_found_implement);
    case OPEN_BLOCK_TOKEN:;
        return scan_inner_class(data);
    default:
        throw_unexpected_token(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_derive(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        data->st_data->class.derives_from = strmcpy(token->identifier);
        transition(scan_class_found_derive_name);
    default:
        throw_expected_name(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_derive_name(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case K_IMPLEMENTS_TOKEN:
        transition(scan_class_found_implement);
    case OPEN_BLOCK_TOKEN:
        return scan_inner_class(data);
    case DOT_TOKEN:
        transition(scan_class_found_derive_name_dot)
    default:
        throw_unexpected_token(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_derive_name_dot(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:;
        char** impl = &data->st_data->class.derives_from;
        *impl = append_accessor_str(*impl, token->identifier);
        transition(scan_class_found_implement_name);
    default:
        throw_expected_name(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_implement(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        data->st_data->class.implements = realloc(data->st_data->class.implements, data->st_data->class.implements_len++);
        data->st_data->class.implements[data->st_data->class.implements_len - 1] = strmcpy(token->identifier);
        transition(scan_class_found_implement_name);
    default:
        throw_expected_name(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_implement_name(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case SEPERATOR_TOKEN:
        transition(scan_class_found_implement);
    case OPEN_BLOCK_TOKEN:
        return scan_inner_class(data);
    case DOT_TOKEN:
        transition(scan_class_found_implement_name_dot);
    default:
        throw_unexpected_token(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_implement_name_dot(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:;
        char** impl = &data->st_data->class.implements[data->st_data->class.implements_len - 1];
        *impl = append_accessor_str(*impl, token->identifier);
        transition(scan_class_found_implement_name);
    default:
        throw_expected_name(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_type(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        data->st_data->name = strmcpy(token->identifier);
        data->st_data->causing = token;
        transition(scan_class_found_name);
    default:
        throw_raw_expected(token, "expected an identifier");
        transition(NULL, END_ERROR);
    }
}

state skip(state_data* data) {
    while (true) {
        switch (data->token_list.tokens[data->index].type) {
        case END_TOKEN:
            data->st_data = empty_base_entry(data->token_list);
            transition(scan_class, END_FINE);
        case SEPERATOR_TOKEN:
            transition(scan_class_found_type_no_method);
        case K_PUBLIC_TOKEN:
        case K_PRIVATE_TOKEN:
        case K_PROTECTED_TOKEN:
        case K_STATIC_TOKEN:
        case K_CLASS_TOKEN:
            data->index--;
            data->st_data = empty_base_entry(data->token_list);
            transition(scan_class, END_ERROR);
        default:
            data->index++;
            break;
        }
    }
}

state scan_class_found_name(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case END_TOKEN:
        data->st_data->type = ENTRY_VARIABLE;
        data->st_data->text_start = data->index;
        data->st_data->text_end = data->index;
        flush_stacked_data(data);
        data->st_data = empty_base_entry(data->token_list);
        transition(scan_class, data->on_lowest_level ? END_FINE : 0);
    case ASSIGN_TOKEN:
        data->st_data->type = ENTRY_VARIABLE;
        transition(var_got_assigned);
    case OPEN_PARANTHESIS_TOKEN:
        data->st_data->type = ENTRY_METHOD;
        transition(scan_method_expect_arg_type);
    case SEPERATOR_TOKEN:;
        StackedData* old = data->st_data;
        flush_stacked_data(data);
        *data->st_data = *old;
        data->st_data->var.type = strmcpy(old->var.type);
        transition(scan_class_found_type);
    default:
        throw_perhaps_missing_assign(token);
        return skip(data);
    }
}

state scan_class_found_type_no_method(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        data->st_data->name = strmcpy(token->identifier);
        data->st_data->causing = token;
        transition(scan_class_found_name_no_method);
    default:
        throw_raw_expected(token, "expected an identifier");
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_name_no_method(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case END_TOKEN:
        data->st_data->type = ENTRY_VARIABLE;
        data->st_data->text_start = data->index;
        data->st_data->text_end = data->index;
        flush_stacked_data(data);
        transition(scan_class, data->on_lowest_level ? END_FINE : 0);
    case ASSIGN_TOKEN:
        data->st_data->type = ENTRY_VARIABLE;
        transition(var_got_assigned);
    default:
        throw_perhaps_missing_assign(token);
        transition(NULL, END_ERROR);
    }
}

char inverse_bracket(char bracket) {
    return bracket == '{' ? '}' : bracket == '[' ? ']' : bracket == '(' ? ')' : 0;
}

state var_got_assigned(state_data* data) {
    data->st_data->text_start = data->index;
    Stack type_stack;
    stack_init(&type_stack);
    while (true) {
        if (data->index >= data->token_list.cursor) {
            data->index--;
            data->st_data->text_end = data->index;
            stack_destroy(&type_stack);
            transition(scan_class);
        }
        Token* token = get_token(data);
        char type = (unsigned long) peek(&type_stack);
        switch (token->type) {
        case END_TOKEN:
            data->st_data->text_end = data->index;
            flush_stacked_data(data);
            if (type_stack.count == 0) {
                stack_destroy(&type_stack);
                transition(scan_class, data->on_lowest_level ? END_FINE : 0);
            }
            if (type != '{') {
                throw_raw_expected(token, "you are missing a %c", inverse_bracket(type));
            }
            break;
        case OPEN_BLOCK_TOKEN:
            push(&type_stack, (void*) (unsigned long) '{');
            break;
        case CLOSE_BLOCK_TOKEN:
            if (type != '{') {
                throw_raw_expected(token, "%c expected", inverse_bracket(type));
                stack_destroy(&type_stack);
                return skip(data);
            }
            if (pop(&type_stack) == NULL) {
                throw_raw_expected(token, "unexpected %c", inverse_bracket(type));
                stack_destroy(&type_stack);
                return skip(data);
            }
            break;
        case OPEN_INDEX_TOKEN:
            push(&type_stack, (void*) (unsigned long) '[');
            break;
        case CLOSE_INDEX_TOKEN:
            if (type != '[') {
                throw_raw_expected(token, "%c expected", inverse_bracket(type));
                stack_destroy(&type_stack);
                return skip(data);
            }
            if (pop(&type_stack) == NULL) {
                throw_raw_expected(token, "unexpected %c", inverse_bracket(type));
                stack_destroy(&type_stack);
                return skip(data);
            }
            break;
        case OPEN_PARANTHESIS_TOKEN:
            push(&type_stack, (void*) (unsigned long) '(');
            break;
        case CLOSE_PARANTHESIS_TOKEN:
            if (type != '(') {
                throw_raw_expected(token, "%c expected", inverse_bracket(type));
                stack_destroy(&type_stack);
                return skip(data);
            }
            if (pop(&type_stack) == NULL) {
                throw_raw_expected(token, "unexpected %c", inverse_bracket(type));
                stack_destroy(&type_stack);
                return skip(data);
            }
            break;
        case SEPERATOR_TOKEN:;
            if (type_stack.count == 0) {
                StackedData* old_entry = data->st_data;
                flush_stacked_data(data);
                *data->st_data = *old_entry;
                data->st_data->var.type = strmcpy(old_entry->var.type);
                old_entry->text_end = data->index;
                stack_destroy(&type_stack);
                transition(scan_class_found_type_no_method);
            }
        default:
            break;
        }
        data->index++;
    }
}

state scan_method_expect_arg_type(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case CLOSE_PARANTHESIS_TOKEN:
        transition(scan_method_args_ended);
    case IDENTIFIER_TOKEN:
        data->st_data->method.args = realloc(data->st_data->method.args, sizeof(struct argument_s) * (data->st_data->method.arg_count + 1));
        data->st_data->method.args[data->st_data->method.arg_count].name = NULL;
        data->st_data->method.args[data->st_data->method.arg_count].type = strmcpy(token->identifier);
        transition(scan_method_expect_arg_name);
    default:
        throw_raw_expected(token, "expected a type");
        transition(NULL, END_ERROR);
    }
}

state scan_method_expect_arg_name(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        data->st_data->method.args[data->st_data->method.arg_count++].name = strmcpy(token->identifier);
        transition(scan_method_found_arg);
    default:
        throw_raw_expected(token, "expected an identifier");
        transition(NULL, END_ERROR);
    }
}

state scan_method_found_arg(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case SEPERATOR_TOKEN:
        transition(scan_method_expect_arg_type);
    case CLOSE_PARANTHESIS_TOKEN:
        transition(scan_method_args_ended);
    default:
        throw_raw_expected(token, "expected \"{\"");
        transition(NULL, END_ERROR);
    }
}

state scan_method_args_ended(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case OPEN_BLOCK_TOKEN:
        transition(scan_method_body);
    default:
        throw_raw_expected(token, "expected \"{\"");
        transition(NULL, END_ERROR);
    }
}

state scan_method_body(state_data* data) {
    data->st_data->text_start = data->index;
    Token* token = get_token(data);
    switch (token->type) {
    case OPEN_BLOCK_TOKEN:
        data->depth++;
        transition(scan_method_body);
    case CLOSE_BLOCK_TOKEN:
        if (data->depth > 0) {
            data->depth--;
            transition(scan_method_body);
        }
        else {
            data->depth = 0;
            append_method(data->dest, token, data->st_data, data->st_data->name);
            data->st_data = empty_base_entry(data->token_list);
            data->st_data->text_end = data->index;
            transition(scan_class, data->on_lowest_level ? END_FINE : 0);
        }
    default:
        transition(scan_method_body);
    }
}

StringDict* scan_content(TokenList tokens, unsigned int* index, bool on_lowest_level) {
    state next_state = { scan_class };
    StringDict* dict = malloc(sizeof(StringDict));
    string_dict_init(dict);
    bool error = false;
    state_data data = {
        .dest = dict,
        .st_data = empty_base_entry(tokens),
        .token_list = tokens,
        .index = *index,
        .flags = 0,
        .on_lowest_level = on_lowest_level
    };
    while (next_state.func) {
        if (data.index >= data.token_list.cursor) {
            if (data.flags & END_FINE) {
                *index = data.index;
                free(data.st_data);
                return dict;
            }
            else {
                throw_unexpected_end(&tokens.tokens[data.index - 1]);
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
        *index = data.index;
        free(data.st_data);
        return NULL;
    }
    data.index--;
    *index = data.index;
    free(data.st_data);
    return dict;
} 