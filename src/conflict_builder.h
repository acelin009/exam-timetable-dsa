#ifndef CONFLICT_BUILDER_H
#define CONFLICT_BUILDER_H

#include "data_loader.h"
#include "graph.h"

/*
 * conflict_builder.h
 * ------------------
 * Builds the ConflictGraph directly from student_subjects.csv
 * (never from subject_conflicts.csv -- that file is reference-only,
 * per the project spec).
 *
 * ALGORITHM (student_subjects.csv -> group by student -> for each
 * student's subject list, generate every pair -> add/increment an
 * edge for each pair):
 *
 *   for student in students:
 *       subjects_taken = subjects registered by student      # via hash map lookup
 *       for i in range(len(subjects_taken)):
 *           for j in range(i+1, len(subjects_taken)):
 *               add_edge(subjects_taken[i], subjects_taken[j])  # normalized, weight++
 *
 * A pair (A,B) and (B,A) are the same edge -- graph_add_edge()
 * always updates both adjacency[a] and adjacency[b], and indexes
 * the weight matrix symmetrically, so it's naturally deduplicated;
 * we do not store "A-B" and "B-A" as separate edges.
 */

void build_conflict_graph(const Dataset *ds, ConflictGraph *g);

#endif /* CONFLICT_BUILDER_H */
