#ifndef INCLUDE_PARSER_H
#define INCLUDE_PARSER_H
#include<compiler.h>

char* strmcpy(const char* src);

Module* parse(TokenList tokens);

#endif