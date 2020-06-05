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

#include "slices.h"
#include "common.h"

static R_altrep_class_t slice_altrep;

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

SEXP slice_new(SEXP source, R_xlen_t start, R_xlen_t size) {

    assert(TYPEOF(source) == INTSXP
        || TYPEOF(source) == REALSXP
        || TYPEOF(source) == CPLXSXP
        || TYPEOF(source) == LGLSXP
        || TYPEOF(source) == VECSXP
        || TYPEOF(source) == STRSXP);

                   // FIXME check vector size vs start and end

    if (get_debug_mode()) {
        Rprintf("slice_new\n");
        Rprintf("           SEXP: %p\n", source);
        Rprintf("          start: %li\n", start);
        Rprintf("           size: %li\n", size);
    }

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
    SET_TAG(data, R_NilValue); // Starts as R_NilValue, becomes a vector if it the slice is written to
    SETCDR (data, R_NilValue); // Nothing here

    return R_new_altrep(slice_altrep, window, data);
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

static inline void set_materialized_data(SEXP x, SEXP data) {
    SEXP/*LISTSXP*/ cell =  R_altrep_data2(x);
    SET_TAG(cell, data);
}

static inline bool is_materialized(SEXP x) {
    SEXP/*LISTSXP*/ cell =  R_altrep_data2(x);
    return TAG(cell) != R_NilValue;
}

SEXP slice_duplicate(SEXP x, Rboolean deep) {

    if (get_debug_mode()) {
        Rprintf("slice_duplicate\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("           deep: %i\n", deep);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP           source = get_source(x);

    if (deep) {
        R_xlen_t start = 0;
        R_xlen_t size  = 0;
        read_start_and_size(window, &start, &size);
        SEXP slice = slice_new(source, start, size);
        if (is_materialized(x)) {
            SEXP data = get_materialized_data(x);
            set_materialized_data(slice, duplicate(data));
        }
        return slice;
    } else {
        SEXP/*LISTSXP*/ meta = allocSExp(LISTSXP);
        SETCAR (meta, source);     // The original vector
        SEXP data = is_materialized(x) ? get_materialized_data(x) : R_NilValue;
        SET_TAG(meta, data);       // Starts as R_NilValue, becomes a vector if it the slice is written to
        SETCDR (meta, R_NilValue); // Nothing here
        return R_new_altrep(slice_altrep, window, meta);
    }
}

static Rboolean slice_inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int)) {

    Rprintf("slice_altrep %s\n", type2char(TYPEOF(x)));

    inspect_subtree(R_altrep_data1(x), pre, deep, pvec);
    inspect_subtree(R_altrep_data2(x), pre, deep, pvec);

    return FALSE;
}

static R_xlen_t slice_length(SEXP x) {
    if (get_debug_mode()) {
        Rprintf("slice_length\n");
        Rprintf("           SEXP: %p\n", x);
    }

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
        default: Rf_error("Illegal source type for a slice: %i.\n", type);
    }
}

static void *slice_dataptr(SEXP x, Rboolean writeable) {
    assert(x != NULL);

    if (get_debug_mode()) {
        Rprintf("slice_dataptr\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("      writeable: %i\n", writeable);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP data = get_materialized_data(x);
        return (writeable) ? DATAPTR(data) : ((void *) DATAPTR_RO(data));
    }

    if (writeable) {
        SEXP/*INTSXP*/ window = get_window(x);
        SEXP           source = get_source(x);

        R_xlen_t start = 0;
        R_xlen_t size  = 0;
        read_start_and_size(window, &start, &size);

        SEXP data = copy_data_in_range(source, start, size);
        set_materialized_data(x, data);
        return DATAPTR(data);
    }

    return (void *) extract_read_only_data_pointer(x);
}

static const void *slice_dataptr_or_null(SEXP x) {
    assert(x != NULL);

    if (get_debug_mode()) {
        Rprintf("slice_dataptr_or_null\n");
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

    if (get_debug_mode()) {
        Rprintf("    projecting index\n");
        Rprintf("         window SEXP: %p\n",  window);
        Rprintf("               start: %li\n", start);
        Rprintf("                size: %li\n", size);
        Rprintf("         input index: %li\n", index);
        Rprintf("     projected index: %li\n", projected_index);
    }

    return projected_index;
}

static int slice_integer_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("slice_integer_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*INTSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == INTSXP);
        return INTEGER_ELT(data, i);
    }

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*INTSXP*/ source = get_source(x);

    assert(TYPEOF(source) == INTSXP);

    R_xlen_t projected_index = project_index(window, i);

    return INTEGER_ELT(source, projected_index);
}

static double slice_numeric_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("slice_numeric_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*REALSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == REALSXP);
        return REAL_ELT(data, i);
    }

    SEXP/*INTSXP*/  window = get_window(x);
    SEXP/*REALSXP*/ source = get_source(x);

    assert(TYPEOF(source) == REALSXP);

    R_xlen_t projected_index = project_index(window, i);

    return REAL_ELT(source, projected_index);
}

static Rbyte slice_raw_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("slice_raw_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*RAWSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == RAWSXP);
        return RAW_ELT(data, i);
    }

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*RAWSXP*/ source = get_source(x);

    assert(TYPEOF(source) == RAWSXP);

    R_xlen_t projected_index = project_index(window, i);

    return RAW_ELT(source, projected_index);
}

static Rcomplex slice_complex_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("slice_complex_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*CPLXSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == CPLXSXP);
        return COMPLEX_ELT(data, i);
    }

    SEXP/*INTSXP*/  window = get_window(x);
    SEXP/*CPLXSXP*/ source = get_source(x);

    assert(TYPEOF(source) == CPLXSXP);

    R_xlen_t projected_index = project_index(window, i);

    return COMPLEX_ELT(source, projected_index);
}

static int slice_logical_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("slice_logical_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*LGLSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == LGLSXP);
        return LOGICAL_ELT(data, i);
    }

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*LGLSXP*/ source = get_source(x);

    assert(TYPEOF(source) == LGLSXP);

    R_xlen_t projected_index = project_index(window, i);

    return LOGICAL_ELT(source, projected_index);
}

static R_xlen_t slice_integer_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("slice_integer_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*INTSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == INTSXP);
        return INTEGER_GET_REGION(data, i, n, buf);
    }

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*INTSXP*/ source = get_source(x);

    assert(TYPEOF(source) == INTSXP);

    R_xlen_t projected_index = project_index(window, i);

    return INTEGER_GET_REGION(source, projected_index, n, buf);
}

static R_xlen_t slice_numeric_get_region(SEXP x, R_xlen_t i, R_xlen_t n, double *buf) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("slice_numeric_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*REALSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == REALSXP);
        return REAL_GET_REGION(data, i, n, buf);
    }

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*REALSXP*/ source = get_source(x);

    assert(TYPEOF(source) == REALSXP);

    R_xlen_t projected_index = project_index(window, i);

    return REAL_GET_REGION(source, projected_index, n, buf);
}

static R_xlen_t slice_raw_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rbyte *buf) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("slice_raw_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*RAWSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == RAWSXP);
        return RAW_GET_REGION(data, i, n, buf);
    }

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*RAWSXP*/ source = get_source(x);

    assert(TYPEOF(source) == RAWSXP);

    R_xlen_t projected_index = project_index(window, i);

    return RAW_GET_REGION(source, projected_index, n, buf);
}

static R_xlen_t slice_complex_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rcomplex *buf) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("slice_complex_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*CPLXSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == CPLXSXP);
        return COMPLEX_GET_REGION(data, i, n, buf);
    }

    SEXP/*INTSXP*/  window = get_window(x);
    SEXP/*CPLXSXP*/ source = get_source(x);

    assert(TYPEOF(source) == CPLXSXP);

    R_xlen_t projected_index = project_index(window, i);

    return COMPLEX_GET_REGION(source, projected_index, n, buf);
}

static R_xlen_t slice_logical_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("slice_logical_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*LGLSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == LGLSXP);
        return LOGICAL_GET_REGION(data, i, n, buf);
    }

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP/*LGLSXP*/ source = get_source(x);

    assert(TYPEOF(source) == LGLSXP);

    R_xlen_t projected_index = project_index(window, i);

    return LOGICAL_GET_REGION(source, projected_index, n, buf);
}


//SEXP copy_data_at_integer_indices(SEXP source, SEXP/*INTSXP*/ indices) {
//    assert(TYPEOF(indices) == INTSXP);
//
//    R_xlen_t size = XLENGTH(indices);
//    SEXP target = allocVector(TYPEOF(source), size);
//
//    for (R_xlen_t i = 0; i < size; i++) {
//        int index = INTEGER_ELT(indices, i);
//        copy_element(source, (R_xlen_t) index, target, i);
//    }
//
//    return target;
//}
//
//SEXP copy_data_at_numeric_indices(SEXP source, SEXP/*REALSXP*/ indices) {
//    R_xlen_t size = XLENGTH(indices);
//    SEXP target = allocVector(TYPEOF(source), size);
//
//    for (R_xlen_t i = 0; i < size; i++) {
//        double index = REAL_ELT(indices, i);
//        copy_element(source, (R_xlen_t) index, target, i);
//    }
//
//    return target;
//}
//
////SEXP copy_data_at_indices(SEXP source, SEXP/*INTSXP | REALSXP*/ indices) {
////    SEXPTYPE type = TYPEOF(indices);
////    assert(type == INTSXP || type == REALSXP);
////
////    switch (type) {
////        case INTSXP:  return copy_data_at_integer_indices(source, indices);
////        case REALSXP: return copy_data_at_numeric_indices(source, indices);
////        default:      Rf_error("Slices can be indexed by integer or numeric vectors but found: %d\n", type);
////    }
////}



static SEXP slice_extract_subset(SEXP x, SEXP indices, SEXP call) {
    assert(x != NULL);

    if (get_debug_mode()) {
        Rprintf("slice_extract_subset\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("        indices: %p\n", indices);
        Rprintf("           call: %p\n", call);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP           source = get_source(x);

    // No indices.
    R_xlen_t size = XLENGTH(indices);
    if (size == 0) {
        return allocVector(TYPEOF(source), 0);
    }

    if (is_materialized(x)) {
        return copy_data_at_indices(source, indices);
    }

    if (!are_indices_contiguous(indices)) {
        Rf_error("Non-contiguous slices are not implemented yet.\n"); // FIXME
    }

    // Contiguous indices.
    R_xlen_t start = get_first_element_as_length(indices) - 1;
    R_xlen_t projected_start = project_index(window, start);
    return slice_new(source, projected_start, size);
}

// R_set_altstring_Set_elt_method
// static void string_set_elt(SEXP x, R_xlen_t i, SEXP v)

// R_set_altstring_Elt_method
// string_elt(SEXP x, R_xlen_t i)

// UFO Inits
void init_slice_altrep_class(DllInfo * dll) {
    //R_altrep_class_t slice_altrep;
    R_altrep_class_t cls = R_make_altinteger_class("slice_altrep", "viewport_altrep", dll);
    slice_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, slice_duplicate);
    R_set_altrep_Inspect_method(cls, slice_inspect);
    R_set_altrep_Length_method(cls, slice_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, slice_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, slice_dataptr_or_null);

    R_set_altinteger_Elt_method(cls, slice_integer_element);
    R_set_altlogical_Elt_method(cls, slice_logical_element);
    R_set_altreal_Elt_method   (cls, slice_numeric_element);
    R_set_altcomplex_Elt_method(cls, slice_complex_element);
    R_set_altraw_Elt_method    (cls, slice_raw_element);

    R_set_altinteger_Get_region_method(cls, slice_integer_get_region);
    R_set_altlogical_Get_region_method(cls, slice_logical_get_region);
    R_set_altreal_Get_region_method   (cls, slice_numeric_get_region);
    R_set_altcomplex_Get_region_method(cls, slice_complex_get_region);
    R_set_altraw_Get_region_method    (cls, slice_raw_get_region);

    R_set_altvec_Extract_subset_method(cls, slice_extract_subset);
}

//SEXP/*NILSXP*/ slice(SEXP, SEXP/*INTSXP|REALSXP*/ start, SEXP/*INTSXP|REALSXP*/ size);

SEXP create_slice(SEXP source, SEXP/*INTSXP|REALSXP*/ start_sexp, SEXP/*INTSXP|REALSXP*/ size_sexp) {
    assert(TYPEOF(start_sexp) == INTSXP || TYPEOF(start_sexp) == REALSXP);
    assert(TYPEOF(size_sexp) == INTSXP || TYPEOF(size_sexp) == REALSXP);
    assert(XLENGTH(start_sexp) > 0);
    assert(XLENGTH(size_sexp) > 0);

    R_xlen_t start = get_first_element_as_length(start_sexp) - 1;
    R_xlen_t size  = get_first_element_as_length(size_sexp);

    if (get_debug_mode()) {
        Rprintf("create slice\n");
        Rprintf("           SEXP: %p\n", source);
        Rprintf("          start: %li\n", start);
        Rprintf("           size: %li\n", size);
    }

    return slice_new(source, start, size);
}