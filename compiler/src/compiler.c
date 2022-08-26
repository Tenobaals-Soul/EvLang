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

static char enviroment[256] = "unspecified enviroment";

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

UnresolvedEntry get_from_ident_dot_seq(StringDict *src, const char *name, TokenList *tokens, int token_index, bool throw) {
    const char *env = enviroment;
    const char *acs = name;
    UnresolvedEntry found = NULL;
    int word = 0;
    for (int i = 0; acs[i]; word++, i++) {
        found = src ? string_dict_get(src, acs + i) : NULL;
        if (found == NULL) {
            if (throw) {
                if (!src || src->count == 0) { // found is not a class
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
        env = name;
        for (; acs[i]; i++);
        if (found->entry_type == ENTRY_CLASS) {
            src = &((Class) found)->class_content;
        }
        else if (found->entry_type == ENTRY_MODULE) {
            src = &((Module) found)->module_content;
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

// "src\0\0", "append\0" -> "src\0append\0\0"
char* append_accessor_str(char *src, char *append) {
    int i = 0;
    if (src) {
        for (; src[i] || src[i + 1]; i++);
        i++;
    }
    int n_len = strlen(append);
    src = realloc(src, i + n_len + 2);
    strcpy(src + i, append);
    src[i + n_len + 1] = 0;
    return src;
}

void print_accessor_str(char *accstr) {
    int len;
    for (len = 0; accstr[len] || accstr[len + 1]; len++);
    char *buffer = malloc(len + 1);
    for (int i = 0; i < len; i++) {
        buffer[i] = accstr[i] ? accstr[i] : '.';
    }
    buffer[len] = 0;
    printf("%s", buffer);
    free(buffer);
}

void print_fixed_value(Token *data) {
    switch (data->fixed_value.type) {
    case STRING:
        printf("\033[32m\"%s\"\033[0m", data->fixed_value.value.string);
        break;
    case CHARACTER:
        printf("\033[32m\'%s\'\033[0m", data->fixed_value.value.string);
        break;
    case INTEGER:
        printf("\033[32m%llu\033[0m", (unsigned long long int)data->fixed_value.value.integer);
        break;
    case FLOATING:
        printf("\033[32m%Lf\033[0m", data->fixed_value.value.floating);
        break;
    case BOOLEAN:
        printf("\033[32m%s\033[0m", data->fixed_value.value.boolean ? "true" : "false");
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
    default:
        printf("detected fatal internal error - error type detected - %d\n", __LINE__);
        exit(1);
    }
}

void print_tokens(TokenList l) {
    for (unsigned long i = 0; i < l.cursor; i++) {
        if (i)
            printf(" ");
        switch (l.tokens[i].type) {
        case END_TOKEN:
            printf(";");
            break;
        case K_CLASS_TOKEN:
            printf("\033[94m%s\033[0m", "class");
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
        case K_DERIVES_TOKEN:
            printf("\033[94m%s\033[0m", "derives");
            break;
        case K_IMPLEMENTS_TOKEN:
            printf("\033[94m%s\033[0m", "implements");
            break;
        case K_STATIC_TOKEN:
            printf("\033[94m%s\033[0m", "static");
            break;
        case C_BREAK_TOKEN:
            printf("\033[94m%s\033[0m", "break");
            break;
        case C_CASE_TOKEN:
            printf("\033[94m%s\033[0m", "case");
            break;
        case C_DEFAULT_TOKEN:
            printf("\033[94m%s\033[0m", "default");
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
        case C_SWITCH_TOKEN:
            printf("\033[94m%s\033[0m", "switch");
            break;
        case C_WHILE_TOKEN:
            printf("\033[94m%s\033[0m", "while");
            break;
        case IDENTIFIER_TOKEN:
            printf("%s", l.tokens[i].identifier);
            break;
        case FIXED_VALUE_TOKEN:
            print_fixed_value(&l.tokens[i]);
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
    for (unsigned long j = 0; j < token_list.cursor; j++) {
        Token *t = token_list.tokens + j;
        if (t->type == IDENTIFIER_TOKEN) {
            free(t->identifier);
        }
        else if (t->type == FIXED_VALUE_TOKEN && (t->fixed_value.type == STRING || t->fixed_value.type == CHARACTER)) {
            free(t->fixed_value.value.string);
        }
    }
    free(token_list.tokens);
}

void print_ast_internal2(Expression exp) {
    if (exp == NULL) {
        printf("?");
        return;
    }
    switch (exp->expression_type) {
    case EXPRESSION_CALL:
        if (exp->expression_call.call == NULL) {
            printf("(unknown function)");
            break;
        }
        printf("%s(", exp->expression_call.call->name);
        unsigned int i;
        if (exp->expression_call.arg_count) {
            for (i = 0; i < exp->expression_call.arg_count; i++) {
                print_ast_internal2(exp->expression_call.args[i]);
                printf(", ");
            }
            print_ast_internal2(exp->expression_call.args[i]);
        }
        printf(")");
        break;
    case EXPRESSION_FIXED_VALUE:
        print_fixed_value(exp->fixed_value);
        break;
    case EXPRESSION_OPEN_PARANTHESIS_GUARD:
        printf("detected fatal internal error - error type detected - %d\n", __LINE__);
        exit(1);
    case EXPRESSION_OPERATOR:
        printf("(");
        print_ast_internal2(exp->expression_operator.left);
        printf(") ");
        print_operator(exp->expression_operator.operator);
        printf(" (");
        print_ast_internal2(exp->expression_operator.right);
        printf(")");
        break;
    case EXPRESSION_VAR:
        printf("%s", exp->expression_variable->meta.name ?
               exp->expression_variable->meta.name : "error-var");
        break;
    case EXPRESSION_UNARY_OPERATOR:
        print_operator(exp->expression_operator.operator);
        printf("(");
        print_ast_internal2(exp->expression_operator.left);
        printf(")");
        break;
    case EXPRESSION_INDEX:
        print_ast_internal2(exp->expression_index.from);
        printf("[");
        print_ast_internal2(exp->expression_index.key);
        printf("]");
        break;
    case EXPRESSION_ASSIGN:
        printf("(");
        print_ast_internal2(exp->expression_operator.left);
        printf(") = (");
        print_ast_internal2(exp->expression_operator.right);
        printf(")");
        break;
    }
}

void print_statement(Statement st) {
    switch ((st->statement_type)) {
        case STATEMENT_CALC:
            print_ast_internal2(st->statement_calc.calc);
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
        case STATEMENT_SWITCH:
            printf("switch // no more debug information supported");
            break;
        case STATEMENT_WHILE:
            break;
    }
}

void free_ast_expression(Expression exp);
void print_text(Text text) {
    (void) text;
}

void print_struct_data(StructData struct_data, const char* indent) {
    printf("%s{\n", indent);
    for (unsigned int i = 0; i < struct_data.len; i++) {
        printf("    %s%s %s", indent, struct_data.value[i].type.unresolved, struct_data.value[i].meta.name);
    }
    printf("%s}\n", indent);
}

void print_ast_method(Method method, char* indent) {
    printf("%s%s %s(", indent, method->return_type.unresolved, method->name);
    unsigned int i;
    if (method->arguments.len) {
        for (i = 0; i < method->arguments.len - 1; i++) {
            printf("%s %s, ", method->arguments.value[i].type.unresolved, method->arguments.value[i].meta.name);
        }
        printf("%s %s, ", method->arguments.value[i].type.unresolved, method->arguments.value[i].meta.name);
    }
    printf(")");
    print_struct_data(method->stack_data, indent);
    printf("with code:%s\n", method->text.len ? "" : " NONE");
    for (unsigned int i = 0; i < method->text.len; i++) {
        printf("%s    ", indent);
        print_text(method->text);
        printf(";\n");
    }
}

void print_ast_internal(void *env, const char *name, void *val) {
    int layer = *((int *) env);
    char indent[layer * 4 + 1];
    memset(indent, ' ', layer * 4);
    indent[layer * 4] = 0;
    UnresolvedEntry entry = val;
    switch (entry->entry_type) {
    case ENTRY_PACKAGE:;
        Package package = (Package) entry;
        printf("package %s with %d entrys:\n", name, package->package_content.count);
        layer++;
        string_dict_complex_foreach(&package->package_content, print_ast_internal, &layer);
        break;
    case ENTRY_MODULE:;
        Module module = (Module) entry;
        printf("module %s with %d entrys:\n", name, module->module_content.count);
        layer++;
        string_dict_complex_foreach(&module->module_content, print_ast_internal, &layer);
        break;
    case ENTRY_CLASS:;
        Class class = (Class) entry;
        printf("%sclass %s with %d entrys:\n", indent, name, class->class_content.count);
        layer++;
        string_dict_complex_foreach(&class->class_content, print_ast_internal, &layer);
        break;
    case ENTRY_METHODS:;
        MethodTable table = (MethodTable) entry;
        printf("%smethod %s with %d overloaded variant(s):\n", indent, name, table->len);
        layer++;
        for (unsigned int i = 0; i < table->len; i++) {
            print_ast_method(table->value[i], indent);
        }
        break;
    case ENTRY_FIELD:
        break;
    }
}

void print_ast(StringDict *dict) {
    int layer = 0;
    string_dict_complex_foreach(dict, print_ast_internal, &layer);
}

void free_ast_expression(Expression exp) {
    (void) exp;
}

void free_ast_statements(Text st) {
    for (uint32_t i = 0; i < st.len; i++) {
        switch (st.statements[i]->statement_type) {
            case STATEMENT_CALC:
                free_ast_expression(st.statements[i]->statement_calc.calc);
                break;
            case STATEMENT_FOR:
                free_ast_expression(st.statements[i]->statement_for.condition);
                free_ast_statements(st.statements[i]->statement_for.first);
                free_ast_statements(st.statements[i]->statement_for.last);
                free_ast_statements(st.statements[i]->statement_for.text);
                break;
            case STATEMENT_IF:
                free_ast_expression(st.statements[i]->statement_if.condition);
                free_ast_statements(st.statements[i]->statement_if.on_true);
                free_ast_statements(st.statements[i]->statement_if.on_false);
                break;
            case STATEMENT_RETURN:
                free_ast_expression(st.statements[i]->statement_return.return_value);
                break;
            case STATEMENT_SWITCH:
                printf("switch is not implemented yet\n");
                exit(1);
            case STATEMENT_WHILE:
                free_ast_expression(st.statements[i]->statement_while.condition);
                free_ast_statements(st.statements[i]->statement_while.text);
                break;
        }
        free(st.statements);
    }
}

void free_struct_data(StructData struct_data) {
    for (uint32_t i = 0; i < struct_data.len; i++) {
        free(struct_data.value[i].meta.name);
        free(struct_data.value[i].type.unresolved);
    }
}

void free_ast(const char *key, void *val) {
    (void) key;
    UnresolvedEntry entry = val;
    switch (entry->entry_type) {
    case ENTRY_MODULE:;
        Module module = (Module) entry;
        string_dict_foreach(&module->module_content, free_ast);
        string_dict_destroy(&module->module_content);
        free_ast_statements(module->text);
        break;
    case ENTRY_CLASS:;
        Class class = (Class) entry;
        free(entry->name);
        string_dict_foreach(&class->class_content, free_ast);
        string_dict_destroy(&class->class_content);
        free(class->implements.values);
        if (class->implements.values)
            free(class->implements.values);
        if (class->implements.values)
            free(class->implements.values);
        break;
    case ENTRY_METHODS:;
        MethodTable methods = (MethodTable) entry;
        for (uint32_t i = 0; i < methods->len; i++) {
            Method method = methods->value[i];
            free_ast_statements(method->text);
            free_struct_data(method->arguments);
            free_struct_data(method->stack_data);
            free(method->return_type.unresolved);
            free(method->name);
            free(method);
        }
        break;
    case ENTRY_PACKAGE:;
        Package package = (Package) entry;
        string_dict_foreach(&package->package_content, free_ast);
        string_dict_destroy(&package->package_content);
        break;
    case ENTRY_FIELD:;
        Field field = (Field) entry;
        free(field->type.unresolved);
        break;
    }
    if (entry->name)
        free(entry->name);
    free(entry);
}

char *fmalloc(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        return NULL;
    off_t len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char *file_content = malloc(len + 1);
    if (file_content == NULL)
        return NULL;
    ssize_t r = read(fd, file_content, len);
    if (r < 0) {
        free(file_content);
        return NULL;
    }
    close(fd);
    file_content[len] = 0;
    return file_content;
}

void verify_files(int argc, char** argv) {
    bool exit_err = false;
    for (int i = 1; i < argc; i++) {
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

UnresolvedEntry pack(Module module, char* path) {
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
    module->meta.name = malloc(len + 1);
    memcpy(module->meta.name, name, len);
    module->meta.name[len] = 0;
    return &module->meta;
}

int main(int argc, char **argv) {
    verify_files(argc, argv);
    StringDict general_identifier_dict;
    string_dict_init(&general_identifier_dict);
    char *file_contents[argc - 1];
    TokenList *token_lists = malloc(sizeof(TokenList) * (argc - 1));
    bool has_errors = false;
    for (int i = 1; i < argc; i++) {
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
        Module module = parse(token_list);
        if (module) {
            module->meta.name = strmcpy(get_enviroment());
            string_dict_put(&general_identifier_dict, module->meta.name, module);
        }
        else {
            has_errors = true;
        }
    }
    print_ast(&general_identifier_dict);

    printf("%s\n", has_errors ? "errors found" : "error free code");
    
    for (int i = 0; i < argc - 1; i++) {
        free_tokens(token_lists[i]);
        free(file_contents[i]);
    }
    free(token_lists);
    string_dict_foreach(&general_identifier_dict, free_ast);
    string_dict_destroy(&general_identifier_dict);
}