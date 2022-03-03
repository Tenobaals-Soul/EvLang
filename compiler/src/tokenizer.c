#include"tokenizer.h"
#include"string_dict.h"
#include<stdlib.h>
#include<stdbool.h>
#include<ctype.h>
#include<memory.h>

unsigned long advance(TokenList * list, unsigned long capacity) {
    list->size++;
    if (list->size >= capacity) {
        capacity += 128;
        list->tokens = realloc(list->tokens, capacity);
    }
    return capacity;
}

char * extract_identifier(const char * src, unsigned long * len) {
    *len = 0;
    if (isalpha(*src) || *src == '_') {
        while (isalpha(*src) || *src == '_' || isdigit(*src)) {
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

int try_extract_identifier(StringDict * dict, const char ** src, TokenList * list) {
    unsigned long len;
    char * identifier = extract_identifier(*src, &len);
    if (identifier) {
        void * val = string_dict_get(dict, identifier);
        if (val) {
            list->tokens[list->size].type = KEYWORD_TOKEN;
            list->tokens[list->size].keyword = val;
        }
        else {
            list->tokens[list->size].type = IDENTIFIER_TOKEN;
            list->tokens[list->size].identifier = identifier;
        }
        *src += len;
        return 1;
    }
    return 0;
}

void extract_next(StringDict * dict, const char ** src, TokenList * list) {
    while (isspace(**src) || **src == '\n') ++*src;
    if (try_extract_identifier(dict, src, list)) return;
}

void init_keyword_dict(StringDict * dict) {
    string_dict_init(dict);
    string_dict_put(dict, "main", "main");
    string_dict_put(dict, "public", "public");
    string_dict_put(dict, "protected", "protected");
    string_dict_put(dict, "private", "private");
    string_dict_put(dict, "class", "class");
    string_dict_put(dict, "static", "stativ");

    string_dict_put(dict, "if", "if");
    string_dict_put(dict, "for", "for");
    string_dict_put(dict, "while", "while");
    string_dict_put(dict, "break", "break");
    string_dict_put(dict, "return", "return");
    string_dict_put(dict, "switch", "switch");
    string_dict_put(dict, "case", "case");
}

TokenList lex(const char * src) {
    StringDict dict;
    init_keyword_dict(&dict);
    unsigned long capacity = 128;
    TokenList list = {
        .tokens = malloc(sizeof(Token) * capacity),
        .size = 0
    };
    while (*src) {
        extract_next(&dict, &src, &list);
    }
    string_dict_destroy(&dict);
    list.tokens = realloc(list.tokens, list.size);
    return list;
}