//
// Created by Thomas Siemion on 25.04.26.
//

#ifndef HASHMAPDENAY_15WATT_HASHMAP_H
#define HASHMAPDENAY_15WATT_HASHMAP_H

#define TABLE_SIZE 101

typedef struct Entry {
    char *key;
    int value;
    struct Entry *next;
} Entry;

typedef struct {
    Entry *buckets[TABLE_SIZE];
} HashMap;

unsigned int hash(const char *key);
char        *str_copy(const char *s);
void         hashmap_init(HashMap *map);
Entry       *entry_create(const char *key, int value);
void         hashmap_put(HashMap *map, const char *key, int value);
int          hashmap_get(const HashMap *map, const char *key, int *out_value);
int          hashmap_remove(HashMap *map, const char *key);
void         hashmap_free(HashMap *map);

#endif //HASHMAPDENAY_15WATT_HASHMAP_H
