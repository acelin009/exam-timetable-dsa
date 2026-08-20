#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "graph.h"
#include "data_loader.h"
#include "timetable.h"

/*
 * evaluator.h
 * -----------
 * Soft-constraint scoring. Hard constraints already passed (or the
 * caller shouldn't be here) -- this only measures QUALITY.
 *
 *   Final Score = 100 - (S2 penalty + S1/S5 penalty + S6 penalty + S3 penalty)
 *
 * All weights live in config.h and are documented there; nothing
 * here is a magic number. Score is clamped to [0, 100].
 */

typedef struct {
    int same_day_pairs;       /* S2: (student, day) pairs with >=2 exams */
    int same_day_penalty;

    int unevenness_units;     /* S1/S5: sum of |count_day - average| over days */
    int unevenness_penalty;

    int extra_slots_used;     /* S6: slots used beyond the coloring's minimum */
    int extra_slots_penalty;

    int high_conflict_adjacent_days; /* S3: high-weight edges placed on the same day */
    int high_conflict_penalty;

    int final_score;
} SoftEvaluation;

SoftEvaluation evaluate_soft_constraints(const Timetable *t, const ConflictGraph *g,
                                          const Dataset *ds, int minimum_colors_needed);

void print_soft_evaluation(const SoftEvaluation *e);

#endif /* EVALUATOR_H */
