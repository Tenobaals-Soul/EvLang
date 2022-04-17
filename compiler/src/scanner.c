#include"scanner.h"
#include<stdlib.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>

bool get_class_content();

bool skip_optional_ends(TokenList tokens, unsigned int* i) {
    if (*i >= tokens.cursor) return true;
    while (tokens.tokens[*i].type == OPTIONAL_END_TOKEN) {
        if (*i >= tokens.cursor) return true;
        ++*i;
    }
    if (*i >= tokens.cursor) return true;
    return false;
}

Accessability get_accessability(TokenList tokens, unsigned int* i) {
    if (tokens.tokens[*i].type == K_PRIVATE_TOKEN) {
        ++*i;
        return PRIVATE;
    }
    else if (tokens.tokens[*i].type == K_PUBLIC_TOKEN) {
        ++*i;
        return PUBLIC;
    }
    else if (tokens.tokens[*i].type == K_PROTECTED_TOKEN) {
        ++*i;
        return PROTECTED;
    }
    else {
        return PROTECTED;
    }
}

Entry* get_class(TokenList tokens, unsigned int* i, char** name) {
    unsigned int new_i = *i;
    if (new_i + 4 >= tokens.cursor) return NULL;
    Entry* current_class = malloc(sizeof(Entry));
    current_class->line_no = tokens.tokens[*i].line_in_file;
    current_class->type = ENTRY_CLASS;
    current_class->accessability = get_accessability(tokens, &new_i);
    
    if (skip_optional_ends(tokens, &new_i)) return false;
    if (tokens.tokens[new_i].type != K_CLASS_TOKEN) {
        free(current_class);
        return NULL;
    }
    new_i++;
    if (skip_optional_ends(tokens, &new_i)) return false;
    if (tokens.tokens[new_i].type != IDENTIFIER_TOKEN) {
        free(current_class);
        return NULL;
    }
    *name = tokens.tokens[new_i].identifier;
    new_i++;
    if (skip_optional_ends(tokens, &new_i)) return false;
    current_class->class_content = malloc(sizeof(StringDict));
    string_dict_init(current_class->class_content);
    if (tokens.tokens[new_i].type == K_DERIVES_TOKEN) {
        new_i++;
        if (skip_optional_ends(tokens, &new_i)) return false;
        if (tokens.tokens[new_i].type != IDENTIFIER_TOKEN) {
            string_dict_destroy(current_class->class_content);
            free(current_class);
            return NULL;
        }
        string_dict_put(current_class->class_content, "super", tokens.tokens[new_i].identifier);
        new_i++;
        if (skip_optional_ends(tokens, &new_i)) return false;
    }
    if (!get_class_content(current_class->class_content, tokens, &new_i)) {
        string_dict_destroy(current_class->class_content);
        free(current_class->class_content);
        free(current_class);
        return NULL;
    }
    *i = new_i;
    return current_class;
}

Entry* get_variable(TokenList tokens, unsigned int* i, char** name) {
    unsigned int new_i = *i;
    if (new_i + 2 > tokens.cursor) return NULL;
    Entry* current_variable = malloc(sizeof(Entry));
    current_variable->line_no = tokens.tokens[*i].line_in_file;
    current_variable->type = ENTRY_VARIABLE;
    current_variable->accessability = get_accessability(tokens, &new_i);
    if (tokens.tokens[new_i++].type != IDENTIFIER_TOKEN) {
        free(current_variable);
        return NULL;
    }
    current_variable->var_type = tokens.tokens[new_i].identifier;
    if (tokens.tokens[new_i].type != IDENTIFIER_TOKEN) {
        free(current_variable);
        return NULL;
    }
    *name = tokens.tokens[new_i++].identifier;
    *i = new_i;
    return current_variable;
}

char* get_function_argument(TokenList tokens, unsigned int* i) {
    unsigned int new_i = *i;
    if (new_i + 2 >= tokens.cursor) return NULL;
    if (tokens.tokens[new_i].type != IDENTIFIER_TOKEN) {
        make_error(tokens.tokens[new_i].line_content, tokens.tokens[new_i].line_in_file,
            tokens.tokens[new_i].char_in_line, tokens.tokens[new_i].text_len, "expected a type name");
        return NULL;
    }
    new_i++;
    if (skip_optional_ends(tokens, &new_i)) return false;
    if (tokens.tokens[new_i].type != IDENTIFIER_TOKEN) {
        make_error(tokens.tokens[new_i].line_content, tokens.tokens[new_i].line_in_file,
            tokens.tokens[new_i].char_in_line, tokens.tokens[new_i].text_len, "expected am identifier");
        return NULL;
    }
    new_i++;
    if (skip_optional_ends(tokens, &new_i)) return false;
    *i = new_i;
    return tokens.tokens[new_i - 1].identifier;
}

bool extract_parameters(TokenList tokens, unsigned int* i, char*** args, int* arg_len) {
    unsigned int new_i = *i;
    if (skip_optional_ends(tokens, &new_i)) return false;
    if (tokens.tokens[new_i].type != OPEN_PARANTHESIS_TOKEN) {
        return false;
    }
    new_i++;
    if (skip_optional_ends(tokens, &new_i)) return false;
    if (tokens.tokens[new_i].type != CLOSE_PARANTHESIS_TOKEN) {
        while (true) {
            char* arg;
            if (!(arg = get_function_argument(tokens, &new_i))) {
                return false;
            }
            ++*arg_len;
            *args = realloc(*args, *arg_len * sizeof(char*));
            (*args)[(*arg_len) - 1] = arg;
            if (tokens.tokens[new_i].type != SEPERATOR_TOKEN) {
                break;
            }
            else {
                new_i++;
            }
        }
        if (tokens.tokens[new_i].type != CLOSE_PARANTHESIS_TOKEN) {
            if (*args) free(*args);
            return false;
        }
    }
    *args = realloc(*args, (1 + *arg_len) * sizeof(char*));
    (*args)[*arg_len] = NULL;
    *i = new_i + 1;
    return true;
}

Entry* get_method(TokenList tokens, unsigned int* i, char** name) {
    unsigned int new_i = *i;
    int arg_len = 0;
    char** args = NULL;
    if (new_i + 4 > tokens.cursor) return NULL;
    Entry* current_method = malloc(sizeof(Entry));
    current_method->line_no = tokens.tokens[new_i].line_in_file;
    current_method->type = ENTRY_METHOD;
    current_method->accessability = get_accessability(tokens, &new_i);
    if (tokens.tokens[new_i].type != IDENTIFIER_TOKEN) {
        free(current_method);
        return NULL;
    }
    current_method->return_type = tokens.tokens[new_i].identifier;
    new_i++;
    if (tokens.tokens[new_i].type != IDENTIFIER_TOKEN) {
        free(current_method);
        return NULL;
    }
    *name = tokens.tokens[new_i].identifier;
    new_i++;
    if (!extract_parameters(tokens, &new_i, &args, &arg_len)) {
        free(current_method);
        return NULL;
    }
    if (skip_optional_ends(tokens, &new_i)) {
        free(args);
        free(current_method);
        return NULL;
    }
    if (tokens.tokens[new_i].type != OPEN_BLOCK_TOKEN) {
        free(args);
        free(current_method);
        return NULL;
    }
    unsigned int layer = 1;
    while (layer) {
        new_i++;
        if (tokens.cursor <= *i) return false;
        if (tokens.tokens[new_i].type == OPEN_BLOCK_TOKEN) layer++;
        else if (tokens.tokens[new_i].type == CLOSE_BLOCK_TOKEN) layer--;
    }
    new_i++;
    current_method->args = args;
    *i = new_i;
    return current_method;
}

bool append_method(StringDict* class_content, Token* pos, Entry* entry_found, char* name) {
    Entry* found = string_dict_get(class_content, name);
    Entry* method_table;
    if (found) {
        if (found->type != ENTRY_METHOD_TABLE) {
            make_error(pos->line_content, pos->line_in_file, 0, ~0, "\"%s\" is already defined in line %u.", found->line_no);
            return false;
        }
        method_table = found;
    }
    else {
        method_table = malloc(sizeof(Entry));
        method_table->type = ENTRY_METHOD_TABLE;
        method_table->accessability = PUBLIC;
        method_table->table.len = 0;
        method_table->table.methods = NULL;
        string_dict_put(class_content, name, method_table);
    }
    method_table->table.len++;
    method_table->table.methods = realloc(method_table->table.methods, method_table->table.len * sizeof(Entry*));
    method_table->table.methods[method_table->table.len - 1] = entry_found;
    return true;
}

StringDict* scan_file(TokenList tokens) {
    StringDict* out = malloc(sizeof(StringDict));
    string_dict_init(out);
    for (unsigned int i = 0; i < tokens.cursor; i++) {
        if (skip_optional_ends(tokens, &i)) return out;
        char* name;
        Entry* class_found = get_class(tokens, &i, &name);
        if (class_found == NULL) {
            free(out);
            return NULL;
        }
        string_dict_put(out, name, class_found);
    }
    return out;
}

bool get_class_content(StringDict* class_content, TokenList tokens, unsigned int* i) {
    unsigned int new_i = *i + 1;
    if (new_i >= tokens.cursor) return true;
    if (skip_optional_ends(tokens, &new_i)) return false;
    while(tokens.tokens[new_i].type != CLOSE_BLOCK_TOKEN) {
        Entry* new_entry;
        char* name;
        unsigned int old_i = new_i;
        if ((new_entry = get_class(tokens, &new_i, &name))) {
            if (string_dict_get(class_content, name)) {
                make_error(tokens.tokens[old_i].line_content, tokens.tokens[old_i].line_in_file,
                        tokens.tokens[old_i].char_in_line, tokens.tokens[old_i].text_len, "redefinition of symbol \"%s\"", name);
                return false;
            }
            string_dict_put(class_content, name, new_entry);
        }
        else if ((new_entry = get_method(tokens, &new_i, &name))) {
            if (!append_method(class_content, tokens.tokens + old_i, new_entry, name)) {
                return false;
            }
        }
        else if ((new_entry = get_variable(tokens, &new_i, &name))) {
            if (string_dict_get(class_content, name)) {
                make_error(tokens.tokens[old_i].line_content, tokens.tokens[old_i].line_in_file,
                        tokens.tokens[old_i].char_in_line, tokens.tokens[old_i].text_len, "redefinition of symbol \"%s\"", name);
                return false;
            }
            string_dict_put(class_content, name, new_entry);
        }
        /*else if (new_entry = get_lambda_function(tokens, &new_i, &name, &args)) {
            if (!append_method(class_content, new_entry, name, args)) {
                return false;
            }
        }*/
        else return false;
        if (skip_optional_ends(tokens, &new_i)) break;
    }
    *i = new_i;
    return true;
}