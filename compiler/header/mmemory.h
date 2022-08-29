#ifndef INCLUDE_MMEMORY_H
#define INCLUDE_MMEMORY_H
#include<stdlib.h>

void* mmalloc(ssize_t size);
void* mcalloc(ssize_t size);
void* mrealloc(void* memblock, ssize_t size);
char* strmcpy(const char* src);

void mfree(void* memblock);

#endif