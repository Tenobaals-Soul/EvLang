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
    char* var_type;
    bool is_static;
    struct {
        StringDict* class_content;
        char* derives_from;
        char** implements;
        unsigned int implements_len;
    } class;
    struct {
        char* return_type;
        char** args;
    } method;
    struct {
        char* return_type;
        unsigned int len;
        struct StackedData** methods;
    } method_table;
} StackedData;

StringDict* scan_file(TokenList tokens);

#endif