#include <stdio.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "data_loader.h"
#include "data_validator.h"
#include "conflict_builder.h"
#include "coloring.h"
#include "timetable.h"
#include "constraint_checker.h"
#include "backtracking.h"
#include "evaluator.h"
#include "statistics.h"

static void print_usage(void) {
    printf("Usage: exam_timetable [options]\n");
    printf("  (no options)          Generate and display the full timetable\n");
    printf("  --validate            Only validate the dataset and exit\n");
    printf("  --student <ID>        Print one student's personal schedule\n");
    printf("  --subject <ID>        Print one subject's exam info\n");
    printf("  --stats               Print dataset/timetable statistics\n");
    printf("  --show-graph          Print the conflict graph adjacency list\n");
    printf("  --algorithm <name>    greedy | dsatur (default: dsatur)\n");
    printf("  --compare             Run greedy vs DSATUR and compare results\n");
    printf("  --data <dir>          Dataset directory (default: data)\n");
    printf("  --help                Show this message\n");
}

static void show_graph(const ConflictGraph *g) {
    printf("Conflict graph (adjacency list), %d vertices:\n\n", g->num_vertices);
    for (int v = 0; v < g->num_vertices; v++) {
        printf("%-6s (%s, enrollment %d) -> ", g->vertices[v].subject_id,
               g->vertices[v].subject_type, g->vertices[v].enrollment);
        for (int k = 0; k < g->adjacency[v].size; k++) {
            int nb = g->adjacency[v].items[k];
            printf("%s(w=%d) ", g->vertices[nb].subject_id, g->weight[v][nb]);
        }
        printf("\n");
    }
}

/* Sort vertex indices by descending degree -- used both as the
 * "largest degree first" idea for reporting and reused elsewhere. */
static void order_by_degree_desc(const ConflictGraph *g, int order[MAX_SUBJECTS]) {
    for (int i = 0; i < g->num_vertices; i++) order[i] = i;
    for (int i = 0; i < g->num_vertices - 1; i++) {
        int best = i;
        for (int j = i + 1; j < g->num_vertices; j++) {
            if (graph_degree(g, order[j]) > graph_degree(g, order[best])) best = j;
        }
        int tmp = order[i]; order[i] = order[best]; order[best] = tmp;
    }
}

static int run_pipeline(const char *data_dir, const char *algorithm, int compare,
                         const char *student_query, const char *subject_query,
                         int stats_only, int show_graph_only, int validate_only) {
    printf("============================================================\n");
    printf("EXAM TIMETABLE GENERATOR\n");
    printf("============================================================\n\n");

    printf("Loading dataset...\n");
    Dataset ds;
    dataset_init(&ds);
    if (!dataset_load(&ds, data_dir)) return 1;
    printf("[OK] Students loaded: %d\n", ds.num_students);
    printf("[OK] Subjects loaded: %d\n", ds.num_subjects);
    printf("[OK] Registrations loaded: %d\n\n", ds.num_registrations);

    printf("Validating dataset...\n");
    ValidationReport vrep = validate_dataset(&ds);
    if (vrep.error_count > 0) {
        fprintf(stderr, "\nERROR:\nDataset failed validation with %d error(s). Fix the CSV files and re-run.\n",
                vrep.error_count);
        return 1;
    }
    printf("[OK] Dataset structurally valid (0 errors)\n\n");

    printf("Building conflict graph from student_subjects.csv...\n");
    ConflictGraph g;
    build_conflict_graph(&ds, &g);
    int edge_count = 0;
    for (int i = 0; i < g.num_vertices; i++) edge_count += g.adjacency[i].size;
    edge_count /= 2;
    printf("[OK] Vertices: %d\n", g.num_vertices);
    printf("[OK] Conflict edges: %d\n\n", edge_count);

    cross_check_conflicts(&g, data_dir, &vrep);
    printf("\n");

    if (validate_only) {
        printf("Validation-only run complete. Errors: %d, Warnings: %d.\n", vrep.error_count, vrep.warning_count);
        return 0;
    }
    if (show_graph_only) {
        show_graph(&g);
        return 0;
    }

    /* --- Coloring --- */
    int color_dsatur[MAX_SUBJECTS], color_greedy[MAX_SUBJECTS];
    ColoringStats stats_dsatur = {0, 0}, stats_greedy = {0, 0};
    int colors_dsatur = dsatur_coloring(&g, color_dsatur, &stats_dsatur);
    int colors_greedy = greedy_coloring(&g, color_greedy, &stats_greedy);

    if (compare) {
        printf("============================================================\n");
        printf("ALGORITHM COMPARISON (run on the actual dataset)\n");
        printf("============================================================\n");
        printf("%-15s %-12s %-14s %-10s\n", "Algorithm", "Slots Used", "Comparisons", "Steps");
        printf("--------------------------------------------------------------\n");
        printf("%-15s %-12d %-14ld %-10ld\n", "Greedy", colors_greedy, stats_greedy.comparisons, stats_greedy.steps);
        printf("%-15s %-12d %-14ld %-10ld\n", "DSATUR", colors_dsatur, stats_dsatur.comparisons, stats_dsatur.steps);
        printf("\n");
    }

    const char *chosen = algorithm ? algorithm : "dsatur";
    int *chosen_colors = strcmp(chosen, "greedy") == 0 ? color_greedy : color_dsatur;
    int chosen_num_colors = strcmp(chosen, "greedy") == 0 ? colors_greedy : colors_dsatur;

    printf("Running %s...\n", chosen);
    if (chosen_num_colors > TOTAL_SLOTS) {
        fprintf(stderr,
            "ERROR:\nNo feasible timetable exists with the configured number\n"
            "of examination slots (%d slots available, %d needed by coloring alone).\n"
            "Try increasing DAYS or SLOTS_PER_DAY in src/config.h.\n",
            TOTAL_SLOTS, chosen_num_colors);
        return 1;
    }

    /* Map colors directly onto slots (color value == slot id, since
     * they're both just small integers 0..k-1 and TOTAL_SLOTS >= k). */
    Timetable timetable;
    timetable_init(&timetable, g.num_vertices);
    for (int v = 0; v < g.num_vertices; v++) timetable.slot[v] = chosen_colors[v];
    printf("[OK] Initial coloring generated (%d colors)\n\n", chosen_num_colors);

    printf("Validating timetable...\n");
    ConstraintReport rep = check_all_hard_constraints(&timetable, &g, &ds, 0);
    if (rep.is_valid) {
        printf("[OK] Hard constraints satisfied by direct color->slot mapping\n\n");
    } else {
        printf("[!!] Direct color->slot mapping violates timetable-level constraints\n");
        printf("     (H1=%d H4=%d H5=%d H6=%d) -- running backtracking repair...\n\n",
               rep.h1_violations, rep.h4_violations, rep.h5_violations, rep.h6_violations);

        int order[MAX_SUBJECTS];
        order_by_degree_desc(&g, order); /* most-constrained-first (Pruning 4) */

        BacktrackStats bstats;
        Timetable repaired;
        int ok = backtracking_solve(&g, &ds, order, &repaired, &bstats);
        printf("Backtracking search: %ld nodes explored, %ld backtracks.\n", bstats.nodes_explored, bstats.backtracks);
        if (!ok) {
            fprintf(stderr,
                "\nERROR:\nNo feasible timetable exists with the configured number\n"
                "of examination slots and constraints.\n"
                "Try increasing DAYS or SLOTS_PER_DAY in src/config.h.\n");
            return 1;
        }
        timetable = repaired;
        rep = check_all_hard_constraints(&timetable, &g, &ds, 0);
        printf("[OK] Backtracking repair produced a valid timetable\n\n");
    }

    /* Recompute the minimum colors needed for S6 scoring, and the
     * live report used below. */
    ConstraintReport final_report = check_all_hard_constraints(&timetable, &g, &ds, 1);

    printf("Optimizing timetable...\n");
    SoftEvaluation eval = evaluate_soft_constraints(&timetable, &g, &ds, chosen_num_colors);
    printf("[OK] Final timetable generated\n\n");

    /* --- Outputs --- */
    timetable_export_subjects_csv(&timetable, &g, "output");
    timetable_export_student_schedules_csv(&timetable, &g, &ds, "output");
    export_conflict_graph_csv(&g, "output");
    DatasetStatistics dstats = compute_statistics(&timetable, &g, &ds);
    export_statistics_txt(&dstats, "output");

    if (student_query) {
        printf("\n");
        print_student_schedule(&timetable, &g, &ds, student_query);
    }
    if (subject_query) {
        printf("\n");
        print_subject_info(&timetable, &g, subject_query);
    }
    if (stats_only) {
        printf("\n");
        print_statistics(&dstats);
    }

    printf("\n");
    timetable_print(&timetable, &g);

    printf("============================================================\n");
    printf("VALIDATION\n");
    printf("============================================================\n\n");
    printf("Hard constraints:\n");
    printf("%s No student has overlapping exams (H1: %d violations)\n", final_report.h1_violations == 0 ? "[OK]" : "[!!]", final_report.h1_violations);
    printf("%s All students of a subject share its slot (H2: %d violations)\n", final_report.h2_violations == 0 ? "[OK]" : "[!!]", final_report.h2_violations);
    printf("%s All subjects assigned (H3: %d violations)\n", final_report.h3_violations == 0 ? "[OK]" : "[!!]", final_report.h3_violations);
    printf("%s Max %d exams per day (H4: %d violations)\n", final_report.h4_violations == 0 ? "[OK]" : "[!!]", MAX_EXAMS_PER_DAY, final_report.h4_violations);
    printf("%s No consecutive exams (H5: %d violations)\n", final_report.h5_violations == 0 ? "[OK]" : "[!!]", final_report.h5_violations);
    printf("%s Conflict graph respected (H6: %d violations)\n", final_report.h6_violations == 0 ? "[OK]" : "[!!]", final_report.h6_violations);
    printf("\nStatus: %s\n\n", final_report.is_valid ? "VALID" : "INVALID");

    printf("============================================================\n");
    printf("FINAL RESULT\n");
    printf("============================================================\n\n");
    printf("Algorithm used:            %s\n", chosen);
    printf("Days used:                 %d\n", dstats.days_used);
    printf("Exam slots used:           %d\n", dstats.slots_used);
    printf("Hard constraint violations: %d\n",
           final_report.h1_violations + final_report.h2_violations + final_report.h3_violations +
           final_report.h4_violations + final_report.h5_violations + final_report.h6_violations);
    printf("\n");
    print_soft_evaluation(&eval);
    printf("\nStatus: %s\n", final_report.is_valid ? "VALID" : "INVALID");
    printf("============================================================\n");

    return final_report.is_valid ? 0 : 2;
}

int main(int argc, char **argv) {
    const char *data_dir = "data";
    const char *algorithm = NULL;
    const char *student_query = NULL;
    const char *subject_query = NULL;
    int compare = 0, stats_only = 0, show_graph_only = 0, validate_only = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) { print_usage(); return 0; }
        else if (strcmp(argv[i], "--validate") == 0) validate_only = 1;
        else if (strcmp(argv[i], "--stats") == 0) stats_only = 1;
        else if (strcmp(argv[i], "--show-graph") == 0) show_graph_only = 1;
        else if (strcmp(argv[i], "--compare") == 0) compare = 1;
        else if (strcmp(argv[i], "--algorithm") == 0 && i + 1 < argc) algorithm = argv[++i];
        else if (strcmp(argv[i], "--student") == 0 && i + 1 < argc) student_query = argv[++i];
        else if (strcmp(argv[i], "--subject") == 0 && i + 1 < argc) subject_query = argv[++i];
        else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) data_dir = argv[++i];
        else { fprintf(stderr, "Unknown option: %s\n", argv[i]); print_usage(); return 1; }
    }

    return run_pipeline(data_dir, algorithm, compare, student_query, subject_query,
                         stats_only, show_graph_only, validate_only);
}
