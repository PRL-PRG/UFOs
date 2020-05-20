#pragma once

#include "mappedMemory/userfaultCore.h"

#include <R.h>
#include <Rinternals.h>

typedef enum {
    UFO_CHAR = CHARSXP,
    UFO_LGL  = LGLSXP,
    UFO_INT  = INTSXP,
    UFO_REAL = REALSXP,
    UFO_CPLX = CPLXSXP,
    UFO_RAW  = RAWSXP,
    UFO_STR  = STRSXP,
} ufo_vector_type_t;

// Function types for ufo_source_t
typedef void (*ufo_destructor_t)(ufUserData*);

// Source definition
typedef struct {
    ufUserData*         data;
    ufPopulateRange     population_function;
    ufo_destructor_t    destructor_function;
    ufo_vector_type_t   vector_type;
    //ufUpdateRange     update_function;
    /*R_len_t*/size_t   vector_size;
    size_t              element_size;
    int                 *dimensions;        // because they are `ints` are in R
    size_t              dimensions_length;
    int32_t             min_load_count;
} ufo_source_t;

// Initialization and shutdown
SEXP ufo_shutdown();
SEXP ufo_initialize();

// Constructor
SEXP ufo_new(ufo_source_t*);
SEXP ufo_new_multidim(ufo_source_t* source);

// Auxiliary functions.
SEXPTYPE ufo_type_to_vector_type (ufo_vector_type_t);

// Function types for R dynloader.
typedef SEXP (*ufo_new_t)(ufo_source_t*);
typedef SEXPTYPE (*ufo_type_to_vector_type_t)(ufo_vector_type_t);