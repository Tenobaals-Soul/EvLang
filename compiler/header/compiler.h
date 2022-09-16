#ifndef INCLUDE_COMPILER_H
#define INCLUDE_COMPILER_H
#include<inttypes.h>
#include<stdbool.h>
#include<string_dict.h>
#include<stdarg.h>
#include<mmemory.h>

extern bool debug_run;

// "src\0\0", "append\0" -> "src\0append\0\0"
char* append_accessor_str(char* src, char* append);

typedef enum FixedDataType {
    INTEGER, FLOATING, STRING, BOOLEAN, CHARACTER
} FixedDataType;

typedef enum BasicOperator {
    ADD_OPERATOR, SUBTRACT_OPERATOR, 
    MULTIPLY_OPERATOR, DIVIDE_OPERATOR,
    RIGHT_SHIFT_OPERATOR, LEFT_SHIFT_OPERATOR,
    BINARY_XOR_OPERATOR, BINARY_NOT_OPERATOR,
    BINARY_OR_OPERATOR, BINARY_AND_OPERATOR,

    INCREMENT_OPERATOR, DECREMENT_OPERATOR,

    EQUALS_OPERATOR,
    SMALLER_THAN_OPERATOR, SMALLER_EQUAL_OPERATOR,
    GREATER_THAN_OPERATOR, GREATER_EQUAL_OPERATOR,
    NOT_EQUAL_OPERATOR, BOOL_NOT_OPERATOR,
    BOOL_AND_OPERATOR, BOOL_OR_OPERATOR,
    
    INPLACE_ADD_OPERATOR, INPLACE_SUBTRACT_OPERATOR,
    INPLACE_MULTIPLY_OPERATOR, INPLACE_DIVIDE_OPERATOR,
    INPLACE_POW_OPERATOR, INPLACE_AND_OPERATOR,
    INPLACE_OR_OPERATOR, INPLACE_RSHIFT_OPERAOR,
    INPLACE_LSHIFT_OPERATOR
} BasicOperator;

typedef enum TokenType {
    END_TOKEN, IDENTIFIER_TOKEN,
    FIXED_VALUE_TOKEN, OPERATOR_TOKEN,
    OPEN_PARANTHESIS_TOKEN, CLOSE_PARANTHESIS_TOKEN,
    OPEN_BLOCK_TOKEN, CLOSE_BLOCK_TOKEN,
    OPEN_INDEX_TOKEN, CLOSE_INDEX_TOKEN,
    SEPERATOR_TOKEN, DOT_TOKEN,
    K_PUBLIC_TOKEN, K_PROTECTED_TOKEN,
    K_PRIVATE_TOKEN, K_NAMESPACE_TOKEN,
    K_IMPLEMENTS_TOKEN, K_STRUCT_TOKEN,
    K_UNION_TOKEN, K_EXTENDS_TOKEN,
    C_IF_TOKEN, C_ELSE_TOKEN,
    C_FOR_TOKEN, C_WHILE_TOKEN,
    C_BREAK_TOKEN, C_RETURN_TOKEN,
    ASSIGN_TOKEN
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
const char* get_enviroment();

void make_error(const char * line, unsigned int line_no, unsigned int char_no, 
        unsigned int len, const char * error_message, ...);

void make_warning(const char* line, unsigned int line_no, unsigned int char_no, 
        unsigned int len, const char * warning_message, ...);

void make_debug_message(const char *line, unsigned int line_no, unsigned int char_no,
                  unsigned int len, const char *warning_message, ...);

void vmake_error(const char * line, unsigned int line_no, unsigned int char_no, 
        unsigned int len, const char * error_message, va_list l);

void vmake_warning(const char* line, unsigned int line_no, unsigned int char_no, 
        unsigned int len, const char * warning_message, va_list l);

void vmake_debug_message(const char *line, unsigned int line_no, unsigned int char_no,
                  unsigned int len, const char *warning_message, va_list l);

void free_ast(const char* key, void* val);

// src\0src\0\0 -> StackedData

typedef struct Expression* Expression;
typedef struct Statement* Statement;

typedef struct Text Text;
typedef struct Type Type;
typedef struct StructData StructData;

typedef struct Method* Method;

typedef struct EntryBase* UnresolvedEntry;

typedef struct Package* Package;
typedef struct Module* Module;
typedef struct Class* Class;
typedef struct Field* Field;
typedef struct MethodTable* MethodTable;

UnresolvedEntry get_from_ident_dot_seq(StringDict* src, const char* name,
                                       TokenList* tokens, int token_index, bool throw);

struct Text {
    Statement* statements;
    unsigned int len;
};

struct Type {
    Class resolved;
    char* unresolved;
};

struct StructData {
    struct Field* value;
    unsigned int len;
};

struct Method {
    char* name;
    StructData arguments;
    Type return_type;
    StructData stack_data;
    Text text;
};

enum EntryType {
    ENTRY_PACKAGE,
    ENTRY_MODULE,
    ENTRY_CLASS,
    ENTRY_METHODS,
    ENTRY_FIELD
};

struct EntryBase {
    enum EntryType entry_type;
    char* name;
    unsigned int line;
};

struct Field {
    struct EntryBase meta;
    Type type;
};

struct Package {
    struct EntryBase meta;
    StringDict package_content;
};

struct Module {
    struct EntryBase meta;
    StructData globals;
    StringDict module_content;
    Text text;
};

struct Class {
    struct EntryBase meta;
    StructData static_struct;
    StructData instance_struct;
    StringDict class_content;
    struct {
        Type* values;
        unsigned int len;
    } derives;
    struct {
        Type* values;
        unsigned int len;
    } implements;
};

struct MethodTable {
    struct EntryBase meta;
    Method* value;
    unsigned int len;
};

struct Expression {
    enum ExpressionType {
        EXPRESSION_CALL, EXPRESSION_OPERATOR, EXPRESSION_INDEX,
        EXPRESSION_UNARY_OPERATOR, EXPRESSION_FIXED_VALUE,
        EXPRESSION_OPEN_PARANTHESIS_GUARD, EXPRESSION_VAR,
        EXPRESSION_ASSIGN
    } expression_type;
    union {
        Field expression_variable;
        Token* fixed_value;
        struct {
            struct Expression* left;
            struct Expression* right;
            BasicOperator operator;
        } expression_operator;
        struct {
            Method call;
            struct Expression** args;
            unsigned int arg_count;
        } expression_call;
        struct {
            struct Expression* from;
            struct Expression* key;
        } expression_index;
    };
};

struct Statement {
    enum StatementType {
        STATEMENT_CALC, STATEMENT_IF, STATEMENT_WHILE,
        STATEMENT_FOR, STATEMENT_SWITCH, STATEMENT_RETURN
    } statement_type;
    union {
        struct {
            Expression calc;
        } statement_calc;
        struct {
            Expression condition;
            Text on_true;
            Text on_false;
        } statement_if;
        struct {
            Expression condition;
            Text text;
        } statement_while;
        struct {
            Text first;
            Expression condition;
            Text last;
            Text text;
        } statement_for;
        struct {
            Expression return_value;
        } statement_return;
    };
};

#endif