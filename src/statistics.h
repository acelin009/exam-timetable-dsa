#ifndef STATISTICS_H
#define STATISTICS_H

#include "graph.h"
#include "data_loader.h"
#include "timetable.h"

/*
 * statistics.h
 * ------------
 * Aggregate reporting over the dataset + final timetable.
 */

typedef struct {
    int total_students;
    int total_subjects;
    int total_registrations;
    int total_conflicts;       /* edges in the conflict graph */
    int slots_used;
    int days_used;
    int max_exams_per_student_per_day;
    int min_gap_slots;         /* smallest gap (in slots) between any two exams for any student, -1 if N/A */
    char most_conflicting_subject[MAX_NAME_LEN];
    char highest_enrollment_subject[MAX_NAME_LEN];
    double average_subjects_per_day;
} DatasetStatistics;

DatasetStatistics compute_statistics(const Timetable *t, const ConflictGraph *g, const Dataset *ds);
void print_statistics(const DatasetStatistics *s);
void export_statistics_txt(const DatasetStatistics *s, const char *out_dir);

#endif /* STATISTICS_H */
