#ifndef BACKTRACKING_H
#define BACKTRACKING_H

#include "graph.h"
#include "data_loader.h"
#include "timetable.h"

/*
 * backtracking.h
 * --------------
 * Graph coloring alone only guarantees H6 (adjacent subjects don't
 * share a slot). Once colors are mapped onto a real day/slot grid
 * with SLOTS_PER_DAY > 1, a student can still end up with too many
 * exams on one day (H4) or two consecutive exams (H5), because
 * those constraints depend on which *day* a slot falls on, not just
 * on which color it is.
 *
 * BacktrackingScheduler performs a real constraint-satisfaction
 * search directly over (subject -> slot) assignments:
 *
 *   choose most-constrained unassigned subject (MRV: highest degree
 *   first, i.e. DSATUR-style ordering)
 *       -> try each slot (most-used-so-far slots last, to spread load)
 *           -> prune immediately if it violates H6, H4 or H5
 *           -> if consistent, recurse into the rest of the subjects
 *           -> if the recursive call fails, UNDO this assignment
 *              (backtrack) and try the next slot
 *
 * This is a genuine backtracking search with pruning (not a stub):
 * it selects, checks feasibility, recurses, and undoes assignments
 * on failure. Worst case is exponential in the number of subjects
 * (see docs/complexity.md); it is only tractable here because V is
 * small (~11) and the pruning rules cut the search space sharply.
 */

typedef struct {
    long nodes_explored;   /* number of (subject, slot) trials attempted */
    long backtracks;       /* number of times we undid an assignment */
} BacktrackStats;

/* Attempts to find a slot assignment for every subject in g that
 * satisfies H1, H3, H4, H5 and H6 simultaneously. order[] should be
 * a subject-index ordering (most-constrained-first is recommended --
 * see coloring.c's DSATUR run) of length g->num_vertices.
 * Returns 1 and fills timetable->slot[] if a valid assignment was
 * found, 0 if the search exhausted all possibilities without one
 * (meaning: increase TOTAL_SLOTS in config.h). */
int backtracking_solve(const ConflictGraph *g, const Dataset *ds, const int order[MAX_SUBJECTS],
                        Timetable *timetable, BacktrackStats *stats);

#endif /* BACKTRACKING_H */
