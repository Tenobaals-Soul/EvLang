#include <string_dict.h>
#include <tokenizer.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <parser.h>
#include <stdbool.h>

bool debug_run = false;

static char enviroment[256] = "<unspecified enviroment>";

void set_enviroment(const char *new_enviroment) {
    if (strlen(new_enviroment) > 255) {
        strncpy(enviroment, new_enviroment, 252);
        strcat(enviroment, "...");
    }
    else {
        strcpy(enviroment, new_enviroment);
    }
}

const char* get_enviroment() {
    return enviroment;
}

void message_internal(const char* color_code, const char* line, const char* source,
                      unsigned int line_no, unsigned int char_no, unsigned int len,
                      const char* message, va_list l) {
    printf("%s %u:%u %s%s%s: ", enviroment, line_no, char_no + 1, color_code, source, "\033[0m");
    vprintf(message, l);
    printf("\n");
    unsigned int print_len = 0;
    printf("%4d | ", line_no);
    unsigned int i = 0;
    for (; line[i] && line[i] != '\n' && i < char_no; i++) {
        if (!iscntrl(line[i])) {
            putchar(line[i]);
            print_len += (isprint(line[i]) || line[i] & (1 << 6));
        }
    }
    printf("%s", color_code);
    for (; line[i] && line[i] != '\n' && i < char_no + len; i++) {
        if (!iscntrl(line[i])) {
            putchar(line[i]);
            print_len += (isprint(line[i]) || line[i] & (1 << 6));
        }
    }
    printf("\033[0m");
    for (; line[i] && line[i] != '\n'; i++) {
        if (!iscntrl(line[i])) {
            putchar(line[i]);
        }
    }
    if (print_len == char_no) {
        print_len++;
    }
    putchar('\n');
    if (char_no <= print_len) {
        printf("     | ");
        for (unsigned int i = 0; i < char_no; i++) {
            putchar(' ');
        }
        printf("%s", color_code);
        for (unsigned int i = char_no; i < print_len; i++) {
            putchar('^');
        }
        printf("\033[0m\n");
    }
    fflush(stdout);
}

char* ident_dot_seq_append(char* seq, char* name) {
    unsigned int len = 0, nlen = strlen(name);
    for (; seq[len] || seq[len + 1]; len++);
    seq = mrealloc(seq, len + nlen + 3);
    seq[len] = 0;
    memcpy(&seq[len] + 1, name, nlen);
    seq[len + nlen + 1] = 0;
    seq[len + nlen + 2] = 0;
    return seq;
}

void ident_dot_seq_print(char *seq) {
    int len;
    for (len = 0; seq[len] || seq[len + 1]; len++);
    char *buffer = mmalloc(len + 1);
    for (int i = 0; i < len; i++) {
        buffer[i] = seq[i] ? seq[i] : '.';
    }
    buffer[len] = 0;
    printf("%s", buffer);
    mfree(buffer);
}

UnresolvedEntry* get_from_ident_dot_seq(StringDict* src, const char* seq, TokenList* tokens,
                                        int token_index, bool throw) {
    const char *env = enviroment;
    const char *acs = seq;
    UnresolvedEntry* found = NULL;
    int word = 0;
    for (int i = 0; acs[i]; word++, i++) {
        found = src ? string_dict_get(src, acs + i) : NULL;
        if (found == NULL) {
            if (throw) {
                if (!src || src->count == 0) {
                    Token *t = &tokens->tokens[token_index + word * 2];
                    make_error(t->line_content, t->line_in_file, t->char_in_line,
                            t->text_len, "%s has no members", env);
                }
                else {
                    Token *t = &tokens->tokens[token_index + word * 2];
                    make_error(t->line_content, t->line_in_file, t->char_in_line,
                            t->text_len, "%s is not defined", env, acs + i);
                }
            }
            return NULL;
        }
        env = seq;
        for (; acs[i]; i++);
        if (found->entry_type == ENTRY_NAMESPACE) {
            src = &((Namespace*) found)->scope;
        }
        else if (found->entry_type == ENTRY_MODULE) {
            src = &((Module*) found)->scope;
        }
    }
    return found;
}

void vmake_error(const char *line, unsigned int line_no, unsigned int char_no,
                 unsigned int len, const char *error_message, va_list l) {
    message_internal("\033[31;1m", line, "error", line_no, char_no, len, error_message, l);
}

void vmake_warning(const char *line, unsigned int line_no, unsigned int char_no,
                  unsigned int len, const char *warning_message, va_list l) {
    message_internal("\033[93m", line, "warning", line_no, char_no, len, warning_message, l);
}

void vmake_debug_message(const char *line, unsigned int line_no, unsigned int char_no,
                  unsigned int len, const char *warning_message, va_list l) {
    message_internal("\033[36m", line, "<DEBUG>", line_no, char_no, len, warning_message, l);
}

void make_error(const char *line, unsigned int line_no, unsigned int char_no,
                unsigned int len, const char *error_message, ...) {
    va_list l;
    va_start(l, error_message);
    vmake_error(line, line_no, char_no, len, error_message, l);
    va_end(l);
}

void make_warning(const char *line, unsigned int line_no, unsigned int char_no,
                  unsigned int len, const char *warning_message, ...) {
    va_list l;
    va_start(l, warning_message);
    vmake_warning(line, line_no, char_no, len, warning_message, l);
    va_end(l);
}

void make_debug_message(const char *line, unsigned int line_no, unsigned int char_no,
                  unsigned int len, const char *warning_message, ...) {
    va_list l;
    va_start(l, warning_message);
    vmake_warning(line, line_no, char_no, len, warning_message, l);
    va_end(l);
}

void print_fixed_value(struct fixed_value val) {
    switch (val.type) {
    case STRING:
        printf("\033[32m\"%s\"\033[0m", val.value.string);
        break;
    case CHARACTER:
        printf("\033[32m\'%s\'\033[0m", val.value.string);
        break;
    case INTEGER:
        printf("\033[32m%llu\033[0m", (unsigned long long int) val.value.integer);
        break;
    case FLOATING:
        printf("\033[32m%Lf\033[0m", val.value.floating);
        break;
    case BOOLEAN:
        printf("\033[32m%s\033[0m", val.value.boolean ? "true" : "false");
        break;
    default:
        printf("detected fatal internal error - error type detected - %d\n", __LINE__);
        exit(1);
    }
}

void print_operator(BasicOperator operator) {
    switch (operator) {
    case ADD_OPERATOR:
        printf("+");
        break;
    case SUBTRACT_OPERATOR:
        printf("-");
        break;
    case MULTIPLY_OPERATOR:
        printf("*");
        break;
    case DIVIDE_OPERATOR:
        printf("/");
        break;
    case RIGHT_SHIFT_OPERATOR:
        printf(">>");
        break;
    case LEFT_SHIFT_OPERATOR:
        printf("<<");
        break;
    case BINARY_OR_OPERATOR:
        printf("|");
        break;
    case BINARY_AND_OPERATOR:
        printf("&");
        break;
    case BINARY_XOR_OPERATOR:
        printf("^");
        break;
    case BINARY_NOT_OPERATOR:
        printf("~");
        break;
    case EQUALS_OPERATOR:
        printf("==");
        break;
    case SMALLER_THAN_OPERATOR:
        printf("<");
        break;
    case SMALLER_EQUAL_OPERATOR:
        printf("<=");
        break;
    case GREATER_THAN_OPERATOR:
        printf(">");
        break;
    case GREATER_EQUAL_OPERATOR:
        printf(">=");
        break;
    case NOT_EQUAL_OPERATOR:
        printf("!=");
        break;
    case BOOL_OR_OPERATOR:
        printf("or");
        break;
    case BOOL_AND_OPERATOR:
        printf("and");
        break;
    case BOOL_NOT_OPERATOR:
        printf("not");
        break;
    case INPLACE_ADD_OPERATOR:
        printf("&=");
        break;
    case INPLACE_SUBTRACT_OPERATOR:
        printf("-=");
        break;
    case INPLACE_MULTIPLY_OPERATOR:
        printf("*=");
        break;
    case INPLACE_DIVIDE_OPERATOR:
        printf("/=");
        break;
    case INPLACE_POW_OPERATOR:
        printf("^=");
        break;
    case INPLACE_AND_OPERATOR:
        printf("&=");
        break;
    case INPLACE_OR_OPERATOR:
        printf("|=");
        break;
    case INPLACE_RSHIFT_OPERAOR:
        printf(">>=");
        break;
    case INPLACE_LSHIFT_OPERATOR:
        printf("<<=");
        break;
    case INCREMENT_OPERATOR:
        printf("++");
        break;
    case DECREMENT_OPERATOR:
        printf("--");
        break;
    default:
        printf("detected fatal internal error - error type %d detected - %d\n", operator, __LINE__);
        exit(1);
    }
}

void print_tokens(TokenList l) {
    for (unsigned long i = 0; i < l.length; i++) {
        if (i)
            printf(" ");
        switch (l.tokens[i].type) {
        case END_TOKEN:
            printf(";");
            break;
        case K_NAMESPACE_TOKEN:
            printf("\033[94m%s\033[0m", "namespace");
            break;
        case K_PRIVATE_TOKEN:
            printf("\033[94m%s\033[0m", "private");
            break;
        case K_PROTECTED_TOKEN:
            printf("\033[94m%s\033[0m", "protected");
            break;
        case K_PUBLIC_TOKEN:
            printf("\033[94m%s\033[0m", "public");
            break;
        case K_IMPLEMENTS_TOKEN:
            printf("\033[94m%s\033[0m", "implements");
            break;
        case K_STRUCT_TOKEN:
            printf("\033[94m%s\033[0m", "struct");
            break;
        case K_UNION_TOKEN:
            printf("\033[94m%s\033[0m", "union");
            break;
        case K_EXTENDS_TOKEN:
            printf("\033[94m%s\033[0m", "extends");
            break;
        case C_BREAK_TOKEN:
            printf("\033[94m%s\033[0m", "break");
            break;
        case C_FOR_TOKEN:
            printf("\033[94m%s\033[0m", "for");
            break;
        case C_IF_TOKEN:
            printf("\033[94m%s\033[0m", "if");
            break;
        case C_ELSE_TOKEN:
            printf("\033[94m%s\033[0m", "else");
            break;
        case C_RETURN_TOKEN:
            printf("\033[94m%s\033[0m", "return");
            break;
        case C_WHILE_TOKEN:
            printf("\033[94m%s\033[0m", "while");
            break;
        case IDENTIFIER_TOKEN:
            printf("%s", l.tokens[i].identifier);
            break;
        case FIXED_VALUE_TOKEN:
            print_fixed_value(l.tokens[i].fixed_value);
            break;
        case OPERATOR_TOKEN:
            print_operator(l.tokens[i].operator_type);
            break;
        case OPEN_PARANTHESIS_TOKEN:
            printf("(");
            break;
        case CLOSE_PARANTHESIS_TOKEN:
            printf(")");
            break;
        case OPEN_BLOCK_TOKEN:
            printf("{");
            break;
        case CLOSE_BLOCK_TOKEN:
            printf("}");
            break;
        case OPEN_INDEX_TOKEN:
            printf("[");
            break;
        case CLOSE_INDEX_TOKEN:
            printf("]");
            break;
        case SEPERATOR_TOKEN:
            printf(",");
            break;
        case DOT_TOKEN:
            printf(".");
            break;
        case ASSIGN_TOKEN:
            printf("=");
            break;
        default:
            printf("detected fatal internal error - error type detected - %d\n", __LINE__);
            exit(1);
        }
    }
    printf("\n");
}

void free_tokens(TokenList token_list) {
    for (unsigned long j = 0; j < token_list.length; j++) {
        Token *t = token_list.tokens + j;
        if (t->type == IDENTIFIER_TOKEN) {
            mfree(t->identifier);
        }
        else if (t->type == FIXED_VALUE_TOKEN && (t->fixed_value.type == STRING || t->fixed_value.type == CHARACTER)) {
            mfree(t->fixed_value.value.string);
        }
    }
    mfree(token_list.tokens);
}

void print_ast_internal2(Expression* exp) {
    if (exp == NULL) {
        printf("?");
        return;
    }
    switch (exp->expression_type) {
    case EXPRESSION_CALL:
        if (exp->pa_call->call == NULL) {
            printf("(unknown function)");
            break;
        }
        printf("%s(", exp->pa_call->call->name);
        unsigned int i;
        if (exp->pa_call->arg_count) {
            for (i = 0; i < exp->pa_call->arg_count; i++) {
                print_ast_internal2(exp->pa_call->args[i]);
                printf(", ");
            }
            print_ast_internal2(exp->pa_call->args[i]);
        }
        printf(")");
        break;
    case EXPRESSION_FIXED_VALUE:
        print_fixed_value(*exp->pa_fixed_value);
        break;
    case EXPRESSION_OPERATOR:
        printf("(");
        print_ast_internal2(exp->pa_operator->left);
        printf(") ");
        print_operator(exp->pa_operator->operator);
        printf(" (");
        print_ast_internal2(exp->pa_operator->right);
        printf(")");
        break;
    case EXPRESSION_VAR:
        printf("%s", exp->pa_variable->meta.name ?
               exp->pa_variable->meta.name : "error-var");
        break;
    case EXPRESSION_UNARY_OPERATOR:
        print_operator(exp->pa_operator->operator);
        printf("(");
        print_ast_internal2(exp->pa_operator->left);
        printf(")");
        break;
    case EXPRESSION_ARR_ACCESS:
        print_ast_internal2(exp->pa_arr_access->from);
        printf("[");
        print_ast_internal2(exp->pa_arr_access->key);
        printf("]");
        break;
    case EXPRESSION_ASSIGN:
        printf("(");
        print_ast_internal2(exp->pa_operator->left);
        printf(") = (");
        print_ast_internal2(exp->pa_operator->right);
        printf(")");
        break;
    }
}

void print_statement(Statement* st) {
    switch (st->statement_type) {
        case STATEMENT_EXPRESSION:
            print_ast_internal2(st->pa_expression);
            break;
        case STATEMENT_FOR:
            printf("for // no more debug information supported");
            break;
        case STATEMENT_IF:
            printf("if // no more debug information supported");
            break;
        case STATEMENT_RETURN:
            printf("return // no more debug information supported");
            break;
        case STATEMENT_WHILE:
            break;
    }
}

void free_ast_expression(Expression* exp);
void print_text(Text text) {
    (void) text;
}

void print_struct_data(StructData struct_data, const char* indent) {
    printf("%s{\n", indent);
    for (unsigned int i = 0; i < struct_data.len; i++) {
        printf("    %s%s %s", indent, struct_data.value[i].type->name, struct_data.value[i].meta.name);
    }
    printf("%s}\n", indent);
}

void print_type(Type* t) {
    printf("%s", t->name);
    if (t->generics.len) {
        printf("<");
        print_type(t->generics.types[0]);
        for (unsigned int i = 1; i < t->generics.len; i++) {
            printf(", ");
            print_type(t->generics.types[i]);
        }
        printf(">");
    }
}

void print_args(StructData arguments) {
    unsigned int i = 0;
    printf("(");
    if (arguments.len) {
        print_type(arguments.value[i].type);
        printf(" %s", arguments.value[i].meta.name);
        for (i = 0; i < arguments.len - 1; i++) {
            print_type(arguments.value[i].type);
            printf(", %s", arguments.value[i].meta.name);
        }
    }
    printf(")");
}

void print_ast_method(Function* function, char* indent) {
    printf("%s ", indent);
    print_type(function->return_type);
    if (function->arguments[0].len) {
        printf(" ");
        print_args(function->arguments[0]);
    }
    printf(" %s", function->name);
    print_args(function->arguments[1]);
    print_struct_data(function->stack_data, indent);
    printf("with code:%s\n", function->text.len ? "" : " NONE");
    for (unsigned int i = 0; i < function->text.len; i++) {
        printf("%s    ", indent);
        print_text(function->text);
        printf(";\n");
    }
}

void print_ast_internal(void *env, const char *name, void *val) {
    int layer = *((int *) env);
    char indent[layer * 4 + 1];
    memset(indent, ' ', layer * 4);
    indent[layer * 4] = 0;
    UnresolvedEntry* entry = val;
    switch (entry->entry_type) {
    case ENTRY_MODULE:;
        Module* module = (Module*) entry;
        printf("module %s with %d entrys:\n", name, module->scope.count);
        layer++;
        string_dict_complex_foreach(&module->scope, print_ast_internal, &layer);
        break;
    case ENTRY_STRUCT:;
        Struct* struct_entry = (Struct*) entry;
        printf("%sstruct %s with %d entrys:\n", indent, name, struct_entry->data.len);
        for (unsigned int i = 0; i < struct_entry->data.len; i++) {
            print_ast_internal(&layer, struct_entry->data.value[i].meta.name, &struct_entry->data.value[i]);
        }
        break;
    case ENTRY_UNION:;
        Struct* union_entry = (Struct*) entry;
        printf("%sunion %s with %d entrys:\n", indent, name, union_entry->data.len);
        for (unsigned int i = 0; i < union_entry->data.len; i++) {
            print_ast_internal(&layer, union_entry->data.value[i].meta.name, &union_entry->data.value[i]);
        }
        break;
    case ENTRY_NAMESPACE:;
        Namespace* namespace = (Namespace*) entry;
        printf("%sunion %s with %d entrys:\n", indent, name, namespace->struct_data.len + namespace->scope.count);
        for (unsigned int i = 0; i < namespace->struct_data.len; i++) {
            print_ast_internal(&layer, NULL, &namespace->struct_data.value[i]);
        }
        layer++;
        string_dict_complex_foreach(&namespace->scope, print_ast_internal, &layer);
        break;
    case ENTRY_FUNCTIONS:;
        NameTable* table = (NameTable*) entry;
        printf("%smethod %s with %d overloaded variant(s):\n", indent, name, table->fcount);
        layer++;
        for (unsigned int i = 0; i < table->fcount; i++) {
            print_ast_method(table->functions[i], indent);
        }
        break;
    case ENTRY_FIELD:;
        Field* field = (Field*) entry;
        printf("%s    ", indent);
        print_type(field->type);
        printf(" %s\n", name);
        break;
    }
}

void print_ast(StringDict *dict) {
    int layer = 0;
    string_dict_complex_foreach(dict, print_ast_internal, &layer);
}

void free_ast_expression(Expression* exp) {
    (void) exp;
}

void free_ast_statements(Text* st) {
    for (uint32_t i = 0; i < st->len; i++) {
        switch (st->statements[i]->statement_type) {
            case STATEMENT_EXPRESSION:
                free_ast_expression(st->statements[i]->pa_expression);
                break;
            case STATEMENT_FOR:
                free_ast_expression(st->statements[i]->pa_for->condition);
                free_ast_statements(&st->statements[i]->pa_for->first);
                free_ast_statements(&st->statements[i]->pa_for->last);
                free_ast_statements(&st->statements[i]->pa_for->text);
                break;
            case STATEMENT_IF:
                free_ast_expression(st->statements[i]->pa_if->condition);
                free_ast_statements(&st->statements[i]->pa_if->on_true);
                free_ast_statements(&st->statements[i]->pa_if->on_false);
                break;
            case STATEMENT_RETURN:
                free_ast_expression(st->statements[i]->pa_return);
                break;
            case STATEMENT_WHILE:
                free_ast_expression(st->statements[i]->pa_while->condition);
                free_ast_statements(&st->statements[i]->pa_while->text);
                break;
        }
        mfree(st->statements);
    }
}

void free_type(Type* t) {
    for (unsigned int i = 0; i < t->generics.len; i++) free_type(t->generics.types[i]);
    mfree(t->name);
}

void free_struct_data(StructData struct_data) {
    for (uint32_t i = 0; i < struct_data.len; i++) {
        mfree(struct_data.value[i].meta.name);
        free_type(struct_data.value[i].type);
    }
}

void free_ast(const char *key, void *val) {
    (void) key;
    UnresolvedEntry* entry = val;
    switch (entry->entry_type) {
    case ENTRY_MODULE:;
        Module* module = (Module*) entry;
        string_dict_foreach(&module->scope, free_ast);
        string_dict_destroy(&module->scope);
        free_ast_statements(&module->text);
        break;
    case ENTRY_NAMESPACE:;
        Namespace* namespace = (Namespace*) entry;
        string_dict_foreach(&namespace->scope, free_ast);
        string_dict_destroy(&namespace->scope);
        for (unsigned int i = 0; i < namespace->struct_data.len; i++) {
            free_ast(NULL, &namespace->struct_data.value[i]);
        }
        break;
    case ENTRY_STRUCT:;
        Struct* struct_entry = (Struct*) entry;
        for (unsigned int i = 0; i < struct_entry->data.len; i++) {
            free_type(struct_entry->data.value[i].type);
            mfree(struct_entry->data.value[i].meta.name);
        }
        break;
    case ENTRY_UNION:;
        Union* union_entry = (Union*) entry;
        for (unsigned int i = 0; i < union_entry->data.len; i++) {
            free_type(union_entry->data.value[i].type);
            mfree(union_entry->data.value[i].meta.name);
        }
        break;
    case ENTRY_FUNCTIONS:;
        NameTable* methods = (NameTable*) entry;
        for (uint32_t i = 0; i < methods->fcount; i++) {
            Function* func = methods->functions[i];
            free_ast_statements(&func->text);
            free_struct_data(func->arguments[0]);
            free_struct_data(func->arguments[1]);
            free_struct_data(func->stack_data);
            free_type(func->return_type);
            mfree(func->name);
        }
        break;
    case ENTRY_FIELD:;
        Field* field = (Field*) entry;
        free_type(field->type);
        break;
    }
    if (entry->name)
        mfree(entry->name);
    mfree(entry);
}

char *fmalloc(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        return NULL;
    off_t len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char *file_content = mmalloc(len + 1);
    if (file_content == NULL)
        return NULL;
    ssize_t r = read(fd, file_content, len);
    if (r < 0) {
        mfree(file_content);
        return NULL;
    }
    close(fd);
    file_content[len] = 0;
    return file_content;
}

void verify_files(int argc, char** argv) {
    bool exit_err = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--internal-debug") == 0) {
            debug_run = true;
            continue;
        }
        if (access(argv[i], R_OK)) {
            printf("file \"%s\" could not be found\n", argv[i]);
            exit_err = true;
        }
        int dots_found = 0;
        if (!isalpha(argv[i][0])) {
            exit_err = true;
        }
        else {
            for (int j = 1; argv[i][j]; j++) {
                if (isalnum(argv[i][j]))
                    continue;
                else if (argv[i][j] == '.') {
                    if (dots_found > 0) {
                        printf("filename \"%s\" contains more than one dot", argv[i]);
                        exit_err = true;
                        break;
                    }
                    dots_found++;
                }
                else {
                    printf("filename \"%s\" contains an illegal character", argv[i]);
                    exit_err = true;
                    break;
                }
            }
        }
    }
    if (exit_err)
        exit(0);
}

int strcreplace(char* str, char search, char replace, int len) {
    for (int i = 0; i < len; i++) {
        if (str[i] == search) str[i] = replace;
        if (str[i] == '\0') return i;
    }
    return len;
}

UnresolvedEntry* pack(Module* module, char* path) {
    char* name = path;
    for (; *path; path++) {
        if (*path == '/') {
            name = path + 1;
        }
    }
    char* end = path;
    for (path = name; *path; path++) {
        if (*path == '.') {
            end = path;
            break;
        }
    }
    unsigned long len = (unsigned long) end - (unsigned long) name;
    module->meta.name = mmalloc(len + 1);
    memcpy(module->meta.name, name, len);
    module->meta.name[len] = 0;
    return &module->meta;
}

int main(int argc, char **argv) {
    verify_files(argc, argv);
    StringDict general_identifier_dict;
    string_dict_init(&general_identifier_dict);
    char *file_contents[argc - 1];
    TokenList *token_lists = mcalloc(sizeof(TokenList) * (argc - 1));
    bool has_errors = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--internal-debug") == 0) continue;
        if (string_dict_get(&general_identifier_dict, argv[i]))
            continue;
        char *file_content = fmalloc(argv[i]);
        if (!file_content) {
            printf("could not open file \"%s\"", argv[i]);
            exit(1);
        }
        file_contents[i - 1] = file_content;
        set_enviroment(argv[i]);
        TokenList token_list = lex(file_content);
        has_errors |= token_list.has_error;
        token_lists[i - 1] = token_list;
        print_tokens(token_list);
        printf("\n");
        Module* module = parse(token_list);
        if (module) {
            module->meta.name = strmdup(get_enviroment());
            string_dict_put(&general_identifier_dict, module->meta.name, module);
        }
        else {
            has_errors = true;
        }
    }
    print_ast(&general_identifier_dict);

    printf("%s\n", has_errors ? "errors found" : "error free code");
    
    for (int i = 0; i < argc - 1; i++) {
        if (token_lists[i].length == 0) continue;
        free_tokens(token_lists[i]);
        mfree(file_contents[i]);
    }
    mfree(token_lists);
    string_dict_foreach(&general_identifier_dict, free_ast);
    string_dict_destroy(&general_identifier_dict);
}