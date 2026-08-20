#ifndef MAX_HEAP_H
#define MAX_HEAP_H

#include "config.h"

/*
 * max_heap.h
 * ----------
 * A hand-rolled binary MAX-HEAP used as the priority queue that
 * drives DSATUR: "always process the uncolored subject with the
 * highest saturation degree next".
 *
 * WHY NOT Python-style sort()? Sorting the whole uncolored set
 * after every single color assignment is O(V log V) per step and
 * O(V^2 log V) overall. A heap gives O(log V) extract-max and,
 * because DSATUR's priorities change as neighbors get colored, we
 * also need O(log V) key updates -- that's exactly the "lazy
 * update" binary heap with a position index below.
 *
 * PRIORITY ORDER (max-heap, highest priority popped first):
 *   1. saturation degree   (distinct colors already used by
 *                            colored neighbors)
 *   2. conflict degree     (tie-break: total number of neighbors)
 *   3. enrollment          (tie-break: number of students taking it)
 *
 * DESIGN: array-backed complete binary tree (classic heap), plus a
 * position[] array mapping a subject's vertex id -> its current
 * index in the heap array, so update_priority() can locate the
 * node and sift it up in O(log n) instead of doing a linear scan.
 */

typedef struct {
    int id;          /* subject vertex index */
    int saturation;
    int degree;
    int enrollment;
} HeapNode;

typedef struct {
    HeapNode data[MAX_SUBJECTS];
    int position[MAX_SUBJECTS]; /* position[vertex_id] = index in data[], or -1 if not in heap */
    int size;
} MaxHeap;

void heap_init(MaxHeap *h);

/* O(log n) */
void heap_insert(MaxHeap *h, int id, int saturation, int degree, int enrollment);

/* O(log n). Caller must check heap_is_empty() first. */
HeapNode heap_extract_max(MaxHeap *h);

/* O(1) */
HeapNode heap_peek(const MaxHeap *h);

int heap_is_empty(const MaxHeap *h);

/* Update the saturation (and degree, in case it also changed) of a
 * node still sitting in the heap, then re-heapify around it.
 * O(log n) because we locate it via position[] instead of scanning. */
void heap_update_saturation(MaxHeap *h, int id, int new_saturation);

/* Build a heap from n nodes in O(n) using bottom-up heapify,
 * instead of n sequential O(log n) inserts (O(n log n)). Used for
 * the initial population of the priority queue. */
void heap_build(MaxHeap *h, const HeapNode *nodes, int n);

#endif /* MAX_HEAP_H */
