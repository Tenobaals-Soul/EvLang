#ifndef INCLUDE_TOKENIZER_H
#define INCLUDE_TOKENIZER_H
#include<inttypes.h>
#include<stdbool.h>

typedef enum FixedDataType {
    NUMBER, STRING, BOOLEAN
} FixedDataType;

typedef enum BasicOperator {
    ADD_OPERATOR, SUBTRACT_OPERATOR, 
    MULTIPLY_OPERATOR, DIVIDE_OPERATOR,
    RIGHT_SHIFT_OPERATOR, LEFT_SHIFT_OPERATOR, 
    EQUALS_OPERATOR, ASSIGN_OPERATOR
} BasicOperator;

typedef enum TokenType {
    OPTIONAL_END_TOKEN, KEYWORD_TOKEN, IDENTIFIER_TOKEN,
    FIXED_VALUE_TOKEN, OPERATOR_TOKEN,
    OPEN_PARANTHESIS_TOKEN, CLOSE_PARANTHESIS_TOKEN,
    SEPERATOR_TOKEN, DOT_TOKEN
} TokenType;

typedef struct Token {
    TokenType type;
    union {
        char * keyword;
        char * identifier;
        struct {
            FixedDataType type;
            union {
                uint64_t integer;
                long double floating;
                bool boolean;
            } value;
        } fixed_value_token;
        BasicOperator operator_type;
    };
} Token;

typedef struct TokenList {
    Token * tokens;
    unsigned long size;
} TokenList;

TokenList lex(const char * src);

#endif