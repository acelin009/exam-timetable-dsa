#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

/* Portable strdup (not guaranteed by plain C11; avoid relying on
 * POSIX feature-test macros for such a tiny helper). */
static char *dup_string(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

/* djb2 string hash - simple, fast, good distribution for short IDs. */
static unsigned long djb2_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + (unsigned long)c; /* hash * 33 + c */
    }
    return hash;
}

static void hashmap_grow(HashMap *map);

void hashmap_init(HashMap *map, size_t initial_capacity) {
    if (initial_capacity < 8) initial_capacity = 8;
    map->capacity = initial_capacity;
    map->count = 0;
    map->entries = calloc(map->capacity, sizeof(HashEntry));
}

void hashmap_free(HashMap *map) {
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].used) free(map->entries[i].key);
    }
    free(map->entries);
    map->entries = NULL;
    map->capacity = 0;
    map->count = 0;
}

static size_t find_slot(const HashEntry *entries, size_t capacity, const char *key) {
    size_t idx = djb2_hash(key) % capacity;
    size_t start = idx;
    do {
        if (!entries[idx].used || strcmp(entries[idx].key, key) == 0) {
            return idx;
        }
        idx = (idx + 1) % capacity; /* linear probing */
    } while (idx != start);
    return (size_t)-1; /* table full - should not happen, we grow first */
}

static void hashmap_grow(HashMap *map) {
    size_t new_capacity = map->capacity * 2;
    HashEntry *new_entries = calloc(new_capacity, sizeof(HashEntry));

    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].used) {
            size_t slot = find_slot(new_entries, new_capacity, map->entries[i].key);
            new_entries[slot].key = map->entries[i].key; /* transfer ownership */
            new_entries[slot].value = map->entries[i].value;
            new_entries[slot].used = 1;
        }
    }
    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;
}

void hashmap_put(HashMap *map, const char *key, int value) {
    if ((double)(map->count + 1) / (double)map->capacity > 0.7) {
        hashmap_grow(map);
    }
    size_t slot = find_slot(map->entries, map->capacity, key);
    if (map->entries[slot].used) {
        map->entries[slot].value = value; /* overwrite */
        return;
    }
    map->entries[slot].key = dup_string(key);
    map->entries[slot].value = value;
    map->entries[slot].used = 1;
    map->count++;
}

int hashmap_get(const HashMap *map, const char *key, int *out_value) {
    if (map->capacity == 0) return 0;
    size_t idx = djb2_hash(key) % map->capacity;
    size_t start = idx;
    do {
        if (!map->entries[idx].used) return 0;
        if (strcmp(map->entries[idx].key, key) == 0) {
            *out_value = map->entries[idx].value;
            return 1;
        }
        idx = (idx + 1) % map->capacity;
    } while (idx != start);
    return 0;
}

int hashmap_contains(const HashMap *map, const char *key) {
    int tmp;
    return hashmap_get(map, key, &tmp);
}
