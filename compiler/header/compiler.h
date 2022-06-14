#ifndef INCLUDE_COMPILER_H
#define INCLUDE_COMPILER_H
#include<inttypes.h>
#include<stdbool.h>
#include<string_dict.h>

// "src\0\0", "append\0" -> "src\0append\0\0"
char* append_accessor_str(char* src, char* append);

typedef enum FixedDataType {
    INTEGER, FLOATING, STRING, BOOLEAN, CHARACTER
} FixedDataType;

typedef enum BasicOperator {
    ADD_OPERATOR, SUBTRACT_OPERATOR, 
    MULTIPLY_OPERATOR, DIVIDE_OPERATOR,
    RIGHT_SHIFT_OPERATOR, LEFT_SHIFT_OPERATOR, 
    EQUALS_OPERATOR,
    SMALLER_THAN_OPERATOR, SMALLER_EQUAL_OPERATOR,
    GREATER_THAN_OPERATOR, GREATER_EQUAL_OPERATOR,
    NOT_EQUAL_OPERATOR
} BasicOperator;

typedef enum TokenType {
    END_TOKEN, IDENTIFIER_TOKEN,
    FIXED_VALUE_TOKEN, OPERATOR_TOKEN,
    OPEN_PARANTHESIS_TOKEN, CLOSE_PARANTHESIS_TOKEN,
    OPEN_BLOCK_TOKEN, CLOSE_BLOCK_TOKEN,
    OPEN_INDEX_TOKEN, CLOSE_INDEX_TOKEN,
    SEPERATOR_TOKEN, DOT_TOKEN,
    K_PUBLIC_TOKEN, K_PROTECTED_TOKEN,
    K_PRIVATE_TOKEN, K_CLASS_TOKEN, K_DERIVES_TOKEN,
    K_IMPLEMENTS_TOKEN, K_STATIC_TOKEN,
    C_IF_TOKEN, C_FOR_TOKEN, C_WHILE_TOKEN,
    C_BREAK_TOKEN, C_RETURN_TOKEN,
    C_SWITCH_TOKEN, C_CASE_TOKEN,
    C_DEFAULT_TOKEN, ASSIGN_TOKEN
} TokenType;

typedef struct Token {
    TokenType type;
    unsigned int char_in_line;
    unsigned int line_in_file;
    unsigned int text_len;
    const char* line_content;
    union {
        char* identifier;
        struct {
            FixedDataType type;
            union {
                uint64_t integer;
                long double floating;
                bool boolean;
                char character;
                char* string;
            } value;
        } fixed_value;
        BasicOperator operator_type;
    };
} Token;

typedef struct TokenList {
    Token* tokens;
    unsigned int cursor;
    bool has_error;
} TokenList;

void set_enviroment(const char* enviroment);

void make_error(const char * line, unsigned int line_no, unsigned int char_no, 
        unsigned int len, const char * error_message, ...);

void make_warning(const char* line, unsigned int line_no, unsigned int char_no, 
        unsigned int len, const char * warning_message, ...);

void free_ast(const char* key, void* val);

// src\0src\0\0 -> StackedData
struct StackedData* get_from_ident_dot_seq(StringDict* src, const char* name, TokenList* tokens, int token_index, bool throw);

#endif