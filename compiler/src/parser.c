#include<parser.h>
#include<stdlib.h>
#include<stdarg.h>
#include<stack.h>
#include<stdio.h>
#include<compiler.h>
#include<tokenizer.h>
#include<string.h>
#include<stdarg.h>
#include<stack.h>
#include<limits.h>

#define IS_POSTFIX_UNARY(operator) (!!((1 << (operator)) & (\
    SET(ADD_OPERATOR) | SET(SUBTRACT_OPERATOR) |\
    SET(BINARY_NOT_OPERATOR) | SET(BOOL_NOT_OPERATOR)\
)))

#define IS_PREFIX_UNARY(operator) (!!((1 << (operator)) & (\
    SET(INCREMENT_OPERATOR) | SET(DECREMENT_OPERATOR)\
)))

#define IS_FORBIDDEN(operator) (!!((1 << (operator)) & (\
    SET(LAMBDA_ARROW_OPERATOR)\
)))

void throw(Token* token, const char* format, ...) {
    va_list l;
    va_start(l, format);
    vmake_error(token->line_content, token->line_in_file, token->char_in_line,
                token->text_len, format, l);
    va_end(l);
}

void debug_log_throw(Token* token, const char* format, ...) {
    va_list l;
    va_start(l, format);
    vmake_debug_message(token->line_content, token->line_in_file, token->char_in_line,
                        token->text_len, format, l);
    va_end(l);
}

typedef struct TokenListWrapper {
    TokenList token_list;
    unsigned int read_pos;
} Cursor;

typedef struct ParsingError {
    enum ExceptionType {
        ERR_UNEXPECTED_TOKEN
    } type;
    char text[1028];
} ParsingError;

Token* cursor_get(Cursor cursor) {
    if (cursor.read_pos >= cursor.token_list.length) return NULL;
    else return &cursor.token_list.tokens[cursor.read_pos];
}

void cursor_advance(Cursor cursor) {
    if (cursor.read_pos < cursor.token_list.length) cursor.read_pos++;
}

typedef struct Declaration {
    Field* field_data;
    Expression* initializer;
} Declaration;

void struct_data_append(StructData* struct_data, Field* field_data) {
    struct_data->value = mrealloc(
        struct_data->value,
        (struct_data->len + 1) * sizeof(struct_data->value[0])
    );
    field_data->offset = struct_data->len;
    struct_data->value[struct_data->len++] = field_data[0];
}

void text_append(Text* text, Statement* field_data) {
    text->statements = mrealloc(
        text->statements,
        (text->len + 1) * sizeof(text->statements[0])
    );
    text->statements[text->len++] = field_data;
}

void add_function_to_table(StringDict* dict, Function* to_add) {
    const char* name = ((struct EntryBase*) to_add)->name;
    NameTable* table = string_dict_get(dict, name);
    if (table == NULL) {
        table = mcalloc(sizeof(struct NameTable));
        table->name = strmdup(name);
        string_dict_put(dict, name, table);
    }
    table->fcount++;
    table->functions = mrealloc(table->functions, table->fcount * sizeof(table->functions[0]));
    table->functions[table->fcount - 1] = to_add;
}

void add_to_table(StringDict* dict, void* to_add) {
    const char* name = ((UnresolvedEntry*) to_add)->name;
    NameTable* table = string_dict_get(dict, name);
    if (table == NULL) {
        table = mcalloc(sizeof(NameTable));
        table->name = strmdup(name);
        string_dict_put(dict, name, table);
    }
    switch (((UnresolvedEntry*) to_add)->entry_type) {
    case ENTRY_FIELD:
        table->variable = to_add;
        break;
    case ENTRY_FUNCTIONS:
        exit(1);
    case ENTRY_MODULE:
        table->container.module = to_add;
        break;
    case ENTRY_NAMESPACE:
        table->container.nspc = to_add;
        break;
    case ENTRY_STRUCT:
        table->type.stct = to_add;
        break;
    case ENTRY_UNION:
        table->type.onion = to_add;
        break;
    }
}

Statement* expression_to_statement(Expression* exp) {
    Statement* stm = mcalloc(sizeof(Statement));
    stm->statement_type = STATEMENT_EXPRESSION;
    stm->pa_expression = exp;
    return stm;
}

void free_exception(ParsingError* err) {
    free(err);
}

#define swap(a, b) do { typeof(a) c = a; a = b; b = c; } while(0)

void prio_err(int* max_matched_token, int* current_matched_token,
              ParsingError** best_match_error, ParsingError** err_wr_buffer) {
    if (*current_matched_token > *max_matched_token) {
        *max_matched_token = *current_matched_token;
        swap(*best_match_error, *err_wr_buffer);
        free_exception(*err_wr_buffer);
    }
}

/**
 * PARSING RULES
 * module:      [decl|struct|union|namespc|func]*
 * name:        ident[.ident]*
 * type:        name[<name[,name]*>]?[\[num?\]]?
 * decl:        type ident [;| = exp;]
 * struct:      "struct" ident[<ident[,ident]*]? { [decl]* }
 * union:       "struct" ident[<ident[,ident]*]? { [type ident]* }
 * namespc:     "namespace" ident { [func|struct|namespc|union]* }
 * exp:         [[op|call|val]|(exp)]
 * val:         [num|str|bool|char|lambda]
 * op:          exp operator exp|prefix-unary exp|exp postfix-unary|exp[\[exp\]]
 * call:        val([exp[,exp]*]?)
 * args:        (type ident[,type ident]*)
 * func:        type args? .ident args [block| = exp;]
 * stm:         ;|block|exp;|if exp stm [else stm]?|while exp stm|
 *              forl|return exp;|do stm while exp
 * forl:        for [decl|exp][,decl|,exp]*;exp[,exp]*;exp[,exp]*stm
 */

Module* parse_module(Cursor l, ParsingError* eptr, int* matched_tokens);
char* parse_name(Cursor l, ParsingError* eptr, int* parsed_tokens);
Type* parse_type(Cursor l, ParsingError* eptr, int* parsed_tokens);
Declaration* parse_declaration(Cursor l, ParsingError* eptr, int* parsed_tokens);
Struct* parse_struct(Cursor l, ParsingError* eptr, int* parsed_tokens);
Union* parse_union(Cursor l, ParsingError* eptr, int* parsed_tokens);
Namespace* parse_namespace(Cursor l, ParsingError* eptr, int* parsed_tokens);
Expression* parse_expression(Cursor l, ParsingError* eptr, int* parsed_tokens);
Expression* parse_value(Cursor l, ParsingError* eptr, int* parsed_tokens);
Expression* parse_operation(Cursor l, ParsingError* eptr, int* parsed_tokens);
Expression* parse_call(Cursor l, ParsingError* eptr, int* parsed_tokens);
StructData* parse_args(Cursor l, ParsingError* eptr, int* parsed_tokens);
Function* parse_function(Cursor l, ParsingError* eptr, int* parsed_tokens);
Statement* parse_statement(Cursor l, ParsingError* eptr, int* parsed_tokens);
Statement* parse_for_loop(Cursor l, ParsingError* eptr, int* parsed_tokens);

Module* parse_module(Cursor l, ParsingError* eptr, int* parsed_tokens) {
    (void) eptr;
    ParsingError e[2];
    int max_matched_token = 0, current_matched_token = 0;
    ParsingError* best_match_error = &e[0];
    ParsingError* err_wr_buffer= &e[1];
    Module* module = mcalloc(sizeof(module));
    for (;;) {
        if (cursor_get(l) == NULL) {
            *parsed_tokens = 0; // fix later
            return module;
        }
        union {
            Declaration* decl;
            UnresolvedEntry* base;
            Struct* stct;
            Union* onion;
            Namespace* nspc;
            Function* func;
        } parse_result;
        if ((parse_result.decl = parse_declaration(l, err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            struct_data_append(&module->globals, parse_result.decl->field_data);
            add_to_table(&module->scope, parse_result.decl->field_data);
            text_append(&module->text, expression_to_statement(parse_result.decl->initializer));
        }
        else if ((parse_result.stct = parse_struct(l, err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            add_to_table(&module->scope, parse_result.base);
        }
        else if ((parse_result.onion = parse_union(l, err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            add_to_table(&module->scope, parse_result.base);
        }
        else if ((parse_result.nspc = parse_namespace(l, err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            add_to_table(&module->scope, parse_result.base);
        }
        else if ((parse_result.func = parse_function(l, err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            add_function_to_table(&module->scope, parse_result.func);
        }
        else {
            free_ast(NULL, module);
            return NULL;
        }
    }
}

Module* parse(TokenList tokens) {

}