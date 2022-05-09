#ifndef INCLUDE_SCANNER_H
#define INCLUDE_SCANNER_H
#include"string_dict.h"
#include"compiler.h"

typedef enum EntryType {
    ERROR_TYPE, ENTRY_CLASS, ENTRY_METHOD_TABLE, ENTRY_VARIABLE, ENTRY_METHOD
} EntryType;

typedef enum Accessability {
    PUBLIC, PROTECTED, PRIVATE
} Accessability;

typedef struct StackedData {
    EntryType type;
    char* name;
    unsigned int line_no;
    Accessability accessability;
    bool is_static;
    Token* causing;
    struct {
        char* type;
        unsigned int expression_start_index;
    } var;
    struct {
        StringDict* class_content;
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