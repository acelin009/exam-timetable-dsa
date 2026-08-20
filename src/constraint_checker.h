#ifndef CONSTRAINT_CHECKER_H
#define CONSTRAINT_CHECKER_H

#include "graph.h"
#include "data_loader.h"
#include "timetable.h"

/*
 * constraint_checker.h
 * ---------------------
 * Hard constraints (must NEVER be violated):
 *   H1  A student cannot have two exams in the same exam slot.
 *   H2  All students taking the same subject write it at the same
 *       time (guaranteed by construction: one subject -> one slot).
 *   H3  Every subject must be assigned exactly one examination slot.
 *   H4  Maximum MAX_EXAMS_PER_DAY examinations per student per day.
 *   H5  No consecutive examinations for the same student (same day,
 *       adjacent time-slot indices).
 *   H6  Two subjects with a common student cannot share an exam slot.
 */

typedef struct {
    int h1_violations;
    int h2_violations;
    int h3_violations;
    int h4_violations;
    int h5_violations;
    int h6_violations;
    int is_valid;
} ConstraintReport;

/* Runs all six hard-constraint checks and prints a summary
 * ("check_all" from the spec's ConstraintChecker class). Set
 * verbose=1 to print each individual violation found. */
ConstraintReport check_all_hard_constraints(const Timetable *t, const ConflictGraph *g,
                                             const Dataset *ds, int verbose);

#endif /* CONSTRAINT_CHECKER_H */
