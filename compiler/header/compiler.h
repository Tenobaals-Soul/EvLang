#ifndef INCLUDE_COMPILER_H
#define INCLUDE_COMPILER_H
#include<inttypes.h>
#include<stdbool.h>
#include<string_dict.h>
#include<stdarg.h>
#include<mmemory.h>

extern bool debug_run;

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
        struct fixed_value {
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
    unsigned int length;
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

typedef struct Text Text;
typedef struct Type Type;
typedef struct StructData StructData;

typedef struct Function Function;

typedef struct EntryBase UnresolvedEntry;

typedef struct Module Module;
typedef struct Struct Struct;
typedef struct Union Union;
typedef struct Namespace Namespace;
typedef struct Field Field;
typedef struct NameTable NameTable;

typedef struct Statement Statement;
typedef struct Expression Expression;

// "src\0\0", "append\0" -> "src\0append\0\0"
char* ident_dot_seq_append(char* seq, char* name);
void ident_dot_seq_print(char *seq);
UnresolvedEntry* get_from_ident_dot_seq(StringDict* src, const char* seq, TokenList* tokens,
                                        int token_index, bool throw);

void free_type(Type* t);

struct Text {
    Statement** statements;
    unsigned int len;
};

struct Type {
    char* name;
    struct {
        Type** types;
        unsigned int len;
    } generics;
};

struct StructData {
    struct Field* value;
    unsigned int len;
};

enum EntryType {
    ENTRY_MODULE,
    ENTRY_STRUCT,
    ENTRY_UNION,
    ENTRY_NAMESPACE,
    ENTRY_FUNCTIONS,
    ENTRY_FIELD
};

struct EntryBase {
    enum EntryType entry_type;
    char* name;
    unsigned int line;
};

struct Function {
    char* name;
    StructData arguments[2];
    Type* return_type;
    StructData stack_data;
    Text text;
};

struct Field {
    struct EntryBase meta;
    unsigned int offset;
    Type* type;
};

struct Module {
    struct EntryBase meta;
    StructData globals;
    StringDict scope;
    Text text;
};

struct Struct {
    struct EntryBase meta;
    StructData data;
};

struct Union {
    struct EntryBase meta;
    StructData data;
};

struct Namespace {
    struct EntryBase meta;
    StructData struct_data;
    StringDict scope;
};

struct NameTable {
    const char* name;
    Function** functions;
    unsigned int fcount;
    union {
        Struct* stct;
        Union* onion;
    } type;
    union {
        Namespace* nspc;
        Module* module;
    } container;
    Field* variable;
};

typedef struct PA_operator {
    struct Expression* left;
    struct Expression* right;
    BasicOperator operator;
} PA_operator;

typedef struct PA_call {
    Function* call;
    struct Expression** args;
    unsigned int arg_count;
} PA_call;

typedef struct PA_access {
    struct Expression* from;
    struct Expression* key;
} PA_access;

typedef struct fixed_value PA_fixed_value;

struct Expression {
    enum ExpressionType {
        EXPRESSION_CALL, EXPRESSION_OPERATOR, EXPRESSION_ARR_ACCESS,
        EXPRESSION_UNARY_OPERATOR, EXPRESSION_FIXED_VALUE,
        EXPRESSION_VAR, EXPRESSION_ASSIGN
    } expression_type;
    union {
        Field* pa_variable;
        PA_fixed_value* pa_fixed_value;
        PA_operator* pa_operator;
        PA_call* pa_call;
        PA_access* pa_arr_access;
    };
};

typedef struct PA_if {
    Expression* condition;
    Text on_true;
    Text on_false;
} PA_if;

typedef struct PA_while {
    Expression* condition;
    Text text;
} PA_while;

typedef struct PA_for {
    Text first;
    Expression* condition;
    Text last;
    Text text;
} PA_for;

typedef Expression PA_return;

struct Statement {
    enum StatementType {
        STATEMENT_EXPRESSION, STATEMENT_IF, STATEMENT_WHILE,
        STATEMENT_FOR, STATEMENT_RETURN, STATEMENT_EMPTY
    } statement_type;
    union {
        Expression* pa_expression;
        PA_if* pa_if;
        PA_while* pa_while;
        PA_for* pa_for;
        PA_return* pa_return;
    };
};

#endif