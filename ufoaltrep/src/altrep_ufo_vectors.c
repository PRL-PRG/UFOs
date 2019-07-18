#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Altrep.h>

#include "altrep_ufo_files.h"
#include "altrep_ufo_vectors.h"

int altrep_ufo_debug_mode = 0;

SEXP/*NILSXP*/ altrep_ufo_vectors_set_debug_mode(SEXP/*LGLSXP*/ debug) {
    altrep_ufo_debug_mode = __extract_boolean_or_die(debug);
    return R_NilValue;
}

typedef struct {
    char const *path;
    size_t element_size;
    size_t vector_size;
    SEXPTYPE type;
} altrep_ufo_config_t;

static R_altrep_class_t ufo_integer_altrep;
static R_altrep_class_t ufo_numeric_altrep;
static R_altrep_class_t ufo_raw_altrep;
static R_altrep_class_t ufo_complex_altrep;
static R_altrep_class_t ufo_logical_altrep;

R_altrep_class_t __get_class_from_type(SEXPTYPE type) {
    switch (type) {
        case LGLSXP:
            return ufo_logical_altrep;
        case INTSXP:
            return ufo_integer_altrep;
        case REALSXP:
            return ufo_numeric_altrep;
        case CPLXSXP:
            return ufo_complex_altrep;
        case RAWSXP:
            return ufo_raw_altrep;
        default:
            Rf_error("Unrecognized vector type: %d\n", type);
    }
}

// ALTREP Methods
//static SEXP ufo_integer_Duplicate(SEXP x, Rboolean deep);
//static SEXP ufo_numeric_Duplicate(SEXP x, Rboolean deep);
//static SEXP ufo_logical_Duplicate(SEXP x, Rboolean deep);
//static SEXP ufo_raw_Duplicate(SEXP x, Rboolean deep);
//static SEXP ufo_complex_Duplicate(SEXP x, Rboolean deep);

//static Rboolean ufo_integer_Inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int));
//static Rboolean ufo_numeric_Inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int));
//static Rboolean ufo_logical_Inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int));
//static Rboolean ufo_raw_Inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int));
//static Rboolean ufo_complex_Inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int));

//static R_xlen_t ufo_integer_Length(SEXP x);
//static R_xlen_t ufo_numeric_Length(SEXP x);
//static R_xlen_t ufo_logical_Length(SEXP x);
//static R_xlen_t ufo_raw_Length(SEXP x);
//static R_xlen_t ufo_complex_Length(SEXP x);

// ALTVEC Methods
//static void *ufo_integer_Dataptr(SEXP x, Rboolean writeable);
//static void *ufo_numeric_Dataptr(SEXP x, Rboolean writeable);
//static void *ufo_logical_Dataptr(SEXP x, Rboolean writeable);
//static void *ufo_raw_Dataptr(SEXP x, Rboolean writeable);
//static void *ufo_complex_Dataptr(SEXP x, Rboolean writeable);

//static const void *ufo_integer_Dataptr_or_null(SEXP x);
//static const void *ufo_numeric_Dataptr_or_null(SEXP x);
//static const void *ufo_logical_Dataptr_or_null(SEXP x);
//static const void *ufo_raw_Dataptr_or_null(SEXP x);
//static const void *ufo_complex_Dataptr_or_null(SEXP x);

// ALTINTEGER Methods
//static int ufo_integer_Elt(SEXP x, R_xlen_t i);
//static R_xlen_t ufo_integer_Get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf);

// UFO constructors
SEXP ufo_vector_new_altrep(SEXPTYPE type, char const *path) {

    altrep_ufo_config_t *cfg =
            (altrep_ufo_config_t *) malloc(sizeof(altrep_ufo_config_t));

    cfg->type = type;
    cfg->path = path;
    cfg->element_size = __get_element_size(type);
    cfg->vector_size = __get_vector_length_from_file_or_die(cfg->path, cfg->element_size);

    SEXP wrapper = allocSExp(EXTPTRSXP);
    EXTPTR_PTR(wrapper) = (void *) cfg;
    EXTPTR_TAG(wrapper) = Rf_install("ALTREP UFO CFG");

    return R_new_altrep(__get_class_from_type(type), wrapper, R_NilValue);
}
SEXP/*INTSXP*/ altrep_ufo_vectors_intsxp_bin(SEXP/*STRSXP*/ path) {
    return ufo_vector_new_altrep(INTSXP, __extract_path_or_die(path));
}
SEXP/*REALSXP*/ altrep_ufo_vectors_realsxp_bin(SEXP/*STRSXP*/ path) {
    return ufo_vector_new_altrep(REALSXP, __extract_path_or_die(path));
}
SEXP/*LGLSXP*/ altrep_ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path) {
    return ufo_vector_new_altrep(LGLSXP, __extract_path_or_die(path));
}
SEXP/*CPLXSXP*/ altrep_ufo_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path) {
    return ufo_vector_new_altrep(CPLXSXP, __extract_path_or_die(path));
}
SEXP/*RAWSXP*/ altrep_ufo_vectors_rawsxp_bin(SEXP/*STRSXP*/ path) {
    return ufo_vector_new_altrep(RAWSXP, __extract_path_or_die(path));
}

static SEXP ufo_vector_Duplicate(SEXP x, Rboolean deep) {

    altrep_ufo_config_t *new_cfg =
            (altrep_ufo_config_t *) malloc(sizeof(altrep_ufo_config_t));

    altrep_ufo_config_t *old_cfg =
            (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));

    new_cfg->type = old_cfg->type;
    new_cfg->path = old_cfg->path;
    new_cfg->element_size = old_cfg->element_size;
    new_cfg->vector_size = old_cfg->vector_size;

    SEXP wrapper = allocSExp(EXTPTRSXP);
    EXTPTR_PTR(wrapper) = (void *) new_cfg;
    EXTPTR_TAG(wrapper) = Rf_install("ALTREP UFO CFG");

    if (R_altrep_data2(x) == R_NilValue) {
        return R_new_altrep(__get_class_from_type(new_cfg->type), wrapper, R_NilValue);

    } else {
        SEXP payload = PROTECT(allocVector(INTSXP, XLENGTH(R_altrep_data2(x))));

        for (int i = 0; i < XLENGTH(R_altrep_data2(x)); i++) {
            INTEGER(payload)[i] = INTEGER(R_altrep_data2(x))[i];
        }

        SEXP ans = R_new_altrep(__get_class_from_type(new_cfg->type), wrapper, payload);
        UNPROTECT(1);
        return ans;
    }
}

static Rboolean ufo_vector_Inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int)) {

    if (R_altrep_data1(x) == R_NilValue) {
        Rprintf(" ufo_integer_altrep %s\n", type2char(TYPEOF(x)));
    } else {
        Rprintf(" ufo_integer_altrep %s (materialized)\n", type2char(TYPEOF(x)));
    }

    if (R_altrep_data1(x) != R_NilValue) {
        inspect_subtree(R_altrep_data1(x), pre, deep, pvec);
    }

    if (R_altrep_data2(x) != R_NilValue) {
        inspect_subtree(R_altrep_data2(x), pre, deep, pvec);
    }

    return FALSE;
}

static R_xlen_t ufo_vector_Length(SEXP x) {
    if (altrep_ufo_debug_mode) {
        Rprintf("ufo_vector_Length\n");
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
    SEXP payload = allocVector(INTSXP, cfg->vector_size);
    int *data = INTEGER(payload);
    __load_from_file(altrep_ufo_debug_mode, 0, cfg->vector_size,
                     cfg->path, cfg->element_size, cfg->vector_size,
                     (char *) data);
    R_set_altrep_data2(x, payload);
    UNPROTECT(1);
}

static void *ufo_vector_Dataptr(SEXP x, Rboolean writeable) {
    if (altrep_ufo_debug_mode) {
        Rprintf("ufo_vector_Dataptr\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("      writeable: %i\n", writeable);
    }
    if (writeable) {
        if (R_altrep_data2(x) == R_NilValue)
            __materialize_data(x);
        return DATAPTR(R_altrep_data1(x));
    } else {
        if (R_altrep_data2(x) == R_NilValue)
            __materialize_data(x);
        return DATAPTR(R_altrep_data1(x));
    }
}

static const void *ufo_vector_Dataptr_or_null(SEXP x) {
    if (altrep_ufo_debug_mode) {
        Rprintf("ufo_vector_Dataptr_or_null\n");
        Rprintf("           SEXP: %p\n", x);
    }
    if (R_altrep_data2(x) == R_NilValue)
        __materialize_data(x);
    return DATAPTR_OR_NULL(R_altrep_data1(x));
}

static void ufo_vector_element(SEXP x, R_xlen_t i, void *target) {
    if (altrep_ufo_debug_mode) {
        Rprintf("ufo_vector_element\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("         target: %p\n", target);
    }
    altrep_ufo_config_t *cfg =
            (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));

    __load_from_file(altrep_ufo_debug_mode, i, i+1,
                     cfg->path, cfg->element_size, cfg->vector_size,
                     (char *) &target);
}

static int ufo_integer_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return INTEGER_ELT(R_altrep_data2(x), i);
    int val;
    ufo_vector_element(x, i, &val);
    return val;
}

static double ufo_numeric_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return REAL_ELT(R_altrep_data2(x), i);
    double val;
    ufo_vector_element(x, i, &val);
    return val;
}

static Rbyte ufo_raw_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return RAW_ELT(R_altrep_data2(x), i);
    Rbyte val;
    ufo_vector_element(x, i, &val);
    return val;
}

static Rcomplex ufo_complex_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return COMPLEX_ELT(R_altrep_data2(x), i);
    Rcomplex val;
    ufo_vector_element(x, i, &val);
    return val;
}

static int ufo_logical_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return LOGICAL_ELT(R_altrep_data2(x), i);
    Rboolean val;
    ufo_vector_element(x, i, &val);
    return (int) val;
}

static R_xlen_t ufo_vector_get_region(SEXP x, R_xlen_t i, R_xlen_t n, void *buf) {
    if (altrep_ufo_debug_mode) {
        Rprintf("ufo_vector_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("         length: %li\n", n);
        Rprintf("         target: %p\n", buf);
    }
    altrep_ufo_config_t *cfg =
            (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));
    return __load_from_file(altrep_ufo_debug_mode, i, i+n,
                            cfg->path, cfg->element_size, cfg->vector_size,
                            (char *) buf);
}

static R_xlen_t ufo_integer_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return INTEGER_GET_REGION(R_altrep_data2(x), i, n, buf);
    return ufo_vector_get_region(x, i, n, buf);
}

static R_xlen_t ufo_numeric_get_region(SEXP x, R_xlen_t i, R_xlen_t n, double *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return REAL_GET_REGION(R_altrep_data2(x), i, n, buf);
    return ufo_vector_get_region(x, i, n, buf);
}

static R_xlen_t ufo_raw_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rbyte *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return RAW_GET_REGION(R_altrep_data2(x), i, n, buf);
    return ufo_vector_get_region(x, i, n, buf);
}

static R_xlen_t ufo_complex_get_region(SEXP x, R_xlen_t i, R_xlen_t n, Rcomplex *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return COMPLEX_GET_REGION(R_altrep_data2(x), i, n, buf);
    return ufo_vector_get_region(x, i, n, buf);
}

static R_xlen_t ufo_logical_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return LOGICAL_GET_REGION(R_altrep_data2(x), i, n, buf);
    return ufo_vector_get_region(x, i, n, buf);
}

// UFO Inits
void init_ufo_integer_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_integer_altrep",
                                                   "ufo_altrep", dll);
    ufo_integer_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_vector_Duplicate);
    R_set_altrep_Inspect_method(cls, ufo_vector_Inspect);
    R_set_altrep_Length_method(cls, ufo_vector_Length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_vector_Dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_vector_Dataptr_or_null);

    /* Override ALTINTEGER methods */
    R_set_altinteger_Elt_method(cls, ufo_integer_element);
    R_set_altinteger_Get_region_method(cls, ufo_integer_get_region);
}

void init_ufo_numeric_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_numeric_altrep",
                                                   "ufo_altrep", dll);
    ufo_numeric_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_vector_Duplicate);
    R_set_altrep_Inspect_method(cls, ufo_vector_Inspect);
    R_set_altrep_Length_method(cls, ufo_vector_Length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_vector_Dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_vector_Dataptr_or_null);

    /* Override ALTREAL methods */
    R_set_altreal_Elt_method(cls, ufo_numeric_element);
    R_set_altreal_Get_region_method(cls, ufo_numeric_get_region);
}

void init_ufo_logical_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_logical_altrep",
                                                   "ufo_altrep", dll);
    ufo_logical_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_vector_Duplicate);
    R_set_altrep_Inspect_method(cls, ufo_vector_Inspect);
    R_set_altrep_Length_method(cls, ufo_vector_Length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_vector_Dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_vector_Dataptr_or_null);

    /* Override ALTLOGICAL methods */
    R_set_altlogical_Elt_method(cls, ufo_logical_element);
    R_set_altlogical_Get_region_method(cls, ufo_logical_get_region);
}

void init_ufo_complex_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_complex_altrep",
                                                   "ufo_altrep", dll);
    ufo_complex_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_vector_Duplicate);
    R_set_altrep_Inspect_method(cls, ufo_vector_Inspect);
    R_set_altrep_Length_method(cls, ufo_vector_Length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_vector_Dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_vector_Dataptr_or_null);

    /* Override ALTCOMPLEX methods */
    R_set_altcomplex_Elt_method(cls, ufo_complex_element);
    R_set_altcomplex_Get_region_method(cls, ufo_complex_get_region);
}

void init_ufo_raw_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("ufo_raw_altrep",
                                                   "ufo_altrep", dll);
    ufo_raw_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_vector_Duplicate);
    R_set_altrep_Inspect_method(cls, ufo_vector_Inspect);
    R_set_altrep_Length_method(cls, ufo_vector_Length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_vector_Dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_vector_Dataptr_or_null);

    /* Override ALTRAW methods */
    R_set_altraw_Elt_method(cls, ufo_raw_element);
    R_set_altraw_Get_region_method(cls, ufo_raw_get_region);
}