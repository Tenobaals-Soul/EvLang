#include"tokenizer.h"
#include"string_dict.h"
#include<stdlib.h>
#include<stdbool.h>
#include<ctype.h>
#include<memory.h>

unsigned long advance(TokenList * list, unsigned long capacity) {
    list->cursor++;
    if (list->cursor >= capacity) {
        capacity += 128;
        list->tokens = realloc(list->tokens, sizeof(Token) * capacity);
    }
    return capacity;
}

char* read_next_identifier(const char * src, unsigned long * len) {
    *len = 0;
    if (isalpha(*src) || *src == '_') {
        while (isalpha(src[*len]) || src[*len] == '_' || isdigit(src[*len])) {
            ++*len;
        }
        char * found = malloc(*len + 1);
        memcpy(found, src, *len);
        found[*len] = 0;
        return found;
    }
    else {
        return NULL;
    }
}

typedef struct current_read_data {
    const char * line_begin;
    unsigned int current_char;
    unsigned int current_line;
} current_read_data;

int try_extract_end(current_read_data * cr, const char ** src, TokenList * list) {
    if (**src == ';') {
        ++*src;
        cr->current_char++;
        list->tokens[list->cursor].char_in_line = cr->current_char;
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = 1;
        list->tokens[list->cursor].type = END_TOKEN;
        return 1;
    }
    return 0;
}

int t_get_bool(char * identifier, unsigned long len, current_read_data * cr, TokenList * list) {
    if (strcmp(identifier, "true") == 0) {
        list->tokens[list->cursor].char_in_line = cr->current_char;
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = len;
        list->tokens[list->cursor].type = FIXED_VALUE_TOKEN;
        list->tokens[list->cursor].fixed_value.type = NUMBER;
        list->tokens[list->cursor].fixed_value.value.boolean = true;
        return 1;
    }
    else if (strcmp(identifier, "false") == 0) {
        list->tokens[list->cursor].char_in_line = cr->current_char;
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = len;
        list->tokens[list->cursor].type = FIXED_VALUE_TOKEN;
        list->tokens[list->cursor].fixed_value.type = NUMBER;
        list->tokens[list->cursor].fixed_value.value.boolean = false;
        return 1;
    }
    return 0;
}

void t_get_keyword_or_identifier(char* identifier, unsigned long len, current_read_data* cr, StringDict* keyword_dict, TokenList* list) {
    void* val = string_dict_get(keyword_dict, identifier);
    list->tokens[list->cursor].char_in_line = cr->current_char + 1;
    list->tokens[list->cursor].line_in_file = cr->current_line;
    list->tokens[list->cursor].line_content = cr->line_begin;
    list->tokens[list->cursor].text_len = len;
    if (val) {
        list->tokens[list->cursor].type = (TokenType) val;
        free(identifier);
    }
    else {
        list->tokens[list->cursor].type = IDENTIFIER_TOKEN;
        list->tokens[list->cursor].identifier = identifier;
    }
}

int try_extract_identifier_or_keyword(current_read_data * cr, StringDict * dict, const char ** src, TokenList * list) {
    unsigned long len;
    char * identifier = read_next_identifier(*src, &len);
    if (identifier) {
        if (!t_get_bool(identifier, len, cr, list)) {
            t_get_keyword_or_identifier(identifier, len, cr, dict, list);
        }
        *src += len;
        cr->current_char += len;
        return 1;
    }
    return 0;
}

static int t_get_number(current_read_data* cr, const char** src, TokenList* list) {
    const char * before = *src;
    long double value_found = strtold(before, (void*) src);
    if (before != *src) {
        unsigned long len = ((ulong) *src) - ((ulong) before);
        cr->current_char += len;
        list->tokens[list->cursor].char_in_line = cr->current_char;
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = len;
        list->tokens[list->cursor].type = FIXED_VALUE_TOKEN;
        list->tokens[list->cursor].fixed_value.type = NUMBER;
        list->tokens[list->cursor].fixed_value.value.floating = value_found;
        return 1;
    }
    return 0;
}

static int t_get_str(current_read_data * cr, const char ** src, TokenList * list) {
    if (**src == '\"') {
        ++*src;
        cr->current_char++;
        unsigned long len = 0;
        while ((*src)[len] != '\"' && (*src)[len] != 0) {
            len++;
        }
        char * str = malloc(len + 1);
        memcpy(str, *src, len);
        str[len] = 0;
        cr->current_char += len + 1;
        *src += len + 1;
        list->tokens[list->cursor].char_in_line = cr->current_char;
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = len + 2;
        list->tokens[list->cursor].type = FIXED_VALUE_TOKEN;
        list->tokens[list->cursor].fixed_value.type = STRING;
        list->tokens[list->cursor].fixed_value.value.string = str;
        return 1;
    }
    return 0;
}

static int t_get_char(current_read_data * cr, const char ** src, TokenList * list) {
    if (**src == '\'') {
        ++*src;
        cr->current_char++;
        unsigned long len = 0;
        while ((*src)[len] != '\'' && (*src)[len] != 0) {
            len++;
        }
        char * str = malloc(len + 1);
        memcpy(str, *src, len);
        str[len] = 0;
        cr->current_char += len + 1;
        *src += len + 1;
        list->tokens[list->cursor].char_in_line = cr->current_char;
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = len + 2;
        list->tokens[list->cursor].type = FIXED_VALUE_TOKEN;
        list->tokens[list->cursor].fixed_value.type = CHARACTER;
        list->tokens[list->cursor].fixed_value.value.string = str;
        return 1;
    }
    return 0;
}

static int try_extract_fixed_value(current_read_data * cr, const char ** src, TokenList * list) {
    if (t_get_number(cr, src, list)) return 1;
    if (t_get_str(cr, src, list)) return 1;
    if (t_get_char(cr, src, list)) return 1;
    return 0;
}

static int starts_with(const char* src, const char* search) {
    int i;
    for (i = 0; search[i] && src[i] && !isspace(src[i]) && !isalpha(src[i]) && !isdigit(src[i]); i++) {
        if (src[i] != search[i]) return 0;
    }
    return search[i] == 0;
}

static int make_operator(current_read_data * cr, const char ** src, TokenList * list, BasicOperator op_type, const char * cmp) {
    if (starts_with(*src, cmp)) {
        list->tokens[list->cursor].type = OPERATOR_TOKEN;
        list->tokens[list->cursor].operator_type = op_type;
        list->tokens[list->cursor].char_in_line = cr->current_char;
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        int len = strlen(cmp);
        list->tokens[list->cursor].text_len = len;
        cr->current_char += len;
        *src += len;
        return 1;
    }
    return 0;
}

static int try_extract_operator(current_read_data * cr, const char ** src, TokenList * list) {
    if (make_operator(cr, src, list, ADD_OPERATOR, "+")) return 1;
    if (make_operator(cr, src, list, SUBTRACT_OPERATOR, "-")) return 1;
    if (make_operator(cr, src, list, MULTIPLY_OPERATOR, "*")) return 1;
    if (make_operator(cr, src, list, DIVIDE_OPERATOR, "/")) return 1;
    if (make_operator(cr, src, list, EQUALS_OPERATOR, "==")) return 1;
    if (make_operator(cr, src, list, ASSIGN_OPERATOR, "=")) return 1;
    if (make_operator(cr, src, list, SMALLER_THAN_OPERATOR, "<")) return 1;
    if (make_operator(cr, src, list, GREATER_THAN_OPERATOR, "<=")) return 1;
    if (make_operator(cr, src, list, SMALLER_EQUAL_OPERATOR, ">")) return 1;
    if (make_operator(cr, src, list, GREATER_EQUAL_OPERATOR, ">=")) return 1;
    if (make_operator(cr, src, list, NOT_EQUAL_OPERATOR, "!=")) return 1;
    if (make_operator(cr, src, list, LEFT_SHIFT_OPERATOR, "<<")) return 1;
    if (make_operator(cr, src, list, RIGHT_SHIFT_OPERATOR, ">>")) return 1;
    return 0;
}

int try_extract_any_paranthesis(current_read_data * cr, const char ** src, TokenList * list) {
    switch (**src) {
    case '(':
        list->tokens[list->cursor].type = OPEN_PARANTHESIS_TOKEN;
        break;
    case ')':
        list->tokens[list->cursor].type = CLOSE_PARANTHESIS_TOKEN;
        break;
    case '{':
        list->tokens[list->cursor].type = OPEN_BLOCK_TOKEN;
        break;
    case '}':
        list->tokens[list->cursor].type = CLOSE_BLOCK_TOKEN;
        break;
    case '[':
        list->tokens[list->cursor].type = OPEN_INDEX_TOKEN;
        break;
    case ']':
        list->tokens[list->cursor].type = CLOSE_INDEX_TOKEN;
        break;
    default:
        return 0;
    }
    list->tokens[list->cursor].char_in_line = cr->current_char;
    list->tokens[list->cursor].line_in_file = cr->current_line;
    list->tokens[list->cursor].line_content = cr->line_begin;
    list->tokens[list->cursor].text_len = 1;
    ++*src;
    cr->current_char++;
    return 1;
}

int try_extract_seperator(current_read_data* cr, const char** src, TokenList* list) {
    if (**src == ',') {
        list->tokens[list->cursor].type = SEPERATOR_TOKEN;
        list->tokens[list->cursor].char_in_line = cr->current_char;
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = 1;
        ++*src;
        return 1;
    }
    return 0;
}

int try_extract_dot(current_read_data * cr, const char ** src, TokenList * list) {
    if (**src == '.') {
        list->tokens[list->cursor].type = DOT_TOKEN;
        list->tokens[list->cursor].char_in_line = cr->current_char;
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = 1;
        return 1;
    }
    return 0;
}

unsigned int t_err(current_read_data * cr, const char ** src) {
    unsigned int len = 0;
    unsigned int off = 0;
    while (**src && **src != '\n' && !isspace((*src)[len])) len++;
    char * str_buffer = malloc(len * 4 + 1);
    for (unsigned int i = 0; i < len; i++) {
        if (isprint((*src)[i])) {
            str_buffer[i + off] = (*src)[i];
        }
        else {
            for (unsigned long j = 0; j < sizeof("�") - 1; j++) {
                str_buffer[i + off] = "�"[j];
                off++;
            }
            off--;
        }
    }
    str_buffer[len + off] = 0;
    make_error(cr->line_begin, cr->current_line, cr->current_char, len, "unrecognized token \"%s\"", str_buffer);
    free(str_buffer);
    return len;
}

int extract_next(current_read_data* cr, StringDict* dict, const char** src, TokenList* list) {
    while (isspace(**src) || **src == '\n') {
        if (**src == '\n') {
            cr->current_line++;
            cr->current_char = 1;
            ++*src;
            cr->line_begin = *src;
        }
        else {
            cr->current_char++;
            ++*src;
        }
    }
    if (try_extract_end(cr, src, list)) return 1;
    if (try_extract_identifier_or_keyword(cr, dict, src, list)) return 1;
    if (try_extract_fixed_value(cr, src, list)) return 1;
    if (try_extract_any_paranthesis(cr, src, list)) return 1;
    if (try_extract_operator(cr, src, list)) return 1;
    if (try_extract_seperator(cr, src, list)) return 1;
    if (try_extract_dot(cr, src, list)) return 1;
    unsigned int len = t_err(cr, src);
    cr->current_char += len;
    *src += len;
    return 0;
}

void init_keyword_dict(StringDict * dict) {
    string_dict_init(dict);
    string_dict_put(dict, "public", (void*) K_PUBLIC_TOKEN);
    string_dict_put(dict, "protected", (void*) K_PROTECTED_TOKEN);
    string_dict_put(dict, "private", (void*) K_PRIVATE_TOKEN);
    string_dict_put(dict, "class", (void*) K_CLASS_TOKEN);
    string_dict_put(dict, "static", (void*) K_STATIC_TOKEN);

    string_dict_put(dict, "if", (void*) C_IF_TOKEN);
    string_dict_put(dict, "for", (void*) C_FOR_TOKEN);
    string_dict_put(dict, "while", (void*) C_WHILE_TOKEN);
    string_dict_put(dict, "break", (void*) C_BREAK_TOKEN);
    string_dict_put(dict, "return", (void*) C_RETURN_TOKEN);
    string_dict_put(dict, "switch", (void*) C_SWITCH_TOKEN);
    string_dict_put(dict, "case", (void*) C_CASE_TOKEN);
    string_dict_put(dict, "default", (void*) C_DEFAULT_TOKEN);
}

TokenList lex(const char * src) {
    StringDict keyword_dict;
    init_keyword_dict(&keyword_dict);
    unsigned long capacity = 128;
    TokenList list = {
        .tokens = malloc(sizeof(Token) * capacity),
        .cursor = 0
    };
    current_read_data cr = {
        .line_begin = src,
        .current_line = 1,
        .current_char = 1
    };
    while (*src) {
        if (extract_next(&cr, &keyword_dict, &src, &list)) {
            capacity = advance(&list, capacity);
        }
    }
    string_dict_destroy(&keyword_dict);
    list.tokens = realloc(list.tokens, sizeof(Token) * list.cursor);
    return list;
}