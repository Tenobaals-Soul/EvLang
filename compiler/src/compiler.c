#include"string_dict.h"
#include"tokenizer.h"
#include"scanner.h"
#include<stdarg.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<ctype.h>

static char enviroment[256] = "unspecified enviroment";

void set_enviroment(const char* new_enviroment) {
    if (strlen(new_enviroment) > 255) {
        strncpy(enviroment, new_enviroment, 252);
        strcat(enviroment, "...");
    }
    else {
        strcpy(enviroment, new_enviroment);
    }
}

void message_internal(const char* color_code, const char* line, unsigned int line_no,
        unsigned int char_no, unsigned int len, const char* message, ...) {
    printf("%s %u:%u ", enviroment, line_no, char_no);
    char_no--;
    va_list l;
    va_start(l, message);
    vprintf(message, l);
    va_end(l);
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
    printf("\e[0m");
    for (; line[i] && line[i] != '\n'; i++) {
        if (!iscntrl(line[i])) {
            putchar(line[i]);
        }
    }
    if (print_len == char_no) {
        print_len++;
    }
    putchar('\n');
    if (char_no < print_len) {
        printf("     | ");
        for (unsigned int i = 0; i < char_no; i++) {
            putchar(' ');
        }
        printf("%s", color_code);
        for (unsigned int i = char_no; i < print_len; i++) {
            putchar('^');
        }
        printf("\e[0m\n");
    }
}

void make_error(const char* line, unsigned int line_no, unsigned int char_no, 
        unsigned int len, const char* error_message, ...) {
    printf("%s %u:%u ", enviroment, line_no, char_no);
    char_no--;
    va_list l;
    va_start(l, error_message);
    message_internal("\033[91m", line, line_no, char_no, len, error_message, l);
    va_end(l);
}

void make_warning(const char* line, unsigned int line_no, unsigned int char_no, 
        unsigned int len, const char * warning_message, ...) {
    printf("%s %u:%u ", enviroment, line_no, char_no);
    char_no--;
    va_list l;
    va_start(l, warning_message);
    message_internal("\033[91m", line, line_no, char_no, len, warning_message, l);
    va_end(l);
}

void print_tokens(TokenList l) {
    for (unsigned long i = 0; i < l.cursor; i++) {
        if (i) printf(" ");
        switch (l.tokens[i].type) {
        case END_TOKEN:
            printf(";");
            break;
        case K_CLASS_TOKEN:
            printf("\e[94m%s\e[0m", "class");
            break; 
        case K_PRIVATE_TOKEN:
            printf("\e[94m%s\e[0m", "private");
            break; 
        case K_PROTECTED_TOKEN:
            printf("\e[94m%s\e[0m", "protected");
            break; 
        case K_PUBLIC_TOKEN:
            printf("\e[94m%s\e[0m", "public");
            break; 
        case K_DERIVES_TOKEN:
            printf("\e[94m%s\e[0m", "derives");
            break;
        case K_IMPLEMENTS_TOKEN:
            printf("\e[94m%s\e[0m", "implements");
            break;
        case K_STATIC_TOKEN:
            printf("\e[94m%s\e[0m", "static");
            break;
        case C_BREAK_TOKEN:
            printf("\e[94m%s\e[0m", "break");
            break;
        case C_CASE_TOKEN:
            printf("\e[94m%s\e[0m", "case");
            break;
        case C_DEFAULT_TOKEN:
            printf("\e[94m%s\e[0m", "default");
            break;
        case C_FOR_TOKEN:
            printf("\e[94m%s\e[0m", "for");
            break;
        case C_IF_TOKEN:
            printf("\e[94m%s\e[0m", "if");
            break;
        case C_RETURN_TOKEN:
            printf("\e[94m%s\e[0m", "return");
            break;
        case C_SWITCH_TOKEN:
            printf("\e[94m%s\e[0m", "switch");
            break;
        case C_WHILE_TOKEN:
            printf("\e[94m%s\e[0m", "while");
            break;
        case IDENTIFIER_TOKEN:
            printf("%s", l.tokens[i].identifier);
            break;
        case FIXED_VALUE_TOKEN:
            if (l.tokens[i].fixed_value.type == STRING) {
                printf("\e[32m\"%s\"\e[0m", l.tokens[i].fixed_value.value.string);
            }
            else if (l.tokens[i].fixed_value.type == CHARACTER) {
                printf("\e[32m\'%s\'\e[0m", l.tokens[i].fixed_value.value.string);
            }
            else if (l.tokens[i].fixed_value.type == NUMBER) {
                printf("\e[32m%Lf\e[0m", l.tokens[i].fixed_value.value.floating);
            }
            else if (l.tokens[i].fixed_value.type == BOOLEAN) {
                printf("\e[32m%s\e[0m", l.tokens->fixed_value.value.boolean ? "true" : "false");
            }
            else {
                printf("detected fatal internal error - error type detected - %d\n", __LINE__);
                exit(1);
            }
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
        default:
            printf("detected fatal internal error - error type detected - %d\n", __LINE__);
            exit(1);
        }
    }
    printf("\n");
}

void free_tokens(TokenList* token_lists, int count) {
    for (int i = 1; i < count; i++) {
        for (unsigned long j = 0; j < token_lists->cursor; j++) {
            Token * t = token_lists[i - 1].tokens + j;
            if (t->type == IDENTIFIER_TOKEN) {
                free(t->identifier);
            }
            else if (t->type == FIXED_VALUE_TOKEN && (t->fixed_value.type == STRING || t->fixed_value.type == CHARACTER)) {
                free(t->fixed_value.value.string);
            }
        }
        free(token_lists[i - 1].tokens);
    }
    free(token_lists);
}

void print_single_result_internal(void* enviroment, const char* name, void* val) {
    int layer = *((int*) enviroment);
    char indent[layer * 4 + 1];
    memset(indent, ' ', layer * 4);
    indent[layer * 4] = 0;
    StackedData* entry = val;
    switch (entry->type) {
    case ENTRY_CLASS:;
        layer++;
        printf("%sclass %s with %d entrys:\n", indent, name, entry->class.class_content->count);
        string_dict_complex_foreach(entry->class.class_content, print_single_result_internal, &layer);
        break;
    case ENTRY_METHOD:
        printf("%s%s %s(", indent, entry->method.return_type, name);
        unsigned int i;
        for (i = 0; i < entry->method.arg_count - 1; i++) {
            printf("%s %s, ", entry->method.args[i].type, entry->method.args[i].name);
        }
        if (entry->method.arg_count) {
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
    case ERROR_TYPE:
        printf("detected fatal internal error - error type detected - %d\n", __LINE__);
        exit(1);
        break;
    }
}

void print_single_result(const char* key, void* val) {
    StackedData* entry = val;
    int env = 0;
    print_single_result_internal(&env, key, entry);
}

void print_scan_result(StringDict* content) {
    string_dict_foreach(content, print_single_result);
}

void free_single_scan_result(const char* key, void* val) {
    StackedData* entry = val;
    switch (entry->type) {
    case ENTRY_CLASS:
        string_dict_foreach(entry->class.class_content, free_single_scan_result);
        string_dict_destroy(entry->class.class_content);
        free(entry->class.class_content);
        break;
    case ENTRY_METHOD:
        free(entry->method.args);
        break;
    case ENTRY_METHOD_TABLE:
        for (unsigned int i = 0; i < entry->method_table.len; i++) {
            free_single_scan_result(key, entry->method_table.methods[i]);
        }
        free(entry->method_table.methods);
        break;
    case ENTRY_VARIABLE:
        break;
    case ERROR_TYPE:
        printf("detected fatal internal error - error type detected - %d\n", __LINE__);
        exit(1);
        break;
    }
    free(entry);
}

void free_scan_result(const char* key, void* val) {
    (void) key;
    string_dict_foreach(val, free_single_scan_result);
    string_dict_destroy(val);
    free(val);
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (access(argv[i], R_OK)) {
            printf("file \"%s\" could not be found\n", argv[i]);
            exit(0);
        }
    }
    TokenList* token_lists = malloc(sizeof(TokenList) * (argc - 1));
    StringDict general_identifier_dict;
    string_dict_init(&general_identifier_dict);
    char* file_contents[argc - 1];
    for (int i = 1; i < argc; i++) {
        if (string_dict_get(&general_identifier_dict, argv[i])) continue;
        int fd = open(argv[i], O_RDONLY);
        set_enviroment(argv[i]);
        if (fd == -1) {
            printf("could not open file \"%s\"", argv[i]);
            exit(1);
        }
        off_t len = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
        char* file_content = malloc(len + 1);
        read(fd, file_content, len);
        close(fd);
        file_content[len] = 0;
        token_lists[i - 1] = lex(file_content);
        file_contents[i - 1] = file_content;
        print_tokens(token_lists[i - 1]);
        unsigned int index = 0;
        StringDict* content = scan_content(token_lists[i - 1], &index);
        if (content) {
            print_scan_result(content);
            string_dict_put(&general_identifier_dict, argv[i], content);   
        }
    }
    for (int i = 0; i < argc - 1; i++) {
        free(file_contents[i]);
    }
    string_dict_foreach(&general_identifier_dict, free_scan_result);
    string_dict_destroy(&general_identifier_dict);
    free_tokens(token_lists, argc);
}