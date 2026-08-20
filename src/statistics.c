#include "statistics.h"
#include <stdio.h>
#include <string.h>

DatasetStatistics compute_statistics(const Timetable *t, const ConflictGraph *g, const Dataset *ds) {
    DatasetStatistics s;
    memset(&s, 0, sizeof(s));

    s.total_students = ds->num_students;
    s.total_subjects = g->num_vertices;
    s.total_registrations = ds->num_registrations;

    int edges = 0;
    int most_degree = -1, most_idx = -1;
    int most_enroll = -1, enroll_idx = -1;
    for (int a = 0; a < g->num_vertices; a++) {
        edges += g->adjacency[a].size;
        if (graph_degree(g, a) > most_degree) { most_degree = graph_degree(g, a); most_idx = a; }
        if (g->vertices[a].enrollment > most_enroll) { most_enroll = g->vertices[a].enrollment; enroll_idx = a; }
    }
    s.total_conflicts = edges / 2; /* each undirected edge counted from both ends */
    if (most_idx >= 0) strncpy(s.most_conflicting_subject, g->vertices[most_idx].subject_id, MAX_NAME_LEN - 1);
    if (enroll_idx >= 0) strncpy(s.highest_enrollment_subject, g->vertices[enroll_idx].subject_id, MAX_NAME_LEN - 1);

    int max_slot_used = -1;
    int subjects_per_day[DAYS];
    memset(subjects_per_day, 0, sizeof(subjects_per_day));
    for (int v = 0; v < g->num_vertices; v++) {
        if (t->slot[v] > max_slot_used) max_slot_used = t->slot[v];
        if (t->slot[v] >= 0) subjects_per_day[slot_to_day(t->slot[v])]++;
    }
    s.slots_used = max_slot_used + 1;
    for (int d = 0; d < DAYS; d++) if (subjects_per_day[d] > 0) s.days_used++;
    s.average_subjects_per_day = s.days_used > 0 ? (double)g->num_vertices / (double)s.days_used : 0.0;

    /* per-student stats: max exams/day, min gap between any two exams */
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

    int global_min_gap = -1;
    for (int st = 0; st < ds->num_students; st++) {
        int n = counts[st];
        /* simple insertion sort - n is tiny (<=16) */
        for (int i = 1; i < n; i++) {
            int key = slots[st][i], j = i - 1;
            while (j >= 0 && slots[st][j] > key) { slots[st][j + 1] = slots[st][j]; j--; }
            slots[st][j + 1] = key;
        }
        int per_day[DAYS];
        memset(per_day, 0, sizeof(per_day));
        for (int i = 0; i < n; i++) per_day[slot_to_day(slots[st][i])]++;
        for (int d = 0; d < DAYS; d++) if (per_day[d] > s.max_exams_per_student_per_day) s.max_exams_per_student_per_day = per_day[d];

        for (int i = 1; i < n; i++) {
            int gap = slots[st][i] - slots[st][i - 1];
            if (global_min_gap == -1 || gap < global_min_gap) global_min_gap = gap;
        }
    }
    s.min_gap_slots = global_min_gap;

    return s;
}

void print_statistics(const DatasetStatistics *s) {
    printf("Total students:                 %d\n", s->total_students);
    printf("Total subjects:                 %d\n", s->total_subjects);
    printf("Total registrations:            %d\n", s->total_registrations);
    printf("Total conflicts (edges):        %d\n", s->total_conflicts);
    printf("Exam slots used:                %d\n", s->slots_used);
    printf("Days used:                      %d\n", s->days_used);
    printf("Max exams/student/day:          %d\n", s->max_exams_per_student_per_day);
    printf("Min gap between exams (slots):  %d\n", s->min_gap_slots);
    printf("Most conflicting subject:       %s\n", s->most_conflicting_subject);
    printf("Highest enrollment subject:     %s\n", s->highest_enrollment_subject);
    printf("Average subjects/day:           %.2f\n", s->average_subjects_per_day);
}

void export_statistics_txt(const DatasetStatistics *s, const char *out_dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/timetable_statistics.txt", out_dir);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "TIMETABLE STATISTICS\n=====================\n\n");
    fprintf(f, "Total students: %d\n", s->total_students);
    fprintf(f, "Total subjects: %d\n", s->total_subjects);
    fprintf(f, "Total registrations: %d\n", s->total_registrations);
    fprintf(f, "Total conflicts (edges): %d\n", s->total_conflicts);
    fprintf(f, "Exam slots used: %d\n", s->slots_used);
    fprintf(f, "Days used: %d\n", s->days_used);
    fprintf(f, "Max exams/student/day: %d\n", s->max_exams_per_student_per_day);
    fprintf(f, "Min gap between exams (slots): %d\n", s->min_gap_slots);
    fprintf(f, "Most conflicting subject: %s\n", s->most_conflicting_subject);
    fprintf(f, "Highest enrollment subject: %s\n", s->highest_enrollment_subject);
    fprintf(f, "Average subjects/day: %.2f\n", s->average_subjects_per_day);
    fclose(f);
}
