#include<string_dict.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<mmemory.h>

struct string_dict_item {
    char* key;
    void* val;
    unsigned int hash;
};

void string_dict_init(StringDict* dict) {
    dict->count = 0;
    dict->capacity = STRING_DICT_TABLE_SIZE;
    dict->items = calloc(sizeof(struct string_dict_item), dict->capacity);
}

static int hash_string(const char* string) {
    int hash = 0;
    for (unsigned int i = 0; i < 8 && string[i]; i++) {
        hash += string[i];
    }
    return hash;
}

static inline void string_dict_set(struct string_dict_item* array,
                                   unsigned int capacity,
                                   struct string_dict_item new) {
    for (unsigned int i = new.hash; i < capacity; i++) {
        if (array[i].key == NULL || strcmp(array[i].key, new.key) == 0) {
            array[i] = new;
            return;
        }
    };
    for (unsigned int i = 0; i < capacity; i++) {
        if (array[i].key == NULL || strcmp(array[i].key, new.key) == 0) {
            array[i] = new;
            return;
        }
    };
    assert(false);
}

static inline void string_dict_remove(struct string_dict_item* array,
                                      unsigned int capacity,
                                      const char* key, unsigned int hash) {
    for (unsigned int i = hash; i < capacity; i++) {
        if (array[i].key == NULL) {
            return;
        }
        if (strcmp(array[i].key, key) == 0) {
            array[i] = (struct string_dict_item) {0};
            return;
        }
    };
    for (unsigned int i = 0; i < capacity; i++) {
        if (array[i].key == NULL) {
            return;
        }
        if (strcmp(array[i].key, key) == 0) {
            array[i] = (struct string_dict_item) {0};
            return;
        }
    };
    assert(false);
}

static void mrealloc_dict(StringDict* dict) {
    struct string_dict_item* new_items = calloc(
        sizeof(struct string_dict_item),
        dict->capacity + STRING_DICT_TABLE_SIZE
    );
    unsigned int i = dict->capacity;
    do {
        i--;
        struct string_dict_item item = dict->items[i];
        string_dict_set(new_items, dict->capacity + STRING_DICT_TABLE_SIZE, item);
    } while(i);
    mfree(dict->items);
    dict->items = new_items;
    dict->capacity += STRING_DICT_TABLE_SIZE;
}

void string_dict_put(StringDict* dict, const char* key, void* val) {
    if (dict->count == dict->capacity) mrealloc_dict(dict);
    unsigned int raw_hash = hash_string(key);
    unsigned int hash = raw_hash % dict->capacity;
    if (val == NULL) {
        string_dict_remove(dict->items, dict->capacity, key, hash);
        dict->count--;
    }
    else {
        string_dict_set(dict->items, dict->capacity, (struct string_dict_item) {strmcpy(key), val, raw_hash});
        dict->count++;
    }
}

void* string_dict_get(StringDict* dict, const char* key) {
    unsigned int hash = ((unsigned) hash_string(key)) % dict->capacity;
    for (unsigned int i = hash; i < dict->capacity; i++) {
        if (dict->items[i].key == NULL) {
            return NULL;
        }
        if (strcmp(dict->items[i].key, key) == 0) {
            return dict->items[i].val;
        }
    };
    for (unsigned int i = 0; i < dict->capacity; i++) {
        if (dict->items[i].key == NULL) {
            return NULL;
        }
        if (strcmp(dict->items[i].key, key) == 0) {
            return dict->items[i].val;
        }
    };
    return NULL;
}

unsigned int string_dict_get_size(StringDict* dict) {
    return dict->count;
}

void string_dict_destroy(StringDict* dict) {
    unsigned int i = dict->capacity;
    if (i)
    do {
        i--;
        char* key = dict->items[i].key;
        if (key) mfree(key);
    } while(i);
    if (dict->items) mfree(dict->items);
}

void string_dict_foreach(StringDict* dict, void (*action)(const char* key, void* val)) {
    unsigned int i = dict->capacity;
    if (i)
    do {
        i--;
        if (dict->items[i].key) {
            action(dict->items[i].key, dict->items[i].val);
        }
    } while(i);
}

void string_dict_complex_foreach(StringDict* dict, void (*action)
        (void* enviroment, const char* key, void* val), void* enviroment) {
    unsigned int i = dict->capacity;
    if (i)
    do {
        i--;
        if (dict->items[i].key) {
            action(enviroment, dict->items[i].key, dict->items[i].val);
        }
    } while(i);
}

struct copy_env {
    StringDict* dest;
    unsigned int counter;
    void* enviroment;
    union {
        bool (*condition)(const char* key, void* val);
        bool (*complex_condition)(void* enviroment, const char* key, void* val);
    };
};

static void string_dict_copy_internal(void* enviroment, const char* key, void* val) {
    struct copy_env* env = enviroment;
    string_dict_put(env->dest, key, val);
    env->counter++;
}

static void string_dict_copy_if_internal(void* enviroment, const char* key, void* val) {
    struct copy_env* env = enviroment;
    if (env->condition(key, val)) {
        string_dict_put(env->dest, key, val);
        env->counter++;
    }
}

static void string_dict_copy_complex_if_internal(void* enviroment, const char* key, void* val) {
    struct copy_env* env = enviroment;
    if (env->complex_condition(env->enviroment, key, val)) {
        string_dict_put(env->dest, key, val);
        env->counter++;
    }
}

unsigned int string_dict_copy(StringDict* dest, StringDict* src) {
    struct copy_env env = { dest, 0, NULL, {NULL}};
    string_dict_complex_foreach(src, string_dict_copy_internal, &env);
    return env.counter;
}

unsigned int string_dict_copy_if(StringDict* dest, StringDict* src, bool (*condition)(const char* key, void* val)) {
    struct copy_env env = { dest, 0, NULL, {.condition = condition}};
    string_dict_complex_foreach(src, string_dict_copy_if_internal, &env);
    return env.counter;
}

unsigned int string_dict_copy_complex_if(StringDict* dest, StringDict* src, bool (*condition)
        (void* enviroment, const char* key, void* val), void* enviroment) {
    struct copy_env env = { dest, 0, enviroment, {.complex_condition = condition}};
    string_dict_complex_foreach(src, string_dict_copy_complex_if_internal, &env);
    return env.counter;
}