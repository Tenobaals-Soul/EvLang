#ifndef INCLUDE_SCANNER_H
#define INCLUDE_SCANNER_H
#include"string_dict.h"
#include"compiler.h"

typedef enum EntryType {
    ERROR_TYPE, ENTRY_CLASS, ENTRY_METHOD_TABLE, ENTRY_VARIABLE, ENTRY_METHOD, ENTRY_ERROR
} EntryType;

typedef enum Accessability {
    PUBLIC, PROTECTED, PRIVATE
} Accessability;

typedef struct StackedData {
    EntryType type;
    char* name;
    char* path;
    unsigned int line_no;
    Accessability accessability;
    bool is_static;
    Token* causing;
    unsigned int text;
    struct {
        char* type;
    } var;
    struct {
        StringDict* class_content;
        StringDict* packed;
        char* derives_from;
        char** implements;
        unsigned int implements_len;
    } class;
    struct {
        char* return_type;
        struct argument_s {
            char* type;
            char* name;
        }* args;
        unsigned int arg_count;
    } method;
    struct {
        char* return_type;
        unsigned int len;
        struct StackedData** methods;
    } method_table;
} StackedData;

StringDict* scan_content(TokenList tokens, unsigned int* index);

#endif
