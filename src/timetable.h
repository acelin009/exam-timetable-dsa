#ifndef TIMETABLE_H
#define TIMETABLE_H

#include "config.h"
#include "graph.h"
#include "data_loader.h"

/*
 * timetable.h
 * -----------
 * Separates the abstract "color" produced by graph coloring from
 * the concrete calendar. A Timetable is just: for each subject
 * (vertex), which of the TOTAL_SLOTS exam slots it was assigned.
 * Slot -> (day, time) is a pure, stateless mapping:
 *
 *     day       = slot / SLOTS_PER_DAY
 *     time_idx  = slot % SLOTS_PER_DAY
 *
 * so slot 0 = Day0/Time0, slot 1 = Day0/Time1, slot 2 = Day1/Time0, ...
 */

typedef struct {
    int slot[MAX_SUBJECTS];   /* slot[v] = assigned exam slot, or -1 if unassigned */
    int num_subjects;
    int slots_used;           /* highest slot index used + 1 */
} Timetable;

void timetable_init(Timetable *t, int num_subjects);

static inline int slot_to_day(int slot)   { return slot / SLOTS_PER_DAY; }
static inline int slot_to_time_idx(int slot) { return slot % SLOTS_PER_DAY; }
const char *slot_to_day_name(int slot);
const char *slot_to_time_str(int slot);

/* Prints the human-readable console timetable (grouped by day). */
void timetable_print(const Timetable *t, const ConflictGraph *g);

/* Writes output/final_timetable.csv */
void timetable_export_subjects_csv(const Timetable *t, const ConflictGraph *g, const char *out_dir);

/* Writes output/student_schedules.csv */
void timetable_export_student_schedules_csv(const Timetable *t, const ConflictGraph *g,
                                             const Dataset *ds, const char *out_dir);

/* Writes output/conflict_graph.csv */
void export_conflict_graph_csv(const ConflictGraph *g, const char *out_dir);

/* Prints one student's personal exam schedule (for --student). */
void print_student_schedule(const Timetable *t, const ConflictGraph *g,
                             const Dataset *ds, const char *student_id);

/* Prints one subject's exam info (for --subject). */
void print_subject_info(const Timetable *t, const ConflictGraph *g, const char *subject_id);

#endif /* TIMETABLE_H */
