#include <string_dict.h>
#include <tokenizer.h>
#include <scanner.h>
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

void message_internal(const char *color_code, const char *line, unsigned int line_no,
                      unsigned int char_no, unsigned int len, const char *message, va_list l) {
    printf("%s %u:%u ", enviroment, line_no, char_no);
    vprintf(message, l);
    putchar('\n');
    unsigned int print_len = 0;
    printf("%4d | ", line_no);
    unsigned int i = 0;
    for (; line[i] && line[i] != '\n' && i < char_no; i++) {
        if (!iscntrl(line[i])) {
            putchar(line[i]);
            print_len += isprint(line[i]) ? 1 : 0;
        }
    }
    printf("%s", color_code);
    for (; line[i] && line[i] != '\n' && i < char_no + len; i++) {
        if (!iscntrl(line[i])) {
            putchar(line[i]);
            print_len += isprint(line[i]) ? 1 : 0;
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
}

StackedData *get_from_ident_dot_seq(StringDict *src, const char *name, TokenList *tokens, int token_index, bool throw) {
    const char *env = enviroment;
    const char *acs = name;
    StackedData *found;
    int word = 0;
    for (int i = 0; name[i] || name[i + 1]; word++) {
        found = string_dict_get(src, acs);
        if (found == NULL) {
            if (throw) {
                if (!src || src->count == 0) {
                    Token *t = &tokens->tokens[token_index + word * 2];
                    make_error(t->line_content, t->line_in_file, t->char_in_line,
                            t->text_len, "%s has no members", env, name + i);
                }
                else {
                    Token *t = &tokens->tokens[token_index + word * 2];
                    make_error(t->line_content, t->line_in_file, t->char_in_line,
                            t->text_len, "%s has no member %s", env, name + i);
                }
            }
            return NULL;
        }
        env = name;
        for (acs = name + i; acs[i]; i++);
        src = found->type == ENTRY_CLASS ? found->class.class_content : NULL;
    }
    return found;
}

void make_error(const char *line, unsigned int line_no, unsigned int char_no,
                unsigned int len, const char *error_message, ...) {
    va_list l;
    va_start(l, error_message);
    message_internal("\033[91m", line, line_no, char_no, len, error_message, l);
    va_end(l);
}

void make_warning(const char *line, unsigned int line_no, unsigned int char_no,
                  unsigned int len, const char *warning_message, ...) {
    va_list l;
    va_start(l, warning_message);
    message_internal("\033[91m", line, line_no, char_no, len, warning_message, l);
    va_end(l);
}

// "src\0\0", "append\0" -> "src\0append\0\0"
char *append_accessor_str(char *src, char *append) {
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
    for (len = 0; accstr[len] || accstr[len + 1]; len++)
        ;
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
            printf("<op>");
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

void print_ast_internal2(Expression *exp) {
    switch (exp->expression_type) {
    case EXPRESSION_CALL:
        printf("%s(", exp->expression_call.call->name);
        unsigned int i;
        for (i = 0; exp->expression_call.args[i]; i++) {
            print_ast_internal2(exp->expression_call.args[i]);
        }
        if (exp->expression_call.args[i]) {
            print_ast_internal2(exp->expression_call.args[i]);
        }
        printf(")");
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
        printf("%s", exp->expression_variable ? exp->expression_variable->name : "error-var");
        break;
    }
}

void print_ast_internal(void *env, const char *name, void *val) {
    int layer = *((int *) env);
    char indent[layer * 4 + 1];
    memset(indent, ' ', layer * 4);
    indent[layer * 4] = 0;
    StackedData *entry = val;
    switch (entry->type) {
    case ENTRY_CLASS:;
        printf("%sclass %s with %d entrys:\n", indent, entry->name, entry->class.class_content->count);
        layer++;
        string_dict_complex_foreach(entry->class.class_content, print_ast_internal, &layer);
    case ENTRY_METHOD:
        printf("%s%s %s(", indent, entry->method.return_type, name);
        unsigned int i;
        if (entry->method.arg_count) {
            for (i = 0; i < entry->method.arg_count - 1; i++) {
                printf("%s %s, ", entry->method.args[i].type, entry->method.args[i].name);
            }
            printf("%s %s", entry->method.args[i].type, entry->method.args[i].name);
        }
        printf(") with code: NONE\n");
        break;
    case ENTRY_METHOD_TABLE:
        printf("%smethod %s with %d overloaded variant(s):\n", indent, name, entry->method_table.len);
        layer++;
        for (unsigned int i = 0; i < entry->method_table.len; i++) {
            print_ast_internal(&layer, name, entry->method_table.methods[i]);
        }
        break;
    case ENTRY_VARIABLE:
        printf("%s%s %s = ", indent, entry->var.type, name);
        if (entry->var.exec_text) {
            print_ast_internal2(entry->var.exec_text);
        }
        else {
            printf("NONE");
        }
        printf("\n");
        break;
    case ENTRY_ERROR:
        make_error(entry->causing->line_content, entry->causing->line_in_file, entry->causing->char_in_line,
                   entry->causing->text_len, entry->name);
        break;
    case ERROR_TYPE:
        printf("detected fatal internal error - error type detected - %d\n", __LINE__);
        exit(1);
        break;
    }
}

void print_ast_wrap(const char* key, void* val) {
    int layer = 1;
    printf("module %s with %d entrys:\n", key, ((StringDict*) val)->count);
    string_dict_complex_foreach(val, print_ast_internal, &layer);
}

void print_ast(StringDict *dict) {
    string_dict_foreach(dict, print_ast_wrap);
}

void print_single_result_internal(void *enviroment, const char *name, void *val) {
    int layer = *((int *)enviroment);
    char indent[layer * 4 + 1];
    memset(indent, ' ', layer * 4);
    indent[layer * 4] = 0;
    StackedData *entry = val;
    switch (entry->type) {
    case ENTRY_CLASS:;
        layer++;
        printf("%sclass %s", indent, name);
        if (entry->class.derives_from) {
            printf(" derives from ");
            print_accessor_str(entry->class.derives_from);
        }
        if (entry->class.implements_len) {
            printf(" implements ");
            unsigned int i;
            for (i = 0; i < entry->class.implements_len - 1; i++) {
                print_accessor_str(entry->class.implements[i]);
                printf(", ");
            }
            print_accessor_str(entry->class.implements[i]);
        }
        printf(" with %d entrys:\n", entry->class.class_content->count);
        string_dict_complex_foreach(entry->class.class_content, print_single_result_internal, &layer);
        break;
    case ENTRY_METHOD:
        printf("%s%s %s(", indent, entry->method.return_type, name);
        unsigned int i;
        if (entry->method.arg_count) {
            for (i = 0; i < entry->method.arg_count - 1; i++) {
                printf("%s %s, ", entry->method.args[i].type, entry->method.args[i].name);
            }
            printf("%s %s", entry->method.args[i].type, entry->method.args[i].name);
        }
        printf(")\n");
        break;
    case ENTRY_METHOD_TABLE:
        printf("%smethod %s with %d overloaded variant(s):\n", indent, name, entry->method_table.len);
        layer++;
        for (unsigned int i = 0; i < entry->method_table.len; i++) {
            print_single_result_internal(&layer, name, entry->method_table.methods[i]);
        }
        break;
    case ENTRY_VARIABLE:
        printf("%s%s %s\n", indent, entry->var.type, name);
        break;
    case ENTRY_ERROR:
        make_error(entry->causing->line_content, entry->causing->line_in_file, entry->causing->char_in_line,
                   entry->causing->text_len, entry->name);
        break;
    case ERROR_TYPE:
        printf("detected fatal internal error - error type detected - %d\n", __LINE__);
        exit(1);
        break;
    }
}

void print_single_result(const char *key, void *val) {
    StackedData *entry = val;
    int env = 0;
    print_single_result_internal(&env, key, entry);
}

void print_scan_result(StringDict *content) {
    string_dict_foreach(content, print_single_result);
}

void free_ast_expression(Expression* exp) {
    switch (exp->expression_type) {
    case EXPRESSION_CALL:
        for (int i = 0; exp->expression_call.args[i]; i++) {
            free_ast_expression(exp->expression_call.args[i]);
        }
        free(exp->expression_call.args);
        break;
    case EXPRESSION_OPEN_PARANTHESIS_GUARD:
        printf("fatal internal error detected - %d", __LINE__);
        exit(1);
    case EXPRESSION_OPERATOR:
        free_ast_expression(exp->expression_operator.left);
        free_ast_expression(exp->expression_operator.right);
        break;
    case EXPRESSION_UNARY_OPERATOR:
        free_ast_expression(exp->expression_operator.left);
        break;
    case EXPRESSION_VAR:
    case EXPRESSION_FIXED_VALUE:
        break;
    }
    free(exp);
}

void free_ast_statements(Statement** st) {
    switch ((*st)->statement_type) {
        default:
            break;
    }
}

void free_ast_item(const char *key, void *val) {
    StackedData *entry = val;
    switch (entry->type) {
    case ENTRY_CLASS:
        free(entry->name);
        string_dict_foreach(entry->class.class_content, free_ast_item);
        string_dict_destroy(entry->class.class_content);
        for (unsigned int i = 0; i < entry->class.implements_len; i++) {
            free(entry->class.implements[i]);
        }
        if (entry->class.implements)
            free(entry->class.implements);
        if (entry->class.derives_from)
            free(entry->class.derives_from);
        free(entry->class.class_content);
        break;
    case ENTRY_METHOD:
        free(entry->name);
        free(entry->method.return_type);
        if (entry->method.exec_text) free_ast_statements(entry->method.exec_text);
        for (unsigned int i = 0; i < entry->method.arg_count; i++) {
            free(entry->method.args[i].name);
            free(entry->method.args[i].type);
        }
        free(entry->method.args);
        break;
    case ENTRY_METHOD_TABLE:
        for (unsigned int i = 0; i < entry->method_table.len; i++) {
            free_ast_item(key, entry->method_table.methods[i]);
        }
        free(entry->method_table.methods);
        break;
    case ENTRY_VARIABLE:
        free(entry->name);
        free(entry->var.type);
        if (entry->var.exec_text) free_ast_expression(entry->var.exec_text);
        break;
    case ENTRY_ERROR:
        free(entry->name);
        break;
    case ERROR_TYPE:
        printf("detected fatal internal error - error type detected - %d\n", __LINE__);
        exit(1);
        break;
    }
    free(entry);
}

void free_ast(const char *key, void *val) {
    (void)key;
    string_dict_foreach(val, free_ast_item);
    string_dict_destroy(val);
    free(val);
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

int main(int argc, char **argv) {
    bool exit_err = false;
    for (int i = 1; i < argc; i++) {
        if (access(argv[i], R_OK)) {
            printf("file \"%s\" could not be found\n", argv[i]);
            exit_err = true;
        }
    }
    if (exit_err)
        exit(0);
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
        token_lists[i - 1] = token_list;
        print_tokens(token_list);
        printf("\n");
        unsigned int index = 0;
        StringDict *content = scan_content(token_list, &index);
        if (content) {
            print_scan_result(content);
            string_dict_put(&general_identifier_dict, argv[i], content);
        }
        if (token_list.has_error)
            has_errors = true;
    }
    parse(&general_identifier_dict);
    printf("\n");
    print_ast(&general_identifier_dict);
    
    for (int i = 0; i < argc - 1; i++) {
        free_tokens(token_lists[i]);
        free(file_contents[i]);
    }
    free(token_lists);
    string_dict_foreach(&general_identifier_dict, free_ast);
    string_dict_destroy(&general_identifier_dict);
}