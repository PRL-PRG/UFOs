#pragma once
#include <stdbool.h>
#include <stdio.h>
#include "string_set.h"
#include "token.h"
#include "tokenizer.h"

typedef struct {
    long interval;
    long *offsets;
    size_t size;
    size_t allocated;
} offset_record_t;

typedef struct {
    size_t rows;
    size_t columns;
    token_type_t *column_types;
    char **column_names;
    offset_record_t *row_offsets;
} scan_results_t;

typedef struct {
    size_t size;
    tokenizer_token_t **tokens;
} read_results_t; // FIXME rename


size_t              offset_record_human_readable_key(offset_record_t *, size_t i);
scan_results_t     *ufo_csv_perform_initial_scan(tokenizer_t *, const char *path, long record_row_offsets_at_interval, bool header, size_t initial_buffer_size);
string_set_t       *ufo_csv_read_column_unique_values(tokenizer_t *, const char *path, size_t target_column, scan_results_t *, size_t limit, size_t initial_buffer_size);
read_results_t      ufo_csv_read_column(tokenizer_t *, const char *path, size_t target_column, scan_results_t *, size_t first_row, size_t last_row, size_t initial_buffer_size);
void                scan_results_free(scan_results_t *);
