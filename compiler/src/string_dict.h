#ifndef INCLUDE_STRING_DICT_H
#define INCLUDE_STRING_DICT_H

#define STRING_DICT_TABLE_SIZE 512

typedef struct StringDict {
    unsigned int count;
    struct string_dict_item* items[STRING_DICT_TABLE_SIZE];
} StringDict;

void string_dict_init(StringDict * dict);
void string_dict_put(StringDict * dict, const char * key, void * val);
void * string_dict_get(StringDict * dict, const char * key);
unsigned int string_dict_get_size(StringDict * dict);
void string_dict_destroy(StringDict * dict);
void string_dict_foreach(StringDict * dict, void (*action)(const char * key, void * val));
void string_dict_complex_foreach(StringDict * dict, void (*action)
        (void * enviroment, const char * key, void * val), void * enviroment);

#endif