#ifndef CONFIG_H
#define CONFIG_H

/*
 * config.h
 * ---------
 * Central configuration for the exam timetable generator.
 * Change the numbers here (not the algorithm code) to retarget
 * the project at a different number of days / slots / rules.
 */

/* Random seed kept only for documentation/reproducibility of the
 * dataset; the solver itself is deterministic (no randomness). */
#define RANDOM_SEED 42

/* Calendar shape: DAYS days, each with SLOTS_PER_DAY exam slots. */
#define MAX_DAYS 10
#define DAYS 5
#define SLOTS_PER_DAY 2
/* Total number of exam slots available to the colorer. Colors
 * (graph-coloring output) are mapped 1:1 onto these slots in
 * chronological order: slot 0 = Day0 Time0, slot 1 = Day0 Time1,
 * slot 2 = Day1 Time0, ... */
#define TOTAL_SLOTS (DAYS * SLOTS_PER_DAY)

/* The clock times used for the slots within a single day, in order.
 * Must have exactly SLOTS_PER_DAY entries. Defined once in
 * timetable.c to avoid an unused-variable warning in every other
 * translation unit that merely includes config.h. */
extern const char *SLOT_TIMES[SLOTS_PER_DAY];
extern const char *DAY_NAMES[DAYS];

/* Hard constraint parameters */
#define MAX_EXAMS_PER_DAY 2      /* H4 */
#define FORBID_CONSECUTIVE 1     /* H5: 1 = enabled */

/* Soft-constraint scoring weights (documented in docs/complexity.md
 * and README.md). Final score = 100 - sum(penalties). */
#define PENALTY_PER_SAME_DAY_PAIR      5   /* S2 */
#define PENALTY_PER_UNEVEN_DAY_UNIT    2   /* S1/S5 */
#define PENALTY_PER_EXTRA_SLOT_USED    3   /* S6 */
#define PENALTY_HIGH_CONFLICT_ADJACENT 4   /* S3 */
#define HIGH_CONFLICT_THRESHOLD        15  /* students, for S3 */

/* Sizing limits (this is an academic dataset: 60 students,
 * 11 subjects -- these caps are generous headroom, not tuning). */
#define MAX_SUBJECTS 64
#define MAX_STUDENTS 1024
#define MAX_REGISTRATIONS 8192
#define MAX_NAME_LEN 64
#define MAX_LINE_LEN 2048

#endif /* CONFIG_H */
