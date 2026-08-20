#include "data_loader.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void dataset_init(Dataset *ds) {
    ds->num_students = 0;
    ds->num_subjects = 0;
    ds->num_registrations = 0;
    hashmap_init(&ds->student_index, 128);
    hashmap_init(&ds->subject_index, 32);
}

static FILE *open_or_error(const char *dir, const char *filename) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr,
            "ERROR:\n"
            "Could not open required dataset file '%s'.\n"
            "Make sure the --data directory points at a folder containing\n"
            "students.csv, subjects.csv and student_subjects.csv.\n",
            path);
    }
    return f;
}

static int load_students(Dataset *ds, const char *dir) {
    FILE *f = open_or_error(dir, "students.csv");
    if (!f) return 0;

    char line[MAX_LINE_LEN];
    if (!fgets(line, sizeof(line), f)) { fprintf(stderr, "ERROR:\nEmpty file.\n"); fclose(f); return 0; } /* header */
    while (fgets(line, sizeof(line), f)) {
        chomp(line);
        if (line[0] == '\0') continue;
        CsvRow row;
        csv_split_line(line, &row);
        if (row.count < 2) continue;

        if (ds->num_students >= MAX_STUDENTS) {
            fprintf(stderr, "ERROR:\nstudents.csv has more than MAX_STUDENTS (%d) rows.\n", MAX_STUDENTS);
            fclose(f);
            return 0;
        }
        Student *s = &ds->students[ds->num_students];
        strncpy(s->student_id, trim(row.fields[0]), MAX_NAME_LEN - 1);
        s->student_id[MAX_NAME_LEN - 1] = '\0';
        strncpy(s->class_name, trim(row.fields[1]), 7);
        s->class_name[7] = '\0';

        hashmap_put(&ds->student_index, s->student_id, ds->num_students);
        ds->num_students++;
    }
    fclose(f);
    return 1;
}

static int load_subjects(Dataset *ds, const char *dir) {
    FILE *f = open_or_error(dir, "subjects.csv");
    if (!f) return 0;

    char line[MAX_LINE_LEN];
    if (!fgets(line, sizeof(line), f)) { fprintf(stderr, "ERROR:\nEmpty file.\n"); fclose(f); return 0; } /* header */
    while (fgets(line, sizeof(line), f)) {
        chomp(line);
        if (line[0] == '\0') continue;
        CsvRow row;
        csv_split_line(line, &row);
        if (row.count < 3) continue;

        if (ds->num_subjects >= MAX_SUBJECTS) {
            fprintf(stderr, "ERROR:\nsubjects.csv has more than MAX_SUBJECTS (%d) rows.\n", MAX_SUBJECTS);
            fclose(f);
            return 0;
        }
        SubjectRecord *s = &ds->subjects[ds->num_subjects];
        strncpy(s->subject_id, trim(row.fields[0]), MAX_NAME_LEN - 1);
        s->subject_id[MAX_NAME_LEN - 1] = '\0';
        strncpy(s->subject_name, trim(row.fields[1]), MAX_NAME_LEN - 1);
        s->subject_name[MAX_NAME_LEN - 1] = '\0';
        strncpy(s->subject_type, trim(row.fields[2]), 15);
        s->subject_type[15] = '\0';

        hashmap_put(&ds->subject_index, s->subject_id, ds->num_subjects);
        ds->num_subjects++;
    }
    fclose(f);
    return 1;
}

static int load_student_subjects(Dataset *ds, const char *dir) {
    FILE *f = open_or_error(dir, "student_subjects.csv");
    if (!f) return 0;

    char line[MAX_LINE_LEN];
    if (!fgets(line, sizeof(line), f)) { fprintf(stderr, "ERROR:\nEmpty file.\n"); fclose(f); return 0; } /* header */
    while (fgets(line, sizeof(line), f)) {
        chomp(line);
        if (line[0] == '\0') continue;
        CsvRow row;
        csv_split_line(line, &row);
        if (row.count < 3) continue;

        if (ds->num_registrations >= MAX_REGISTRATIONS) {
            fprintf(stderr, "ERROR:\nstudent_subjects.csv has more than MAX_REGISTRATIONS (%d) rows.\n", MAX_REGISTRATIONS);
            fclose(f);
            return 0;
        }
        Registration *r = &ds->registrations[ds->num_registrations];
        strncpy(r->student_id, trim(row.fields[0]), MAX_NAME_LEN - 1);
        r->student_id[MAX_NAME_LEN - 1] = '\0';
        strncpy(r->subject_id, trim(row.fields[2]), MAX_NAME_LEN - 1);
        r->subject_id[MAX_NAME_LEN - 1] = '\0';
        ds->num_registrations++;
    }
    fclose(f);
    return 1;
}

int dataset_load(Dataset *ds, const char *data_dir) {
    if (!load_students(ds, data_dir)) return 0;
    if (!load_subjects(ds, data_dir)) return 0;
    if (!load_student_subjects(ds, data_dir)) return 0;
    return 1;
}
