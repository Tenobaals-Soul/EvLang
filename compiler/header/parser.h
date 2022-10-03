#ifndef INCLUDE_PARSER_H
#define INCLUDE_PARSER_H
#include<compiler.h>

char* strmdup(const char* src);

Module* parse(TokenList tokens);

#endif