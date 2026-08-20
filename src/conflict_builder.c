#include "conflict_builder.h"
#include <string.h>

void build_conflict_graph(const Dataset *ds, ConflictGraph *g) {
    graph_init(g);

    /* Register every subject as a vertex up front (order = subjects.csv
     * order), so vertex indices are stable and deterministic. */
    for (int i = 0; i < ds->num_subjects; i++) {
        graph_add_vertex(g, ds->subjects[i].subject_id,
                             ds->subjects[i].subject_name,
                             ds->subjects[i].subject_type);
    }

    /* Group registrations by student: student_subject_list[s] = the
     * list of subject-vertex-indices that student s takes. */
    static int student_subject_list[MAX_STUDENTS][16];
    static int student_subject_count[MAX_STUDENTS];
    memset(student_subject_count, 0, sizeof(student_subject_count));

    for (int i = 0; i < ds->num_registrations; i++) {
        const Registration *r = &ds->registrations[i];
        int student_idx;
        if (!hashmap_get(&ds->student_index, r->student_id, &student_idx)) continue;
        int subject_idx = graph_find_vertex(g, r->subject_id);
        if (subject_idx < 0) continue;

        int *cnt = &student_subject_count[student_idx];
        if (*cnt < 16) {
            student_subject_list[student_idx][*cnt] = subject_idx;
            (*cnt)++;
        }
        g->vertices[subject_idx].enrollment++;
    }

    /* For each student, generate every subject pair and add/increment
     * a conflict edge. Sorting is unnecessary for correctness (the
     * graph is undirected and add_edge is symmetric) but we still
     * iterate i<j so each unordered pair is only processed once per
     * student. */
    for (int s = 0; s < ds->num_students; s++) {
        int n = student_subject_count[s];
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                graph_add_edge(g, student_subject_list[s][i],
                                   student_subject_list[s][j], 1);
            }
        }
    }
}
