#include "constraint_checker.h"
#include <stdio.h>
#include <string.h>

/* Per-student list of (slot) for every subject they take, built
 * fresh each check so this module has no hidden state. */
static int build_student_slot_lists(const Timetable *t, const ConflictGraph *g, const Dataset *ds,
                                     int slots[MAX_STUDENTS][16], int counts[MAX_STUDENTS]) {
    memset(counts, 0, sizeof(int) * (size_t)ds->num_students);
    for (int i = 0; i < ds->num_registrations; i++) {
        const Registration *r = &ds->registrations[i];
        int student_idx;
        if (!hashmap_get(&ds->student_index, r->student_id, &student_idx)) continue;
        int v = graph_find_vertex(g, r->subject_id);
        if (v < 0) continue;
        int slot = t->slot[v];
        if (counts[student_idx] < 16) {
            slots[student_idx][counts[student_idx]++] = slot;
        }
    }
    return 1;
}

ConstraintReport check_all_hard_constraints(const Timetable *t, const ConflictGraph *g,
                                             const Dataset *ds, int verbose) {
    ConstraintReport rep;
    memset(&rep, 0, sizeof(rep));

    /* H3: every subject assigned exactly one slot. */
    for (int v = 0; v < g->num_vertices; v++) {
        if (t->slot[v] < 0) {
            rep.h3_violations++;
            if (verbose) fprintf(stderr, "H3 VIOLATION: subject '%s' has no assigned slot.\n", g->vertices[v].subject_id);
        }
    }

    /* H6: adjacent (conflicting) subjects must not share a slot. */
    for (int a = 0; a < g->num_vertices; a++) {
        for (int k = 0; k < g->adjacency[a].size; k++) {
            int b = g->adjacency[a].items[k];
            if (b <= a) continue; /* each undirected edge once */
            if (t->slot[a] >= 0 && t->slot[a] == t->slot[b]) {
                rep.h6_violations++;
                if (verbose) fprintf(stderr, "H6 VIOLATION: '%s' and '%s' share a student but both got slot %d.\n",
                                      g->vertices[a].subject_id, g->vertices[b].subject_id, t->slot[a]);
            }
        }
    }

    /* H1/H4/H5: per-student checks. */
    static int slots[MAX_STUDENTS][16];
    static int counts[MAX_STUDENTS];
    build_student_slot_lists(t, g, ds, slots, counts);

    for (int s = 0; s < ds->num_students; s++) {
        int n = counts[s];

        /* H1: no duplicate slot for this student. */
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                if (slots[s][i] == slots[s][j]) {
                    rep.h1_violations++;
                    if (verbose) fprintf(stderr, "H1 VIOLATION: student '%s' has two exams in slot %d.\n",
                                          ds->students[s].student_id, slots[s][i]);
                }
            }
        }

        /* H4: max exams per day. */
        int per_day[DAYS];
        memset(per_day, 0, sizeof(per_day));
        for (int i = 0; i < n; i++) {
            if (slots[s][i] < 0) continue;
            per_day[slot_to_day(slots[s][i])]++;
        }
        for (int d = 0; d < DAYS; d++) {
            if (per_day[d] > MAX_EXAMS_PER_DAY) {
                rep.h4_violations++;
                if (verbose) fprintf(stderr, "H4 VIOLATION: student '%s' has %d exams on %s (max %d).\n",
                                      ds->students[s].student_id, per_day[d], DAY_NAMES[d], MAX_EXAMS_PER_DAY);
            }
        }

        /* H5: no consecutive time-slots on the same day. */
        if (FORBID_CONSECUTIVE) {
            int used[DAYS][SLOTS_PER_DAY];
            memset(used, 0, sizeof(used));
            for (int i = 0; i < n; i++) {
                if (slots[s][i] < 0) continue;
                used[slot_to_day(slots[s][i])][slot_to_time_idx(slots[s][i])] = 1;
            }
            for (int d = 0; d < DAYS; d++) {
                for (int ti = 0; ti < SLOTS_PER_DAY - 1; ti++) {
                    if (used[d][ti] && used[d][ti + 1]) {
                        rep.h5_violations++;
                        if (verbose) fprintf(stderr,
                            "H5 VIOLATION: student '%s' has consecutive exams on %s (%s and %s).\n",
                            ds->students[s].student_id, DAY_NAMES[d], SLOT_TIMES[ti], SLOT_TIMES[ti + 1]);
                    }
                }
            }
        }
    }

    /* H2 (all students of a subject share its slot) holds by
     * construction: Timetable maps one slot per subject, so there is
     * nothing to check -- record it as satisfied. */
    rep.h2_violations = 0;

    rep.is_valid = (rep.h1_violations == 0 && rep.h2_violations == 0 && rep.h3_violations == 0 &&
                    rep.h4_violations == 0 && rep.h5_violations == 0 && rep.h6_violations == 0);
    return rep;
}
