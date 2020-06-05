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
#include "bitmap_sexp.h"

#include "common.h"
#include "mosaics.h"

static R_altrep_class_t mosaic_altrep;

#define how_many_ints_in_R_xlen_t (sizeof(R_xlen_t) / sizeof(int))

R_xlen_t convert_logical_mask_to_bitmap(SEXP/*LGLSXP*/ mask, SEXP/*INTSXP*/ bitmap) {
    assert(TYPEOF(mask) == LGLSXP);

    R_xlen_t size = XLENGTH(mask);
    assert(XLENGTH(mask) == XTRUELENGTH(bitmap));

    R_xlen_t elements = 0;
    for (int i = 0; i < size; i++) {
        Rboolean current = LOGICAL_ELT(mask, i);
        if (current == NA_LOGICAL) {
            Rf_error("Mosaics cannot be created from a logical mask containing NA\n");
        }
        if (current == TRUE) {
            elements++;
            bitmap_set(bitmap, (R_xlen_t) current);
        }
    }

    return elements;
}

R_xlen_t convert_integer_indices_to_bitmap(SEXP/*INTSXP*/ indices, SEXP/*INTSXP*/ bitmap) {
    assert(TYPEOF(indices) == INTSXP);
    R_xlen_t size = XLENGTH(indices);

    int previous = NA_INTEGER;
    for (int i = 0; i < size; i++) {
        int current = INTEGER_ELT(indices, i);
        if (current == NA_INTEGER) {
            Rf_error("Mosaics cannot be created from an ordered index containing NA\n");
        }
        if (previous != NA_INTEGER && previous >= current) {
            Rf_error("Mosaics can only be created from an ordered index list, but %li >= %li\n", previous, current);
        }
        assert(current > 0);
        bitmap_set(bitmap, (R_xlen_t) current - 1);
        previous = current;
    }

    return size;
}

R_xlen_t convert_numeric_indices_to_bitmap(SEXP/*REALSXP*/ indices, SEXP/*INTSXP*/ bitmap){
    assert(TYPEOF(indices) == REALSXP);
    R_xlen_t size = XLENGTH(indices);

    double previous = NA_REAL;
    for (int i = 0; i < size; i++) {
        double current = REAL_ELT(indices, i);
        if (current == NA_REAL) {
            Rf_error("Mosaics cannot be created from an ordered index containing NA\n");
        }
        if (previous != NA_REAL && previous >= current) {
            Rf_error("Mosaics can only be created from an ordered index list, but %li >= %li\n", previous, current);
        }
        assert(current > 0);
        bitmap_set(bitmap, (R_xlen_t) current - 1);
        previous = current;
    }

    return size;
}


R_xlen_t convert_indices_to_bitmap(SEXP/*INTSXP | REALSXP | LGLSXP*/ indices, SEXP/*INTSXP*/ bitmap) {
    SEXPTYPE type = TYPEOF(indices);
    assert(type == INTSXP || type == REALSXP || type == LGLSXP);

    switch (type) {
        case INTSXP:  return convert_integer_indices_to_bitmap(indices, bitmap);
        case REALSXP: return convert_numeric_indices_to_bitmap(indices, bitmap);
        case LGLSXP:  return convert_logical_mask_to_bitmap   (indices, bitmap);
        default:      Rf_error("Mosaics can be indexed by logical, integer, or numeric vectors but found: %d\n", type);
    }
}

SEXP/*A*/ mosaic_new(SEXP/*A*/ source, SEXP/*INTSXP*/ bitmap, R_xlen_t size) {

    assert(TYPEOF(source) == INTSXP
           || TYPEOF(source) == REALSXP
           || TYPEOF(source) == CPLXSXP
           || TYPEOF(source) == LGLSXP
           || TYPEOF(source) == VECSXP
           || TYPEOF(source) == STRSXP);

    //assert(TYPEOF(indices) == INTSXP);

    if (get_debug_mode()) {
        Rprintf("mosaic_new\n");
        Rprintf("           SEXP: %p\n", source);
        Rprintf("         bitmap: %p\n",  bitmap);
        Rprintf("           size: %li\n", size);
    }

    SEXP/*REALSXP*/ length_vector = allocVector(REALSXP, 1);
    SET_REAL_ELT(length_vector, 0, size);

    SEXP/*LISTSXP*/ data = allocSExp(LISTSXP);
    SETCAR (data, source);        // The original vector
    SET_TAG(data, R_NilValue);    // Starts as R_NilValue, becomes a vector if it the mosaic is written to
    SETCDR (data, length_vector); // The number of set bits in the mask goes here, aka length

    return R_new_altrep(mosaic_altrep, bitmap, data);
}

//SEXP/*A*/ mosaic_new_from_bitmap(SEXP/*A*/ source, SEXP/*INTSXP*/ source_bitmap) {
//
//    assert(TYPEOF(source) == INTSXP
//           || TYPEOF(source) == REALSXP
//           || TYPEOF(source) == CPLXSXP
//           || TYPEOF(source) == LGLSXP
//           || TYPEOF(source) == VECSXP
//           || TYPEOF(source) == STRSXP);
//
//    assert(TYPEOF(source_bitmap) == INTSXP);
//
//    if (get_debug_mode()) {
//        Rprintf("mosaic_new_from_bitmap\n");
//        Rprintf("             SEXP: %p\n", source);
//        Rprintf("           bitmap: %p\n", source_bitmap);
//        Rprintf("      bitmap type: %li\n", TYPEOF(source_bitmap));
//        Rprintf("    bitmap length: %li\n", XLENGTH(source_bitmap));
//        Rprintf("bitmap truelength: %li\n", XTRUELENGTH(source_bitmap));
//    }
//
//    SEXP/*INTSXP*/ bitmap = bitmap_clone(source_bitmap);
//
//    R_xlen_t how_many_bits = bitmap_count_set_bits(bitmap);
//    SEXP/*REALSXP*/ length_vector = allocVector(REALSXP, 1);
//    SET_REAL_ELT(length_vector, 0, how_many_bits);
//
//    SEXP/*LISTSXP*/ data = allocSExp(LISTSXP);
//    SETCAR (data, source);     // The original vector
//    SET_TAG(data, R_NilValue); // Starts as R_NilValue, becomes a vector if it the mosaic is written to
//    SETCDR (data, length_vector); // The number of set bits in the mask goes here, aka length
//
//    return R_new_altrep(mosaic_altrep, bitmap, data);
//}

static inline SEXP/*INTSXP*/ get_bitmap(SEXP x) {
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

static inline SEXP get_length_vector(SEXP x) {
    SEXP/*LISTSXP*/ cell =  R_altrep_data2(x);
    return CDR(cell);
}

static inline R_xlen_t get_length(SEXP x) {
    SEXP/*LISTSXP*/ cell =  R_altrep_data2(x);
    return REAL_ELT(CDR(cell), 0);
}

SEXP mosaic_duplicate(SEXP x, Rboolean deep) {

    if (get_debug_mode()) {
        Rprintf("mosaic_duplicate\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("           deep: %i\n", deep);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*INTSXP*/ bitmap = get_bitmap(x);
    SEXP           source = get_source(x);
    R_xlen_t       length = get_length(x);

    if (deep) {
        SEXP mosaic = mosaic_new(source, bitmap, length);
        if (is_materialized(x)) {
            SEXP data = get_materialized_data(x);
            set_materialized_data(mosaic, duplicate(data));
        }
        return mosaic;
    } else {
        SEXP/*LISTSXP*/ meta = allocSExp(LISTSXP);
        SETCAR (meta, source);               // The original vector
        SEXP data = is_materialized(x) ? get_materialized_data(x) : R_NilValue;
        SET_TAG(meta, data);                 // Starts as R_NilValue, becomes a vector if it the mosaic is written to
        SETCDR (meta, get_length_vector(x)); // Length vector here
        return R_new_altrep(mosaic_altrep, bitmap, meta);
    }
}

static Rboolean mosaic_inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int)) {

    Rprintf("mosaic_altrep %s\n", type2char(TYPEOF(x)));

    inspect_subtree(R_altrep_data1(x), pre, deep, pvec);
    inspect_subtree(R_altrep_data2(x), pre, deep, pvec);

    return FALSE;
}

static R_xlen_t mosaic_length(SEXP x) {
    if (get_debug_mode()) {
        Rprintf("mosaic_length\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("         length: %p\n", get_length(x));
    }
    return get_length(x);
}

SEXP copy_from_source(SEXP x) {
    SEXP/*INTSXP*/ bitmap = get_bitmap(x);
    SEXP           source = get_source(x);
    R_xlen_t       length = get_length(x);

    assert(XTRUELENGTH(bitmap) == XLENGTH(source));

    SEXP materialized = allocVector(TYPEOF(source), length);
    R_xlen_t cursor = 0;
    for (R_xlen_t index = 0; index < XLENGTH(source); index++) {
        if (bitmap_get(bitmap, index)) {
            copy_element(source, index, materialized, cursor);
            cursor++;
        }
    }
    assert(cursor == length);
    return materialized;
}

static void *mosaic_dataptr(SEXP x, Rboolean writeable) {
    assert(x != NULL);

    if (get_debug_mode()) {
        Rprintf("mosaic_dataptr\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("      writeable: %i\n", writeable);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP data = get_materialized_data(x);
        return writeable ? DATAPTR(data) : (void *) DATAPTR_RO(data);
    }

    SEXP data = copy_from_source(x);
    set_materialized_data(x, data);
    return writeable ? DATAPTR(data) : (void *) DATAPTR_RO(data);
}

static const void *mosaic_dataptr_or_null(SEXP x) {
    assert(x != NULL);

    if (get_debug_mode()) {
        Rprintf("mosaic_dataptr_or_null\n");
        Rprintf("           SEXP: %p\n", x);
    }

    if (is_materialized(x)) {
        SEXP data = get_materialized_data(x);
        return DATAPTR_RO(data);
    }

    SEXP data = copy_from_source(x);
    set_materialized_data(x, data);
    return DATAPTR_RO(data);
}

//R_xlen_t project_index(SEXP/*INTSXP*/ bitmap, R_xlen_t index) {
//    assert(bitmap != R_NilValue);
//
//    R_xlen_t projected_index = bitmap_index_of_nth_set_bit(bitmap, index);
//
//    if (get_debug_mode()) {
//        Rprintf("    projecting index\n");
//        Rprintf("         bitmap SEXP: %p\n",  bitmap);
//        Rprintf("         input index: %li\n", index);
//        Rprintf("     projected index: %li\n", projected_index);
//    }
//
//    return projected_index;
//}

static int mosaic_integer_element(SEXP x, R_xlen_t i) {  // CONTINUE HERE
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("mosaic_integer_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*INTSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == INTSXP);
        return INTEGER_ELT(data, i);
    }

    SEXP/*INTSXP*/ bitmap = get_bitmap(x);
    SEXP/*INTSXP*/ source = get_source(x);

    assert(TYPEOF(source) == INTSXP);
    //R_xlen_t projected_index = project_index(bitmap, i);
    R_xlen_t projected_index = bitmap_index_of_nth_set_bit(bitmap, i);
    return INTEGER_ELT(source, projected_index);
}

static double mosaic_numeric_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("mosaic_numeric_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*REALSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == REALSXP);
        return REAL_ELT(data, i);
    }

    SEXP/*INTSXP*/  bitmap = get_bitmap(x);
    SEXP/*REALSXP*/ source = get_source(x);

    assert(TYPEOF(source) == REALSXP);
    R_xlen_t projected_index = bitmap_index_of_nth_set_bit(bitmap, i);
    //R_xlen_t projected_index = project_index(bitmap, i);
    return REAL_ELT(source, projected_index);
}

static Rbyte mosaic_raw_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("mosaic_raw_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*RAWSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == RAWSXP);
        return RAW_ELT(data, i);
    }

    SEXP/*INTSXP*/ bitmap = get_bitmap(x);
    SEXP/*RAWSXP*/ source = get_source(x);

    assert(TYPEOF(source) == RAWSXP);
    // R_xlen_t projected_index = project_index(bitmap, i);
    R_xlen_t projected_index = bitmap_index_of_nth_set_bit(bitmap, i);
    return RAW_ELT(source, projected_index);
}

static Rcomplex mosaic_complex_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("mosaic_complex_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*CPLXSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == CPLXSXP);
        return COMPLEX_ELT(data, i);
    }

    SEXP/*INTSXP*/  bitmap = get_bitmap(x);
    SEXP/*CPLXSXP*/ source = get_source(x);

    assert(TYPEOF(source) == CPLXSXP);
    //R_xlen_t projected_index = project_index(bitmap, i);
    R_xlen_t projected_index = bitmap_index_of_nth_set_bit(bitmap, i);
    return COMPLEX_ELT(source, projected_index);
}

static int mosaic_logical_element(SEXP x, R_xlen_t i) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("mosaic_logical_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    if (is_materialized(x)) {
        SEXP/*LGLSXP*/ data = get_materialized_data(x);
        assert(TYPEOF(data) == LGLSXP);
        return LOGICAL_ELT(data, i);
    }

    SEXP/*INTSXP*/ bitmap = get_bitmap(x);
    SEXP/*LGLSXP*/ source = get_source(x);

    assert(TYPEOF(source) == LGLSXP);
    //R_xlen_t projected_index = project_index(bitmap, i);
    R_xlen_t projected_index = bitmap_index_of_nth_set_bit(bitmap, i);
    return LOGICAL_ELT(source, projected_index);
}

static R_xlen_t mosaic_integer_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("mosaic_integer_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*INTSXP*/ data;
    if (!is_materialized(x)) {
        data = copy_from_source(x);
        set_materialized_data(x, data);
    } else {
        data = get_materialized_data(x);
    }
    assert(TYPEOF(data) == INTSXP);
    return INTEGER_GET_REGION(data, i, n, buf);
}

static R_xlen_t mosaic_numeric_get_region(SEXP x, R_xlen_t i, R_xlen_t n, double *buf) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("mosaic_numeric_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*REALSXP*/ data;
    if (!is_materialized(x)) {
        data = copy_from_source(x);
        set_materialized_data(x, data);
    } else {
        data = get_materialized_data(x);
    }
    assert(TYPEOF(data) == REALSXP);
    return REAL_GET_REGION(data, i, n, buf);
}

static R_xlen_t mosaic_raw_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rbyte *buf) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("mosaic_raw_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*RAWSXP*/ data;
    if (!is_materialized(x)) {
        data = copy_from_source(x);
        set_materialized_data(x, data);
    } else {
        data = get_materialized_data(x);
    }
    assert(TYPEOF(data) == RAWSXP);
    return RAW_GET_REGION(data, i, n, buf);
}

static R_xlen_t mosaic_complex_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rcomplex *buf) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("mosaic_complex_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*CPLXSXP*/ data;
    if (!is_materialized(x)) {
        data = copy_from_source(x);
        set_materialized_data(x, data);
    } else {
        data = get_materialized_data(x);
    }
    assert(TYPEOF(data) == CPLXSXP);
    return COMPLEX_GET_REGION(data, i, n, buf);
}

static R_xlen_t mosaic_logical_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    assert(x != R_NilValue);

    if (get_debug_mode()) {
        Rprintf("mosaic_logical_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("           size: %li\n", n);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*LGLSXP*/ data;
    if (!is_materialized(x)) {
        data = copy_from_source(x);
        set_materialized_data(x, data);
    } else {
        data = get_materialized_data(x);
    }
    assert(TYPEOF(data) == LGLSXP);
    return LOGICAL_GET_REGION(data, i, n, buf);
}



static SEXP mosaic_extract_subset(SEXP x, SEXP indices, SEXP call) {
    assert(x != NULL);
    assert(TYPEOF(indices) == INTSXP || TYPEOF(indices) == REALSXP);

    if (get_debug_mode()) {
        Rprintf("mosaic_extract_subset\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("        indices: %p\n", indices);
        Rprintf("           call: %p\n", call);
        Rprintf("is_materialized: %i\n", is_materialized(x));
    }

    SEXP/*INTSXP*/ bitmap = get_bitmap(x);
    SEXP           source = get_source(x);

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

    if (!are_indices_monotonic(indices)) {
        return copy_data_at_indices(source, indices);
    }

    // Monotonic indices.
    R_xlen_t translated_bitmap_size = get_length(x);
    SEXP translated_bitmap = bitmap_new(translated_bitmap_size);
    R_xlen_t viewport_index = 0;
    R_xlen_t indices_index = 0;
    for (R_xlen_t i = 0; i < translated_bitmap_size; i++) {
        R_xlen_t index = TYPEOF(indices) == INTSXP ? (R_xlen_t) INTEGER_ELT(indices, indices_index)
                                                   : (R_xlen_t) REAL_ELT(indices, indices_index);
        if (bitmap_get(bitmap, i)) {
            if (viewport_index == index) {
                bitmap_set(translated_bitmap, i);
                indices_index++;
            }
            viewport_index++;
        }
    }

    return mosaic_new(source, translated_bitmap, translated_bitmap_size);
}

// R_set_altstring_Set_elt_method
// static void string_set_elt(SEXP x, R_xlen_t i, SEXP v)

// R_set_altstring_Elt_method
// string_elt(SEXP x, R_xlen_t i)

// UFO Inits
void init_mosaic_altrep_class(DllInfo * dll) {
    //R_altrep_class_t mosaic_altrep;
    R_altrep_class_t cls = R_make_altinteger_class("mosaic_altrep", "viewport_altrep", dll);
    mosaic_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, mosaic_duplicate);
    R_set_altrep_Inspect_method(cls, mosaic_inspect);
    R_set_altrep_Length_method(cls, mosaic_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, mosaic_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, mosaic_dataptr_or_null);

    R_set_altinteger_Elt_method(cls, mosaic_integer_element);
    R_set_altlogical_Elt_method(cls, mosaic_logical_element);
    R_set_altreal_Elt_method   (cls, mosaic_numeric_element);
    R_set_altcomplex_Elt_method(cls, mosaic_complex_element);
    R_set_altraw_Elt_method    (cls, mosaic_raw_element);

    R_set_altinteger_Get_region_method(cls, mosaic_integer_get_region);
    R_set_altlogical_Get_region_method(cls, mosaic_logical_get_region);
    R_set_altreal_Get_region_method   (cls, mosaic_numeric_get_region);
    R_set_altcomplex_Get_region_method(cls, mosaic_complex_get_region);
    R_set_altraw_Get_region_method    (cls, mosaic_raw_get_region);

    R_set_altvec_Extract_subset_method(cls, mosaic_extract_subset);
}


SEXP/*A*/ create_mosaic(SEXP/*A*/ source, SEXP/*INTSXP|REALSXP|LGLSXP*/ indices) {

    assert(TYPEOF(source) == INTSXP
           || TYPEOF(source) == REALSXP
           || TYPEOF(source) == CPLXSXP
           || TYPEOF(source) == LGLSXP
           || TYPEOF(source) == VECSXP
           || TYPEOF(source) == STRSXP);

    assert(TYPEOF(indices) == INTSXP
           || TYPEOF(indices) == REALSXP
           || (TYPEOF(indices) == LGLSXP && (XTRUELENGTH(indices) == XLENGTH(indices))));

    if (get_debug_mode()) {
        Rprintf("mosaic_new\n");
        Rprintf("           SEXP: %p\n", source);
        Rprintf("        indices: %p\n", indices);
        Rprintf("   indices type: %li\n", TYPEOF(indices));
        Rprintf(" indices length: %li\n", XLENGTH(indices));
    }

    R_xlen_t how_many_indices = XLENGTH(indices);
    SEXP/*INTSXP*/ bitmap = bitmap_new(how_many_indices);
    R_xlen_t how_many_set_bits = convert_indices_to_bitmap(indices, bitmap);

    //SEXP/*REALSXP*/ length_vector = allocVector(REALSXP, 1);
    //SET_REAL_ELT(length_vector, 0, how_many_set_bits);

    //SEXP/*LISTSXP*/ data = allocSExp(LISTSXP);
    //SETCAR (data, source);        // The original vector
    //SET_TAG(data, R_NilValue);    // Starts as R_NilValue, becomes a vector if it the mosaic is written to
    //SETCDR (data, length_vector); // The number of set bits in the mask goes here, aka length

    return mosaic_new(source, bitmap, how_many_set_bits);
}