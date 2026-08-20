#ifndef GRAPH_H
#define GRAPH_H

#include "config.h"
#include "hashmap.h"

/*
 * graph.h
 * -------
 * ConflictGraph: an undirected, unweighted-for-coloring-purposes
 * graph where:
 *   vertex  = subject
 *   edge    = "at least one student is registered for both subjects"
 *
 * REPRESENTATION: adjacency list, not adjacency matrix.
 * WHY: V (number of subjects) is small (~11) but the underlying
 * idea generalises -- for a real university with hundreds of
 * subjects, an adjacency matrix would cost O(V^2) memory even
 * though the conflict graph is typically sparse (most subject
 * pairs never share a student). An adjacency list costs O(V + E).
 * Each vertex's neighbor set is itself a hand-rolled dynamic
 * "IntSet" (sorted-free, membership by linear scan since a
 * vertex's degree is small in this dataset; documented as a
 * conscious trade-off in docs/complexity.md).
 */

typedef struct {
    int *items;
    int size;
    int capacity;
} IntSet;

void intset_init(IntSet *s);
void intset_free(IntSet *s);
int  intset_contains(const IntSet *s, int value);
/* returns 1 if newly added, 0 if it was already present */
int  intset_add(IntSet *s, int value);

typedef struct {
    char subject_id[MAX_NAME_LEN];
    char subject_name[MAX_NAME_LEN];
    char subject_type[16];
    int enrollment; /* number of students registered, for tie-breaks */
} SubjectVertex;

typedef struct {
    SubjectVertex vertices[MAX_SUBJECTS];
    int num_vertices;
    IntSet adjacency[MAX_SUBJECTS];      /* adjacency[i] = neighbor indices of vertex i */
    int weight[MAX_SUBJECTS][MAX_SUBJECTS]; /* conflict_weight[i][j] = #students causing i-j conflict */
    HashMap subject_index;               /* subject_id -> vertex index */
} ConflictGraph;

void graph_init(ConflictGraph *g);

/* Returns the index of the vertex (adds it if it does not exist yet). */
int graph_add_vertex(ConflictGraph *g, const char *subject_id,
                      const char *subject_name, const char *subject_type);

/* Adds an undirected edge a<->b with the given number of common
 * students contributing to it (weight is ADDED, since a pair of
 * subjects can share students found across multiple passes). */
void graph_add_edge(ConflictGraph *g, int a, int b, int extra_weight);

int graph_has_edge(const ConflictGraph *g, int a, int b);
int graph_degree(const ConflictGraph *g, int vertex);
int graph_find_vertex(const ConflictGraph *g, const char *subject_id);

#endif /* GRAPH_H */
