#include "coloring.h"
#include "max_heap.h"
#include <string.h>

int greedy_coloring(const ConflictGraph *g, int color_out[MAX_SUBJECTS], ColoringStats *stats) {
    int n = g->num_vertices;
    memset(color_out, -1, sizeof(int) * (size_t)MAX_SUBJECTS);
    if (stats) { stats->comparisons = 0; stats->steps = 0; }

    /* "Largest degree first" ordering: process the vertices with the
     * most conflicts first, as a fixed order (no dynamic priority
     * queue -- this is the naive baseline the assignment explicitly
     * asks DSATUR to be compared against). */
    int order[MAX_SUBJECTS];
    for (int i = 0; i < n; i++) order[i] = i;
    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if (graph_degree(g, order[j]) > graph_degree(g, order[best])) best = j;
        }
        int tmp = order[i]; order[i] = order[best]; order[best] = tmp;
    }

    int max_color_used = -1;
    for (int oi = 0; oi < n; oi++) {
        int v = order[oi];
        int used[MAX_SUBJECTS];
        memset(used, 0, sizeof(used));
        for (int k = 0; k < g->adjacency[v].size; k++) {
            int nb = g->adjacency[v].items[k];
            if (color_out[nb] >= 0) used[color_out[nb]] = 1;
        }
        int c = 0;
        while (1) {
            if (stats) stats->comparisons++;
            if (!used[c]) break;
            c++;
        }
        color_out[v] = c;
        if (c > max_color_used) max_color_used = c;
        if (stats) stats->steps++;
    }
    return max_color_used + 1;
}

int dsatur_coloring(const ConflictGraph *g, int color_out[MAX_SUBJECTS], ColoringStats *stats) {
    int n = g->num_vertices;
    memset(color_out, -1, sizeof(int) * (size_t)MAX_SUBJECTS);
    if (stats) { stats->comparisons = 0; stats->steps = 0; }

    /* neighbor_colors[v] = set of distinct colors used by v's
     * already-colored neighbors. Its size IS the saturation degree. */
    IntSet neighbor_colors[MAX_SUBJECTS];
    for (int i = 0; i < n; i++) intset_init(&neighbor_colors[i]);

    /* Build the priority queue: one node per subject, initial
     * saturation 0 (nothing colored yet), tie-broken by degree then
     * enrollment. Built in O(n) via bottom-up heapify. */
    HeapNode nodes[MAX_SUBJECTS];
    for (int i = 0; i < n; i++) {
        nodes[i].id = i;
        nodes[i].saturation = 0;
        nodes[i].degree = graph_degree(g, i);
        nodes[i].enrollment = g->vertices[i].enrollment;
    }
    MaxHeap heap;
    heap_init(&heap);
    heap_build(&heap, nodes, n);

    int max_color_used = -1;
    while (!heap_is_empty(&heap)) {
        HeapNode top = heap_extract_max(&heap);
        int v = top.id;

        /* Lowest feasible color: not used by any already-colored
         * neighbor. */
        int used[MAX_SUBJECTS];
        memset(used, 0, sizeof(used));
        for (int k = 0; k < g->adjacency[v].size; k++) {
            int nb = g->adjacency[v].items[k];
            if (color_out[nb] >= 0) used[color_out[nb]] = 1;
        }
        int c = 0;
        while (1) {
            if (stats) stats->comparisons++;
            if (!used[c]) break;
            c++;
        }
        color_out[v] = c;
        if (c > max_color_used) max_color_used = c;
        if (stats) stats->steps++;

        /* Propagate: for every still-uncolored neighbor, record that
         * color c is now used among its neighbors, and if that's a
         * NEW color for that neighbor, its saturation degree went up
         * -- push the change into the heap in O(log n). */
        for (int k = 0; k < g->adjacency[v].size; k++) {
            int u = g->adjacency[v].items[k];
            if (color_out[u] >= 0) continue; /* already colored, not in heap anymore */
            if (intset_add(&neighbor_colors[u], c)) {
                heap_update_saturation(&heap, u, neighbor_colors[u].size);
            }
        }
    }

    for (int i = 0; i < n; i++) intset_free(&neighbor_colors[i]);
    return max_color_used + 1;
}
