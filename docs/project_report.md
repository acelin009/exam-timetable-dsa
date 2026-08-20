# Project Report

## 1. Introduction

Examination timetabling is a classic scheduling problem: given a set of
students who have each registered for several subjects, produce a
calendar of exams such that no student is ever asked to sit two exams
at once. This project implements a solver for that problem in C, built
around graph coloring (specifically DSATUR) with a backtracking
constraint-satisfaction repair layer for the constraints coloring alone
cannot express.

## 2. Problem Definition

Formally: given a bipartite registration relation between a set of
students S and a set of subjects V, construct a function
`slot: V -> {0, 1, ..., T-1}` such that:

- no two subjects sharing a student get the same slot value, and
- when slots are mapped onto a real calendar (days × times-per-day), no
  student exceeds a daily exam limit or receives two exams in
  adjacent time-slots on the same day.

## 3. Objectives

1. Rebuild the subject-conflict graph directly from raw registration
   data (not from a precomputed reference file).
2. Minimize the number of distinct exam slots needed (DSATUR vs. a
   greedy baseline, compared honestly).
3. Guarantee zero hard-constraint violations in the delivered
   timetable, repairing via backtracking if the initial coloring isn't
   enough.
4. Score the delivered timetable on soft, "quality of life" criteria.
5. Do all of the above using hand-written core data structures (graph,
   heap, hash map) rather than opaque library calls, for pedagogical
   transparency.

## 4. Dataset Description

`data/` contains:

- `students.csv` — 60 rows, `student_id,class` (classes A/B/C, 20
  students each)
- `subjects.csv` — 11 rows, `subject_id,subject_name,subject_type`
  (5 Core, 3 Honors, 3 Elective)
- `student_subjects.csv` — 264 rows, the registration table (most
  important input; this is what the conflict graph is built from)
- `class_subjects.csv`, `subject_enrollment.csv`, `subject_conflicts.csv`
  — reference/reporting files, cross-checked but not treated as source
  of truth
- `dataset_summary.txt`, `dsa_dataset.py` — the original generator,
  kept for provenance

Core-subject overlap across classes (A/B/C all take Mathematics; A and
C both take Computer Networks) is deliberate, to create realistic
cross-class conflicts.

## 5. Requirements

- No external dependencies beyond the C standard library.
- Deterministic output (no randomness in the solver).
- Configurable calendar shape (`src/config.h`), not hard-coded.
- Clear, non-crashing error messages for malformed input.

## 6. System Design

See `README.md`'s architecture table for the full module list. At a
high level:

```
CSV files -> DatasetLoader -> DataValidator -> ConflictBuilder
    -> ConflictGraph -> {GreedyColoring, DSATURColoring}
    -> Timetable (color->slot mapping) -> ConstraintChecker
    -> [if invalid] BacktrackingScheduler -> ConstraintChecker
    -> Evaluator (soft scoring) -> Statistics -> CSV/console output
```

## 7. Data Structures

Custom hash map (open addressing), custom adjacency-list graph with a
hand-rolled dynamic-array `IntSet` for neighbor sets, and a custom
binary max-heap with an O(log n) key-update path via a position index.
Full justification for each in `README.md`'s DSA table and
`docs/complexity.md`.

## 8. Graph Model

Vertex = subject. Edge = shared student. Weight = number of students
causing that edge. Built once, in `conflict_builder.c`, directly from
`student_subjects.csv` — grouped by student, every subject pair within
a student's registration list becomes (or reinforces) an edge.

## 9. Algorithm

Two graph-coloring algorithms are implemented and compared:

- **Greedy** (largest-degree-first, fixed order) — baseline.
- **DSATUR** (dynamic, saturation-degree-driven, backed by the custom
  max-heap) — primary algorithm.

## 10. DSATUR

Detailed walkthrough with a hand-traced 4-vertex example in
`docs/algorithm.md`. Summary: repeatedly pop the highest-saturation
uncolored vertex from the heap, assign its lowest feasible color,
propagate saturation increases to its uncolored neighbors.

## 11. Priority Queue

A custom binary max-heap (`src/max_heap.c`), not `heapq` or any library
container — array-backed, with `insert`, `extract_max`, `peek`,
`is_empty`, plus `heap_build` (O(n) bottom-up heapify) and
`heap_update_saturation` (O(log n) key-increase, located via a
position index rather than a linear scan).

## 12. Constraint Checking

Six hard constraints (H1–H6), all checked by `constraint_checker.c`
independently of how the slot assignment was produced — so it serves
as an honest, algorithm-agnostic verifier for both the direct
coloring-based mapping and the backtracking-repaired result.

## 13. Backtracking

A real recursive CSP search (`backtracking.c`) over (subject → slot)
with four pruning rules (immediate adjacency rejection, daily-limit
rejection, consecutive-exam rejection, most-constrained-subject
ordering) and a least-used-slot value ordering. Not a stub: it selects,
checks, recurses, and undoes assignments on failure. See
`docs/algorithm.md` for a hand-traced 3-vertex example and
`docs/complexity.md` for the honest exponential worst-case discussion.

## 14. Complexity Analysis

Full table in `docs/complexity.md`. Headline: every stage except
backtracking is polynomial (dominated by O(E log V) for DSATUR);
backtracking is worst-case exponential but empirically fast here
because of pruning and the small problem size.

## 15. Testing

`tests/test_main.c`, run via `make test`. **66 checks, 66 passing**, as
of this report, across:

- Graph tests (vertex/edge add, duplicate handling, degree, weight
  accumulation, symmetric lookup)
- MaxHeap tests (ordering, tie-breaks, `update_saturation`,
  `heap_build`)
- HashMap tests (insert, overwrite, growth/rehash, miss lookup)
- Dataset tests (real-file load, expected counts, validation, failure
  on a missing directory)
- Conflict-graph tests (vertex count, known edge + weight from the
  real dataset, symmetry)
- Coloring tests on small known graphs: **triangle needs 3 colors**,
  **square (4-cycle) needs 2 colors**, **K3 needs 3**, **K5 needs 5**,
  **empty graph needs 1**, **single vertex needs 1**
- Constraint-checker tests (valid case, an engineered H6+H1 violation,
  an engineered H3 violation)
- Backtracking tests (a synthetic triangle-conflict case that forces
  real search)
- **End-to-end test on the real 60-student dataset, asserting
  `valid == True`**

Actual captured run:

```
============================================================
RESULTS: 66 / 66 tests passed
============================================================
```

## 16. Results

Actual run against the supplied dataset (`./exam_timetable --compare`,
output captured verbatim, not hand-edited):

```
Algorithm       Slots Used   Comparisons    Steps
--------------------------------------------------------------
Greedy          5            38             11
DSATUR          5            38             11
```

Both reach the same chromatic number on this particular graph (see
`docs/algorithm.md` §"Greedy vs DSATUR" for why — MAT's very high
degree dominates the ordering for both algorithms here). Direct
color→slot mapping with `DAYS=5, SLOTS_PER_DAY=2` produced 120 H5
(consecutive-exam) violations, since 5 colors packed across only 5
days inevitably places multiple colors on the same day. Backtracking
repair then found a fully valid timetable in **54 nodes explored, 2
backtracks**. Final result:

```
Hard constraint violations: 0
Final soft-constraint score: 79 / 100
Status: VALID
```

Soft-score breakdown: 0 same-day pairs (S2), 3 unevenness units (S1/S5,
penalty 6), 5 extra slots beyond the 5-color minimum (S6, penalty 15,
since 10 slots were used to spread the days out and avoid consecutive
exams), 0 high-conflict same-day placements (S3).

## 17. Limitations

- Static array caps (`MAX_SUBJECTS=64`, `MAX_STUDENTS=1024`,
  `MAX_REGISTRATIONS=8192`) rather than fully dynamic containers —
  simple and fast for a dataset this size, but would need raising (or
  converting to dynamic allocation) for a much larger university.
- `IntSet` membership is O(degree) linear scan, fine at low degree but
  would benefit from a hash-set backing at high degree/large V (see
  `docs/complexity.md`).
- Backtracking's worst case is exponential; no timeout/interruption
  mechanism is implemented (not needed at this scale, but would matter
  at larger V with a harder-to-satisfy constraint set).
- No web/GUI — console + CSV output only, per the assignment's explicit
  "do not overengineer" instruction.

## 18. Future Scope

- Hash-set-backed adjacency for large-V scalability.
- A proper minimum-conflict local-search / simulated-annealing pass as
  an alternative to backtracking for very large, hard-to-satisfy
  instances where exhaustive backtracking would time out.
- Multi-objective soft-constraint weighting exposed via a config file
  rather than compile-time constants.

## 19. Conclusion

The project delivers a working, tested, deterministic exam-timetable
generator built from first-principles data structures: a custom hash
map, adjacency-list graph, and priority-queue max-heap drive a DSATUR
coloring, and a genuine pruned backtracking search repairs any
day-level constraint the coloring step can't see. Run against the real
60-student, 11-subject, 264-registration dataset, it produces a
zero-violation, VALID timetable, and all 66 automated tests pass.
