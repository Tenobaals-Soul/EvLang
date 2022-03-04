#ifndef INCLUDE_COMPILER_H
#define INCLUDE_COMPILER_H
#include<inttypes.h>
#include<stdbool.h>

typedef enum FixedDataType {
    NUMBER, STRING, BOOLEAN, CHARACTER
} FixedDataType;

typedef enum BasicOperator {
    ADD_OPERATOR, SUBTRACT_OPERATOR, 
    MULTIPLY_OPERATOR, DIVIDE_OPERATOR,
    RIGHT_SHIFT_OPERATOR, LEFT_SHIFT_OPERATOR, 
    EQUALS_OPERATOR, ASSIGN_OPERATOR,
    SMALLER_THAN_OPERATOR, SMALLER_EQUAL_OPERATOR,
    GREATER_THAN_OPERATOR, GREATER_EQUAL_OPERATOR,
    NOT_EQUAL_OPERATOR
} BasicOperator;

typedef enum TokenType {
    OPTIONAL_END_TOKEN, KEYWORD_TOKEN, IDENTIFIER_TOKEN,
    FIXED_VALUE_TOKEN, OPERATOR_TOKEN,
    OPEN_PARANTHESIS_TOKEN, CLOSE_PARANTHESIS_TOKEN,
    OPEN_BLOCK_TOKEN, CLOSE_BLOCK_TOKEN,
    OPEN_INDEX_TOKEN, CLOSE_INDEX_TOKEN,
    SEPERATOR_TOKEN, DOT_TOKEN
} TokenType;

typedef struct Token {
    TokenType type;
    unsigned int char_in_line;
    unsigned int line_in_file;
    unsigned int text_len;
    union {
        char * keyword;
        char * identifier;
        struct {
            FixedDataType type;
            union {
                uint64_t integer;
                long double floating;
                bool boolean;
                char character;
                char * string;
            } value;
        } fixed_value;
        BasicOperator operator_type;
    };
} Token;

typedef struct TokenList {
    Token * tokens;
    unsigned long cursor;
} TokenList;

void make_error(const char * file_name, const char * line, unsigned int line_no, unsigned int char_no, 
        unsigned int len, const char * error_message, ...);

#endif