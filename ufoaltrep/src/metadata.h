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
    R_xlen_t            cache_buffer_size;
    R_xlen_t            cache_chunk_size;
} altrep_ufo_config_t;