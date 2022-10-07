#ifndef INCLUDE_MMEMORY_H
#define INCLUDE_MMEMORY_H
#include<stdlib.h>

char* mstrdup(const char* src);

void* mmalloc(ssize_t size);
void* mcalloc(ssize_t size);
void* mrealloc(void* memptr, ssize_t size);
void mfree(void* memptr);

#endif