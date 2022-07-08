#ifndef INCLUDE_STRING_DICT_H
#define INCLUDE_STRING_DICT_H
#include<stdbool.h>

#define STRING_DICT_TABLE_SIZE 512

#define new(type) (new##type())

typedef struct StringDict {
    unsigned int count;
    struct string_dict_item* items[STRING_DICT_TABLE_SIZE];
} StringDict;

void string_dict_init(StringDict* dict);
void string_dict_put(StringDict* dict, const char* key, void* val);
void* string_dict_get(StringDict* dict, const char* key);
unsigned int string_dict_get_size(StringDict* dict);
void string_dict_destroy(StringDict* dict);
void string_dict_foreach(StringDict* dict, void (*action)(const char* key, void* val));
void string_dict_complex_foreach(StringDict* dict, void (*action)
        (void* enviroment, const char* key, void* val), void* enviroment);
unsigned int string_dict_copy(StringDict* dest, StringDict* src);
unsigned int string_dict_copy_if(StringDict* dest, StringDict* src, bool (*condition)(const char* key, void* val));
unsigned int string_dict_copy_complex_if(StringDict* dest, StringDict* src, bool (*condition)
        (void* enviroment, const char* key, void* val), void* enviroment);

StringDict* newStringDict();

#endif