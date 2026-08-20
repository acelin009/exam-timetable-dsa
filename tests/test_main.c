/*
 * tests/test_main.c
 * ------------------
 * A small self-contained test harness (no external framework --
 * this is a C project, not Python/pytest). Each test_* function
 * returns 1 on pass, 0 on fail, and prints a diagnostic on failure.
 * main() runs them all and reports a summary; a non-zero exit code
 * means at least one test failed, so this doubles as a CI gate.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../src/graph.h"
#include "../src/max_heap.h"
#include "../src/hashmap.h"
#include "../src/data_loader.h"
#include "../src/data_validator.h"
#include "../src/conflict_builder.h"
#include "../src/coloring.h"
#include "../src/timetable.h"
#include "../src/constraint_checker.h"
#include "../src/backtracking.h"

static int tests_run = 0, tests_passed = 0;

#define CHECK(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; } \
    else { printf("  FAIL: %s\n", msg); } \
} while (0)

#define SECTION(name) printf("\n-- %s --\n", name)

/* ============================================================ */
/* Graph tests                                                   */
/* ============================================================ */
static void test_graph(void) {
    SECTION("Graph tests");
    ConflictGraph g;
    graph_init(&g);

    int a = graph_add_vertex(&g, "A", "Subject A", "Core");
    int b = graph_add_vertex(&g, "B", "Subject B", "Core");
    int c = graph_add_vertex(&g, "C", "Subject C", "Core");
    CHECK(g.num_vertices == 3, "adding 3 vertices should give num_vertices==3");

    /* re-adding an existing vertex must not create a duplicate */
    int a_again = graph_add_vertex(&g, "A", "Subject A", "Core");
    CHECK(a_again == a, "re-adding vertex A should return the same index");
    CHECK(g.num_vertices == 3, "re-adding a vertex must not change num_vertices");

    graph_add_edge(&g, a, b, 5);
    CHECK(graph_has_edge(&g, a, b), "A-B edge should exist after add_edge");
    CHECK(graph_has_edge(&g, b, a), "edge should be undirected: B-A should exist too");
    CHECK(!graph_has_edge(&g, a, c), "A-C should not exist yet");
    CHECK(g.weight[a][b] == 5, "A-B weight should be 5");
    CHECK(g.weight[b][a] == 5, "weight should be symmetric");

    /* duplicate edge should not create a second adjacency entry */
    graph_add_edge(&g, a, b, 3);
    CHECK(graph_degree(&g, a) == 1, "duplicate edge must not increase degree beyond 1 neighbor");
    CHECK(g.weight[a][b] == 8, "duplicate edge should ADD weight (5+3=8)");

    graph_add_edge(&g, b, c, 1);
    CHECK(graph_degree(&g, b) == 2, "B should have degree 2 (A and C)");

    CHECK(graph_find_vertex(&g, "A") == a, "graph_find_vertex should locate A");
    CHECK(graph_find_vertex(&g, "ZZZ") == -1, "graph_find_vertex should return -1 for unknown id");
}

/* ============================================================ */
/* MaxHeap tests                                                 */
/* ============================================================ */
static void test_heap(void) {
    SECTION("MaxHeap tests");
    MaxHeap h;
    heap_init(&h);
    CHECK(heap_is_empty(&h), "freshly initialized heap should be empty");

    heap_insert(&h, 0, 1, 5, 10);
    heap_insert(&h, 1, 3, 2, 10);
    heap_insert(&h, 2, 2, 9, 10);
    CHECK(!heap_is_empty(&h), "heap with 3 items should not be empty");

    HeapNode top = heap_peek(&h);
    CHECK(top.id == 1, "peek should return the node with highest saturation (id=1, sat=3)");

    HeapNode m1 = heap_extract_max(&h);
    CHECK(m1.id == 1, "first extract should return id=1 (saturation 3)");
    HeapNode m2 = heap_extract_max(&h);
    CHECK(m2.id == 2, "second extract should return id=2 (saturation 2)");
    HeapNode m3 = heap_extract_max(&h);
    CHECK(m3.id == 0, "third extract should return id=0 (saturation 1)");
    CHECK(heap_is_empty(&h), "heap should be empty after extracting all items");

    /* tie-break by degree, then enrollment */
    heap_init(&h);
    heap_insert(&h, 0, 1, 5, 100); /* sat=1 deg=5 */
    heap_insert(&h, 1, 1, 5, 200); /* sat=1 deg=5, higher enrollment */
    heap_insert(&h, 2, 1, 9, 50);  /* sat=1, higher degree */
    HeapNode tb = heap_extract_max(&h);
    CHECK(tb.id == 2, "tie-break: same saturation should prefer higher degree");

    /* update_saturation reorders correctly */
    heap_init(&h);
    heap_insert(&h, 0, 0, 1, 1);
    heap_insert(&h, 1, 0, 1, 1);
    heap_insert(&h, 2, 0, 1, 1);
    heap_update_saturation(&h, 2, 5);
    HeapNode after_update = heap_extract_max(&h);
    CHECK(after_update.id == 2, "after raising id=2's saturation to 5, it should be extracted first");

    /* heap_build (bottom-up heapify) */
    HeapNode nodes[4] = {
        {0, 2, 1, 1}, {1, 4, 1, 1}, {2, 1, 1, 1}, {3, 3, 1, 1}
    };
    heap_init(&h);
    heap_build(&h, nodes, 4);
    HeapNode b1 = heap_extract_max(&h);
    CHECK(b1.id == 1, "heap_build: highest saturation (id=1, sat=4) should come out first");
}

/* ============================================================ */
/* HashMap tests                                                 */
/* ============================================================ */
static void test_hashmap(void) {
    SECTION("HashMap tests");
    HashMap map;
    hashmap_init(&map, 4); /* small capacity to force growth */

    hashmap_put(&map, "MAT", 0);
    hashmap_put(&map, "DB", 1);
    hashmap_put(&map, "CN", 2);
    hashmap_put(&map, "AI", 3);
    hashmap_put(&map, "SE", 4); /* forces at least one rehash */

    int v;
    CHECK(hashmap_get(&map, "MAT", &v) && v == 0, "MAT should map to 0");
    CHECK(hashmap_get(&map, "SE", &v) && v == 4, "SE should map to 4 after growth");
    CHECK(!hashmap_contains(&map, "NOPE"), "unknown key should not be found");

    hashmap_put(&map, "MAT", 99); /* overwrite */
    CHECK(hashmap_get(&map, "MAT", &v) && v == 99, "overwriting MAT should update its value");

    hashmap_free(&map);
}

/* ============================================================ */
/* Dataset loading + validation tests                             */
/* ============================================================ */
static void test_dataset(void) {
    SECTION("Dataset tests");
    Dataset ds;
    dataset_init(&ds);
    int ok = dataset_load(&ds, "data");
    CHECK(ok, "dataset_load should succeed on the real data/ directory");
    CHECK(ds.num_students == 60, "expected 60 students in the supplied dataset");
    CHECK(ds.num_subjects == 11, "expected 11 subjects in the supplied dataset");
    CHECK(ds.num_registrations == 264, "expected 264 registrations in the supplied dataset");

    ValidationReport rep = validate_dataset(&ds);
    CHECK(rep.error_count == 0, "the real dataset should validate with zero errors");

    /* Failure case: nonexistent directory */
    Dataset bad;
    dataset_init(&bad);
    int bad_ok = dataset_load(&bad, "data/does_not_exist_dir");
    CHECK(!bad_ok, "loading from a missing directory should fail gracefully (return 0)");
}

/* ============================================================ */
/* Conflict-graph construction tests                              */
/* ============================================================ */
static void test_conflicts(void) {
    SECTION("Conflict-graph tests");
    Dataset ds;
    dataset_init(&ds);
    dataset_load(&ds, "data");
    ConflictGraph g;
    build_conflict_graph(&ds, &g);

    CHECK(g.num_vertices == 11, "graph should have 11 vertices, one per subject");

    int mat = graph_find_vertex(&g, "MAT");
    int db = graph_find_vertex(&g, "DB");
    int ai = graph_find_vertex(&g, "AI");
    CHECK(mat >= 0 && db >= 0 && ai >= 0, "MAT, DB, AI vertices should exist");
    CHECK(graph_has_edge(&g, mat, db), "MAT and DB share class-A students -> edge should exist");
    CHECK(g.weight[mat][db] == 20, "MAT-DB conflict weight should be 20 (class A size)");

    /* no duplicate conflicts: an edge is stored once regardless of order */
    CHECK(graph_has_edge(&g, db, mat), "edge lookup should be symmetric");

    /* enrollment recorded */
    CHECK(g.vertices[mat].enrollment == 60, "MAT enrollment should be 60 (every student takes it)");
}

/* ============================================================ */
/* Coloring tests: triangle needs 3 colors, square needs 2       */
/* ============================================================ */
static void test_coloring_small_graphs(void) {
    SECTION("Coloring tests (small known graphs)");

    /* Triangle: A-B, B-C, A-C -- requires exactly 3 colors */
    {
        ConflictGraph g;
        graph_init(&g);
        int a = graph_add_vertex(&g, "A", "A", "Core");
        int b = graph_add_vertex(&g, "B", "B", "Core");
        int c = graph_add_vertex(&g, "C", "C", "Core");
        graph_add_edge(&g, a, b, 1);
        graph_add_edge(&g, b, c, 1);
        graph_add_edge(&g, a, c, 1);

        int colors[MAX_SUBJECTS];
        int k = dsatur_coloring(&g, colors, NULL);
        CHECK(k == 3, "triangle graph should need exactly 3 colors via DSATUR");
        CHECK(colors[a] != colors[b] && colors[b] != colors[c] && colors[a] != colors[c],
              "triangle: all three vertices must get distinct colors");

        int gcolors[MAX_SUBJECTS];
        int gk = greedy_coloring(&g, gcolors, NULL);
        CHECK(gk == 3, "triangle graph should need exactly 3 colors via greedy too");
    }

    /* Square: A-B, B-C, C-D, D-A -- bipartite, requires exactly 2 colors */
    {
        ConflictGraph g;
        graph_init(&g);
        int a = graph_add_vertex(&g, "A", "A", "Core");
        int b = graph_add_vertex(&g, "B", "B", "Core");
        int c = graph_add_vertex(&g, "C", "C", "Core");
        int d = graph_add_vertex(&g, "D", "D", "Core");
        graph_add_edge(&g, a, b, 1);
        graph_add_edge(&g, b, c, 1);
        graph_add_edge(&g, c, d, 1);
        graph_add_edge(&g, d, a, 1);

        int colors[MAX_SUBJECTS];
        int k = dsatur_coloring(&g, colors, NULL);
        CHECK(k == 2, "square (4-cycle) graph should need exactly 2 colors");
        CHECK(colors[a] == colors[c], "square: opposite corners A and C should share a color");
        CHECK(colors[b] == colors[d], "square: opposite corners B and D should share a color");
        CHECK(colors[a] != colors[b], "square: adjacent corners A and B must differ");
    }

    /* Complete graph K3 (same as triangle but re-stated per spec Case): 3 colors */
    {
        ConflictGraph g;
        graph_init(&g);
        int a = graph_add_vertex(&g, "A", "A", "Core");
        int b = graph_add_vertex(&g, "B", "B", "Core");
        int c = graph_add_vertex(&g, "C", "C", "Core");
        graph_add_edge(&g, a, b, 1);
        graph_add_edge(&g, a, c, 1);
        graph_add_edge(&g, b, c, 1);
        int colors[MAX_SUBJECTS];
        int k = dsatur_coloring(&g, colors, NULL);
        CHECK(k == 3, "complete graph K3 should need exactly 3 colors");
    }

    /* No conflicts: every vertex can take color 0 */
    {
        ConflictGraph g;
        graph_init(&g);
        graph_add_vertex(&g, "A", "A", "Core");
        graph_add_vertex(&g, "B", "B", "Core");
        graph_add_vertex(&g, "C", "C", "Core");
        int colors[MAX_SUBJECTS];
        int k = dsatur_coloring(&g, colors, NULL);
        CHECK(k == 1, "graph with no edges should need only 1 color");
    }

    /* Only one subject */
    {
        ConflictGraph g;
        graph_init(&g);
        graph_add_vertex(&g, "ONLY", "Only Subject", "Core");
        int colors[MAX_SUBJECTS];
        int k = dsatur_coloring(&g, colors, NULL);
        CHECK(k == 1, "single-vertex graph should need exactly 1 color");
    }

    /* Every subject conflicts with every other (complete graph K5) */
    {
        ConflictGraph g;
        graph_init(&g);
        int ids[5];
        char name[8];
        for (int i = 0; i < 5; i++) { snprintf(name, sizeof(name), "S%d", i); ids[i] = graph_add_vertex(&g, name, name, "Core"); }
        for (int i = 0; i < 5; i++)
            for (int j = i + 1; j < 5; j++)
                graph_add_edge(&g, ids[i], ids[j], 1);
        int colors[MAX_SUBJECTS];
        int k = dsatur_coloring(&g, colors, NULL);
        CHECK(k == 5, "K5 complete graph should need exactly 5 colors (one per vertex)");
    }
}

/* ============================================================ */
/* Constraint checker tests                                       */
/* ============================================================ */
static void test_constraints(void) {
    SECTION("Constraint checker tests");

    /* Build a tiny synthetic dataset: 2 students, 3 subjects. */
    Dataset ds;
    dataset_init(&ds);
    strcpy(ds.students[0].student_id, "S1"); strcpy(ds.students[0].class_name, "X");
    strcpy(ds.students[1].student_id, "S2"); strcpy(ds.students[1].class_name, "X");
    ds.num_students = 2;
    hashmap_put(&ds.student_index, "S1", 0);
    hashmap_put(&ds.student_index, "S2", 1);

    ConflictGraph g;
    graph_init(&g);
    int x = graph_add_vertex(&g, "X1", "Subj X1", "Core");
    int y = graph_add_vertex(&g, "X2", "Subj X2", "Core");
    int z = graph_add_vertex(&g, "X3", "Subj X3", "Core");
    graph_add_edge(&g, x, y, 1); /* S1 takes both X1 and X2 */

    strcpy(ds.registrations[0].student_id, "S1"); strcpy(ds.registrations[0].subject_id, "X1");
    strcpy(ds.registrations[1].student_id, "S1"); strcpy(ds.registrations[1].subject_id, "X2");
    strcpy(ds.registrations[2].student_id, "S2"); strcpy(ds.registrations[2].subject_id, "X3");
    ds.num_registrations = 3;

    /* Case 1: valid assignment. X1 (slot 0 = day0/time0) and X2
     * (slot 2 = day1/time0) are on different days for S1, so there
     * is no H5 (consecutive) issue; z reuses slot 0 but shares no
     * student with x, so H6 is fine too. */
    Timetable t;
    timetable_init(&t, 3);
    t.slot[x] = 0; t.slot[y] = 2; t.slot[z] = 0;
    ConstraintReport rep = check_all_hard_constraints(&t, &g, &ds, 0);
    CHECK(rep.is_valid, "non-conflicting slot assignment should be reported VALID");
    CHECK(rep.h6_violations == 0, "no H6 violation expected when adjacent subjects use different slots");

    /* Case 2: invalid -- X1 and X2 (adjacent, shared student S1) given the SAME slot */
    Timetable bad;
    timetable_init(&bad, 3);
    bad.slot[x] = 0; bad.slot[y] = 0; bad.slot[z] = 1;
    ConstraintReport rep2 = check_all_hard_constraints(&bad, &g, &ds, 0);
    CHECK(!rep2.is_valid, "same-slot assignment for adjacent subjects should be INVALID");
    CHECK(rep2.h6_violations == 1, "exactly one H6 violation expected (X1-X2)");
    CHECK(rep2.h1_violations == 1, "student S1 having two exams in the same slot should be one H1 violation");

    /* Case 3: unassigned subject -> H3 violation */
    Timetable partial;
    timetable_init(&partial, 3);
    partial.slot[x] = 0; partial.slot[y] = 1; /* z left as -1 */
    ConstraintReport rep3 = check_all_hard_constraints(&partial, &g, &ds, 0);
    CHECK(rep3.h3_violations == 1, "leaving a subject unassigned should be exactly one H3 violation");
    CHECK(!rep3.is_valid, "a timetable with an unassigned subject must be INVALID");
}

/* ============================================================ */
/* Backtracking tests                                             */
/* ============================================================ */
static void test_backtracking(void) {
    SECTION("Backtracking tests");

    /* Construct a small case where the naive color->slot mapping
     * would violate the daily-limit / consecutive rules, forcing
     * the backtracking repair to actually search: one student takes
     * 3 mutually-conflicting subjects, so graph coloring alone needs
     * 3 colors/slots -- with SLOTS_PER_DAY=2, two of those land on
     * the same day and one pair is guaranteed adjacent unless the
     * solver reorders them across days. */
    Dataset ds;
    dataset_init(&ds);
    strcpy(ds.students[0].student_id, "S1"); strcpy(ds.students[0].class_name, "X");
    ds.num_students = 1;
    hashmap_put(&ds.student_index, "S1", 0);

    ConflictGraph g;
    graph_init(&g);
    int a = graph_add_vertex(&g, "A", "A", "Core");
    int b = graph_add_vertex(&g, "B", "B", "Core");
    int c = graph_add_vertex(&g, "C", "C", "Core");
    graph_add_edge(&g, a, b, 1);
    graph_add_edge(&g, b, c, 1);
    graph_add_edge(&g, a, c, 1); /* triangle: needs 3 distinct slots */

    ds.num_registrations = 3;
    strcpy(ds.registrations[0].student_id, "S1"); strcpy(ds.registrations[0].subject_id, "A");
    strcpy(ds.registrations[1].student_id, "S1"); strcpy(ds.registrations[1].subject_id, "B");
    strcpy(ds.registrations[2].student_id, "S1"); strcpy(ds.registrations[2].subject_id, "C");

    int order[MAX_SUBJECTS] = {a, b, c};
    Timetable result;
    BacktrackStats stats;
    int ok = backtracking_solve(&g, &ds, order, &result, &stats);
    CHECK(ok, "backtracking should find a valid assignment for a small triangle case");

    ConstraintReport rep = check_all_hard_constraints(&result, &g, &ds, 0);
    CHECK(rep.is_valid, "the backtracking-produced timetable must satisfy all hard constraints");
    CHECK(stats.nodes_explored > 0, "backtracking should have explored at least one node");
}

/* ============================================================ */
/* End-to-end test on the real supplied dataset                  */
/* ============================================================ */
static void test_end_to_end(void) {
    SECTION("End-to-end test (real dataset)");

    Dataset ds;
    dataset_init(&ds);
    int loaded = dataset_load(&ds, "data");
    CHECK(loaded, "end-to-end: dataset should load");

    ValidationReport vrep = validate_dataset(&ds);
    CHECK(vrep.error_count == 0, "end-to-end: dataset should validate cleanly");

    ConflictGraph g;
    build_conflict_graph(&ds, &g);
    CHECK(g.num_vertices == 11, "end-to-end: graph should have 11 vertices");

    int colors[MAX_SUBJECTS];
    int k = dsatur_coloring(&g, colors, NULL);
    CHECK(k <= TOTAL_SLOTS, "end-to-end: DSATUR result must fit within configured TOTAL_SLOTS");

    Timetable t;
    timetable_init(&t, g.num_vertices);
    for (int v = 0; v < g.num_vertices; v++) t.slot[v] = colors[v];

    ConstraintReport rep = check_all_hard_constraints(&t, &g, &ds, 0);
    if (!rep.is_valid) {
        int order[MAX_SUBJECTS];
        for (int i = 0; i < g.num_vertices; i++) order[i] = i;
        for (int i = 0; i < g.num_vertices - 1; i++) {
            int best = i;
            for (int j = i + 1; j < g.num_vertices; j++)
                if (graph_degree(&g, order[j]) > graph_degree(&g, order[best])) best = j;
            int tmp = order[i]; order[i] = order[best]; order[best] = tmp;
        }
        BacktrackStats stats;
        Timetable repaired;
        int ok = backtracking_solve(&g, &ds, order, &repaired, &stats);
        CHECK(ok, "end-to-end: backtracking repair should succeed on the real dataset");
        t = repaired;
        rep = check_all_hard_constraints(&t, &g, &ds, 0);
    }

    CHECK(rep.is_valid, "end-to-end: valid == True is required for the full pipeline on the real dataset");
}

int main(void) {
    printf("============================================================\n");
    printf("RUNNING TEST SUITE\n");
    printf("============================================================\n");

    test_graph();
    test_heap();
    test_hashmap();
    test_dataset();
    test_conflicts();
    test_coloring_small_graphs();
    test_constraints();
    test_backtracking();
    test_end_to_end();

    printf("\n============================================================\n");
    printf("RESULTS: %d / %d tests passed\n", tests_passed, tests_run);
    printf("============================================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
