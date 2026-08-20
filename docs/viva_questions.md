# Viva Preparation — 30 Questions and Answers

**Q1. Why did you model the problem as a graph?**
Because the core constraint — "these two things cannot happen at the
same time if they share a student" — is exactly a pairwise
incompatibility relation, which is precisely what a graph's edges
represent. Turning it into a graph lets us reuse a huge, well-studied
body of algorithms (coloring) instead of inventing a bespoke scheduler.

**Q2. What does a vertex represent?**
One subject/exam that needs to be scheduled.

**Q3. What does an edge represent?**
That at least one student is registered for both subjects it connects
— they cannot be scheduled in the same exam slot.

**Q4. What does graph coloring represent here?**
Assigning each subject (vertex) to an exam slot (color) such that no
two adjacent (conflicting) subjects get the same slot.

**Q5. Why adjacency list instead of adjacency matrix?**
An adjacency matrix costs O(V²) space regardless of how sparse the
graph is. Our conflict graph is sparse (46 edges out of a possible
55 pairs among 11 vertices, but conceptually — and at real-university
scale with hundreds of subjects — most subject pairs never share a
student). An adjacency list costs O(V+E), scaling with actual
conflicts, not the square of the subject count.

**Q6. Why use DSATUR instead of plain greedy?**
Plain greedy commits to a fixed vertex order up front (e.g. by raw
degree) and never adapts. DSATUR recomputes priority as it goes: a
vertex whose neighbors have just been forced into many different
colors becomes urgent to color next, even if its raw degree is low.
This generally produces fewer colors (a tighter timetable) than static
orderings, especially on irregular graphs.

**Q7. What is saturation degree?**
For an uncolored vertex, the number of *distinct* colors already used
by its colored neighbors (not the count of colored neighbors — two
neighbors with the same color only count once).

**Q8. Why use a priority queue?**
To always process the most urgent (highest-saturation) vertex next
without re-scanning every uncolored vertex from scratch at each step.
A heap gives O(log n) extraction and O(log n) priority updates instead
of an O(n) linear scan each time.

**Q9. What is the complexity of heap insertion?**
O(log n) — one sift-up pass from a leaf toward the root.

**Q10. Why is backtracking needed if graph coloring already avoids
conflicts?**
Graph coloring alone only guarantees H6 (no shared-student subjects in
the same *slot*, i.e. same color). Once colors are mapped onto a
calendar with multiple slots per day, additional rules — max exams per
day (H4), no back-to-back exams (H5) — depend on which *day* a color
lands on, which coloring itself has no notion of. Backtracking searches
over actual slot assignments to satisfy all of H1–H6 simultaneously.

**Q11. Why can't greedy always guarantee the optimal (minimum-color)
solution?**
Because its vertex order is fixed in advance and doesn't respond to
how earlier choices constrained later vertices. A different order can
need fewer colors; greedy doesn't search for a better order, DSATUR
adapts online but still isn't guaranteed optimal in general (graph
coloring's decision version is NP-complete).

**Q12. What are hard and soft constraints?**
Hard constraints (H1–H6) must never be violated — violating any one
makes the timetable INVALID, full stop. Soft constraints (S1–S6) are
quality-of-life preferences (even distribution, avoiding same-day
pairs, etc.) that only affect a 0–100 score; they can never override a
hard constraint.

**Q13. What happens if no feasible slot exists?**
The program reports a clear error ("No feasible timetable exists with
the configured number of examination slots...") rather than crashing
or silently producing an invalid timetable, and suggests increasing
`DAYS` or `SLOTS_PER_DAY` in `config.h`.

**Q14. What is the worst-case complexity of backtracking?**
Exponential: O(T^V) in the worst case, where T is the number of slots
and V the number of subjects, since every subject could in principle
try every slot before failing. We say this plainly rather than
claiming anything better.

**Q15. Why not simply brute-force every timetable?**
Brute force and backtracking both explore the same search space in the
worst case, but backtracking prunes: it abandons a partial assignment
the moment it can prove (via H6/H4/H5 checks) that no completion can
work, instead of building every full assignment and checking it at the
end. In practice this cuts the explored space enormously — 54 nodes on
our dataset, not thousands.

**Q16. What is the saturation degree of a vertex with no colored
neighbors?**
Zero.

**Q17. How do you break ties in DSATUR when two vertices have equal
saturation?**
First by conflict degree (total neighbor count), then by enrollment
(number of students registered) — both configurable, documented in
`coloring.h`.

**Q18. What data structure tracks "which colors has this vertex's
neighborhood already used"?**
A per-vertex `IntSet` (`neighbor_colors[v]`) — its size is exactly the
saturation degree.

**Q19. Why is the hash map's average lookup O(1) but worst case
O(n)?**
Because open addressing with linear probing normally spreads keys
across the table (O(1) average with a good hash function and load
factor under ~0.7), but a pathological case where every key collides
into the same slot degrades to a linear scan — O(n).

**Q20. Why grow the hash map at load factor 0.7 rather than waiting
until it's full?**
Because linear probing performance degrades sharply as the table
fills up (probe sequences get long); resizing before it's completely
full keeps average-case lookups close to O(1).

**Q21. What is the purpose of the `position[]` array in the max-heap?**
It maps a vertex id to its current index in the heap array, so
`heap_update_saturation` can jump straight to the node in O(1) instead
of scanning the whole heap to find it, before sifting it in O(log n).

**Q22. What ensures H2 (all students of a subject sit it at the same
time)?**
It's true by construction — the `Timetable` data structure assigns
exactly one slot per subject, not per (student, subject) pair, so
there's no way for two students of the same subject to end up with
different times.

**Q23. What is "value ordering" in the backtracking search, and why
did you use it?**
The order in which candidate slots are tried for a given subject. We
try least-globally-used slots first, which tends to spread exams
evenly across the calendar — helping both find a solution faster in
practice and improving the S1/S5/S6 soft-constraint scores of
whatever solution is found.

**Q24. What is MRV (most constrained variable) and where is it used?**
"Most Constrained Variable" — process the variable (subject) most
likely to fail first, so failures are discovered high in the search
tree instead of deep in it. We approximate it with degree-descending
ordering when driving the backtracking search (Pruning rule #4 in the
spec).

**Q25. How did you validate that the greedy and DSATUR results are
real and not fabricated?**
Both algorithms are actually executed against the real 60-student,
11-subject dataset every time the program runs (`--compare`); the
comparison table in the README and project report is copy-pasted
output from an actual run, not hand-written numbers.

**Q26. Why does the dataset validator cross-check against
subject_conflicts.csv instead of trusting it outright?**
Because the spec explicitly treats `subject_conflicts.csv` as a
reference/verification file, not a source of truth — the graph must be
rebuilt from `student_subjects.csv`. Cross-checking catches the case
where the two files have drifted apart (e.g. if the dataset generator
changes) and reports discrepancies instead of silently picking one.

**Q27. What would you change to support 200 subjects instead of 11?**
Swap the `IntSet` linear-scan-based adjacency and neighbor-color sets
for hash-set-backed versions (still O(1) average membership, but no
longer O(degree) per check), and raise the `MAX_SUBJECTS` /
`MAX_STUDENTS` compile-time caps or switch those static arrays to
dynamically allocated ones. The core algorithms (DSATUR, backtracking)
don't need to change.

**Q28. Is the solver deterministic?**
Yes. No random numbers are used anywhere in the solver — coloring
order is decided by saturation/degree/enrollment (all deterministic
given the dataset), backtracking explores slots in a fixed
least-used-first order, and tie-breaks are always resolved the same
way. The same dataset and config always produce the same timetable.

**Q29. Why is graph coloring's decision problem NP-complete, and what
does that mean practically for this project?**
Deciding whether a graph can be colored with k colors (for k ≥ 3) is
NP-complete in general, so no known polynomial algorithm solves it
exactly for all graphs. Practically, this is why we use a strong
heuristic (DSATUR) rather than searching for a provably optimal
coloring, and why the *repair* step (backtracking) is exponential in
the worst case — we accept that and rely on the small problem size and
pruning to keep it fast here.

**Q30. What's the one-sentence summary you'd give a professor?**
"We rebuild a conflict graph from real registration data, color it
with a priority-queue-driven DSATUR algorithm to get a near-minimal
number of exam slots, map those slots onto an actual calendar, and use
a pruned backtracking search to repair any day-level scheduling
constraints DSATUR's coloring alone can't see — all validated against
six explicit hard constraints and scored against four soft ones."
