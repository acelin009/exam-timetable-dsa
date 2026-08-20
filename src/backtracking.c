#include "backtracking.h"
#include <string.h>

/* subject_students[v] = indices (into ds->students[]) of every
 * student registered for subject v. Built once per solve() call. */
static int subject_students[MAX_SUBJECTS][MAX_STUDENTS];
static int subject_student_count[MAX_SUBJECTS];

/* Mutable search state, undone on backtrack. */
static int day_count[MAX_STUDENTS][DAYS];              /* H4 */
static int slot_used_by_student[MAX_STUDENTS][DAYS][SLOTS_PER_DAY]; /* H1/H5 */

static void build_subject_students(const ConflictGraph *g, const Dataset *ds) {
    memset(subject_student_count, 0, sizeof(subject_student_count));
    for (int i = 0; i < ds->num_registrations; i++) {
        const Registration *r = &ds->registrations[i];
        int student_idx;
        if (!hashmap_get(&ds->student_index, r->student_id, &student_idx)) continue;
        int v = graph_find_vertex(g, r->subject_id);
        if (v < 0) continue;
        int *cnt = &subject_student_count[v];
        if (*cnt < MAX_STUDENTS) {
            subject_students[v][*cnt] = student_idx;
            (*cnt)++;
        }
    }
}

/* Pruning 1 (H6): reject if any already-colored neighbor holds this
 * slot. Pruning 2 (H4) and Pruning 3 (H5): reject if placing this
 * subject's exam here would push any of its students over the daily
 * limit or create a same-day adjacent-slot clash. */
static int is_feasible(const ConflictGraph *g, int v, int slot, const Timetable *t) {
    for (int k = 0; k < g->adjacency[v].size; k++) {
        int nb = g->adjacency[v].items[k];
        if (t->slot[nb] == slot) return 0; /* H6 */
    }

    int day = slot_to_day(slot);
    int ti = slot_to_time_idx(slot);

    for (int i = 0; i < subject_student_count[v]; i++) {
        int s = subject_students[v][i];

        if (slot_used_by_student[s][day][ti]) return 0; /* H1: student already has this exact slot taken (defensive) */
        if (day_count[s][day] + 1 > MAX_EXAMS_PER_DAY) return 0; /* H4 */

        if (FORBID_CONSECUTIVE) {
            if (ti > 0 && slot_used_by_student[s][day][ti - 1]) return 0; /* H5 */
            if (ti < SLOTS_PER_DAY - 1 && slot_used_by_student[s][day][ti + 1]) return 0; /* H5 */
        }
    }
    return 1;
}

static void apply_assignment(int v, int slot, Timetable *t) {
    t->slot[v] = slot;
    int day = slot_to_day(slot);
    int ti = slot_to_time_idx(slot);
    for (int i = 0; i < subject_student_count[v]; i++) {
        int s = subject_students[v][i];
        day_count[s][day]++;
        slot_used_by_student[s][day][ti] = 1;
    }
}

static void undo_assignment(int v, int slot, Timetable *t) {
    t->slot[v] = -1;
    int day = slot_to_day(slot);
    int ti = slot_to_time_idx(slot);
    for (int i = 0; i < subject_student_count[v]; i++) {
        int s = subject_students[v][i];
        day_count[s][day]--;
        slot_used_by_student[s][day][ti] = 0;
    }
}

/* Global slot-usage counter used only for value-ordering ("try the
 * most promising slots first" -- here, least-used-so-far, which
 * tends to spread exams evenly and helps the soft-constraint score
 * too). */
static int global_slot_usage[TOTAL_SLOTS];

static int solve_recursive(const ConflictGraph *g, const int order[MAX_SUBJECTS], int depth,
                            int n, Timetable *t, BacktrackStats *stats) {
    if (depth == n) return 1; /* all subjects assigned: success */

    int v = order[depth];

    /* Value ordering: try slots from least-used to most-used. */
    int slot_order[TOTAL_SLOTS];
    for (int i = 0; i < TOTAL_SLOTS; i++) slot_order[i] = i;
    for (int i = 0; i < TOTAL_SLOTS - 1; i++) {
        int best = i;
        for (int j = i + 1; j < TOTAL_SLOTS; j++) {
            if (global_slot_usage[slot_order[j]] < global_slot_usage[slot_order[best]]) best = j;
        }
        int tmp = slot_order[i]; slot_order[i] = slot_order[best]; slot_order[best] = tmp;
    }

    for (int si = 0; si < TOTAL_SLOTS; si++) {
        int slot = slot_order[si];
        stats->nodes_explored++;
        if (!is_feasible(g, v, slot, t)) continue;

        apply_assignment(v, slot, t);
        global_slot_usage[slot]++;

        if (solve_recursive(g, order, depth + 1, n, t, stats)) return 1;

        /* Failure below this choice: undo and try the next slot. */
        global_slot_usage[slot]--;
        undo_assignment(v, slot, t);
        stats->backtracks++;
    }
    return 0;
}

int backtracking_solve(const ConflictGraph *g, const Dataset *ds, const int order[MAX_SUBJECTS],
                        Timetable *timetable, BacktrackStats *stats) {
    build_subject_students(g, ds);
    memset(day_count, 0, sizeof(day_count));
    memset(slot_used_by_student, 0, sizeof(slot_used_by_student));
    memset(global_slot_usage, 0, sizeof(global_slot_usage));
    timetable_init(timetable, g->num_vertices);
    stats->nodes_explored = 0;
    stats->backtracks = 0;

    return solve_recursive(g, order, 0, g->num_vertices, timetable, stats);
}
