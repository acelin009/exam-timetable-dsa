#ifndef COLORING_H
#define COLORING_H

#include "graph.h"

/*
 * coloring.h
 * ----------
 * Two graph-coloring algorithms over the ConflictGraph. A "color"
 * is an abstract exam slot id (0, 1, 2, ...) -- timetable.c is
 * responsible for mapping colors onto actual day/time slots.
 *
 * Both algorithms guarantee: no two adjacent vertices (subjects
 * that share a student) get the same color. That is the entire
 * graph-coloring contract; it does NOT by itself guarantee the
 * timetable-level hard constraints H4/H5 (max exams/day, no
 * consecutive exams) once colors are mapped onto a day/slot grid
 * with more than one slot per day -- constraint_checker.c and
 * backtracking.c handle that layer.
 */

typedef struct {
    long comparisons;   /* number of "is this color free" checks performed */
    long steps;         /* number of vertices colored */
} ColoringStats;

/* Simple greedy coloring, baseline for comparison. Processes
 * vertices in a fixed order (by descending degree, "largest degree
 * first" -- a common, still-naive baseline) and assigns each the
 * lowest color not used by its already-colored neighbors.
 * Returns the number of distinct colors used. */
int greedy_coloring(const ConflictGraph *g, int color_out[MAX_SUBJECTS], ColoringStats *stats);

/* DSATUR: repeatedly picks the uncolored vertex with the highest
 * saturation degree (ties broken by conflict degree, then
 * enrollment), driven by a custom max-heap priority queue
 * (max_heap.h). Returns the number of distinct colors used. */
int dsatur_coloring(const ConflictGraph *g, int color_out[MAX_SUBJECTS], ColoringStats *stats);

#endif /* COLORING_H */
