#ifndef UTIL_H
#define UTIL_H

#include "config.h"

/*
 * util.h
 * ------
 * Small shared helpers: a minimal CSV line splitter (handles
 * double-quoted fields containing commas, as used in
 * subject_conflicts.csv's "conflicting_students" column) and a
 * couple of string utilities. No external CSV library is used --
 * this is intentionally hand-written since parsing is not the
 * point of the assignment, but silently depending on a library for
 * something this small would be pointless indirection.
 */

#define MAX_FIELDS 16

typedef struct {
    char *fields[MAX_FIELDS];
    int count;
} CsvRow;

/* Splits a single CSV line (already stripped of trailing \r\n) into
 * fields, respecting double-quoted fields that may contain commas.
 * The returned CsvRow's fields point into a caller-owned scratch
 * buffer (mutated in place) -- copy them out if you need them to
 * outlive the next call. */
void csv_split_line(char *line, CsvRow *row);

/* Strips trailing \r and \n from a line read by fgets. */
void chomp(char *line);

/* Case-sensitive trim of leading/trailing whitespace, in place. */
char *trim(char *s);

#endif /* UTIL_H */
