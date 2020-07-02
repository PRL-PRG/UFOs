#pragma once

#include "../include/ufos.h"

typedef struct {
    const char*         path;
    ufo_vector_type_t   vector_type;
    size_t              element_size; /* in bytes */
    size_t              vector_size;
    FILE*               file_handle;
    size_t              file_cursor;
} ufo_file_source_data_t;



typedef struct {
    const char*         path;
    ufo_vector_type_t   vector_type;
    size_t              element_size; /* in bytes */
    size_t              vector_size;
    FILE*               file_handle;
    size_t              file_cursor;
} ufo_generator_source_data_t;