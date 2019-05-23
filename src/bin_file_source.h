#pragma once

#include "Rinternals.h"

typedef struct {
    const char* path;
    size_t element_size; /* in bytes */
} ufo_file_source_data_t;

SEXP/*EXTPTRSXP*/ ufo_bin_file_source(SEXP/*STRSXP*/ path);