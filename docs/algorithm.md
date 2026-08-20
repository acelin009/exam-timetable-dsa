# Algorithm Walkthrough

## DSATUR, traced by hand on a 4-vertex example

Take four subjects with these conflicts:

```
MAT - DB
MAT - AI
DB  - CN
```

(a "path-with-a-branch": MAT is adjacent to DB and AI; DB is also
adjacent to CN)

Degrees: MAT=2, DB=2, AI=1, CN=1.

**Step 1.** All saturations start at 0. Tie-break falls to degree, so
MAT and DB are tied at (sat=0, deg=2); enrollment breaks the further
tie (assume MAT has higher enrollment). MAT is popped first.

MAT gets the lowest available color: **0**.

Propagate: DB and AI are MAT's uncolored neighbors. Color 0 is new to
both, so their saturation becomes 1. Heap updated: DB now (sat=1,
deg=2), AI now (sat=1, deg=1).

**Step 2.** Highest priority: DB (sat=1, deg=2) beats AI (sat=1, deg=1).
DB is popped.

DB's colored neighbors: MAT (color 0). Lowest free color for DB: **1**.

Propagate: CN is DB's only uncolored neighbor. Color 1 is new to CN,
saturation → 1.

**Step 3.** Remaining: AI (sat=1, deg=1), CN (sat=1, deg=1) — tied;
say AI has higher enrollment, so AI is popped.

AI's colored neighbors: MAT (color 0). Lowest free color for AI: **1**
(1 is free since AI is not adjacent to DB).

**Step 4.** CN is popped last. CN's colored neighbors: DB (color 1).
Lowest free color for CN: **0**.

Final coloring: MAT=0, DB=1, AI=1, CN=0. Uses **2 colors**. Check: no
adjacent pair shares a color (MAT-DB: 0≠1 ✓, MAT-AI: 0≠1 ✓, DB-CN:
1≠0 ✓). Correct and optimal for this graph (it's bipartite: {MAT, CN}
vs {DB, AI}).

## Backtracking, traced on a 3-subject / 1-student triangle

Subjects A, B, C, all mutually conflicting (one student takes all
three — see `tests/test_main.c :: test_backtracking`), 2 slots/day.

Graph coloring alone needs 3 colors → 3 slots. With `SLOTS_PER_DAY=2`,
slots 0 and 1 fall on the same day; if we naively map color→slot
directly, two of A/B/C land on the same day and (since only one other
subject exists) they'll likely also be placed in adjacent time slots,
tripping H5.

MRV ordering (highest degree first) since all three have equal degree
here: order = [A, B, C].

```
depth 0: assign A.
    try slot 0 (global usage tied, first in order): feasible
    (no colored neighbors yet, no student constraints yet) -> assign A=0

depth 1: assign B.
    B is adjacent to A -> slot 0 is rejected immediately (H6 pruning)
    try slot 1: feasible on H6, but check H5: slot 0 and slot 1 are
        the same day, adjacent time indices, and the shared student
        already has an exam at slot 0 -> REJECTED (H5 pruning)
    try slot 2 (day 1): feasible -> assign B=2

depth 2: assign C.
    C is adjacent to both A (slot 0) and B (slot 2) -> both rejected (H6)
    try slot 1: not adjacent-slot-conflicting with A's slot 0? slot 1
        IS the same day as slot 0 and adjacent time index, and the
        student already has an exam at slot 0 -> REJECTED (H5)
    try slot 3: day 1, time 1. Student already has an exam at slot 2
        (day 1, time 0) -> slot 3 is the adjacent time index on the
        same day -> REJECTED (H5)
    try slot 4 (day 2): feasible -> assign C=4

All three assigned: A=0 (Day0/T0), B=2 (Day1/T0), C=4 (Day2/T0).
No backtracks needed in this particular ordering, but note how many
candidate slots were rejected by H5/H6 pruning before a feasible one
was found — that's the pruning doing its job.
```

This matches what `test_backtracking()` in `tests/test_main.c` asserts,
and is exactly the mechanism that runs (at a slightly larger scale) on
the real 60-student dataset, producing the "54 nodes explored, 2
backtracks" result documented in the README and project report.

## Greedy vs. DSATUR: why they tied on this dataset

Both algorithms found a 5-coloring on the real 11-vertex conflict
graph. This is an honest result, not a failure to differentiate them:
the graph has one very-high-degree hub (MAT, degree 10 — adjacent to
every other subject) and the "largest degree first" ordering that the
greedy baseline uses happens to pick MAT first too, same as DSATUR
would (saturation is 0 for everyone at the start, so the tie-break
falls straight to degree). DSATUR's advantage over plain greedy shows
up once you're past the first few picks, on graphs with more varied,
less hub-dominated degree structure — where saturation (which colors
are *already forced* around a vertex) diverges from raw degree. See
`docs/project_report.md` for the actual comparison table from this
dataset.
