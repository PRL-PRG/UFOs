#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Altrep.h>
#include <R_ext/Rallocators.h>

#include "debug.h"
#include "helpers.h"

#include "viewports.h"

static R_altrep_class_t viewport_altrep;

# define how_many_ints_in_R_xlen_t (sizeof(R_xlen_t) / sizeof(int))
typedef union {
    int      integers[how_many_ints_in_R_xlen_t];
    R_xlen_t length;
} converter_t;

void read_start_and_size(SEXP/*INTSXP*/ window, R_xlen_t *start, R_xlen_t *size) {
    converter_t start_converter;
    for (int i = 0; i < how_many_ints_in_R_xlen_t; i++) {
        start_converter.integers[i] = INTEGER_ELT(window, i);
    }

    converter_t size_converter;
    for (int i = 0; i < how_many_ints_in_R_xlen_t; i++) {
        size_converter.integers[i] = INTEGER_ELT(window, how_many_ints_in_R_xlen_t + i);
    }

    *start = start_converter.length;
    *size = size_converter.length;
}

SEXP viewport_new(SEXP source, R_xlen_t start, R_xlen_t size) {

    Rprintf("viewport_new %li, %li\n", start, size);
    assert(TYPEOF(source) == INTSXP
        || TYPEOF(source) == REALSXP
        || TYPEOF(source) == CPLXSXP
        || TYPEOF(source) == LGLSXP
        || TYPEOF(source) == VECSXP
        || TYPEOF(source) == STRSXP);

                   // FIXME check vector size vs start and end

    SEXP/*INTSXP*/ window = allocVector(INTSXP, 2 * how_many_ints_in_R_xlen_t);

    converter_t start_converter = { .length = start };
    converter_t size_converter  = { .length = size  };

    for (size_t i = 0; i < how_many_ints_in_R_xlen_t; i++) {
        SET_INTEGER_ELT(window, i, start_converter.integers[i]);
    }

    for (size_t i = 0; i < how_many_ints_in_R_xlen_t; i++) {
        SET_INTEGER_ELT(window, how_many_ints_in_R_xlen_t + i, size_converter.integers[i]);
    }

    SEXP/*LISTSXP*/ data = allocSExp(LISTSXP);
    SETCAR (data, source);     // The original vector
    SET_TAG(data, R_NilValue); // Starts as R_NilValue, becomes a vector if it the viewport is written to
    SETCDR (data, R_NilValue); // Nothing here

    return R_new_altrep(viewport_altrep, window, data);
}

static inline SEXP/*INTSXP*/ get_window(SEXP x) {
    return R_altrep_data1(x);
}

static inline SEXP get_source(SEXP x) {
    SEXP/*LISTSXP*/ cell =  R_altrep_data2(x);
    return CAR(cell);
}

static inline SEXP get_materialized_data(SEXP x) {
    SEXP/*LISTSXP*/ cell =  R_altrep_data2(x);
    return TAG(cell);
}

static inline bool is_materialized(SEXP x) {
    SEXP/*LISTSXP*/ cell =  R_altrep_data2(x);
    return TAG(cell) == R_NilValue;
}

SEXP viewport_duplicate(SEXP x, Rboolean deep) {

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP           source = get_source(x);

    if (deep) {
        R_xlen_t start = 0;
        R_xlen_t size  = 0;
        read_start_and_size(window, &start, &size);
        return viewport_new(source, start, size);
    }

    return R_new_altrep(viewport_altrep, window, source);
}

static Rboolean viewport_inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int)) {

    Rprintf("viewport_altrep %s\n", type2char(TYPEOF(x)));

    inspect_subtree(R_altrep_data1(x), pre, deep, pvec);

    if (R_altrep_data2(x) != R_NilValue) {
        inspect_subtree(R_altrep_data2(x), pre, deep, pvec);
    }

    return FALSE;
}

static R_xlen_t viewport_length(SEXP x) {
    //if (__get_debug_mode()) {
        Rprintf("viewport_length\n");
        Rprintf("           SEXP: %p\n", x);
    //}

    SEXP/*INTSXP*/ window = get_window(x);

    R_xlen_t start = 0;
    R_xlen_t size  = 0;
    read_start_and_size(window, &start, &size);

    return size;
}

const void *extract_read_only_data_pointer(SEXP x) {
    SEXP/*INTSXP*/ window = get_window(x);
    SEXP           source = get_source(x);

    R_xlen_t start = 0;
    R_xlen_t size  = 0;
    read_start_and_size(window, &start, &size);

    const void *data = DATAPTR_RO(source);

    SEXPTYPE type = TYPEOF(source);
    switch (type) {
        case INTSXP:  {
            const int *ints = (const int *) data;
            return (const void *) (&ints[start]);
        }
        case REALSXP: {
            const double *doubles = (const double *) data;
            return (const void *) (&doubles[start]);
        }
        case LGLSXP: {
            const Rboolean *booleans = (const Rboolean *) data;
            return (const void *) (&booleans[start]);
        }
        case RAWSXP: {
            const Rbyte *bytes = (const Rbyte *) data;
            return (const void *) (&bytes[start]);
        }
        case CPLXSXP: {
            const Rcomplex *trouble = (const Rcomplex *) data;
            return (const void *) (&trouble[start]);
        }
        case STRSXP:
        case VECSXP: {
            const SEXP *sexps = (const SEXP *) data;
            return (const void *) (&sexps[start]);
        }
        default: Rf_error("Illegal source type for a viewport: %i.\n", type);
    }
}

static void *viewport_dataptr(SEXP x, Rboolean writeable) {
    if (__get_debug_mode()) {
        Rprintf("viewport_dataptr\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("      writeable: %i\n", writeable);
    }
    if (writeable) {
        Rf_error("Not implemented yet: sadfess\n");     // FIXME
    } else {
        return (void *) extract_read_only_data_pointer(x);
    }
}

static const void *viewport_dataptr_or_null(SEXP x) {
    if (__get_debug_mode()) {
        Rprintf("viewport_dataptr_or_null\n");
        Rprintf("           SEXP: %p\n", x);
    }
    return extract_read_only_data_pointer(x);
}

R_xlen_t project_index(SEXP/*INTSXP*/ window, R_xlen_t index) {
    assert(window != R_NilValue);

    R_xlen_t start = 0;
    R_xlen_t size  = 0;
    read_start_and_size(window, &start, &size);

    assert(index < size);
    R_xlen_t projected_index = start + index;

    if (__get_debug_mode()) {
        Rprintf("    projecting index\n");
        Rprintf("         window SEXP: %p\n",  window);
        Rprintf("               start: %li\n", start);
        Rprintf("                size: %li\n", size);
        Rprintf("         input index: %li\n", index);
        Rprintf("     projected index: %li\n", projected_index);
    }

    return projected_index;
}

static int viewport_integer_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*INTSXP*/ source = get_source(x);

    assert(TYPEOF(source) == INTSXP);

    R_xlen_t projected_index = project_index(window, i);

    return INTEGER_ELT(source, projected_index);
}

static double viewport_numeric_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    SEXP/*INTSXP*/  window = get_window(x);
    SEXP/*REALSXP*/ source = get_source(x);

    assert(TYPEOF(source) == REALSXP);

    R_xlen_t projected_index = project_index(window, i);

    return REAL_ELT(source, projected_index);
}

static Rbyte viewport_raw_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*RAWSXP*/ source = get_source(x);

    assert(TYPEOF(source) == RAWSXP);

    R_xlen_t projected_index = project_index(window, i);

    return RAW_ELT(source, projected_index);
}

static Rcomplex viewport_complex_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    SEXP/*INTSXP*/  window = get_window(x);
    SEXP/*CPLXSXP*/ source = get_source(x);

    assert(TYPEOF(source) == CPLXSXP);

    R_xlen_t projected_index = project_index(window, i);

    return COMPLEX_ELT(source, projected_index);
}

static int viewport_logical_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*LGLSXP*/ source = get_source(x);

    assert(TYPEOF(source) == LGLSXP);

    R_xlen_t projected_index = project_index(window, i);

    return LOGICAL_ELT(source, projected_index);
}

static R_xlen_t viewport_integer_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    assert(x != R_NilValue);

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*INTSXP*/ source = get_source(x);

    assert(TYPEOF(source) == INTSXP);

    R_xlen_t projected_index = project_index(window, i);

    return INTEGER_GET_REGION(source, projected_index, n, buf);
}

static R_xlen_t viewport_numeric_get_region(SEXP x, R_xlen_t i, R_xlen_t n, double *buf) {
    assert(x != R_NilValue);

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*REALSXP*/ source = get_source(x);

    assert(TYPEOF(source) == REALSXP);

    R_xlen_t projected_index = project_index(window, i);

    return REAL_GET_REGION(source, projected_index, n, buf);
}

static R_xlen_t viewport_raw_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rbyte *buf) {
    assert(x != R_NilValue);

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*RAWSXP*/ source = get_source(x);

    assert(TYPEOF(source) == RAWSXP);

    R_xlen_t projected_index = project_index(window, i);

    return RAW_GET_REGION(source, projected_index, n, buf);
}

static R_xlen_t viewport_complex_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rcomplex *buf) {
    assert(x != R_NilValue);

    SEXP/*INTSXP*/  window = get_window(x);
    SEXP/*CPLXSXP*/ source = get_source(x);

    assert(TYPEOF(source) == CPLXSXP);

    R_xlen_t projected_index = project_index(window, i);

    return COMPLEX_GET_REGION(source, projected_index, n, buf);
}

static R_xlen_t viewport_logical_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    assert(x != R_NilValue);

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*LGLSXP*/ source = get_source(x);

    assert(TYPEOF(source) == LGLSXP);

    R_xlen_t projected_index = project_index(window, i);

    return LOGICAL_GET_REGION(source, projected_index, n, buf);
}

bool are_integer_indices_contiguous(SEXP/*INTSXP*/ indices) {
    assert(TYPEOF(indices) == INTSXP);

    R_xlen_t size = XLENGTH(indices);

    int previous = NA_INTEGER;

    for (int i = 0; i < size; i++) {
        int current = INTEGER_ELT(indices, i);
        if (current == NA_INTEGER) {
            return false;
        }
        if (previous != NA_INTEGER && previous != current + 1) {
            return false;
        }
        previous = current;
    }

    return true;
}

bool are_numeric_indices_contiguous(SEXP/*REALSXP*/ indices) {
    assert(TYPEOF(indices) == REALSXP);

    R_xlen_t size = XLENGTH(indices);

    double previous = NA_REAL;

    for (int i = 0; i < size; i++) {
        double current = REAL_ELT(indices, i);
        if (current == NA_REAL) {
            return false;
        }
        if (previous != NA_REAL && previous != current + 1) {
            return false;
        }
        previous = current;
    }

    return true;
}

bool are_indices_contiguous(SEXP/*INTSXP | REALSXP*/ indices) {
    SEXPTYPE type = TYPEOF(indices);
    assert(type == INTSXP || type == REALSXP);

	switch (type) {
	    case INTSXP:  return are_integer_indices_contiguous(indices);
	    case REALSXP: return are_numeric_indices_contiguous(indices);
	    default:      Rf_error("Viewports can be indexed by integer or numeric vectors but found: %d\n", type);
	}
}

R_xlen_t get_first_element_as_length(SEXP/*INTSXP | REALSXP*/ indices) {
    SEXPTYPE type = TYPEOF(indices);
    assert(type == INTSXP || type == REALSXP);
    assert(XLENGTH(indices) > 0);

    switch (type) {
        case INTSXP:  return (R_xlen_t) INTEGER_ELT(indices, 0);
        case REALSXP: return (R_xlen_t) REAL_ELT(indices, 0);
        default:      Rf_error("Viewports can be indexed by integer or numeric vectors but found: %d\n", type);
    }
}

static SEXP ufo_extract_subset(SEXP x, SEXP indices, SEXP call) {
    SEXP/*INTSXP*/ window = get_window(x);
    SEXP           source = get_source(x);

    if (!are_indices_contiguous(indices)) {
        Rf_error("Non-contiguous viewports are not implemented yet.\n"); // FIXME
    }

    // No indices.
    R_xlen_t size = XLENGTH(indices);
    if (size == 0) {
        return allocVector(TYPEOF(source), 0);
    }

    // Contiguous indices.
    R_xlen_t start = get_first_element_as_length(indices) - 1;
    R_xlen_t projected_start = project_index(window, start);
    return viewport_new(source, projected_start, size);
}


// UFO Inits
void init_viewport_altrep_class(DllInfo * dll) {
    //R_altrep_class_t viewport_altrep;
    R_altrep_class_t cls = R_make_altinteger_class("viewport_altrep", "ufo_altrep", dll);
    viewport_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, viewport_duplicate);
    R_set_altrep_Inspect_method(cls, viewport_inspect);
    R_set_altrep_Length_method(cls, viewport_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, viewport_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, viewport_dataptr_or_null);

    R_set_altinteger_Elt_method(cls, viewport_integer_element);
    R_set_altlogical_Elt_method(cls, viewport_logical_element);
    R_set_altreal_Elt_method   (cls, viewport_numeric_element);
    R_set_altcomplex_Elt_method(cls, viewport_complex_element);
    R_set_altraw_Elt_method    (cls, viewport_raw_element);

    R_set_altinteger_Get_region_method(cls, viewport_integer_get_region);
    R_set_altlogical_Get_region_method(cls, viewport_logical_get_region);
    R_set_altreal_Get_region_method   (cls, viewport_numeric_get_region);
    R_set_altcomplex_Get_region_method(cls, viewport_complex_get_region);
    R_set_altraw_Get_region_method    (cls, viewport_raw_get_region);

    R_set_altvec_Extract_subset_method(cls, ufo_extract_subset);
}

SEXP/*NILSXP*/ viewport_set_debug_mode(SEXP/*LGLSXP*/);
SEXP/*NILSXP*/ viewport(SEXP, SEXP/*INTSXP|REALSXP*/ start, SEXP/*INTSXP|REALSXP*/ size);

SEXP create_viewport(SEXP source, SEXP/*INTSXP|REALSXP*/ start_sexp, SEXP/*INTSXP|REALSXP*/ size_sexp) {
    assert(TYPEOF(start_sexp) == INTSXP || TYPEOF(start_sexp) == REALSXP);
    assert(TYPEOF(size_sexp) == INTSXP || TYPEOF(size_sexp) == REALSXP);
    assert(XLENGTH(start_sexp) > 0);
    assert(XLENGTH(size_sexp) > 0);

    R_xlen_t start = get_first_element_as_length(start_sexp) - 1;
    R_xlen_t size  = get_first_element_as_length(size_sexp);

    Rprintf("create viewport %li %li\n", start, size);

    return viewport_new(source, start, size);
}