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
    struct ParsingError* caused_by;
    char text[0];
} ParsingError;

Token* cursor_get(Cursor cursor) __attribute_const__;
Token* cursor_get(Cursor cursor) {
    if (cursor.read_pos >= cursor.token_list.length) return NULL;
    else return &cursor.token_list.tokens[cursor.read_pos];
}

void cursor_advance(Cursor* cursor) {
    if (cursor->read_pos < cursor->token_list.length) cursor->read_pos++;
}

bool cursor_assert_type(Cursor cursor, TokenType t) __attribute_const__;
bool cursor_assert_type(Cursor cursor, TokenType t) {
    return (cursor_get(cursor) != NULL && cursor_get(cursor)->type == t);
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
        table->name = mstrdup(name);
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
        table->name = mstrdup(name);
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
    }
    free_exception(*err_wr_buffer);
}

ParsingError* make_parsing_error(ParsingError* caused_by, char* m, ...) {
    va_list l;
    va_start(l, m);
    unsigned int len = vsnprintf(NULL, 0, m, l);
    va_end(l);
    ParsingError* e = malloc(sizeof(ParsingError) + len + 1);
    va_start(l, m);
    vsnprintf(e->text, 0, m, l);
    va_end(l);
    e->caused_by = caused_by;
    return e;
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

Module* parse_module(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
char* parse_name(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Type* parse_type(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Declaration* parse_declaration(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Struct* parse_struct(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Union* parse_union(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Namespace* parse_namespace(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Expression* parse_expression(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Expression* parse_value(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Expression* parse_operation(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Expression* parse_call(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
StructData* parse_args(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Function* parse_function(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Statement* parse_statement(Cursor* lptr, ParsingError** eptr, int* matched_tokens);
Statement* parse_for_loop(Cursor* lptr, ParsingError** eptr, int* matched_tokens);

/**
 * @brief module:[decl|struct|union|namespc|func|stm]*
 * 
 * @param lptr 
 * @param eptr 
 * @param matched_tokens 
 * @return Module* 
 */
Module* parse_module(Cursor* lptr, ParsingError** eptr, int* matched_tokens) {
    (void) eptr;
    Cursor l = *lptr;
    int max_matched_token = 0, current_matched_token = 0;
    ParsingError* best_match_error = NULL;
    ParsingError* err_wr_buffer = NULL;
    Module* module = mcalloc(sizeof(module));
    for (;;) {
        if (cursor_get(l) == NULL) {
            cursor_advance(&l);
            *matched_tokens = l.read_pos - lptr->read_pos;
            *lptr = l;
            return module;
        }
        union {
            Declaration* decl;
            UnresolvedEntry* base;
            Struct* stct;
            Union* onion;
            Namespace* nspc;
            Function* func;
            Statement* stm;
        } parse_result;
        if ((parse_result.decl = parse_declaration(&l, &err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            struct_data_append(&module->globals, parse_result.decl->field_data);
            add_to_table(&module->scope, parse_result.decl->field_data);
            text_append(&module->text, expression_to_statement(parse_result.decl->initializer));
        }
        else if ((parse_result.stct = parse_struct(&l, &err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            add_to_table(&module->scope, parse_result.base);
        }
        else if ((parse_result.onion = parse_union(&l, &err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            add_to_table(&module->scope, parse_result.base);
        }
        else if ((parse_result.nspc = parse_namespace(&l, &err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            add_to_table(&module->scope, parse_result.base);
        }
        else if ((parse_result.func = parse_function(&l, &err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            add_function_to_table(&module->scope, parse_result.func);
        }
        else if ((parse_result.stm = parse_statement(&l, &err_wr_buffer, &current_matched_token))) {
            prio_err(&max_matched_token, &current_matched_token, &best_match_error, &err_wr_buffer);
            text_append(&module->text, parse_result.stm);
        }
        else {
            free_ast(NULL, module);
            *eptr = best_match_error;
            return NULL;
        }
    }
}

char* parse_name(Cursor* lptr, ParsingError** eptr, int* matched_tokens) {
    Cursor l = *lptr;
    Module* module = mcalloc(sizeof(module));
    if (!cursor_assert_type(l, IDENTIFIER_TOKEN)) {
        *eptr = make_parsing_error(NULL, "expected an identifier");
        return NULL;
    }
    char* ret_val = ident_dot_seq_append(NULL, cursor_get(l)->identifier);
    cursor_advance(&l);
    for (;;) {
        *matched_tokens = l.read_pos - lptr->read_pos;
        *lptr = l;
        if (!cursor_assert_type(l, DOT_TOKEN)) {
            return ret_val;
        }
        cursor_advance(&l);
        if (!cursor_assert_type(l, IDENTIFIER_TOKEN)) {
            *eptr = make_parsing_error(NULL, "expected an identifier");
            return NULL;
        }
        cursor_advance(&l);
        ret_val = ident_dot_seq_append(ret_val, cursor_get(l)->identifier);
    }
}

Type* parse_type(Cursor* lptr, ParsingError** eptr, int* matched_tokens) {
    Cursor l = *lptr;
    int current_matched_token = 0;
    ParsingError* err_wr_buffer;
    char* name;
    if (!(name = parse_name(&l, &err_wr_buffer, &current_matched_token))) {
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(err_wr_buffer, "expected a name");
        return NULL;
    }
    Type* t = malloc(sizeof(Type));
    t->name = name;
    cursor_advance(&l);
    if (!cursor_assert_type(l, OPERATOR_TOKEN) ||
        cursor_get(l)->operator_type != SMALLER_THAN_OPERATOR) {
        *matched_tokens = l.read_pos - lptr->read_pos;
        *lptr = l;
        return t;
    }
    cursor_advance(&l);
    Type* generic;
    if (!(generic = parse_type(&l, &err_wr_buffer, &current_matched_token))) {
        free_type(t);
        *matched_tokens = l.read_pos - lptr->read_pos + current_matched_token;
        *eptr = make_parsing_error(err_wr_buffer, "expected a type");
        return NULL;
    }
    generic->generics.types = realloc(generic->generics.types, sizeof(Type*) * (generic->generics.len + 1));
    generic->generics.types[generic->generics.len++] = generic;
    cursor_advance(&l);
    for (;;) {
        if (!cursor_assert_type(l, SEPERATOR_TOKEN)) {
            break;
        }
        cursor_advance(&l);
        if (!(generic = parse_type(&l, &err_wr_buffer, &current_matched_token))) {
            free_type(t);
            *matched_tokens = l.read_pos - lptr->read_pos + current_matched_token;
            *eptr = make_parsing_error(err_wr_buffer, "expected a type");
            return NULL;
        }
        generic->generics.types = realloc(generic->generics.types, sizeof(Type*) * (generic->generics.len + 1));
        generic->generics.types[generic->generics.len++] = generic;
    }
    if (!cursor_assert_type(l, OPERATOR_TOKEN) ||
        cursor_get(l)->operator_type != GREATER_THAN_OPERATOR) {
        free_type(t);
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(err_wr_buffer, "expected a , or >");
        return NULL;
    }
    *matched_tokens = l.read_pos - lptr->read_pos;
    *lptr = l;
    return t;
}

/**
 * @brief decl: type ident [;| = exp;]
 * 
 * @param lptr 
 * @param eptr 
 * @param matched_tokens 
 * @return Declaration* 
 */
Declaration* parse_declaration(Cursor* lptr, ParsingError** eptr, int* matched_tokens) {
    Cursor l = *lptr;
    int current_matched_token = 0;
    ParsingError* err_wr_buffer;
    Type* type = parse_type(&l, &err_wr_buffer, &current_matched_token);
    if (!type) {
        *matched_tokens = l.read_pos - lptr->read_pos + current_matched_token;
        *eptr = make_parsing_error(err_wr_buffer, "expected a type");
        return NULL;
    }
    if (!cursor_assert_type(l, IDENTIFIER_TOKEN)) {
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(err_wr_buffer, "expected an identifier");
        return NULL;
    }
    char* name = cursor_get(l)->identifier;
    cursor_advance(&l);
    if (cursor_assert_type(l, END_TOKEN)) {
        Declaration* decl = malloc(sizeof(Declaration));
        decl->field_data = malloc(sizeof(Field));
        *decl->field_data = (Field) {
            .meta = {
                .entry_type = ENTRY_FIELD,
                .name = mstrdup(name)
            },
            .type = type
        };
        decl->initializer = NULL;
        return decl;
    }
    else if (!cursor_assert_type(l, ASSIGN_TOKEN)) {
        free_type(type);
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(err_wr_buffer, "expected a = or ;");
        return NULL;
    }
    Expression* exp = parse_expression(&l, &err_wr_buffer, &current_matched_token);
    if (exp == NULL) {
        *matched_tokens = l.read_pos - lptr->read_pos + current_matched_token;
        *eptr = make_parsing_error(err_wr_buffer, "expected an expression");
        return NULL;
    }
    Declaration* decl = malloc(sizeof(Declaration));
    decl->field_data = malloc(sizeof(Field));
    *decl->field_data = (Field) {
        .meta = {
            .entry_type = ENTRY_FIELD,
            .name = mstrdup(name)
        },
        .type = type
    };
    decl->initializer = exp;
    return decl;
}

/**
 * @brief struct: "struct" ident[<ident[,ident]*]? { [decl]* }
 * 
 * @param lptr 
 * @param eptr 
 * @param matched_tokens 
 * @return Struct* 
 */
Struct* parse_struct(Cursor* lptr, ParsingError** eptr, int* matched_tokens) {
    Cursor l = *lptr;
    int current_matched_token = 0;
    ParsingError* err_wr_buffer;
    if (!cursor_assert_type(l, K_STRUCT_TOKEN)) {
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(NULL, "expected \"struct\" keyword");
        return NULL;
    }
    cursor_advance(&l);
    if (!cursor_assert_type(l, IDENTIFIER_TOKEN)) {
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(NULL, "expected an identifier");
        return NULL;
    }
    char* name = cursor_get(l)->identifier;
    cursor_advance(&l);
    if (!cursor_assert_type(l, OPEN_BLOCK_TOKEN)) {
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(NULL, "expected {");
        return NULL;
    }
    cursor_advance(&l);
    Struct* stct = malloc(sizeof(Struct));
    *stct = (Struct) {
        .meta = {
            .entry_type = ENTRY_STRUCT,
            .name = mstrdup(name),
        },
        .data = {0}
    };
    for (;;) {
        Declaration* decl = parse_declaration(&l, &err_wr_buffer, &current_matched_token);
        if (decl == NULL) {
            free_exception(err_wr_buffer);
            break;
        }
        struct_data_append(&stct->data, decl->field_data);
        free_expression(decl->initializer);
        free(decl);
    }
    if (!cursor_assert_type(l, CLOSE_BLOCK_TOKEN)) {
        free_struct(stct);
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(NULL, "expected }");
        return NULL;
    }
    cursor_advance(&l);
    *matched_tokens = l.read_pos - lptr->read_pos;
    *lptr = l;
    return stct;
}

/**
 * @brief 
 * 
 * @param lptr 
 * @param eptr 
 * @param matched_tokens 
 * @return Union* 
 */
Union* parse_union(Cursor* lptr, ParsingError** eptr, int* matched_tokens) {
    Cursor l = *lptr;
    int current_matched_token = 0;
    ParsingError* err_wr_buffer;
    if (!cursor_assert_type(l, K_STRUCT_TOKEN)) {
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(NULL, "expected \"struct\" keyword");
        return NULL;
    }
    cursor_advance(&l);
    if (!cursor_assert_type(l, IDENTIFIER_TOKEN)) {
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(NULL, "expected an identifier");
        return NULL;
    }
    char* name = cursor_get(l)->identifier;
    cursor_advance(&l);
    if (!cursor_assert_type(l, OPEN_BLOCK_TOKEN)) {
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(NULL, "expected {");
        return NULL;
    }
    cursor_advance(&l);
    Union* onion = malloc(sizeof(Union));
    *onion = (Union) {
        .meta = {
            .entry_type = ENTRY_STRUCT,
            .name = mstrdup(name),
        },
        .data = {0}
    };
    for (;;) {
        Type* type = parse_type(&l, &err_wr_buffer, &current_matched_token);
        if (type == NULL) {
            free_exception(err_wr_buffer);
            break;
        }
        if (!cursor_assert_type(l, IDENTIFIER_TOKEN)) {
            free_type(type);
            free_union(onion);
            *matched_tokens = l.read_pos - lptr->read_pos;
            *eptr = make_parsing_error(NULL, "expected an identifier");
            return NULL;
        }
        cursor_advance(&l);
        char* name = cursor_get(l)->identifier;
        Field* field = malloc(sizeof(Field));
        *field = (Field) {
            .meta = {
                .entry_type = ENTRY_FIELD,
                .name = name
            },
            .type = type
        };
        struct_data_append(&onion->data, field);
        if (!cursor_assert_type(l, END_TOKEN)) {
            free_type(type);
            free_union(onion);
            *matched_tokens = l.read_pos - lptr->read_pos;
            *eptr = make_parsing_error(NULL, "expected an ;");
            return NULL;
        }
        cursor_advance(&l);
    }
    if (!cursor_assert_type(l, CLOSE_BLOCK_TOKEN)) {
        free_union(onion);
        *matched_tokens = l.read_pos - lptr->read_pos;
        *eptr = make_parsing_error(NULL, "expected }");
        return NULL;
    }
    cursor_advance(&l);
    *matched_tokens = l.read_pos - lptr->read_pos;
    *lptr = l;
    return onion;
}

Module* parse(TokenList tokens) {
    Cursor c = {
        .read_pos = 0,
        .token_list = tokens
    };
    ParsingError* err;
    int matched_tokens;
    Module* out = parse_module(&c, &err, &matched_tokens);
    free_exception(err);
    return out;
}