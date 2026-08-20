#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include "config.h"
#include "hashmap.h"

/*
 * data_loader.h
 * -------------
 * Loads the raw CSV dataset (data/students.csv, subjects.csv,
 * student_subjects.csv) into flat in-memory arrays plus hash maps
 * for O(1) id -> index lookup. This module does NOT build the
 * conflict graph -- that's conflict_builder.c's job -- it only
 * turns CSV rows into typed records.
 */

typedef struct {
    char student_id[MAX_NAME_LEN];
    char class_name[8];
} Student;

typedef struct {
    char subject_id[MAX_NAME_LEN];
    char subject_name[MAX_NAME_LEN];
    char subject_type[16];
} SubjectRecord;

typedef struct {
    char student_id[MAX_NAME_LEN];
    char subject_id[MAX_NAME_LEN];
} Registration;

typedef struct {
    Student students[MAX_STUDENTS];
    int num_students;
    HashMap student_index; /* student_id -> index in students[] */

    SubjectRecord subjects[MAX_SUBJECTS];
    int num_subjects;
    HashMap subject_index; /* subject_id -> index in subjects[] */

    Registration registrations[MAX_REGISTRATIONS];
    int num_registrations;
} Dataset;

void dataset_init(Dataset *ds);

/* Loads students.csv, subjects.csv and student_subjects.csv from
 * the given directory (e.g. "data"). Returns 1 on success, 0 on a
 * fatal I/O error (missing file etc). Prints a clear error message
 * on failure -- no raw tracebacks. */
int dataset_load(Dataset *ds, const char *data_dir);

#endif /* DATA_LOADER_H */
