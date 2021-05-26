#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

typedef struct {
    char const*         path;
    size_t              element_size;
    size_t              vector_size;
    SEXPTYPE            type;
    FILE*               file_handle;
    size_t              file_cursor;
} altrep_ufo_config_t;

typedef struct {
    int from;
    int to;
    int by;
    size_t size;
    SEXPTYPE            type;
} altrep_seq_config_t;