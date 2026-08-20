#include "graph.h"
#include <stdlib.h>
#include <string.h>

/* ---------------- IntSet: minimal dynamic-array-backed set ---------------- */

void intset_init(IntSet *s) {
    s->capacity = 4;
    s->size = 0;
    s->items = malloc(sizeof(int) * (size_t)s->capacity);
}

void intset_free(IntSet *s) {
    free(s->items);
    s->items = NULL;
    s->size = 0;
    s->capacity = 0;
}

int intset_contains(const IntSet *s, int value) {
    for (int i = 0; i < s->size; i++) {
        if (s->items[i] == value) return 1;
    }
    return 0;
}

int intset_add(IntSet *s, int value) {
    if (intset_contains(s, value)) return 0;
    if (s->size == s->capacity) {
        s->capacity *= 2;
        s->items = realloc(s->items, sizeof(int) * (size_t)s->capacity);
    }
    s->items[s->size++] = value;
    return 1;
}

/* ---------------- ConflictGraph ---------------- */

void graph_init(ConflictGraph *g) {
    g->num_vertices = 0;
    hashmap_init(&g->subject_index, 32);
    memset(g->weight, 0, sizeof(g->weight));
    for (int i = 0; i < MAX_SUBJECTS; i++) {
        intset_init(&g->adjacency[i]);
    }
}

int graph_find_vertex(const ConflictGraph *g, const char *subject_id) {
    int idx;
    if (hashmap_get(&g->subject_index, subject_id, &idx)) return idx;
    return -1;
}

int graph_add_vertex(ConflictGraph *g, const char *subject_id,
                      const char *subject_name, const char *subject_type) {
    int existing = graph_find_vertex(g, subject_id);
    if (existing >= 0) return existing;

    int idx = g->num_vertices++;
    SubjectVertex *v = &g->vertices[idx];
    strncpy(v->subject_id, subject_id, MAX_NAME_LEN - 1);
    v->subject_id[MAX_NAME_LEN - 1] = '\0';
    strncpy(v->subject_name, subject_name ? subject_name : "", MAX_NAME_LEN - 1);
    v->subject_name[MAX_NAME_LEN - 1] = '\0';
    strncpy(v->subject_type, subject_type ? subject_type : "", 15);
    v->subject_type[15] = '\0';
    v->enrollment = 0;
    hashmap_put(&g->subject_index, subject_id, idx);
    return idx;
}

void graph_add_edge(ConflictGraph *g, int a, int b, int extra_weight) {
    if (a == b) return; /* no self loops */
    intset_add(&g->adjacency[a], b);
    intset_add(&g->adjacency[b], a);
    g->weight[a][b] += extra_weight;
    g->weight[b][a] += extra_weight;
}

int graph_has_edge(const ConflictGraph *g, int a, int b) {
    return intset_contains(&g->adjacency[a], b);
}

int graph_degree(const ConflictGraph *g, int vertex) {
    return g->adjacency[vertex].size;
}
