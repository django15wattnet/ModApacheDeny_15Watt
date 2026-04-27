//
// Created by Thomas Siemion on 25.04.26.
//

#include "hashMap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned int hash(const char *key) {
    unsigned int h = 5381;
    int c;
    while ((c = *key++)) {
        h = ((h << 5) + h) + (unsigned char)c; // h * 33 + c
    }
    return h % TABLE_SIZE;
}

char *str_copy(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (!copy) return NULL;
    memcpy(copy, s, len);
    return copy;
}

void hashmap_init(HashMap *map) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        map->buckets[i] = NULL;
    }
}

Entry *entry_create(const char *key, int value) {
    Entry *e = malloc(sizeof(Entry));
    if (!e) return NULL;

    e->key = str_copy(key);
    if (!e->key) {
        free(e);
        return NULL;
    }

    e->value = value;
    e->next = NULL;
    return e;
}

void hashmap_put(HashMap *map, const char *key, int value) {
    const unsigned int index = hash(key);
    Entry *current = map->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            current->value = value; // Update
            return;
        }
        current = current->next;
    }

    Entry *new_entry = entry_create(key, value);
    if (!new_entry) return;

    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
}

int hashmap_get(const HashMap *map, const char *key, int *out_value) {
    const unsigned int index = hash(key);
    Entry *current = map->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            *out_value = current->value;
            return 1;
        }
        current = current->next;
    }
    return 0;
}

int hashmap_remove(HashMap *map, const char *key) {
    unsigned int index = hash(key);
    Entry *current = map->buckets[index];
    Entry *prev = NULL;

    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (prev) prev->next = current->next;
            else map->buckets[index] = current->next;

            free(current->key);
            free(current);
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}

void hashmap_free(HashMap *map) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Entry *current = map->buckets[i];
        while (current) {
            Entry *next = current->next;
            free(current->key);
            free(current);
            current = next;
        }
        map->buckets[i] = NULL;
    }
}


int main() {
    HashMap map;
    hashmap_init(&map);

    hashmap_put(&map, "name", 42);
    hashmap_put(&map, "alter", 21);
    hashmap_put(&map, "stadt", 7);

    int value;
    if (hashmap_get(&map, "alter", &value)) {
        printf("alter = %d\n", value);
    } else {
        printf("nicht gefunden\n");
    }

    hashmap_remove(&map, "stadt");

    if (hashmap_get(&map, "stadt", &value)) {
        printf("stadt = %d\n", value);
    } else {
        printf("stadt nicht gefunden\n");
    }

    const char *key = "ilhvglihvMozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/145.0.0.0 Safari/537.36";
    printf("hash = %u\n", hash(key));

    hashmap_free(&map);
    return 0;
}
