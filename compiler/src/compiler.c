#include"string_dict.h"
#include"tokenizer.h"
#include<stdarg.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>

void make_error(const char * file_name, const char * line, unsigned int line_no, unsigned int char_no, 
        unsigned int len, const char * error_message, ...) {
    printf("%s %u:%u", file_name, line_no, char_no);
    va_list l;
    va_start(l, error_message);
    vprintf(error_message, l);
    va_end(l);
    putchar('\n');
    for (unsigned int i = 0; i < char_no; i++) {
        putchar(line[i]);
    }
    printf("\e[91m");
    for (unsigned int i = char_no; i < char_no + len; i++) {
        putchar(line[i]);
    }
    printf("\e[0m");
    for (unsigned int i = char_no + len; line[i] && line[i] != '\n'; i++) {
        putchar(line[i]);
    }
    putchar('\n');
}

void print_tokens(TokenList l) {
    for (unsigned long i = 0; i < l.cursor; i++) {
        switch (l.tokens[i].type) {
        case OPTIONAL_END_TOKEN:
            printf("\n");
            break; 
        case KEYWORD_TOKEN:
            printf("\e[94m%s\e[0m ", l.tokens[i].keyword);
            break; 
        case IDENTIFIER_TOKEN:
            printf("%s ", l.tokens[i].identifier);
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
                printf("<val:?>");
            }
            break;
        case OPERATOR_TOKEN:
            printf(" <op> ");
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
            printf(", ");
            break;
        case DOT_TOKEN:
            printf(".");
            break;
        default:
            printf("???");
        }
    }
    printf("\n");
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (access(argv[i], R_OK)) {
            printf("file \"%s\" could not be found\n", argv[i]);
            exit(0);
        }
    }
    TokenList * token_lists = malloc(sizeof(TokenList) * (argc - 1));
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd == -1) {
            printf("could not open file \"%s\"", argv[i]);
            exit(1);
        }
        off_t len = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
        char * file_content = malloc(len + 1);
        read(fd, file_content, len);
        file_content[len] = 0;
        token_lists[i - 1] = lex(file_content);
        free(file_content);
        print_tokens(token_lists[i - 1]);
        close(fd);
    }
    for (int i = 1; i < argc; i++) {
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