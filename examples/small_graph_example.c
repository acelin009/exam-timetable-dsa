/*
 * examples/small_graph_example.c
 * -------------------------------
 * A small, standalone demonstration of the coloring algorithms on
 * textbook graphs (triangle, square, K5) -- independent of the main
 * dataset, useful for a viva demo ("show me it works on something
 * simple I can check by hand").
 *
 * Build (from the project root):
 *   gcc -Isrc -o examples/small_graph_example \
 *       examples/small_graph_example.c src/graph.c src/max_heap.c \
 *       src/coloring.c src/hashmap.c
 *   ./examples/small_graph_example
 */

#include <stdio.h>
#include "../src/graph.h"
#include "../src/coloring.h"

static void demo(const char *title, void (*build)(ConflictGraph *)) {
    ConflictGraph g;
    graph_init(&g);
    build(&g);

    int colors[MAX_SUBJECTS];
    ColoringStats stats;
    int k = dsatur_coloring(&g, colors, &stats);

    printf("== %s ==\n", title);
    for (int v = 0; v < g.num_vertices; v++) {
        printf("  %s -> color %d\n", g.vertices[v].subject_id, colors[v]);
    }
    printf("  Colors used: %d (comparisons=%ld, steps=%ld)\n\n", k, stats.comparisons, stats.steps);
}

static void build_triangle(ConflictGraph *g) {
    int a = graph_add_vertex(g, "A", "A", "Core");
    int b = graph_add_vertex(g, "B", "B", "Core");
    int c = graph_add_vertex(g, "C", "C", "Core");
    graph_add_edge(g, a, b, 1);
    graph_add_edge(g, b, c, 1);
    graph_add_edge(g, a, c, 1);
}

static void build_square(ConflictGraph *g) {
    int a = graph_add_vertex(g, "A", "A", "Core");
    int b = graph_add_vertex(g, "B", "B", "Core");
    int c = graph_add_vertex(g, "C", "C", "Core");
    int d = graph_add_vertex(g, "D", "D", "Core");
    graph_add_edge(g, a, b, 1);
    graph_add_edge(g, b, c, 1);
    graph_add_edge(g, c, d, 1);
    graph_add_edge(g, d, a, 1);
}

static void build_k5(ConflictGraph *g) {
    int ids[5];
    char name[8];
    for (int i = 0; i < 5; i++) {
        snprintf(name, sizeof(name), "S%d", i);
        ids[i] = graph_add_vertex(g, name, name, "Core");
    }
    for (int i = 0; i < 5; i++)
        for (int j = i + 1; j < 5; j++)
            graph_add_edge(g, ids[i], ids[j], 1);
}

int main(void) {
    demo("Triangle (A-B, B-C, A-C) -- expect 3 colors", build_triangle);
    demo("Square/4-cycle (A-B-C-D-A) -- expect 2 colors", build_square);
    demo("K5 complete graph -- expect 5 colors", build_k5);
    return 0;
}
