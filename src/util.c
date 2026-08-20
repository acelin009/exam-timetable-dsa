#include "util.h"
#include <string.h>
#include <ctype.h>

void chomp(char *line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }
}

char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

void csv_split_line(char *line, CsvRow *row) {
    row->count = 0;
    char *p = line;

    while (*p != '\0' && row->count < MAX_FIELDS) {
        if (*p == '"') {
            /* quoted field: consume until closing quote */
            p++;
            row->fields[row->count++] = p;
            while (*p != '\0' && *p != '"') p++;
            if (*p == '"') {
                *p = '\0';
                p++;
                if (*p == ',') p++;
            }
        } else {
            row->fields[row->count++] = p;
            while (*p != '\0' && *p != ',') p++;
            if (*p == ',') {
                *p = '\0';
                p++;
            }
        }
    }
}
