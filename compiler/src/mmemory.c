#include<mmemory.h>
#include<string.h>

void* mmalloc(ssize_t size) {
    return malloc(size);
}

void* mcalloc(ssize_t size) {
    return calloc(size, 1);
}

void* mrealloc(void* memptr, ssize_t size) {
    return realloc(memptr, size);
}

char* strmdup(const char* src) {
    char* new_str = mmalloc(strlen(src) + 1);
    strcpy(new_str, src);
    return new_str;
}

void mfree(void* memptr) {
    free(memptr);
}