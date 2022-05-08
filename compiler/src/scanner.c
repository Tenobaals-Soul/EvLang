#include"scanner.h"
#include"string_dict.h"
#include<stdlib.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdio.h>
#include<memory.h>

typedef struct state_data {
    StringDict* dest;
    StackedData* st_data;
    TokenList token_list;
    unsigned int index;
    unsigned int flags;
    unsigned int depth;
} state_data;

typedef struct state {
    struct state (*func)(state_data*);
} state;

Token* get_token(state_data* state) {
    return &state->token_list.tokens[state->index];
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

StackedData* empty_base_entry() {
    StackedData* entry = calloc(sizeof(StackedData), 1);
    entry->type = ENTRY_VARIABLE;
    entry->accessability = PROTECTED;
    entry->class.derives_from = "Object";
    // entry->class.implements = NULL;
    // entry->class.implements = 0;
    // entry->is_static = false;
    return entry;
}

bool append_method(StringDict* class_content, Token* pos, StackedData* entry_found, char* name) {
    StackedData* found = string_dict_get(class_content, name);
    StackedData* method_table;
    if (found) {
        if (found->type != ENTRY_METHOD_TABLE) {
            make_error(pos->line_content, pos->line_in_file, 0, ~0, "\"%s\" is already defined in line %u.", found->line_no);
            return false;
        }
        method_table = found;
    }
    else {
        method_table = malloc(sizeof(StackedData));
        method_table->type = ENTRY_METHOD_TABLE;
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

bool throw_expected_def(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a definiton of a class, function or variable");
    return false;
}

bool throw_expected_name(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a name here");
    return false;
}

bool throw_unexpected_token(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "unexpected token");
    return false;
}

bool throw_expected_class(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a class here");
    return false;
}

bool throw_expected_class_or_method(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a \"class\" keyword or a type for a method or a variale");
    return false;
}

bool throw_perhaps_missing_assign(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a \"=\", \"(\" or a \";\", you are propably missing a \"=\" between your variable and your value");
    return false;
}

bool throw_no_semicolon(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "missing \";\"");
    return false;
}

bool throw_unexpected_end(Token* token_with_error) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line + 1, 1,
        "unexpected EOF");
    return false;
}

bool throw_raw_expected(Token* token_with_error, const char* expected) {
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected %s", expected);
    return false;
}

bool assert_assign_operator_throw_expected_end_open(Token* assert_over) {
    if (assert_over->operator_type == ASSIGN_OPERATOR) {
        return true;
    }
    make_error(assert_over->line_content, assert_over->line_in_file,
        assert_over->char_in_line, assert_over->text_len,
        "expected a \"=\", \"(\" or a \";\"");
    return false;
}

#define END_FINE (1 << 2)
#define NO_ADVANCE (1 << 2)
#define END_ERROR (1 << 3)

#define transition(new_state_f, status) { data->flags = status; return (state) { new_state_f }; }

state scan_class(state_data* data);
state scan_class_found_accessability(state_data* data);
state scan_class_found_static(state_data* data);
state scan_class_found_class_keyword(state_data* data);
state scan_class_found_class_name(state_data* data);
state scan_class_found_derive(state_data* data);
state scan_class_found_derive_name(state_data* data);
state scan_class_found_implement(state_data* data);
state scan_class_found_implement_name(state_data* data);
state scan_class_found_implement_seperator(state_data* data);
state scan_class_found_type(state_data* data);
state scan_class_found_name(state_data* data);
state scan_method_expect_arg_type(state_data* data);
state scan_method_expect_arg_name(state_data* data);
state scan_method_found_arg(state_data* data);
state scan_method_args_ended(state_data* data);
state scan_method_body(state_data* data);

state scan_class(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case K_PRIVATE_TOKEN:
        data->st_data->accessability = PRIVATE;
        transition(scan_class_found_accessability, 0);
    case K_PROTECTED_TOKEN:
        data->st_data->accessability = PROTECTED;
        transition(scan_class_found_accessability, 0);
    case K_PUBLIC_TOKEN:
        data->st_data->accessability = PUBLIC;
        transition(scan_class_found_accessability, 0);
    case K_STATIC_TOKEN:
        data->st_data->accessability = PROTECTED;
        data->st_data->is_static = true;
        transition(scan_class_found_static, 0);
    case K_CLASS_TOKEN:
        data->st_data->accessability = PROTECTED;
        data->st_data->is_static = false;
        data->st_data->type = ENTRY_CLASS;
        transition(scan_class_found_class_keyword, 0);
    case IDENTIFIER_TOKEN:
        data->st_data->accessability = PROTECTED;
        data->st_data->is_static = false;
        data->st_data->var_type = token->identifier;
        data->st_data->method.return_type = token->identifier;
        transition(scan_class_found_type, 0);
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
        transition(scan_class_found_static, 0);
    case K_CLASS_TOKEN:
        data->st_data->is_static = false;
        data->st_data->type = ENTRY_CLASS;
        transition(scan_class_found_class_keyword, 0);
    case IDENTIFIER_TOKEN:
        data->st_data->is_static = false;
        data->st_data->var_type = token->identifier;
        data->st_data->method.return_type = token->identifier;
        transition(scan_class_found_type, 0);
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
        transition(scan_class_found_class_keyword, 0);
    case IDENTIFIER_TOKEN:
        data->st_data->var_type = token->identifier;
        data->st_data->method.return_type = token->identifier;
        transition(scan_class_found_type, 0);
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
        data->st_data->name = token->identifier;
        transition(scan_class_found_class_name, 0);
    default:
        throw_expected_name(token);
        data->flags = END_ERROR;
        transition(NULL, END_ERROR);
    }
}

state scan_inner_class(state_data* data) {
    data->index++;
    StringDict* content = scan_content(data->token_list, &data->index);
    if (!content) transition(NULL, END_ERROR);
    data->st_data->class.class_content = content;
    string_dict_put(data->dest, data->st_data->name, data->st_data);
    data->st_data = empty_base_entry();
    transition(scan_class, END_FINE);
}

state scan_class_found_class_name(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case K_DERIVES_TOKEN:
        transition(scan_class_found_derive, 0);
    case K_IMPLEMENTS_TOKEN:
        data->st_data->class.derives_from = NULL;
        transition(scan_class_found_implement, 0);
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
        data->st_data->class.derives_from = token->identifier;
        transition(scan_class_found_derive_name, 0);
    default:
        throw_expected_name(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_derive_name(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case K_IMPLEMENTS_TOKEN:
        data->st_data->class.derives_from = NULL;
        transition(scan_class_found_implement, 0);
    case OPEN_BLOCK_TOKEN:
        return scan_inner_class(data);
    default:
        throw_unexpected_token(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_implement(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        data->st_data->class.derives_from = token->identifier;
        transition(scan_class_found_implement_name, 0);
    default:
        throw_expected_name(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_implement_name(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case SEPERATOR_TOKEN:
        data->st_data->class.derives_from = NULL;
        transition(scan_class_found_implement, 0);
    case OPEN_BLOCK_TOKEN:
        return scan_inner_class(data);
    default:
        throw_unexpected_token(token);
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_type(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        data->st_data->name = token->identifier;
        transition(scan_class_found_name, 0);
    default:
        throw_raw_expected(token, "an identifier");
        transition(NULL, END_ERROR);
    }
}

state scan_class_found_name(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case END_TOKEN:
        data->st_data->type = ENTRY_VARIABLE;
        string_dict_put(data->dest, data->st_data->name, data->st_data);
        transition(scan_class, 0);
    case OPERATOR_TOKEN:
        if (token->operator_type != ASSIGN_OPERATOR) {
            throw_perhaps_missing_assign(token);
            transition(NULL, END_ERROR);
        }
        data->st_data->type = ENTRY_VARIABLE;
        transition(scan_class_found_name, 0);
    case OPEN_PARANTHESIS_TOKEN:
        data->st_data->type = ENTRY_METHOD;
        transition(scan_method_expect_arg_type, 0);
    default:
        throw_perhaps_missing_assign(token);
        transition(NULL, END_ERROR);
    }
}

state scan_method_expect_arg_type(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case CLOSE_PARANTHESIS_TOKEN:
        transition(scan_class, 0);
    case IDENTIFIER_TOKEN:
        data->st_data->method.args = realloc(data->st_data->method.args, sizeof(struct argument_s) * (data->st_data->method.arg_count + 1));
        data->st_data->method.args[data->st_data->method.arg_count].name = NULL;
        data->st_data->method.args[data->st_data->method.arg_count].type = token->identifier;
        transition(scan_method_expect_arg_name, 0);
    default:
        throw_raw_expected(token, "a type");
        transition(NULL, END_ERROR);
    }
}

state scan_method_expect_arg_name(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case IDENTIFIER_TOKEN:
        data->st_data->method.args[data->st_data->method.arg_count++].name = token->identifier;
        transition(scan_method_found_arg, 0);
    default:
        throw_raw_expected(token, "an identifier");
        transition(NULL, END_ERROR);
    }
}

state scan_method_found_arg(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case SEPERATOR_TOKEN:
        transition(scan_method_expect_arg_type, 0);
    case CLOSE_PARANTHESIS_TOKEN:
        transition(scan_method_args_ended, 0);
    default:
        throw_raw_expected(token, "\"{\"");
        transition(NULL, END_ERROR);
    }
}

state scan_method_args_ended(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case OPEN_BLOCK_TOKEN:
        transition(scan_method_body, 0);
    default:
        throw_raw_expected(token, "\"{\"");
        transition(NULL, END_ERROR);
    }
}

state scan_method_body(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case OPEN_BLOCK_TOKEN:
        data->depth++;
        transition(scan_method_body, 0);
    case CLOSE_BLOCK_TOKEN:
        if (data->depth > 0) {
            data->depth--;
            transition(scan_method_body, 0);
        }
        else {
            data->depth = 0;
            append_method(data->dest, token, data->st_data, data->st_data->name);
            transition(scan_class, 0);
        }
    default:
        transition(scan_method_body, 0);
    }
}

StringDict* scan_content(TokenList tokens, unsigned int* index) {
    state next_state = { scan_class };
    StringDict* dict = malloc(sizeof(StringDict));
    string_dict_init(dict);
    state_data data = {
        .dest = dict,
        .st_data = empty_base_entry(),
        .token_list = tokens,
        .index = *index,
        .flags = 0
    };
    while (next_state.func) {
        next_state = next_state.func(&data);
        if (data.flags & END_ERROR) {
            return NULL;
        }
        data.index++;
        if (data.index >= data.token_list.cursor) {
            data.index--;
            if (data.flags & END_FINE) {
                *index = data.index;
                return dict;
            }
            else {
                throw_unexpected_end(&tokens.tokens[data.index]);
                return NULL;
            }
        }
    }
    data.index--;
    *index = data.index;
    return dict;
}