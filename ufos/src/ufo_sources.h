#pragma once

#include "mappedMemory/userfaultCore.h"

typedef int (*reader_function)(FILE* file, u_int64_t start, u_int64_t end,
                               ufUserData user_data);

typedef enum {
    UFO_CHAR = 9,
    UFO_LGL = 10,
    UFO_INT = 13,
    UFO_REAL = 14,
    UFO_CPLX = 15
} ufo_vector_type_t;

typedef struct {
    ufUserData*         data;
    ufPopulateRange     population_function;
    ufo_vector_type_t   vector_type;
    //ufUpdateRange     update_function;
    R_xlen_t            length;
} ufo_source_t;

