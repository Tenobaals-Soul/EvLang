#include<stdlib.h>
#include<string.h>
#include"string_dict.h"

struct string_dict_item {
    char* key;
    void* val;
    struct string_dict_item* next;
};

void string_dict_init(StringDict* dict) {
    unsigned int i = STRING_DICT_TABLE_SIZE;
    do {
        dict->items[--i] = NULL;
    } while(i);
    dict->count = 0;
}

static int hash_string(const char* string) {
    int hash = 0;
    for (int i = 0; i < 8 && string[i]; i++) {
        hash += string[i];
    }
    return hash;
}

static void append_to_string_dict_items(struct string_dict_item** list_pos, const char* key, void* val) {
    while (*list_pos) {
        struct string_dict_item * current = *list_pos;
        if (strcmp(current->key, key) == 0) {
            current->val = val;
            return;
        }
        *list_pos = current->next;
    }
    struct string_dict_item * item = malloc(sizeof(struct string_dict_item));
    item->next = NULL;
    *list_pos = item;
    item->key = malloc(strlen(key) + 1);
    strcpy(item->key, key);
    item->val = val;
}

static void remove_from_string_dict_items(struct string_dict_item ** list_pos, const char* key) {
    while (*list_pos) {
        struct string_dict_item * current = *list_pos;
        if (strcmp(current->key, key) == 0) {
            (*list_pos) = current->next;
            free(current->key);
            free(current);
            return;
        }
        *list_pos = current->next;
    }
}

void string_dict_put(StringDict* dict, const char* key, void* val) {
    int index_pos = ((unsigned) hash_string(key)) % STRING_DICT_TABLE_SIZE;
    if (val == NULL) {
        remove_from_string_dict_items(&dict->items[index_pos], key);
        dict->count--;
    }
    else {
        append_to_string_dict_items(&dict->items[index_pos], key, val);
        dict->count++;
    }
}

void* string_dict_get(StringDict* dict, const char* key) {
    int index_pos = ((unsigned) hash_string(key)) % STRING_DICT_TABLE_SIZE;
    struct string_dict_item * current = dict->items[index_pos];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->val;
        }
        current = current->next;
    }
    return NULL;
}

unsigned int string_dict_get_size(StringDict* dict) {
    return dict->count;
}

void string_dict_destroy(StringDict* dict) {
    unsigned int i = STRING_DICT_TABLE_SIZE;
    do {
        i--;
        if (dict->items[i]) {
            struct string_dict_item* current = dict->items[i];
            while (current) {
                struct string_dict_item* next = current->next;
                free(current->key);
                free(current);
                current = next;
            }
        }
    } while(i);
}

void string_dict_foreach(StringDict* dict, void (*action)(const char* key, void* val)) {
    unsigned int i = STRING_DICT_TABLE_SIZE;
    do {
        if (dict->items[--i]) {
            struct string_dict_item* current = dict->items[i];
            while (current) {
                action(current->key, current->val);
                current = current->next;
            }
        }
    } while(i);
}

void string_dict_complex_foreach(StringDict* dict, void (*action)
        (void* enviroment, const char* key, void* val), void* enviroment) {
    unsigned int i = STRING_DICT_TABLE_SIZE;
    do {
        i--;
        if (dict->items[i]) {
            struct string_dict_item * current = dict->items[i];
            while (current) {
                action(enviroment, current->key, current->val);
                current = current->next;
            }
        }
    } while(i);
}