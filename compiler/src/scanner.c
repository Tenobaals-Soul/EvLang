#include"scanner.h"
#include<stdlib.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdio.h>
#include<memory.h>

#define JUMPMAP_HANDLER_ARGS StringDict* class_contnent, StackedData** stacked_data, TokenList tokens, unsigned int* index

struct jump_map {
    struct parse_tree_s* what_if;
    struct parse_tree_s* after_substate;
    bool (*on_found)(JUMPMAP_HANDLER_ARGS);
};

typedef struct parse_tree_s {
    struct jump_map default_jump;
    struct jump_map jump_mapping[64];
} parse_tree_t;

#define is_null(item) (memcmp(&item, (char[sizeof(item)]) {0}, sizeof(item)) == 0)

bool exit_substate(JUMPMAP_HANDLER_ARGS) {
    (void) class_contnent;
    (void) stacked_data;
    (void) tokens;
    (void) index;
    printf("this function should not be called");
    exit(1);
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

bool throw_no_class_def(JUMPMAP_HANDLER_ARGS) {
    (void) stacked_data; (void) class_contnent;
    Token* token_with_error = tokens.tokens + *index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "files can only contain meta information or classes or wrap this file");
    return false;
}

bool throw_expected_name(JUMPMAP_HANDLER_ARGS) {
    (void) stacked_data; (void) class_contnent;
    Token* token_with_error = tokens.tokens + *index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a name here");
    return false;
}

bool throw_unexpected_token(JUMPMAP_HANDLER_ARGS) {
    (void) stacked_data; (void) class_contnent;
    Token* token_with_error = tokens.tokens + *index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "unexpected token");
    return false;
}

bool throw_expected_class(JUMPMAP_HANDLER_ARGS) {
    (void) stacked_data; (void) class_contnent;
    Token* token_with_error = tokens.tokens + *index;
    make_error(token_with_error->line_content, token_with_error->line_in_file,
        token_with_error->char_in_line, token_with_error->text_len,
        "expected a class here");
    return false;
}

bool set_accessibility_handler(JUMPMAP_HANDLER_ARGS) {
    (void) class_contnent;
    StackedData* temp = *stacked_data;
    temp->accessability = get_accessability(tokens, *index);
    return true;
}

bool set_type_class_handler(JUMPMAP_HANDLER_ARGS) {
    (void) class_contnent; (void) index; (void) tokens;
    StackedData* temp = *stacked_data;
    temp->class.class_content = malloc(sizeof(StringDict));
    string_dict_init(temp->class.class_content);
    temp->type = ENTRY_CLASS;
    return true;
}

bool set_name_handler(JUMPMAP_HANDLER_ARGS) {
    (void) class_contnent; (void) index; (void) tokens;
    StackedData* temp = *stacked_data;
    temp->name = tokens.tokens[*index].identifier;
    return true;
}

bool set_derive_from(JUMPMAP_HANDLER_ARGS) {
    (void) class_contnent; (void) index; (void) tokens;
    StackedData* temp = *stacked_data;
    temp->class.derives_from = tokens.tokens[*index].identifier;
    return true;
}

bool append_implement_from(JUMPMAP_HANDLER_ARGS) {
    (void) class_contnent; (void) index; (void) tokens;
    StackedData* temp = *stacked_data;
    temp->class.implements_len++;
    temp->class.implements = realloc(temp->class.implements, sizeof(char*) * temp->class.implements_len);
    temp->class.implements[temp->class.implements_len] = tokens.tokens[*index].identifier;
    return true;
}

// general stuff
parse_tree_t inner_block;

// parse class
parse_tree_t on_enter_file, get_class_found_accessibility,
    get_class_found_keyword, get_class_found_name, get_class_found_derive,
    get_class_after_derive, get_class_found_implements,
    get_class_after_implement, get_class_found_seperator, get_class_block_start;

parse_tree_t on_enter_file = {
    .default_jump = { NULL, NULL, throw_no_class_def },
    .jump_mapping = {{0}},
    .jump_mapping[K_PUBLIC_TOKEN] = { &get_class_found_accessibility, NULL, set_accessibility_handler },
    .jump_mapping[K_PROTECTED_TOKEN] = { &get_class_found_accessibility, NULL, set_accessibility_handler },
    .jump_mapping[K_PRIVATE_TOKEN] = { &get_class_found_accessibility, NULL, set_accessibility_handler },
    .jump_mapping[K_CLASS_TOKEN] = { &get_class_found_keyword, NULL, set_type_class_handler }
};

parse_tree_t get_class_found_accessibility = {
    .default_jump = { NULL, NULL, throw_unexpected_token },
    .jump_mapping = {{0}},
    .jump_mapping[K_CLASS_TOKEN] = { &get_class_found_keyword, NULL, set_type_class_handler }
};

parse_tree_t get_class_found_keyword = {
    .default_jump = { NULL, NULL, throw_expected_name },
    .jump_mapping = {{0}},
    .jump_mapping[IDENTIFIER_TOKEN] = {&get_class_found_name, NULL, set_name_handler }
};

parse_tree_t get_class_found_name = {
    .default_jump = { NULL, NULL, throw_unexpected_token },
    .jump_mapping = {{0}},
    .jump_mapping[K_DERIVES_TOKEN] = { &get_class_found_derive, NULL, NULL },
    .jump_mapping[K_IMPLEMENTS_TOKEN] = { &get_class_found_implements, NULL, NULL },
    .jump_mapping[OPEN_BLOCK_TOKEN] = { &get_class_block_start, NULL, NULL }
};

parse_tree_t get_class_found_derive = {
    .default_jump = { NULL, NULL, throw_expected_class },
    .jump_mapping = {{0}},
    .jump_mapping[IDENTIFIER_TOKEN] = { &get_class_after_derive, NULL, set_derive_from }
};

parse_tree_t get_class_after_derive = {
    .default_jump = { NULL, NULL, throw_unexpected_token },
    .jump_mapping = {{0}},
    .jump_mapping[K_IMPLEMENTS_TOKEN] = { &get_class_found_implements, NULL, NULL },
    .jump_mapping[OPEN_BLOCK_TOKEN] = { &get_class_block_start, NULL, NULL }
};

parse_tree_t get_class_found_implements = {
    .default_jump = { NULL, NULL, throw_expected_class },
    .jump_mapping = {{0}},
    .jump_mapping[IDENTIFIER_TOKEN] = {&get_class_after_implement, NULL, append_implement_from }
};

parse_tree_t get_class_after_implement = {
    .default_jump = { NULL, NULL, throw_unexpected_token },
    .jump_mapping = {{0}},
    .jump_mapping[SEPERATOR_TOKEN] = { &get_class_found_implements, NULL, NULL },
    .jump_mapping[OPEN_BLOCK_TOKEN] = { &get_class_block_start, NULL, NULL }
};

parse_tree_t get_class_block_start = {
    .default_jump = { &get_class_block_start, NULL, NULL },
    .jump_mapping = {{0}},
    // here variables, methods and inner classes
    .jump_mapping[CLOSE_BLOCK_TOKEN] = { NULL, NULL, exit_substate }
};

parse_tree_t inner_block = {
    .default_jump = { &inner_block, NULL, NULL },
    .jump_mapping = {{0}},
    .jump_mapping[OPEN_BLOCK_TOKEN] = { &inner_block, &inner_block, NULL },
    .jump_mapping[CLOSE_BLOCK_TOKEN] = { NULL, NULL, exit_substate }
};

StackedData* empty_base_class_entry() {
    StackedData* entry = calloc(sizeof(StackedData), 1);
    entry->accessability = PROTECTED;
    entry->class.derives_from = "Object";
    entry->class.implements = NULL;
    entry->class.implements = 0;
    entry->is_static = false;
    return entry;
}

bool scan_file_internal(StringDict* class_content, TokenList tokens,
    parse_tree_t* current_node, unsigned int* index) {
    StackedData* stacked_data = empty_base_class_entry();
    while (*index < tokens.cursor) {
        struct jump_map* jump_to = &current_node->jump_mapping[tokens.tokens[*index].type];
        if (jump_to->on_found == exit_substate) {
            if (stacked_data->name) {
                string_dict_put(class_content, stacked_data->name, stacked_data);
            }
            return true;
        }
        if (is_null(*jump_to)) {
            jump_to = &current_node->default_jump;
        }
        StackedData* o_stacked_data = stacked_data;
        if (jump_to->on_found) {
            if (!jump_to->on_found(class_content, &stacked_data, tokens, index)) {
                free(stacked_data);
                return false;
            }
        }
        if (stacked_data == NULL) free(o_stacked_data);
        current_node = jump_to->what_if ? jump_to->what_if : current_node;
        ++*index;
    }
    if (stacked_data) free(stacked_data);
    return false;
}

StringDict* scan_file(TokenList tokens) {
    parse_tree_t* current_node = &on_enter_file;
    StringDict* out = malloc(sizeof(StringDict));
    unsigned int index = 0;
    string_dict_init(out);
    while (index + 1 < tokens.cursor) {
        if (!scan_file_internal(out, tokens, current_node, &index)) {
            string_dict_destroy(out);
            free(out);
            return NULL;
        }
    }
    return out;
}