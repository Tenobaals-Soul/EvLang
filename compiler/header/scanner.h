#ifndef INCLUDE_SCANNER_H
#define INCLUDE_SCANNER_H
#include<string_dict.h>
#include<compiler.h>

typedef enum EntryType {
    ERROR_TYPE, ENTRY_CLASS, ENTRY_METHOD_TABLE, ENTRY_VARIABLE, ENTRY_METHOD, ENTRY_ERROR, ENTRY_MODULE
} EntryType;

typedef enum Accessability {
    PUBLIC, PROTECTED, PRIVATE
} Accessability;

typedef struct StackedData StackedData;

typedef struct Expression Expression;
typedef struct Statement Statement;
typedef struct Function Function;

struct Expression {
    enum ExpressionType {
        EXPRESSION_CALL, EXPRESSION_OPERATOR,
        EXPRESSION_UNARY_OPERATOR, EXPRESSION_FIXED_VALUE,
        EXPRESSION_OPEN_PARANTHESIS_GUARD, EXPRESSION_VAR
    } expression_type;
    union {
        StackedData* expression_variable;
        Token* fixed_value;
        struct {
            Expression* left;
            Expression* right;
            BasicOperator operator;
        } expression_operator;
        struct {
            StackedData* call;
            Expression** args;
        } expression_call;
    };
};

struct Statement {
    enum StatementType {
        STATEMENT_ASSIGN, STATEMENT_IF, STATEMENT_WHILE,
        STATEMENT_FOR, STATEMENT_SWITCH,
        STATEMENT_FUNCTION_CALL, STATEMENT_RETURN
    } statement_type;
    union {
        struct {
            Expression* left;
            Expression* right;
        } statement_assign;
        struct {
            Expression* condition;
            Statement** on_true;
            Statement** on_false;
        } statement_if;
        struct {
            Expression* condition;
            Statement** text;
        } statement_while;
        struct {
            Statement** first;
            Expression* condition;
            Statement** last;
            Statement** text;
        } statement_for;
        struct {
            Function* call;
            Statement** args;
        } expression_call;
    };
};

struct StackedData {
    EntryType type;
    TokenList env_token;
    char* name;
    char* path;
    unsigned int line_no;
    Accessability accessability;
    bool is_static;
    Token* causing;
    unsigned int text_start;
    unsigned int text_end;
    struct {
        char* type;
        Expression* exec_text;
        bool was_initialized;
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
        Statement** exec_text;
    } method;
    struct {
        char* return_type;
        unsigned int len;
        struct StackedData** methods;
    } method_table;
};

StringDict* scan_content(TokenList tokens, unsigned int* index);

#endif
