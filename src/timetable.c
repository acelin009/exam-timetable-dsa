#include "timetable.h"
#include <stdio.h>
#include <string.h>

const char *SLOT_TIMES[SLOTS_PER_DAY] = { "09:00", "14:00" };
const char *DAY_NAMES[DAYS] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday" };

void timetable_init(Timetable *t, int num_subjects) {
    t->num_subjects = num_subjects;
    t->slots_used = 0;
    for (int i = 0; i < num_subjects; i++) t->slot[i] = -1;
}

const char *slot_to_day_name(int slot) {
    int d = slot_to_day(slot);
    if (d < 0 || d >= DAYS) return "?";
    return DAY_NAMES[d];
}

const char *slot_to_time_str(int slot) {
    int ti = slot_to_time_idx(slot);
    if (ti < 0 || ti >= SLOTS_PER_DAY) return "?";
    return SLOT_TIMES[ti];
}

void timetable_print(const Timetable *t, const ConflictGraph *g) {
    printf("============================================================\n");
    printf("FINAL EXAMINATION TIMETABLE\n");
    printf("============================================================\n\n");

    for (int d = 0; d < DAYS; d++) {
        int any = 0;
        for (int v = 0; v < g->num_vertices; v++) {
            if (t->slot[v] >= 0 && slot_to_day(t->slot[v]) == d) { any = 1; break; }
        }
        if (!any) continue;

        printf("%s\n", DAY_NAMES[d]);
        printf("------------------------------------------------------------\n");
        for (int ti = 0; ti < SLOTS_PER_DAY; ti++) {
            int slot = d * SLOTS_PER_DAY + ti;
            for (int v = 0; v < g->num_vertices; v++) {
                if (t->slot[v] == slot) {
                    printf("%-6s %s\n", SLOT_TIMES[ti], g->vertices[v].subject_name);
                }
            }
        }
        printf("\n");
    }
}

void timetable_export_subjects_csv(const Timetable *t, const ConflictGraph *g, const char *out_dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/final_timetable.csv", out_dir);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "subject_id,subject_name,subject_type,day,slot,time\n");
    for (int v = 0; v < g->num_vertices; v++) {
        int s = t->slot[v];
        fprintf(f, "%s,%s,%s,%s,%d,%s\n",
                g->vertices[v].subject_id, g->vertices[v].subject_name, g->vertices[v].subject_type,
                s >= 0 ? slot_to_day_name(s) : "UNASSIGNED", s,
                s >= 0 ? slot_to_time_str(s) : "");
    }
    fclose(f);
}

void timetable_export_student_schedules_csv(const Timetable *t, const ConflictGraph *g,
                                             const Dataset *ds, const char *out_dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/student_schedules.csv", out_dir);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "student_id,class,subject_id,subject_name,day,time\n");

    for (int i = 0; i < ds->num_registrations; i++) {
        const Registration *r = &ds->registrations[i];
        int student_idx;
        if (!hashmap_get(&ds->student_index, r->student_id, &student_idx)) continue;
        int v = graph_find_vertex(g, r->subject_id);
        if (v < 0) continue;
        int s = t->slot[v];
        fprintf(f, "%s,%s,%s,%s,%s,%s\n",
                r->student_id, ds->students[student_idx].class_name,
                g->vertices[v].subject_id, g->vertices[v].subject_name,
                s >= 0 ? slot_to_day_name(s) : "UNASSIGNED",
                s >= 0 ? slot_to_time_str(s) : "");
    }
    fclose(f);
}

void export_conflict_graph_csv(const ConflictGraph *g, const char *out_dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/conflict_graph.csv", out_dir);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "subject_1,subject_2,weight\n");
    for (int a = 0; a < g->num_vertices; a++) {
        for (int k = 0; k < g->adjacency[a].size; k++) {
            int b = g->adjacency[a].items[k];
            if (b > a) { /* print each undirected edge once */
                fprintf(f, "%s,%s,%d\n", g->vertices[a].subject_id, g->vertices[b].subject_id, g->weight[a][b]);
            }
        }
    }
    fclose(f);
}

void print_student_schedule(const Timetable *t, const ConflictGraph *g,
                             const Dataset *ds, const char *student_id) {
    int student_idx;
    if (!hashmap_get(&ds->student_index, student_id, &student_idx)) {
        printf("ERROR:\nNo student with ID '%s' was found in students.csv.\n", student_id);
        return;
    }
    printf("Student: %s\n", student_id);
    printf("Class: %s\n\n", ds->students[student_idx].class_name);

    /* Gather this student's (slot, subject_name) pairs. */
    int slots[16], count = 0;
    const char *names[16];
    for (int i = 0; i < ds->num_registrations; i++) {
        if (strcmp(ds->registrations[i].student_id, student_id) != 0) continue;
        int v = graph_find_vertex(g, ds->registrations[i].subject_id);
        if (v < 0) continue;
        slots[count] = t->slot[v];
        names[count] = g->vertices[v].subject_name;
        count++;
    }

    for (int d = 0; d < DAYS; d++) {
        int printed_day_header = 0;
        for (int ti = 0; ti < SLOTS_PER_DAY; ti++) {
            int slot = d * SLOTS_PER_DAY + ti;
            for (int i = 0; i < count; i++) {
                if (slots[i] == slot) {
                    if (!printed_day_header) {
                        printf("%s:\n", DAY_NAMES[d]);
                        printed_day_header = 1;
                    }
                    printf("%s %s\n", SLOT_TIMES[ti], names[i]);
                }
            }
        }
        if (printed_day_header) printf("\n");
    }
}

void print_subject_info(const Timetable *t, const ConflictGraph *g, const char *subject_id) {
    int v = graph_find_vertex(g, subject_id);
    if (v < 0) {
        printf("ERROR:\nNo subject with ID '%s' was found in subjects.csv.\n", subject_id);
        return;
    }
    printf("Subject:\n%s\n\n", g->vertices[v].subject_name);
    printf("Type:\n%s\n\n", g->vertices[v].subject_type);
    printf("Students:\n%d\n\n", g->vertices[v].enrollment);
    int s = t->slot[v];
    printf("Exam:\n%s %s\n", s >= 0 ? slot_to_day_name(s) : "UNASSIGNED", s >= 0 ? slot_to_time_str(s) : "");
}
