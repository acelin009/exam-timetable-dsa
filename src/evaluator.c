#include "evaluator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

SoftEvaluation evaluate_soft_constraints(const Timetable *t, const ConflictGraph *g,
                                          const Dataset *ds, int minimum_colors_needed) {
    SoftEvaluation e;
    memset(&e, 0, sizeof(e));

    /* ---- S2: same-day exam pairs per student ---- */
    static int slots[MAX_STUDENTS][16];
    static int counts[MAX_STUDENTS];
    memset(counts, 0, sizeof(int) * (size_t)ds->num_students);
    for (int i = 0; i < ds->num_registrations; i++) {
        const Registration *r = &ds->registrations[i];
        int student_idx;
        if (!hashmap_get(&ds->student_index, r->student_id, &student_idx)) continue;
        int v = graph_find_vertex(g, r->subject_id);
        if (v < 0 || t->slot[v] < 0) continue;
        if (counts[student_idx] < 16) slots[student_idx][counts[student_idx]++] = t->slot[v];
    }
    for (int s = 0; s < ds->num_students; s++) {
        int per_day[DAYS];
        memset(per_day, 0, sizeof(per_day));
        for (int i = 0; i < counts[s]; i++) per_day[slot_to_day(slots[s][i])]++;
        for (int d = 0; d < DAYS; d++) {
            if (per_day[d] >= 2) e.same_day_pairs++;
        }
    }
    e.same_day_penalty = e.same_day_pairs * PENALTY_PER_SAME_DAY_PAIR;

    /* ---- S1/S5: evenness of subjects-per-day distribution ---- */
    int subjects_per_day[DAYS];
    memset(subjects_per_day, 0, sizeof(subjects_per_day));
    int days_used = 0;
    for (int v = 0; v < g->num_vertices; v++) {
        if (t->slot[v] >= 0) subjects_per_day[slot_to_day(t->slot[v])]++;
    }
    for (int d = 0; d < DAYS; d++) if (subjects_per_day[d] > 0) days_used++;
    double avg = days_used > 0 ? (double)g->num_vertices / (double)days_used : 0.0;
    for (int d = 0; d < DAYS; d++) {
        if (subjects_per_day[d] == 0) continue;
        double diff = subjects_per_day[d] - avg;
        if (diff < 0) diff = -diff;
        e.unevenness_units += (int)(diff + 0.5);
    }
    e.unevenness_penalty = e.unevenness_units * PENALTY_PER_UNEVEN_DAY_UNIT;

    /* ---- S6: slots used beyond the coloring-theoretic minimum ---- */
    int max_slot_used = -1;
    for (int v = 0; v < g->num_vertices; v++) if (t->slot[v] > max_slot_used) max_slot_used = t->slot[v];
    int slots_used = max_slot_used + 1;
    e.extra_slots_used = slots_used - minimum_colors_needed;
    if (e.extra_slots_used < 0) e.extra_slots_used = 0;
    e.extra_slots_penalty = e.extra_slots_used * PENALTY_PER_EXTRA_SLOT_USED;

    /* ---- S3: high-conflict subject pairs placed on the same day ---- */
    for (int a = 0; a < g->num_vertices; a++) {
        for (int k = 0; k < g->adjacency[a].size; k++) {
            int b = g->adjacency[a].items[k];
            if (b <= a) continue;
            if (g->weight[a][b] >= HIGH_CONFLICT_THRESHOLD &&
                t->slot[a] >= 0 && t->slot[b] >= 0 &&
                slot_to_day(t->slot[a]) == slot_to_day(t->slot[b])) {
                e.high_conflict_adjacent_days++;
            }
        }
    }
    e.high_conflict_penalty = e.high_conflict_adjacent_days * PENALTY_HIGH_CONFLICT_ADJACENT;

    int total_penalty = e.same_day_penalty + e.unevenness_penalty + e.extra_slots_penalty + e.high_conflict_penalty;
    e.final_score = 100 - total_penalty;
    if (e.final_score < 0) e.final_score = 0;
    if (e.final_score > 100) e.final_score = 100;
    return e;
}

void print_soft_evaluation(const SoftEvaluation *e) {
    printf("Soft constraint evaluation:\n");
    printf("  S2 same-day exam pairs:        %3d  (penalty %d = %d x %d)\n",
           e->same_day_pairs, e->same_day_penalty, e->same_day_pairs, PENALTY_PER_SAME_DAY_PAIR);
    printf("  S1/S5 unevenness units:        %3d  (penalty %d = %d x %d)\n",
           e->unevenness_units, e->unevenness_penalty, e->unevenness_units, PENALTY_PER_UNEVEN_DAY_UNIT);
    printf("  S6 extra slots used:           %3d  (penalty %d = %d x %d)\n",
           e->extra_slots_used, e->extra_slots_penalty, e->extra_slots_used, PENALTY_PER_EXTRA_SLOT_USED);
    printf("  S3 high-conflict same-day:     %3d  (penalty %d = %d x %d)\n",
           e->high_conflict_adjacent_days, e->high_conflict_penalty, e->high_conflict_adjacent_days, PENALTY_HIGH_CONFLICT_ADJACENT);
    printf("  ------------------------------------------------\n");
    printf("  Final soft-constraint score:   %3d / 100\n", e->final_score);
}
