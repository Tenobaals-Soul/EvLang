#include"scanner.h"
#include<stdlib.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdio.h>
#include<memory.h>

struct jumpmap_handler_args {
    StringDict* class_content;
    StackedData* stacked_data;
    TokenList tokens;
    unsigned int index;
};

struct jump_map {
    struct parser_node_s* next;
    struct parser_node_s* call_substate;
    bool (*on_found)(struct jumpmap_handler_args* args);
};

typedef struct parser_node_s {
    struct jump_map default_jump;
    struct jump_map jump_mapping[64];
    bool (*on_end)(struct jumpmap_handler_args* args, bool is_root);
} parser_node_t;

#define is_null(item) (memcmp(&item, (char[sizeof(item)]) {0}, sizeof(item)) == 0)

int exit_substate_flag = 0;
int valid_flag = 0;
bool pexit_substate(struct jumpmap_handler_args* args) {
    (void) args;
    exit_substate_flag = 1;
    return true;
}

void cexit_substate() {
    exit_substate_flag = 1;
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
    entry->class.implements = NULL;
    entry->class.implements = 0;
    entry->is_static = false;
    return entry;
}

bool flush_stacked_data(struct jumpmap_handler_args* args) {
    if (args->stacked_data != NULL) {
        string_dict_put(args->class_content, (args->stacked_data)->name, args->stacked_data);
    }
    args->stacked_data = NULL;
    return true;
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

bool throw_expected_def(struct jumpmap_handler_args* args) {
    Token* token_with_error = args->tokens.tokens + args->index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a definiton of a class or function");
    return false;
}

bool throw_expected_name(struct jumpmap_handler_args* args) {
    Token* token_with_error = args->tokens.tokens + args->index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a name here");
    return false;
}

bool throw_unexpected_token(struct jumpmap_handler_args* args) {
    Token* token_with_error = args->tokens.tokens + args->index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "unexpected token");
    return false;
}

bool throw_expected_class(struct jumpmap_handler_args* args) {
    Token* token_with_error = args->tokens.tokens + args->index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a class here");
    return false;
}

bool throw_expected_class_or_method(struct jumpmap_handler_args* args) {
    Token* token_with_error = args->tokens.tokens + args->index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a \"class\" keyword or a type for a method or a variale");
    return false;
}

bool throw_perhaps_missing_assign(struct jumpmap_handler_args* args) {
    Token* token_with_error = args->tokens.tokens + args->index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a \"=\", \"(\" or a \";\", you are propably missing a \"=\" between your variable and your value");
    valid_flag = 0;
    return false;
}

bool assert_assign_operator_throw_expected_end_open(struct jumpmap_handler_args* args) {
    Token* assert_over = args->tokens.tokens + args->index;
    if (assert_over->operator_type == ASSIGN_OPERATOR) {
        return true;
    }
    make_error(assert_over->line_content, assert_over->line_in_file,
        assert_over->char_in_line, assert_over->text_len,
        "expected a \"=\", \"(\" or a \";\"");
    return false;
}


bool forgot_semicolon_err(struct jumpmap_handler_args* args) {
    Token* token_with_error = args->tokens.tokens + args->index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "missing \";\"");
    --args->index;
    return false;
}

bool forgot_semicolon_err_and_exit_substate(struct jumpmap_handler_args* args) {
    Token* token_with_error = args->tokens.tokens + args->index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "missing \";\"");
    --args->index;
    cexit_substate();
    return false;
}

bool set_accessibility_handler(struct jumpmap_handler_args* args) {
    StackedData* temp = args->stacked_data ? args->stacked_data : (args->stacked_data = empty_base_entry());
    temp->accessability = get_accessability(args->tokens, args->index);
    return true;
}

bool set_type_class_handler(struct jumpmap_handler_args* args) {
    if (args->stacked_data == NULL) args->stacked_data = empty_base_entry();
    args->stacked_data->class.class_content = malloc(sizeof(StringDict));
    string_dict_init(args->stacked_data->class.class_content);
    args->stacked_data->type = ENTRY_CLASS;
    return true;
}

bool set_name_handler(struct jumpmap_handler_args* args) {
    if (args->stacked_data == NULL) args->stacked_data = empty_base_entry();
    args->stacked_data->name = args->tokens.tokens[args->index].identifier;
    return true;
}

bool set_derive_from(struct jumpmap_handler_args* args) {
    if (args->stacked_data == NULL) args->stacked_data = empty_base_entry();
    args->stacked_data->class.derives_from = args->tokens.tokens[args->index].identifier;
    return true;
}

bool set_static_handler(struct jumpmap_handler_args* args) {
    if (args->stacked_data == NULL) args->stacked_data = empty_base_entry();
    (args->stacked_data)->is_static = true;
    return true;
}

bool set_type_handler(struct jumpmap_handler_args* args) {
    if (args->stacked_data == NULL) args->stacked_data = empty_base_entry();
    args->stacked_data->var_type = args->tokens.tokens[args->index].identifier;
    args->stacked_data->method.return_type = args->tokens.tokens[args->index].identifier;
    return true;
}

bool init_class_content_substate(struct jumpmap_handler_args* args) {
    StringDict* new_class_content = args->stacked_data->class.class_content;
    flush_stacked_data(args);
    args->class_content = new_class_content;
    return true;
}

bool append_implement_from(struct jumpmap_handler_args* args) {
    StackedData* temp = args->stacked_data ? args->stacked_data : (args->stacked_data = empty_base_entry());
    temp->class.implements_len++;
    temp->class.implements = realloc(temp->class.implements, sizeof(char*) * temp->class.implements_len);
    temp->class.implements[temp->class.implements_len] = args->tokens.tokens[args->index].identifier;
    return true;
}

bool accept_and_return_true(struct jumpmap_handler_args* args, bool is_root) {
    (void) args;
    return is_root;
}

// general stuff
parser_node_t method_block;

// parse class
parser_node_t
    found_class_keyword, get_class_found_name, get_class_found_derive,
    get_class_after_derive, get_class_found_implements,
    get_class_after_implement, get_class_found_seperator, get_class_content;

parser_node_t found_accessibility, found_static, found_type, found_name, found_function_args_start,
    found_function_arg_type, found_function_arg_name, found_function_arg_seperator,
    found_function_arg_def_end, found_text_start, found_var_operator, skip_to_end_of_statement;

parser_node_t found_accessibility = {
    .default_jump = { NULL, NULL, throw_unexpected_token },
    .jump_mapping[K_CLASS_TOKEN] = { &found_class_keyword, NULL, set_type_class_handler },
    .jump_mapping[K_STATIC_TOKEN] = { &found_static, NULL, set_static_handler },
    .jump_mapping[IDENTIFIER_TOKEN] = { &found_type, NULL, set_name_handler },
    .on_end = NULL
};

parser_node_t found_class_keyword = {
    .default_jump = { NULL, NULL, throw_expected_name },
    .jump_mapping[IDENTIFIER_TOKEN] = {&get_class_found_name, NULL, set_name_handler },
    .on_end = NULL
};

parser_node_t get_class_found_name = {
    .default_jump = { NULL, NULL, throw_unexpected_token },
    .jump_mapping[K_DERIVES_TOKEN] = { &get_class_found_derive, NULL, NULL },
    .jump_mapping[K_IMPLEMENTS_TOKEN] = { &get_class_found_implements, NULL, NULL },
    .jump_mapping[OPEN_BLOCK_TOKEN] = { &get_class_content, &get_class_content, init_class_content_substate },
    .on_end = NULL
};

parser_node_t get_class_found_derive = {
    .default_jump = { NULL, NULL, throw_expected_class },
    .jump_mapping[IDENTIFIER_TOKEN] = { &get_class_after_derive, NULL, set_derive_from },
    .on_end = NULL
};

parser_node_t get_class_after_derive = {
    .default_jump = { NULL, NULL, throw_unexpected_token },
    .jump_mapping[K_IMPLEMENTS_TOKEN] = { &get_class_found_implements, NULL, NULL },
    .jump_mapping[OPEN_BLOCK_TOKEN] = { NULL, &get_class_content, init_class_content_substate },
    .on_end = NULL
};

parser_node_t get_class_found_implements = {
    .default_jump = { NULL, NULL, throw_expected_class },
    .jump_mapping[IDENTIFIER_TOKEN] = {&get_class_after_implement, NULL, append_implement_from },
    .on_end = NULL
};

parser_node_t get_class_after_implement = {
    .default_jump = { NULL, NULL, throw_unexpected_token },
    .jump_mapping[SEPERATOR_TOKEN] = { &get_class_found_implements, NULL, NULL },
    .jump_mapping[OPEN_BLOCK_TOKEN] = { NULL, &get_class_content, init_class_content_substate },
    .on_end = NULL
};

parser_node_t get_file_content = {
    .default_jump = { NULL, NULL, throw_expected_def },
    .jump_mapping[K_PUBLIC_TOKEN] = { &found_accessibility, NULL, set_accessibility_handler },
    .jump_mapping[K_PROTECTED_TOKEN] = { &found_accessibility, NULL, set_accessibility_handler },
    .jump_mapping[K_PRIVATE_TOKEN] = { &found_accessibility, NULL, set_accessibility_handler },

    .jump_mapping[K_STATIC_TOKEN] = { &found_static, NULL, set_static_handler },
    .jump_mapping[K_CLASS_TOKEN] = { &found_class_keyword, NULL, set_type_class_handler },
    .jump_mapping[IDENTIFIER_TOKEN] = { &found_type, NULL, set_type_handler },
    .on_end = NULL
};

parser_node_t get_class_content = {
    .default_jump = { NULL, NULL, throw_expected_def },
    .jump_mapping[K_PUBLIC_TOKEN] = { &found_accessibility, NULL, set_accessibility_handler },
    .jump_mapping[K_PROTECTED_TOKEN] = { &found_accessibility, NULL, set_accessibility_handler },
    .jump_mapping[K_PRIVATE_TOKEN] = { &found_accessibility, NULL, set_accessibility_handler },

    .jump_mapping[K_STATIC_TOKEN] = { &found_static, NULL, set_static_handler },
    .jump_mapping[K_CLASS_TOKEN] = { &found_class_keyword, NULL, set_type_class_handler },
    .jump_mapping[IDENTIFIER_TOKEN] = { &found_type, NULL, set_type_handler },

    .jump_mapping[CLOSE_BLOCK_TOKEN] = { NULL, NULL, pexit_substate },
    .on_end = accept_and_return_true
};

parser_node_t found_static = {
    .default_jump = { NULL, NULL, throw_expected_class_or_method },
    .jump_mapping[K_CLASS_TOKEN] = { &found_class_keyword, NULL, set_type_class_handler },
    .jump_mapping[IDENTIFIER_TOKEN] = { &found_type, NULL, set_type_handler },
    .on_end = NULL
};

parser_node_t found_type = {
    .default_jump = { NULL, NULL, throw_expected_name },
    .jump_mapping[IDENTIFIER_TOKEN] = { &found_name, NULL, set_name_handler },
    .on_end = NULL
};

parser_node_t found_name = {
    .default_jump = { NULL, NULL, throw_unexpected_token },
    .jump_mapping[END_TOKEN] = { &get_class_content, NULL, flush_stacked_data },
    .jump_mapping[OPEN_PARANTHESIS_TOKEN] = { &found_function_args_start, NULL, NULL },
    .jump_mapping[OPERATOR_TOKEN] = { &skip_to_end_of_statement, NULL, assert_assign_operator_throw_expected_end_open },
    .jump_mapping[FIXED_VALUE_TOKEN] = { &skip_to_end_of_statement, NULL, throw_perhaps_missing_assign },
    .on_end = NULL
};

parser_node_t skip_to_end_of_statement = {
    .default_jump = { &skip_to_end_of_statement, NULL, NULL },
    .jump_mapping[END_TOKEN] = { &get_class_content, NULL, flush_stacked_data },

    .jump_mapping[K_PUBLIC_TOKEN] = { NULL, NULL, forgot_semicolon_err },
    .jump_mapping[K_PROTECTED_TOKEN] = { NULL, NULL, forgot_semicolon_err },
    .jump_mapping[K_PRIVATE_TOKEN] = { NULL, NULL, forgot_semicolon_err },

    .jump_mapping[K_STATIC_TOKEN] = { NULL, NULL, forgot_semicolon_err },
    .jump_mapping[K_CLASS_TOKEN] = { NULL, NULL, forgot_semicolon_err },

    .jump_mapping[CLOSE_BLOCK_TOKEN] = { NULL, NULL,  forgot_semicolon_err_and_exit_substate },
    .on_end = NULL
};

parser_node_t method_block = {
    .default_jump = { &method_block, NULL, NULL },
    .jump_mapping[OPEN_BLOCK_TOKEN] = { &method_block, &method_block, NULL },
    .jump_mapping[CLOSE_BLOCK_TOKEN] = { NULL, NULL, pexit_substate },
    .on_end = NULL
};

parser_node_t* scan_class(Token* token) {

}

bool scan_file_internal(parser_node_t* current_node, struct jumpmap_handler_args* args, bool is_root) {
    int substate_flag = 0;
    while (args->index < args->tokens.cursor) {
        struct jump_map* jump_to = &current_node->jump_mapping[args->tokens.tokens[args->index].type];
        if (is_null(*jump_to)) {
            jump_to = &current_node->default_jump;
        }
        StringDict* out = args->class_content;
        if (jump_to->on_found) {
            if (!jump_to->on_found(args)) {
                if (args->stacked_data) free(args->stacked_data);
                return false;
            }
            substate_flag = exit_substate_flag;
            exit_substate_flag = 0;
        }
        ++args->index;
        if (jump_to->call_substate) {
            struct jumpmap_handler_args n_args = *args;
            if (!scan_file_internal(jump_to->call_substate, &n_args, false)) {
                return false;
            }
            args->class_content = out;
            args->index = n_args.index;
        }
        if (substate_flag) {
            return true;
        }
        current_node = jump_to->next ? jump_to->next : current_node;
    }
    if (current_node->on_end) {
        return current_node->on_end(args, is_root);
    }
    Token e_t = args->tokens.tokens[args->index - 1];
    make_error(e_t.line_content, e_t.line_in_file, e_t.char_in_line + e_t.text_len, 1, 
        "unexpected EOF");
    return false;
}

StringDict* scan_file(TokenList tokens) {
    valid_flag = 1;
    parser_node_t* current_node = &get_file_content;
    StringDict* out = malloc(sizeof(StringDict));
    string_dict_init(out);
    struct jumpmap_handler_args args;
    args.index = 0;
    args.tokens = tokens;
    while (args.index + 1 < tokens.cursor) {
        args.class_content = out;
        args.stacked_data = NULL;
        if (!scan_file_internal(current_node, &args, true)) {
            string_dict_destroy(out);
            free(out);
            return NULL;
        }
    }
    return valid_flag ? out : NULL;
}