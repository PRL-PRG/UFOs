#include <stdio.h>
#include <stdlib.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Altrep.h>
#include <R_ext/Rallocators.h>

#include "metadata.h"
#include "debug.h"
#include "helpers.h"
#include "file_manipulation.h"
#include "ring_buffer.h"

#include "altrep_cached_ufo_vectors.h"
#include "R_ext.h"

static R_altrep_class_t ufo_cached_integer_altrep;
static R_altrep_class_t ufo_cached_numeric_altrep;
static R_altrep_class_t ufo_cached_raw_altrep;
static R_altrep_class_t ufo_cached_complex_altrep;
static R_altrep_class_t ufo_cached_logical_altrep;

R_altrep_class_t __get_cached_class_from_type(SEXPTYPE type) {
    switch (type) {
        case LGLSXP:
            return ufo_cached_logical_altrep;
        case INTSXP:
            return ufo_cached_integer_altrep;
        case REALSXP:
            return ufo_cached_numeric_altrep;
        case CPLXSXP:
            return ufo_cached_complex_altrep;
        case RAWSXP:
            return ufo_cached_raw_altrep;
        default:
            Rf_error("Unrecognized vector type: %d\n", type);
    }
}

void ufo_vector_finalize_altrep(SEXP wrapper) {
    altrep_ufo_config_t *cfg = (altrep_ufo_config_t *) EXTPTR_PTR(wrapper);
    fclose(cfg->file_handle);
}

SEXP ufo_cached_vector_get_contents(SEXP sexp) {
    return CAR(R_altrep_data2(sexp));
}
void ufo_cached_vector_set_contents(SEXP sexp, SEXP data) {
    CAR(R_altrep_data2(sexp)) = data;
}
SEXP ufo_cached_vector_get_ring_buffer(SEXP sexp) {
    return TAG(R_altrep_data2(sexp));
}

// UFO constructors
SEXP ufo_cached_vector_new_altrep(SEXPTYPE type, char const *path,
                                  R_xlen_t cache_size /*in segments*/,
                                  R_xlen_t cache_vector_size) {

    altrep_ufo_config_t *cfg =
            (altrep_ufo_config_t *) malloc(sizeof(altrep_ufo_config_t));

    cfg->type = type;
    cfg->path = path;
    cfg->element_size = __get_element_size(type);
    cfg->vector_size = __get_vector_length_from_file_or_die(cfg->path,
                                                            cfg->element_size);
    cfg->file_handle = __open_file_or_die(cfg->path);
    cfg->file_cursor = 0;
    cfg->cache_buffer_size = cache_size;
    cfg->cache_chunk_size = cache_vector_size;

    SEXP wrapper = PROTECT(allocSExp(EXTPTRSXP));
    EXTPTR_PTR(wrapper) = (void *) cfg;
    EXTPTR_TAG(wrapper) = Rf_install("ALTREP w/cache UFO CFG");

    SEXP data = PROTECT(allocSExp(LISTSXP));
    CAR(data) = R_NilValue;                                                     // This is where the vector will materialize.
    TAG(data) = PROTECT(ring_buffer_new(cache_size));                           // This is where the vector will be cached as it is read.

    SEXP ans = R_new_altrep(__get_cached_class_from_type(type), wrapper, data);
    EXTPTR_PROT(wrapper) = ans;

    /* Finalizer */
    R_MakeWeakRefC(wrapper, R_NilValue, ufo_vector_finalize_altrep, TRUE);

    UNPROTECT(2);

    return ans;
}
SEXP/*INTSXP*/ altrep_ufo_cached_vectors_intsxp_bin(SEXP/*STRSXP*/ path,
                                                    SEXP/*INTSXP*/ cache_size,
                                                    SEXP/*INTSXP*/ cache_chunk_size) {

    return ufo_cached_vector_new_altrep(INTSXP,
                                        __extract_path_or_die(path),
                                        __extract_int_or_die(cache_size),
                                        __extract_int_or_die(cache_chunk_size));
}
SEXP/*REALSXP*/ altrep_ufo_cached_vectors_realsxp_bin(SEXP/*STRSXP*/ path,
                                                      SEXP/*INTSXP*/ cache_size,
                                                      SEXP/*INTSXP*/ cache_chunk_size) {

    return ufo_cached_vector_new_altrep(REALSXP,
                                        __extract_path_or_die(path),
                                        __extract_int_or_die(cache_size),
                                        __extract_int_or_die(cache_chunk_size));
}
SEXP/*LGLSXP*/ altrep_ufo_cached_vectors_lglsxp_bin(SEXP/*STRSXP*/ path,
                                                    SEXP/*INTSXP*/ cache_size,
                                                    SEXP/*INTSXP*/ cache_chunk_size) {

    return ufo_cached_vector_new_altrep(LGLSXP,
                                        __extract_path_or_die(path),
                                        __extract_int_or_die(cache_size),
                                        __extract_int_or_die(cache_chunk_size));
}
SEXP/*CPLXSXP*/ altrep_ufo_cached_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path,
                                                      SEXP/*INTSXP*/ cache_size,
                                                      SEXP/*INTSXP*/ cache_chunk_size) {

    return ufo_cached_vector_new_altrep(CPLXSXP,
                                        __extract_path_or_die(path),
                                        __extract_int_or_die(cache_size),
                                        __extract_int_or_die(cache_chunk_size));
}
SEXP/*RAWSXP*/ altrep_ufo_cached_vectors_rawsxp_bin(SEXP/*STRSXP*/ path,
                                                    SEXP/*INTSXP*/ cache_size,
                                                    SEXP/*INTSXP*/ cache_chunk_size) {

    return ufo_cached_vector_new_altrep(RAWSXP,
                                        __extract_path_or_die(path),
                                        __extract_int_or_die(cache_size),
                                        __extract_int_or_die(cache_chunk_size));
}

typedef struct {
    SEXPTYPE                type;
    size_t /*aka R_len_t*/  size;
    char const             *path;
    //size_t               *dimensions;
} altrep_ufo_source_t;

SEXP ufo_cached_vector_new_altrep_wrapper(SEXPTYPE mode, R_xlen_t n, void *data,
                                          R_xlen_t cache_size /*in segments*/,
                                          R_xlen_t cache_vector_size) {
    return ufo_cached_vector_new_altrep(mode, (const char *) data,
                                        cache_size,
                                        cache_vector_size);
}

static SEXP ufo_cached_vector_duplicate(SEXP x, Rboolean deep) {

    altrep_ufo_config_t *old_cfg =
            (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));

    SEXP ans = PROTECT(ufo_cached_vector_new_altrep(old_cfg->type,
                                                    old_cfg->path,
                                                    old_cfg->cache_buffer_size,
                                                    old_cfg->cache_buffer_size));

    ring_buffer_copy(ufo_cached_vector_get_ring_buffer(x),
                     ufo_cached_vector_get_ring_buffer(ans));

    if (ufo_cached_vector_get_contents(x) != R_NilValue) {

        SEXP contents = ufo_cached_vector_get_contents(x);
        R_xlen_t length = XLENGTH(contents);
        SEXP payload = CAR(R_altrep_data2(ans));
        payload = PROTECT(allocVector(TYPEOF(x), length));

        switch (TYPEOF(x)) {
            case INTSXP:
                for (R_xlen_t i = 0; i < length; i++)
                    INTEGER(payload)[i] = INTEGER(contents)[i];
                break;
            case REALSXP:
                for (R_xlen_t i = 0; i < length; i++)
                    REAL(payload)[i] = REAL(contents)[i];
                break;
            case RAWSXP:
                for (R_xlen_t i = 0; i < length; i++)
                    RAW(payload)[i] = RAW(contents)[i];
                break;
            case CPLXSXP:
                for (R_xlen_t i = 0; i < length; i++)
                    COMPLEX(payload)[i] = COMPLEX(contents)[i];
                break;
            case LGLSXP:
                for (R_xlen_t i = 0; i < length; i++)
                    LOGICAL(payload)[i] = LOGICAL(contents)[i];
                break;
            default :
                Rf_error("Cannot duplicate vector: unknown vector type");
        }

        UNPROTECT(1);
    }

    UNPROTECT(1);
    return ans;
}

static Rboolean ufo_cached_vector_inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int)) {

    if (R_altrep_data1(x) == R_NilValue) {
        Rprintf(" ufo_cached_integer_altrep %s\n", type2char(TYPEOF(x)));
    } else {
        Rprintf(" ufo_cached_integer_altrep %s (materialized)\n", type2char(TYPEOF(x)));
    }

    if (R_altrep_data1(x) != R_NilValue) {
        inspect_subtree(R_altrep_data1(x), pre, deep, pvec);
    }

    if (R_altrep_data2(x) != R_NilValue) {
        inspect_subtree(R_altrep_data2(x), pre, deep, pvec);
    }

    return FALSE;
}


static R_xlen_t ufo_cached_vector_length(SEXP x) {
    if (__get_debug_mode()) {
        Rprintf("ufo_cached_vector_Length\n");
        Rprintf("           SEXP: %p\n", x);
    }
    if (R_altrep_data2(x) == R_NilValue) {
        altrep_ufo_config_t *cfg =
                (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));
        return cfg->vector_size;
    } else {
        return XLENGTH(R_altrep_data2(x));
    }
}

static void __materialize_data(SEXP x) {
    altrep_ufo_config_t *cfg =
            (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));
    PROTECT(x);
    SEXP payload = PROTECT(allocVector(INTSXP, cfg->vector_size));
    int *data = INTEGER(payload);
    __load_from_file(__get_debug_mode(), 0, cfg->vector_size, cfg, (char *) data);
    ufo_cached_vector_set_contents(x, payload);
    ring_buffer_clear(ufo_cached_vector_get_ring_buffer(x));
    UNPROTECT(2);
    // TODO materialize smarter: use existing buffer.
}

static void *ufo_cached_vector_dataptr(SEXP x, Rboolean writeable) {
    if (__get_debug_mode()) {
        Rprintf("ufo_cached_vector_Dataptr\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("      writeable: %i\n", writeable);
    }

    if (ufo_cached_vector_get_contents(x) == R_NilValue) {
        __materialize_data(x);
    }

    if (writeable) {
        return DATAPTR(ufo_cached_vector_get_contents(x));
    } else {
        return DATAPTR_RO(ufo_cached_vector_get_contents(x));
    }
}

static const void *ufo_cached_vector_dataptr_or_null(SEXP x) {
    if (__get_debug_mode()) {
        Rprintf("ufo_cached_vector_Dataptr_or_null\n");
        Rprintf("           SEXP: %p\n", x);
    }

    if (ufo_cached_vector_get_contents(x) == R_NilValue) {
        __materialize_data(x);
    }

    return DATAPTR_OR_NULL(ufo_cached_vector_get_contents(x));
}

static void ufo_cached_vector_element(SEXP x, R_xlen_t i, void *target) {
    if (__get_debug_mode()) {
        Rprintf("ufo_cached_vector_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("         target: %p\n", target);
    }

    altrep_ufo_config_t *cfg =
            (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));

    SEXP ring = ufo_cached_vector_get_ring_buffer(x)   

    if(ring_buffer_get_element_at(ring, i, target)) {
        // calculate which range to load
        int range_start = ((i / cfg->cache_chunk_size) + 0) * cache_chunk_size;
        int range_end = ((i / cfg->cache_chunk_size) + 1) * cache_chunk_size;    //exclusive
        int range_length = range_end - range_start;

        // create ring buffer cache vector to hold the data
        SEXP cache = ring_buffer_make_cache_vector(ring, TYPEOF(x), range_start, range_length);

        // load range into buffer
        char *buffer = (char *) DATAPTR(cache);
        __load_from_file(__get_debug_mode(), range_start, range_end, cfg, buffer);
        if (ring_buffer_get_element_at(ring, i, target)) {
            Rf_error("Cannot load element from file into buffer.");
        }
    }
}

static int ufo_cached_integer_element(SEXP x, R_xlen_t i) {
    if (ufo_cached_vector_get_contents(x) != R_NilValue)
        return INTEGER_ELT(ufo_cached_vector_get_contents(x), i);
    int ans = 0x5c5c5c5c;
    ufo_vector_cached_element(x, i, &ans);
    return ans;
}

static double ufo_cached_numeric_element(SEXP x, R_xlen_t i) {
    if (ufo_cached_vector_get_contents(x) != R_NilValue)
        return INTEGER_ELT(ufo_cached_vector_get_contents(x), i);
    double val = 0x5c5c5c5c;
    ufo_vector_element(x, i, &val);
    return val;
}

static Rbyte ufo_cached_raw_element(SEXP x, R_xlen_t i) {
    if (ufo_cached_vector_get_contents(x) != R_NilValue)
        return INTEGER_ELT(ufo_cached_vector_get_contents(x), i);
    Rbyte val = 0;
    ufo_vector_element(x, i, &val);
    return val;
}

static Rcomplex ufo_cached_complex_element(SEXP x, R_xlen_t i) {
    if (ufo_cached_vector_get_contents(x) != R_NilValue)
        return INTEGER_ELT(ufo_cached_vector_get_contents(x), i);
    Rcomplex val = {0x5c5c5c5c, 0x5c5c5c5c};
    ufo_vector_element(x, i, &val);
    return val;
}

static int ufo_cached_logical_element(SEXP x, R_xlen_t i) {
    if (ufo_cached_vector_get_contents(x) != R_NilValue)
        return INTEGER_ELT(ufo_cached_vector_get_contents(x), i);
    Rboolean val = FALSE;
    ufo_vector_element(x, i, &val);
    return (int) val;
}

static R_xlen_t ufo_cached_vector_get_region(SEXP x, R_xlen_t i, R_xlen_t n, void *buf) {
    // FIXME
}

static R_xlen_t ufo_cached_integer_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    // FIXME
}

static R_xlen_t ufo_cached_numeric_get_region(SEXP x, R_xlen_t i, R_xlen_t n, double *buf) {
    // FIXME
}

static R_xlen_t ufo_cached_raw_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rbyte *buf) {
    // FIXME
}

static R_xlen_t ufo_cached_complex_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rcomplex *buf) {
    // FIXME
}

static R_xlen_t ufo_cached_logical_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    // FIXME
}

// UFO Inits
void init_ufo_cached_integer_altrep_class(DllInfo * dll) {
    // FIXME
}

void init_ufo_cached_numeric_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_cached_numeric_altrep",
                                                   "ufo_altrep", dll);
    ufo_cached_numeric_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_cached_vector_duplicate);
    R_set_altrep_Inspect_method(cls, ufo_cached_vector_inspect);
    R_set_altrep_Length_method(cls, ufo_cached_vector_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_cached_vector_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_cached_vector_dataptr_or_null);

    /* Override ALTREAL methods */
    R_set_altreal_Elt_method(cls, ufo_cached_numeric_element);
    R_set_altreal_Get_region_method(cls, ufo_cached_numeric_get_region);
}

void init_ufo_cached_logical_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_cached_logical_altrep",
                                                   "ufo_altrep", dll);
    ufo_cached_logical_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_cached_vector_duplicate);
    R_set_altrep_Inspect_method(cls, ufo_cached_vector_inspect);
    R_set_altrep_Length_method(cls, ufo_cached_vector_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_cached_vector_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_cached_vector_dataptr_or_null);

    /* Override ALTLOGICAL methods */
    R_set_altlogical_Elt_method(cls, ufo_cached_logical_element);
    R_set_altlogical_Get_region_method(cls, ufo_cached_logical_get_region);
}

void init_ufo_cached_complex_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_cached_complex_altrep",
                                                   "ufo_altrep", dll);
    ufo_cached_complex_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_cached_vector_duplicate);
    R_set_altrep_Inspect_method(cls, ufo_cached_vector_inspect);
    R_set_altrep_Length_method(cls, ufo_cached_vector_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_cached_vector_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_cached_vector_dataptr_or_null);

    /* Override ALTCOMPLEX methods */
    R_set_altcomplex_Elt_method(cls, ufo_cached_complex_element);
    R_set_altcomplex_Get_region_method(cls, ufo_cached_complex_get_region);
}

void init_ufo_cached_raw_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_cached_raw_altrep",
                                                   "ufo_altrep", dll);
    ufo_cached_raw_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_cached_vector_duplicate);
    R_set_altrep_Inspect_method(cls, ufo_cached_vector_inspect);
    R_set_altrep_Length_method(cls, ufo_cached_vector_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_cached_vector_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_cached_vector_dataptr_or_null);

    /* Override ALTRAW methods */
    R_set_altraw_Elt_method(cls, ufo_cached_raw_element);
    R_set_altraw_Get_region_method(cls, ufo_cached_raw_get_region);
}
