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

static R_altrep_class_t prism_altrep;

SEXP prism_new(SEXP source, SEXP/*REALSXP*/ indices) {
    assert(sizeof(double) >= sizeof(R_xlen_t));
    assert(source != null);

    if (__get_debug_mode()) {
        Rprintf("prism_new\n");
        Rprintf("           SEXP: %p\n", source);
        for (R_xlen_t i = 0; i < indices_length; i++) {
            Rprintf("           [%l2i]: %li\n", i, (R_xlen_t) REAL_ELT(indices, i));
        }
    }

    SEXP/*LISTSXP*/ data = allocSExp(LISTSXP);
    SETCAR (data, source);     // The original vector
    SET_TAG(data, R_NilValue); // Starts as R_NilValue, becomes a vector if it the prism is written to
    SETCDR (data, R_NilValue); // Nothing here

    return R_new_altrep(prism_altrep, indices, data);
}

static inline SEXP/*REALSXP*/ get_indices(SEXP x) {
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

    SEXP/*INTSXP*/ window = get_window(x);
    SEXP           source = get_source(x);

    if (deep) {
        R_xlen_t start = 0;
        R_xlen_t size  = 0;
        read_start_and_size(window, &start, &size);
        SEXP prism = prism_new(source, start, size);
        if (is_materialized(x)) {
            SEXP data = get_materialized_data(x);
            set_materialized_data(prism, duplicate(data));
        }
        return prism;
    } else {
        SEXP/*LISTSXP*/ meta = allocSExp(LISTSXP);
        SETCAR (meta, source);     // The original vector
        SEXP data = is_materialized(x) ? get_materialized_data(x) : R_NilValue;
        SET_TAG(meta, data);       // Starts as R_NilValue, becomes a vector if it the prism is written to
        SETCDR (meta, R_NilValue); // Nothing here
        return R_new_altrep(prism_altrep, window, meta);
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

const void *extract_read_only_data_pointer(SEXP x) {//TODO
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
        default: Rf_error("Illegal source type for a prism: %i.\n", type);
    }
}

SEXP copy_from_source(SEXP x) {//TODO
    SEXP/*INTSXP*/ window = get_window(x);
    SEXP           source = get_source(x);

    R_xlen_t start = 0;
    R_xlen_t size  = 0;
    read_start_and_size(window, &start, &size);

    const void *data = DATAPTR_RO(source);
    SEXP materialized = allocVector(TYPEOF(source), size);

    SEXPTYPE type = TYPEOF(source);
    switch (type) {
        case INTSXP:  {
            const int *ints = (const int *) data;
            for (R_xlen_t index = start; index < size; index++) {
                SET_INTEGER_ELT(materialized, index, ints[index]);
            }
            break;
        }
        case REALSXP: {
            const double *doubles = (const double *) data;
            for (R_xlen_t index = start; index < size; index++) {
                SET_REAL_ELT(materialized, index, doubles[index]);
            }
            break;
        }
        case LGLSXP: {
            const Rboolean *booleans = (const Rboolean *) data;
            for (R_xlen_t index = start; index < size; index++) {
                SET_LOGICAL_ELT(materialized, index, booleans[index]);
            }
            break;
        }
        case RAWSXP: {
            const Rbyte *bytes = (const Rbyte *) data;
            for (R_xlen_t index = start; index < size; index++) {
                SET_RAW_ELT(materialized, index, bytes[index]);
            }
            break;
        }
        case CPLXSXP: {
            const Rcomplex *trouble = (const Rcomplex *) data;
            for (R_xlen_t index = start; index < size; index++) {
                SET_COMPLEX_ELT(materialized, index, trouble[index]);
            }
            break;
        }
        case STRSXP:
        case VECSXP: {
            const SEXP *pointers = (const SEXP *) data;
            for (R_xlen_t index = start; index < size; index++) {
                SET_VECTOR_ELT(materialized, index, pointers[index]);
            }
            break;
        }
        default: Rf_error("Illegal source type for a prism: %i.\n", type);
    }

    return materialized;
}

static void *prism_dataptr(SEXP x, Rboolean writeable) {//TODO
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

    if (writeable) {
        SEXP data = copy_from_source(x);
        set_materialized_data(x, data);
        return DATAPTR(data);
    }

    return (void *) extract_read_only_data_pointer(x);
}

static const void *prism_dataptr_or_null(SEXP x) {//TODO
    assert(x != NULL);

    if (__get_debug_mode()) {
        Rprintf("prism_dataptr_or_null\n");
        Rprintf("           SEXP: %p\n", x);
    }

    return extract_read_only_data_pointer(x);
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

    SEXP/*REALSXP*/ indices = get_indices(x);
    SEXP/*INTSXP*/  source  = get_source(x);

    assert(TYPEOF(source) == INTSXP);

    R_xlen_t projected_index = (R_xlen_t) REAL_ELT(indices, i);

    if (__get_debug_mode()) {
        Rprintf("projected_index: %li\n", projected_index);
    }

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

    SEXP/*REALSXP*/ indices = get_indices(x);
    SEXP/*REALSXP*/ source  = get_source(x);

    assert(TYPEOF(source) == REALSXP);

    R_xlen_t projected_index = (R_xlen_t) REAL_ELT(indices, i);

    if (__get_debug_mode()) {
        Rprintf("projected_index: %li\n", projected_index);
    }

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

    SEXP/*REALSXP*/ indices = get_indices(x);
    SEXP/*RAWSXP*/  source  = get_source(x);

    assert(TYPEOF(source) == RAWSXP);

    R_xlen_t projected_index = (R_xlen_t) REAL_ELT(indices, i);

    if (__get_debug_mode()) {
        Rprintf("projected_index: %li\n", projected_index);
    }

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

    SEXP/*REALSXP*/ indices = get_indices(x);
    SEXP/*CPLXSXP*/ source  = get_source(x);

    assert(TYPEOF(source) == CPLXSXP);

    R_xlen_t projected_index = (R_xlen_t) REAL_ELT(indices, i);

    if (__get_debug_mode()) {
        Rprintf("projected_index: %li\n", projected_index);
    }

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

    SEXP/*REALSXP*/ indices = get_indices(x);
    SEXP/*LGLSXP*/  source  = get_source(x);

    assert(TYPEOF(source) == LGLSXP);

    R_xlen_t projected_index = (R_xlen_t) REAL_ELT(indices, i);

    if (__get_debug_mode()) {
        Rprintf("projected_index: %li\n", projected_index);
    }

    return LOGICAL_ELT(source, projected_index);
}

static R_xlen_t prism_integer_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) { //TODO
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_integer_get_region\n");
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

static R_xlen_t prism_numeric_get_region(SEXP x, R_xlen_t i, R_xlen_t n, double *buf) {//TODO
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_numeric_get_region\n");
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

static R_xlen_t prism_raw_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rbyte *buf) {//TODO
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_raw_get_region\n");
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

static R_xlen_t prism_complex_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rcomplex *buf) {//TODO
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_complex_get_region\n");
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

static R_xlen_t prism_logical_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {//TODO
    assert(x != R_NilValue);

    if (__get_debug_mode()) {
        Rprintf("prism_logical_get_region\n");
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

static inline void copy_element(SEXP source, R_xlen_t source_index, SEXP target, R_xlen_t target_index) {
    assert(TYPEOF(source) == TYPEOF(target));
    assert(TYPEOF(source) == INTSXP
        || TYPEOF(source) == REALSXP
        || TYPEOF(source) == CPLXSXP
        || TYPEOF(source) == LGLSXP
        || TYPEOF(source) == VECSXP
        || TYPEOF(source) == STRSXP);

    switch(TYPEOF(source)) {
        case INTSXP:  SET_INTEGER_ELT (target, target_index, INTEGER_ELT (source, source_index)); break;
        case REALSXP: SET_REAL_ELT    (target, target_index, REAL_ELT    (source, source_index)); break;
        case LGLSXP:  SET_LOGICAL_ELT (target, target_index, LOGICAL_ELT (source, source_index)); break;
        case CPLXSXP: SET_COMPLEX_ELT (target, target_index, COMPLEX_ELT (source, source_index)); break;
        case RAWSXP:  SET_RAW_ELT     (target, target_index, RAW_ELT     (source, source_index)); break;
        case STRSXP:  SET_VECTOR_ELT  (target, target_index, VECTOR_ELT  (source, source_index)); break;
        case VECSXP:  SET_VECTOR_ELT  (target, target_index, VECTOR_ELT  (source, source_index)); break;
        default:      Rf_error("Unsupported vector type: %d\n", TYPEOF(source));
    }
}

SEXP copy_data_at_integer_indices(SEXP source, SEXP/*INTSXP*/ indices) {//TODO
    assert(TYPEOF(indices) == INTSXP);

    R_xlen_t size = XLENGTH(indices);
    SEXP target = allocVector(TYPEOF(source), size);

    for (R_xlen_t i = 0; i < size; i++) {
        int index = INTEGER_ELT(indices, i);
        copy_element(source, (R_xlen_t) index, target, i);
    }

    return target;
}

SEXP copy_data_at_numeric_indices(SEXP source, SEXP/*REALSXP*/ indices) {//TODO
    R_xlen_t size = XLENGTH(indices);
    SEXP target = allocVector(TYPEOF(source), size);

    for (R_xlen_t i = 0; i < size; i++) {
        double index = REAL_ELT(indices, i);
        copy_element(source, (R_xlen_t) index, target, i);
    }

    return target;
}

SEXP copy_data_at_indices(SEXP source, SEXP/*INTSXP | REALSXP*/ indices) {//TODO
    SEXPTYPE type = TYPEOF(indices);
    assert(type == INTSXP || type == REALSXP);

    switch (type) {
        case INTSXP:  return copy_data_at_integer_indices(source, indices);
        case REALSXP: return copy_data_at_numeric_indices(source, indices);
        default:      Rf_error("Prisms can be indexed by integer or numeric vectors but found: %d\n", type);
    }
}

static SEXP prism_extract_subset(SEXP x, SEXP indices, SEXP call) {//TODO
    assert(x != NULL);

    if (__get_debug_mode()) {
        Rprintf("ufo_extract_subset\n");
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
        Rf_error("Non-contiguous prisms are not implemented yet.\n"); // FIXME
    }

    // Contiguous indices.
    R_xlen_t start = get_first_element_as_length(indices) - 1;
    R_xlen_t projected_start = project_index(window, start);
    return prism_new(source, projected_start, size);
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

    R_xlen_t *indices = indices_vector_to_array(indices_sexp);
    if (TYPEOF(indices) == REALSXP) {
        return prism_new(source, indices));
    } else {
        SEXP real_indices = allocVector(REALSXP, XLENGTH(indices));
        for (R_xlen_t i = 0; i < XLENGTH(indices); i++) {
            SET_REAL_ELT(real_indices, i, (R_xlen_t) INTEGER_ELT(indices, i));
        }
        return prism_new(source, real_indices);
    }
}