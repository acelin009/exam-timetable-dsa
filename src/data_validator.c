#include "data_validator.h"
#include "hashmap.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ValidationReport validate_dataset(const Dataset *ds) {
    ValidationReport report = {0, 0};

    /* --- unique student ids --- */
    HashMap seen;
    hashmap_init(&seen, 128);
    for (int i = 0; i < ds->num_students; i++) {
        if (hashmap_contains(&seen, ds->students[i].student_id)) {
            fprintf(stderr, "ERROR:\nDuplicate student ID '%s' in students.csv.\n",
                    ds->students[i].student_id);
            report.error_count++;
        } else {
            hashmap_put(&seen, ds->students[i].student_id, 1);
        }
    }
    hashmap_free(&seen);

    /* --- unique subject ids --- */
    hashmap_init(&seen, 32);
    for (int i = 0; i < ds->num_subjects; i++) {
        if (hashmap_contains(&seen, ds->subjects[i].subject_id)) {
            fprintf(stderr, "ERROR:\nDuplicate subject ID '%s' in subjects.csv.\n",
                    ds->subjects[i].subject_id);
            report.error_count++;
        } else {
            hashmap_put(&seen, ds->subjects[i].subject_id, 1);
        }
    }
    hashmap_free(&seen);

    /* --- registrations reference real students/subjects, no duplicates --- */
    HashMap reg_seen;
    hashmap_init(&reg_seen, 512);
    for (int i = 0; i < ds->num_registrations; i++) {
        const Registration *r = &ds->registrations[i];

        if (!hashmap_contains(&ds->student_index, r->student_id)) {
            fprintf(stderr,
                "ERROR:\nstudent_subjects.csv references student ID '%s',\n"
                "but that student does not exist in students.csv.\n",
                r->student_id);
            report.error_count++;
        }
        if (!hashmap_contains(&ds->subject_index, r->subject_id)) {
            fprintf(stderr,
                "ERROR:\nstudent_subjects.csv references subject ID '%s',\n"
                "but that subject does not exist in subjects.csv.\n",
                r->subject_id);
            report.error_count++;
        }

        char key[MAX_NAME_LEN * 2 + 2];
        snprintf(key, sizeof(key), "%s|%s", r->student_id, r->subject_id);
        if (hashmap_contains(&reg_seen, key)) {
            fprintf(stderr,
                "ERROR:\nDuplicate registration: student '%s' is registered\n"
                "for subject '%s' more than once.\n",
                r->student_id, r->subject_id);
            report.error_count++;
        } else {
            hashmap_put(&reg_seen, key, 1);
        }
    }
    hashmap_free(&reg_seen);

    return report;
}

void cross_check_conflicts(const ConflictGraph *g, const char *data_dir,
                            ValidationReport *report) {
    char path[512];
    snprintf(path, sizeof(path), "%s/subject_conflicts.csv", data_dir);
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("(no subject_conflicts.csv found to cross-check against -- skipped)\n");
        return;
    }

    char line[MAX_LINE_LEN];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return; } /* header */

    int checked = 0, mismatches = 0;
    while (fgets(line, sizeof(line), f)) {
        chomp(line);
        if (line[0] == '\0') continue;
        CsvRow row;
        csv_split_line(line, &row);
        if (row.count < 5) continue;

        const char *sub1 = trim(row.fields[0]);
        const char *sub2 = trim(row.fields[2]);
        int expected_weight = atoi(row.fields[4]);

        int a = graph_find_vertex(g, sub1);
        int b = graph_find_vertex(g, sub2);
        checked++;

        if (a < 0 || b < 0 || !graph_has_edge(g, a, b)) {
            fprintf(stderr,
                "WARNING:\nsubject_conflicts.csv lists a conflict between '%s' and '%s',\n"
                "but the graph rebuilt from student_subjects.csv does not have that edge.\n",
                sub1, sub2);
            report->warning_count++;
            mismatches++;
            continue;
        }
        int actual_weight = g->weight[a][b];
        if (actual_weight != expected_weight) {
            fprintf(stderr,
                "WARNING:\nConflict weight mismatch for '%s'-'%s': subject_conflicts.csv says %d,\n"
                "rebuilt graph says %d.\n",
                sub1, sub2, expected_weight, actual_weight);
            report->warning_count++;
            mismatches++;
        }
    }
    fclose(f);

    printf("Cross-checked %d reference conflicts from subject_conflicts.csv: %d mismatch(es).\n",
           checked, mismatches);
}
