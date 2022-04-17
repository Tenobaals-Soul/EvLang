#ifndef INCLUDE_SCANNER_H
#define INCLUDE_SCANNER_H
#include"string_dict.h"
#include"compiler.h"

typedef enum EntryType {
    ENTRY_CLASS, ENTRY_METHOD_TABLE, ENTRY_VARIABLE, ENTRY_METHOD
} EntryType;

typedef enum Accessability {
    PUBLIC, PROTECTED, PRIVATE
} Accessability;

typedef struct Entry {
    EntryType type;
    unsigned int line_no;
    Accessability accessability;
    union {
        char* var_type;
        StringDict* class_content;
        struct {
            char* return_type;
            char** args;
        };
        struct {
            char* return_type;
            unsigned int len;
            struct Entry** methods;
        } table;
    };
} Entry;

StringDict * scan_file(TokenList tokens);

#endif