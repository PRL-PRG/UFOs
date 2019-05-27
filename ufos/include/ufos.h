#pragma once

#include "userfaultCore.h"

#include <R.h>
#include <Rinternals.h>

typedef enum {
    UFO_CHAR = 9,
    UFO_LGL  = 10,
    UFO_INT  = 13,
    UFO_REAL = 14,
    UFO_CPLX = 15
} ufo_vector_type_t; // IS THIS NECESSARY

typedef struct {
    ufUserData*         data;
    ufPopulateRange     population_function;
    ufo_vector_type_t   vector_type;
    //ufUpdateRange     update_function;
    R_xlen_t            length;
} ufo_source_t;

// Initialization and shutdown
void ufo_initialize();
void ufo_shutdown();

// Constructor
SEXP ufo_new(ufo_source_t*);

// Function types for R dynloader.
typedef void (*ufo_initialize_t)(void);
typedef void (*ufo_shutdown_t)(void);
typedef SEXP (*ufo_new_t)(ufo_source_t*);