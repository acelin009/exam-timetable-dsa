#ifndef DATA_VALIDATOR_H
#define DATA_VALIDATOR_H

#include "data_loader.h"
#include "graph.h"

/*
 * data_validator.h
 * ----------------
 * Validates the loaded dataset BEFORE any scheduling happens, and
 * (optionally) cross-checks the conflict graph we rebuild from
 * student_subjects.csv against the supplied subject_conflicts.csv,
 * reporting discrepancies instead of silently trusting either file.
 */

typedef struct {
    int error_count;
    int warning_count;
} ValidationReport;

/* Checks: unique student ids, unique subject ids, every
 * registration references a real student and a real subject, and
 * no duplicate (student_id, subject_id) registration. Prints each
 * problem found as a clear, specific message. */
ValidationReport validate_dataset(const Dataset *ds);

/* Compares the graph built from student_subjects.csv against
 * data/subject_conflicts.csv (if present). Any mismatch is
 * reported as a warning, not a fatal error, since
 * subject_conflicts.csv is treated as a reference file only. */
void cross_check_conflicts(const ConflictGraph *g, const char *data_dir,
                            ValidationReport *report);

#endif /* DATA_VALIDATOR_H */
