#include<mmemory.h>
#include<string.h>

void* mmalloc(ssize_t size) {
    return malloc(size);
}

void* mcalloc(ssize_t size) {
    return calloc(size, 1);
}

void* mrealloc(void* memblock, ssize_t size) {
    return realloc(memblock, size);
}

char* strmcpy(const char* src) {
    char* new_str = mmalloc(strlen(src) + 1);
    strcpy(new_str, src);
    return new_str;
}

void mfree(void* memblock) {
    free(memblock);
}