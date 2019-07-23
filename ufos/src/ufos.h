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
    UFO_RAW  = RAWSXP
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
    size_t              vector_size;
    size_t              element_size;
} ufo_source_t;

// Initialization and shutdown
void ufo_initialize();
void ufo_shutdown();

// Constructor
SEXP ufo_new(ufo_source_t*);

// Auxiliary functions.
SEXPTYPE ufo_type_to_vector_type (ufo_vector_type_t);


// Function types for R dynloader.
typedef void (*ufo_initialize_t)(void);
typedef void (*ufo_shutdown_t)(void);
typedef SEXP (*ufo_new_t)(ufo_source_t*);
typedef SEXPTYPE (*ufo_type_to_vector_type_t)(ufo_vector_type_t);