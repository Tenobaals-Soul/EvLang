#include<tokenizer.h>
#include<string_dict.h>
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
    bool error;
    const char * line_begin;
    unsigned int current_line;
} current_read_data;

int try_extract_end(current_read_data * cr, const char ** src, TokenList * list) {
    if (**src == ';') {
        ++*src;
        list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = 1;
        list->tokens[list->cursor].type = END_TOKEN;
        return 1;
    }
    return 0;
}

int t_get_bool(char* identifier, unsigned long len, current_read_data* cr, TokenList* list, const char** src) {
    if (strcmp(identifier, "true") == 0) {
        list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = len;
        list->tokens[list->cursor].type = FIXED_VALUE_TOKEN;
        list->tokens[list->cursor].fixed_value.type = BOOLEAN;
        list->tokens[list->cursor].fixed_value.value.boolean = true;
        return 1;
    }
    else if (strcmp(identifier, "false") == 0) {
        list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = len;
        list->tokens[list->cursor].type = FIXED_VALUE_TOKEN;
        list->tokens[list->cursor].fixed_value.type = BOOLEAN;
        list->tokens[list->cursor].fixed_value.value.boolean = false;
        return 1;
    }
    return 0;
}

void t_get_keyword_or_identifier(char* identifier, unsigned long len, current_read_data* cr, StringDict* keyword_dict, TokenList* list, const char** src) {
    void* val = string_dict_get(keyword_dict, identifier);
    list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
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

int try_extract_identifier_or_keyword(current_read_data* cr, StringDict* dict, const char** src, TokenList* list) {
    unsigned long len;
    char* identifier = read_next_identifier(*src, &len);
    if (identifier) {
        if (!t_get_bool(identifier, len, cr, list, src)) {
            t_get_keyword_or_identifier(identifier, len, cr, dict, list, src);
        }
        *src += len;
        return 1;
    }
    return 0;
}

enum parse_number_res { NO_NUMBER, FLOATING_NUMBER, INTERGER_NUMBER };

enum parse_number_res parse_binary_number(current_read_data* cr, const char** src, long double* f_out, unsigned long long* i_out) {
    const char* old = *src;
    unsigned long long i_buffer = 0;
    while (**src == '0' || **src == '1') {
        i_buffer = i_buffer << 1;
        i_buffer += **src == '1';
        ++*src;
    }
    if (**src != '.') {
        *i_out = i_buffer;
        return INTERGER_NUMBER;
    }
    ++*src;
    if (!(**src == '0' || **src == '1')) {
        make_error(cr->line_begin, cr->current_line, ((unsigned long) *src - (unsigned long) cr->line_begin),
            (unsigned long) *src - (unsigned long) old, "illegal number literal format");
        return NO_NUMBER;
    }
    long double f_buffer = i_buffer;
    for (unsigned int i = 1; (**src == '0' || **src == '1') && (1 << i) != 0; i++, ++*src) {
        f_buffer += (long double) (**src == '1') / (long double) (1 << i);
    }
    *f_out = f_buffer;
    return FLOATING_NUMBER;
}

enum parse_number_res parse_octal_number(current_read_data* cr, const char** src, long double* f_out, unsigned long long* i_out) {
    const char* old = *src;
    unsigned long long i_buffer = 0;
    while (**src >= '0' && **src <= '7') {
        i_buffer = i_buffer << 3; // times 8
        i_buffer += **src - '0';
        ++*src;
    }
    if (**src != '.') {
        *i_out = i_buffer;
        return INTERGER_NUMBER;
    }
    ++*src;
    if (!(**src >= '0' && **src <= '7')) {
        make_error(cr->line_begin, cr->current_line, ((unsigned long) *src - (unsigned long) cr->line_begin),
            (unsigned long) *src - (unsigned long) old + 2, "illegal number literal format");
        return NO_NUMBER;
    }
    long double f_buffer = i_buffer;
    for (unsigned int i = 3; (**src >= '0' && **src <= '7') && (1 << i) != 0; i += 3, ++*src) {
        f_buffer += (long double) (**src - '0') / (long double) (1 << i);
    }
    *f_out = f_buffer;
    return FLOATING_NUMBER;
}

enum parse_number_res parse_decimal_number(current_read_data* cr, const char** src, long double* f_out, unsigned long long* i_out) {
    const char* old = *src;
    *i_out = strtoll(*src, (void*) src, 10);
    if (old == *src) {
        return NO_NUMBER;
    }
    if (**src != '.') {
        return INTERGER_NUMBER;
    }
    *src = old;
    *f_out = strtold(*src, (void*) src);
    if (old == *src) {
        make_error(cr->line_begin, cr->current_line, ((unsigned long) *src - (unsigned long) cr->line_begin),
            (unsigned long) *src - (unsigned long) old, "illegal number literal format");
        return NO_NUMBER;
    }
    return FLOATING_NUMBER;
}

enum parse_number_res parse_hexadecimal_number(current_read_data* cr, const char** src, long double* f_out, unsigned long long* i_out) {
    const char* old = *src;
    *i_out = strtoll(*src + 2, (void*) src, 16);
    if (old == *src) {
        return NO_NUMBER;
    }
    if (**src != '.') {
        return INTERGER_NUMBER;
    }
    *src = old;
    *f_out = strtold(*src, (void*) src);
    if (old == *src) {
        make_error(cr->line_begin, cr->current_line, ((unsigned long) *src - (unsigned long) cr->line_begin),
            (unsigned long) *src - (unsigned long) old, "illegal number literal format");
        return NO_NUMBER;
    }
    return FLOATING_NUMBER;
}

enum parse_number_res parse_number(current_read_data* cr, const char** src, long double* f_out, unsigned long long* i_out) {
    if (**src == '0') {
        ++*src;
        switch (**src) {
        case 'b':
            ++*src;
            return parse_binary_number(cr, src, f_out, i_out);
        case 'o':
            ++*src;
            return parse_octal_number(cr, src, f_out, i_out);
        case 'x':
            --*src;
            return parse_hexadecimal_number(cr, src, f_out, i_out);
        default:
            return parse_octal_number(cr, src, f_out, i_out);
        }
    }
    return parse_decimal_number(cr, src, f_out, i_out);
}

static int t_get_number(current_read_data* cr, const char** src, TokenList* list) {
    const char* before = *src;
    unsigned int char_in_line_before = ((unsigned long) *src - (unsigned long) cr->line_begin);
    long double floating_value_found;
    unsigned long long integer_value_found;
    enum parse_number_res res = parse_number(cr, src, &floating_value_found, &integer_value_found);
    if (res != NO_NUMBER) {
        unsigned long len = ((ulong) *src) - ((ulong) before);
        list->tokens[list->cursor].char_in_line = char_in_line_before;
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = len;
        list->tokens[list->cursor].type = FIXED_VALUE_TOKEN;
        if (res == FLOATING_NUMBER) {
            list->tokens[list->cursor].fixed_value.type = FLOATING;
            list->tokens[list->cursor].fixed_value.value.floating = floating_value_found;
        }
        else {
            list->tokens[list->cursor].fixed_value.type = INTEGER;
            list->tokens[list->cursor].fixed_value.value.integer = integer_value_found;
        }
        return 1;
    }
    else {
        *src = before;
    }
    return 0;
}

static int t_get_str(current_read_data * cr, const char ** src, TokenList * list) {
    if (**src == '\"') {
        ++*src;
        unsigned long len = 0;
        while ((*src)[len] != '\"' && (*src)[len] != 0) {
            len++;
        }
        char * str = malloc(len + 1);
        memcpy(str, *src, len);
        str[len] = 0;
        *src += len + 1;
        list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
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
        unsigned long len = 0;
        while ((*src)[len] != '\'' && (*src)[len] != 0) {
            len++;
        }
        char * str = malloc(len + 1);
        memcpy(str, *src, len);
        str[len] = 0;
        *src += len + 1;
        list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
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
        list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        int len = strlen(cmp);
        list->tokens[list->cursor].text_len = len;
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
    if (make_operator(cr, src, list, LEFT_SHIFT_OPERATOR, "<<")) return 1;
    if (make_operator(cr, src, list, RIGHT_SHIFT_OPERATOR, ">>")) return 1;
    if (make_operator(cr, src, list, BINARY_XOR_OPERATOR, "^")) return 1;
    if (make_operator(cr, src, list, BINARY_NOT_OPERATOR, "~")) return 1;
    if (make_operator(cr, src, list, BINARY_OR_OPERATOR, "|")) return 1;
    if (make_operator(cr, src, list, BINARY_AND_OPERATOR, "&")) return 1;

    if (make_operator(cr, src, list, EQUALS_OPERATOR, "==")) return 1;
    if (make_operator(cr, src, list, SMALLER_THAN_OPERATOR, "<")) return 1;
    if (make_operator(cr, src, list, GREATER_THAN_OPERATOR, "<=")) return 1;
    if (make_operator(cr, src, list, SMALLER_EQUAL_OPERATOR, ">")) return 1;
    if (make_operator(cr, src, list, GREATER_EQUAL_OPERATOR, ">=")) return 1;
    if (make_operator(cr, src, list, NOT_EQUAL_OPERATOR, "!=")) return 1;
    if (make_operator(cr, src, list, BOOL_NOT_OPERATOR, "not")) return 1;
    if (make_operator(cr, src, list, BOOL_OR_OPERATOR, "or")) return 1;
    if (make_operator(cr, src, list, BOOL_AND_OPERATOR, "and")) return 1;
    if (make_operator(cr, src, list, BOOL_NOT_OPERATOR, "!")) return 1;
    if (make_operator(cr, src, list, BOOL_OR_OPERATOR, "||")) return 1;
    if (make_operator(cr, src, list, BOOL_AND_OPERATOR, "&&")) return 1;
    return 0;
}

static int try_extract_assign(current_read_data* cr, const char** src, TokenList* list) {
    if (**src == '=') {
        list->tokens[list->cursor].type = ASSIGN_TOKEN;
        list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = 1;
        *src += 1;
        return 1;
    }
    return 0;
}

static int try_extract_any_paranthesis(current_read_data* cr, const char** src, TokenList* list) {
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
    list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
    list->tokens[list->cursor].line_in_file = cr->current_line;
    list->tokens[list->cursor].line_content = cr->line_begin;
    list->tokens[list->cursor].text_len = 1;
    ++*src;
    return 1;
}

int try_extract_seperator(current_read_data* cr, const char** src, TokenList* list) {
    if (**src == ',') {
        list->tokens[list->cursor].type = SEPERATOR_TOKEN;
        list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
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
        list->tokens[list->cursor].char_in_line = ((unsigned long) *src - (unsigned long) cr->line_begin);
        list->tokens[list->cursor].line_in_file = cr->current_line;
        list->tokens[list->cursor].line_content = cr->line_begin;
        list->tokens[list->cursor].text_len = 1;
        ++*src;
        return 1;
    }
    return 0;
}

unsigned int t_err(current_read_data* cr, const char** src) {
    cr->error = true;
    unsigned int len = 0;
    unsigned int off = 0;
    while (**src && **src != '\n' && !(isspace((*src)[len]) || ispunct((*src)[len]))) len++;
    if (len == 0) {
        while (**src && **src != '\n' && !(isspace((*src)[len]))) len++;
    }
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
    make_error(cr->line_begin, cr->current_line, ((unsigned long) *src - (unsigned long) cr->line_begin), len, "unrecognized token \"%s\"", str_buffer);
    free(str_buffer);
    return len;
}

int extract_next(current_read_data* cr, StringDict* dict, const char** src, TokenList* list) {
    if (try_extract_end(cr, src, list)) return 1;
    if (try_extract_identifier_or_keyword(cr, dict, src, list)) return 1;
    if (try_extract_fixed_value(cr, src, list)) return 1;
    if (try_extract_any_paranthesis(cr, src, list)) return 1;
    if (try_extract_operator(cr, src, list)) return 1;
    if (try_extract_assign(cr, src, list)) return 1;
    if (try_extract_seperator(cr, src, list)) return 1;
    if (try_extract_dot(cr, src, list)) return 1;
    unsigned int len = t_err(cr, src);
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
    string_dict_put(dict, "derives", (void*) K_DERIVES_TOKEN);
    string_dict_put(dict, "implements", (void*) K_IMPLEMENTS_TOKEN);

    string_dict_put(dict, "if", (void*) C_IF_TOKEN);
    string_dict_put(dict, "for", (void*) C_FOR_TOKEN);
    string_dict_put(dict, "while", (void*) C_WHILE_TOKEN);
    string_dict_put(dict, "break", (void*) C_BREAK_TOKEN);
    string_dict_put(dict, "return", (void*) C_RETURN_TOKEN);
    string_dict_put(dict, "switch", (void*) C_SWITCH_TOKEN);
    string_dict_put(dict, "case", (void*) C_CASE_TOKEN);
    string_dict_put(dict, "default", (void*) C_DEFAULT_TOKEN);
}

void consume_white_space(current_read_data* cr, const char** src) {
    while (isspace(**src) || **src == '\n') {
        if (**src == '\n') {
            cr->current_line++;
            ++*src;
            cr->line_begin = *src;
        }
        else {
            ++*src;
        }
    }
}

TokenList lex(const char* src) {
    StringDict keyword_dict;
    init_keyword_dict(&keyword_dict);
    unsigned long capacity = 128;
    TokenList list = {
        .tokens = malloc(sizeof(Token) * capacity),
        .cursor = 0
    };
    current_read_data cr = {
        .error = false,
        .line_begin = src,
        .current_line = 1
    };
    while (true) {
        consume_white_space(&cr, &src);
        if (!*src) break;
        if (extract_next(&cr, &keyword_dict, &src, &list)) {
            capacity = advance(&list, capacity);
        }
    }
    string_dict_destroy(&keyword_dict);
    list.tokens = realloc(list.tokens, sizeof(Token) * list.cursor);
    list.has_error = cr.error;
    return list;
}