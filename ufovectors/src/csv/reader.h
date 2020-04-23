#pragma once
#include <stdbool.h>
#include <stdio.h>

#include "token.h"

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
    offset_record_t *row_offsets;
} scan_results_t;

size_t              offset_record_human_readable_key(offset_record_t *record, size_t i);
scan_results_t     *ufo_csv_perform_initial_scan(char *path, long record_row_offsets_at_interval);
tokenizer_token_t **ufo_csv_read_column(char *path, size_t target_column, scan_results_t *scan_results);