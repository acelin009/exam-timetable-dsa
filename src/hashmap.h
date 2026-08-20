#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>

/*
 * hashmap.h
 * ---------
 * A small, self-contained hash map from C strings to ints.
 *
 * WHY: The project needs fast O(1) average-case lookup from
 * subject_id / student_id strings to their internal array index
 * (e.g. "MAT" -> 0). Doing this with a linear scan over an array
 * of strings would be O(n) per lookup; for 60 students * ~4
 * subjects that's fine, but the assignment explicitly asks for a
 * hash map data structure, and it keeps subject/student lookups
 * O(1) average case regardless of dataset size.
 *
 * DESIGN: Open addressing with linear probing. Custom string hash
 * (djb2). Not a Python dict / std::unordered_map wrapper -- this
 * is implemented from scratch to demonstrate the data structure.
 */

typedef struct {
    char *key;      /* NULL = empty slot, (char*)-1 sentinel = deleted */
    int value;
    int used;       /* 1 if slot currently holds a live entry */
} HashEntry;

typedef struct {
    HashEntry *entries;
    size_t capacity;
    size_t count;
} HashMap;

void hashmap_init(HashMap *map, size_t initial_capacity);
void hashmap_free(HashMap *map);

/* Insert or overwrite. Grows (rehashes) automatically when the
 * load factor exceeds 0.7. Amortized O(1). */
void hashmap_put(HashMap *map, const char *key, int value);

/* Returns 1 and writes *out_value if found, else returns 0. O(1) avg. */
int hashmap_get(const HashMap *map, const char *key, int *out_value);

int hashmap_contains(const HashMap *map, const char *key);

#endif /* HASHMAP_H */
