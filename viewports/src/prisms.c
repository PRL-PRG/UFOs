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

static R_altrep_class_t prism_altrep;

SEXP prism_new(SEXP source, SEXP/*INTSXP|REALSXP*/ indices) {
    assert(sizeof(double) >= sizeof(R_xlen_t));
    assert(TYPEOF(indices) == REALSXP || TYPEOF(indices) == INTSXP);
    assert(source != null);

    if (__get_debug_mode()) {
        Rprintf("prism_new\n");
        Rprintf("           SEXP: %p\n", source);
        R_xlen_t indices_length = XLENGTH(indices);
        for (R_xlen_t i = 0; i < indices_length; i++) {
            R_xlen_t value = (R_xlen_t) (TYPEOF(indices) ? REAL_ELT(indices, i) : INTEGER_ELT(indices, i));
            Rprintf("           [%l2i]: %li\n", i, value);
        }
    }

    SEXP/*LISTSXP*/ data = allocSExp(LISTSXP);
    SETCAR (data, source);     // The original vector
    SET_TAG(data, R_NilValue); // Starts as R_NilValue, becomes a vector if it the prism is written to
    SETCDR (data, R_NilValue); // Nothing here

    return R_new_altrep(prism_altrep, indices, data);
}

static inline SEXP/*INTSXP|REALSXP*/ get_indices(SEXP x) {
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

SEXP prism_duplicate(SEXP x, Rboolean deep) {//TODO

    if (__get_debug_mode()) {
        Rprintf("prism_duplicate\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("           deep: %i\n", deep);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
    SEXP                   source  = get_source(x);

    if (deep) {
        SEXP prism = prism_new(source, duplicate(indices));
        if (is_materialized(x)) {
            SEXP data = get_materialized_data(x);
            set_materialized_data(x, duplicate(data));
        }
        return prism;
    } else {
        SEXP/*LISTSXP*/ data = allocSExp(LISTSXP);
        SETCAR (data, source);
        if (is_materialized(x)) {
            SET_TAG(data, get_materialized_data(x));
        } else {
            SET_TAG(data, R_NilValue);
        }
        SETCDR (data, R_NilValue);
        return R_new_altrep(prism_altrep, indices, data);
    }
}

static Rboolean prism_inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int)) {

    Rprintf("prism_altrep %s\n", type2char(TYPEOF(x)));

    inspect_subtree(R_altrep_data1(x), pre, deep, pvec);
    inspect_subtree(R_altrep_data2(x), pre, deep, pvec);

    return FALSE;
}

static R_xlen_t prism_length(SEXP x) {
    if (__get_debug_mode()) {
        Rprintf("prism_length\n");
        Rprintf("           SEXP: %p\n", x);
    }

    return XLENGTH(get_indices(x));
}

//SEXP copy_from_source(SEXP x) {
//    SEXP/*INTSXP|REALSXP*/  indices = get_indices(x);
//    SEXP                    source  = get_source(x);
//    R_xlen_t                length  = XLENGTH(indices);
//
//    SEXP materialized = allocVector(TYPEOF(source), length);
//    R_xlen_t cursor = 0;
//    switch (TYPEOF(indices)) {
//        case INTSXP:
//            for (R_xlen_t i = 0; i < length; i++) {
//                R_xlen_t source_index = INTEGER_ELT(indices, i - 1);
//                copy_element(source, source_index, materialized, i);
//                cursor++;
//            }
//            break;
//
//        case REALSXP:
//            for (R_xlen_t i = 0; i < length; i++) {
//                R_xlen_t source_index = REAL_ELT(indices, i - 1);
//                copy_element(source, source_index, materialized, i);
//                cursor++;
//            }
//            break;
//
//        default:
//            Rf_error("Prisms can be indexed by integer or numeric vectors but found: %d\n", TYPEOF(indices));
//    }
//
//    assert(cursor == length);
//    return materialized;
//}

static void *prism_dataptr(SEXP x, Rboolean writeable) {
    assert(x != NULL);

    if (__get_debug_mode()) {
        Rprintf("prism_dataptr\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("      writeable: %i\n", writeable);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP data = get_materialized_data(x);
        return (writeable) ? DATAPTR(data) : ((void *) DATAPTR_RO(data));
    }

    SEXP/*INTSXP|REALSXP*/  indices = get_indices(x);
    SEXP                    source  = get_source(x);
    SEXP data = copy_data_at_indices(source, indices);
    set_materialized_data(x, data);
    return writeable ? DATAPTR(data) : (void *) DATAPTR_RO(data);
}

static const void *prism_dataptr_or_null(SEXP x) {
    assert(x != NULL);

    if (__get_debug_mode()) {
        Rprintf("prism_dataptr_or_null\n");
        Rprintf("           SEXP: %p\n", x);
    }

    if (is_materialized(x)) {
        SEXP data = get_materialized_data(x);
        return DATAPTR_RO(data);
    }

    SEXP/*INTSXP|REALSXP*/  indices = get_indices(x);
    SEXP                    source  = get_source(x);
    SEXP data = copy_data_at_indices(source, indices);
    set_materialized_data(x, data);
    return DATAPTR_RO(data);
}

static inline R_xlen_t translate_index(SEXP/*INTSXP|REALSXP*/ indices, R_xlen_t index) {
    SEXPTYPE type = TYPEOF(indices);
    assert(type == INTSXP || type == REALSXP);

    if (type == REALSXP) {
        return (R_xlen_t) REAL_ELT   (indices, index - 1);
    } else {
        return (R_xlen_t) INTEGER_ELT(indices, index - 1);
    }
}

static int prism_integer_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_integer_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*INTSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == INTSXP);
        return INTEGER_ELT(data, i);
    }

    SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
    SEXP/*INTSXP*/         source  = get_source(x);

    R_xlen_t projected_index = translate_index(indices, i);

    if (__get_debug_mode()) {
        Rprintf("projected_index: %li\n", projected_index);
    }

    assert(TYPEOF(source) == INTSXP);
    return INTEGER_ELT(source, projected_index);
}

static double prism_numeric_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_numeric_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*INTSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == INTSXP);
        return INTEGER_ELT(data, i);
    }

    SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
    SEXP/*REALSXP*/        source  = get_source(x);

    R_xlen_t projected_index = translate_index(indices, i);

    if (__get_debug_mode()) {
        Rprintf("projected_index: %li\n", projected_index);
    }

    assert(TYPEOF(source) == REALSXP);
    return REAL_ELT(source, projected_index);
}

static Rbyte prism_raw_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_raw_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*RAWSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == RAWSXP);
        return RAW_ELT(data, i);
    }

    SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
    SEXP/*RAWSXP*/         source  = get_source(x);

    R_xlen_t projected_index = translate_index(indices, i);

    if (__get_debug_mode()) {
        Rprintf("projected_index: %li\n", projected_index);
    }

    assert(TYPEOF(source) == RAWSXP);
    return RAW_ELT(source, projected_index);
}

static Rcomplex prism_complex_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_complex_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*CPLXSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == CPLXSXP);
        return COMPLEX_ELT(data, i);
    }

    SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
    SEXP/*CPLXSXP*/        source  = get_source(x);

    R_xlen_t projected_index = translate_index(indices, i);

    if (__get_debug_mode()) {
        Rprintf("projected_index: %li\n", projected_index);
    }

    assert(TYPEOF(source) == CPLXSXP);
    return COMPLEX_ELT(source, projected_index);
}

static int prism_logical_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_logical_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*LGLSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == LGLSXP);
        return LOGICAL_ELT(data, i);
    }

    SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
    SEXP/*LGLSXP*/         source  = get_source(x);

    R_xlen_t projected_index = translate_index(indices, i);

    if (__get_debug_mode()) {
        Rprintf("projected_index: %li\n", projected_index);
    }

    assert(TYPEOF(source) == LGLSXP);
    return LOGICAL_ELT(source, projected_index);
}

static R_xlen_t prism_integer_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_integer_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*INTSXP*/ data;
    if (!is_materialized(x)) {
        SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
        SEXP                   source  = get_source(x);
        SEXP data = copy_data_at_indices(source, indices);
        set_materialized_data(x, data);
    } else {
        data = get_materialized_data(x);
    }
    assert(TYPEOF(data) == INTSXP);
    return INTEGER_GET_REGION(data, i, n, buf);
}

static R_xlen_t prism_numeric_get_region(SEXP x, R_xlen_t i, R_xlen_t n, double *buf) {
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_numeric_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*REALSXP*/ data;
    if (!is_materialized(x)) {
        SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
        SEXP                   source  = get_source(x);
        SEXP data = copy_data_at_indices(source, indices);
        set_materialized_data(x, data);
    } else {
        data = get_materialized_data(x);
    }
    assert(TYPEOF(data) == REALSXP);
    return REAL_GET_REGION(data, i, n, buf);
}

static R_xlen_t prism_raw_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rbyte *buf) {
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_raw_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*RAWSXP*/ data;
    if (!is_materialized(x)) {
        SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
        SEXP                   source  = get_source(x);
        SEXP data = copy_data_at_indices(source, indices);
        set_materialized_data(x, data);
    } else {
        data = get_materialized_data(x);
    }
    assert(TYPEOF(data) == RAWSXP);
    return RAW_GET_REGION(data, i, n, buf);
}

static R_xlen_t prism_complex_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rcomplex *buf) {
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_complex_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*CPLXSXP*/ data;
    if (!is_materialized(x)) {
        SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
        SEXP                   source  = get_source(x);
        SEXP data = copy_data_at_indices(source, indices);
        set_materialized_data(x, data);
    } else {
        data = get_materialized_data(x);
    }
    assert(TYPEOF(data) == CPLXSXP);
    return COMPLEX_GET_REGION(data, i, n, buf);
}

static R_xlen_t prism_logical_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_logical_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*LGLSXP*/ data;
    if (!is_materialized(x)) {
        SEXP/*INTSXP|REALSXP*/ indices = get_indices(x);
        SEXP                   source  = get_source(x);
        SEXP data = copy_data_at_indices(source, indices);
        set_materialized_data(x, data);
    } else {
        data = get_materialized_data(x);
    }
    assert(TYPEOF(data) == LGLSXP);
    return LOGICAL_GET_REGION(data, i, n, buf);
}

static SEXP prism_extract_subset(SEXP x, SEXP indices, SEXP call) {
    assert(x != NULL);
    assert(TYPEOF(indices) == INTSXP || TYPEOF(indices) == REALSXP);

    if (__get_debug_mode()) {
        Rprintf("prism_extract_subset\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("        indices: %p\n", indices);
        Rprintf("           call: %p\n", call);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*INTSXP|REALSXP*/ prism_indices = get_indices(x);
    SEXP                   source        = get_source(x);

    // No indices.
    R_xlen_t size = XLENGTH(indices);
    if (size == 0) {
        return allocVector(TYPEOF(source), 0);
    }

    if (is_materialized(x)) {
        // TODO maybe instead just return a viewport into the materialized sexp?
        SEXP materialized_data = get_materialized_data(x);
        return copy_data_at_indices(materialized_data, indices);
    }

    if (!do_indices_contain_NAs(indices)) {
        Rf_error("Non-NA prisms are not implemented yet.\n"); // FIXME
    }

    // Non-NA indices.
    SEXP translated_indices = allocVector(REALSXP, size);

    switch (TYPEOF(indices)) {
        case INTSXP:
            for (R_xlen_t i = 0; i < size; i++) {
                R_xlen_t index = (R_xlen_t) INTEGER_ELT(indices, i);
                R_xlen_t translated_index = translate_index(prism_indices, index);
                SET_REAL_ELT(translated_indices, i, translated_index);
            }
            break;

        case REALSXP:
            for (R_xlen_t i = 0; i < size; i++) {
                R_xlen_t index = (R_xlen_t) REAL_ELT(indices, i);
                R_xlen_t translated_index = translate_index(prism_indices, index);
                SET_REAL_ELT(translated_indices, i, translated_index);
            }
            break;
    }

    return prism_new(source, indices);
}

// R_set_altstring_Set_elt_method
// static void string_set_elt(SEXP x, R_xlen_t i, SEXP v)

// R_set_altstring_Elt_method
// string_elt(SEXP x, R_xlen_t i)

// UFO Inits
void init_prism_altrep_class(DllInfo * dll) {
    //R_altrep_class_t prism_altrep;
    R_altrep_class_t cls = R_make_altinteger_class("prism_altrep", "viewport_altrep", dll);
    prism_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, prism_duplicate);
    R_set_altrep_Inspect_method(cls, prism_inspect);
    R_set_altrep_Length_method(cls, prism_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, prism_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, prism_dataptr_or_null);

    R_set_altinteger_Elt_method(cls, prism_integer_element);
    R_set_altlogical_Elt_method(cls, prism_logical_element);
    R_set_altreal_Elt_method   (cls, prism_numeric_element);
    R_set_altcomplex_Elt_method(cls, prism_complex_element);
    R_set_altraw_Elt_method    (cls, prism_raw_element);

    R_set_altinteger_Get_region_method(cls, prism_integer_get_region);
    R_set_altlogical_Get_region_method(cls, prism_logical_get_region);
    R_set_altreal_Get_region_method   (cls, prism_numeric_get_region);
    R_set_altcomplex_Get_region_method(cls, prism_complex_get_region);
    R_set_altraw_Get_region_method    (cls, prism_raw_get_region);

    R_set_altvec_Extract_subset_method(cls, prism_extract_subset);
}

//SEXP/*NILSXP*/ prism(SEXP, SEXP/*INTSXP|REALSXP*/ start, SEXP/*INTSXP|REALSXP*/ size);


SEXP create_prism(SEXP source, SEXP/*INTSXP|REALSXP*/ indices) {
    assert(TYPEOF(indices_sexp) == INTSXP || TYPEOF(indices_sexp) == REALSXP);

    if (__get_debug_mode()) {
        Rprintf("create prism\n");
        Rprintf("           SEXP: %p\n", source);
    }

    return prism_new(source, indices);
}