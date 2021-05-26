#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Altrep.h>
#include <R_ext/Rallocators.h>

#include "metadata.h"
#include "debug.h"
#include "helpers.h"

#include "altrep_seq.h"
#include "R_ext.h"

static R_altrep_class_t seq_integer_altrep;
static R_altrep_class_t seq_numeric_altrep;

R_altrep_class_t __seq_get_class_from_type(SEXPTYPE type) {
    switch (type) {
        case INTSXP:
            return seq_integer_altrep;
        case REALSXP:
            return seq_numeric_altrep;
        default:
            Rf_error("Unrecognized vector type: %d\n", type);
    }
}

void seq_finalize(SEXP wrapper) {
    //altrep_seq_config_t *cfg = (altrep_seq_config_t *) EXTPTR_PTR(wrapper);
    //fclose(cfg->file_handle);
}

SEXP seq_new(SEXPTYPE type, int from, int to, int by) {

    altrep_seq_config_t *cfg =
            (altrep_seq_config_t *) malloc(sizeof(altrep_seq_config_t));

    cfg->to = to;
    cfg->from = from;
    cfg->by = by;
    cfg->type = type;
    cfg->size = (to - from) / by + 1;

    SEXP wrapper = PROTECT(allocSExp(EXTPTRSXP));
    EXTPTR_PTR(wrapper) = (void *) cfg;
    EXTPTR_TAG(wrapper) = Rf_install("ALTREP SEQ CFG");

    SEXP ans = R_new_altrep(__seq_get_class_from_type(type), wrapper, R_NilValue);
    EXTPTR_PROT(wrapper) = ans;

    /* Finalizer */
    R_MakeWeakRefC(wrapper, R_NilValue, seq_finalize, TRUE);

    UNPROTECT(1);
    return ans;
}
SEXP/*INTSXP*/ seq_intsxp_new(SEXP/*INTSXP*/ from, SEXP/*INTSXP*/ to, SEXP/*INTSXP*/ by) {
    return seq_new(INTSXP, __extract_int_or_die(from), __extract_int_or_die(to), __extract_int_or_die(by));
}
SEXP/*REALSXP*/ seq_realsxp_new(SEXP/*INTSXP*/ from, SEXP/*INTSXP*/ to, SEXP/*INTSXP*/ by) {
    return seq_new(REALSXP, __extract_int_or_die(from), __extract_int_or_die(to), __extract_int_or_die(by));
}

// typedef struct {
//     SEXPTYPE                type;
//     size_t /*aka R_len_t*/  size;
//     char const             *path;
//     //size_t               *dimensions;
// } altrep_ufo_source_t;

// SEXP seq_new_wrapper(SEXPTYPE mode, R_xlen_t n, void *data) {
//     return seq_new(mode, (const char *) data); // TODO ?
// }

static SEXP seq_duplicate(SEXP x, Rboolean deep) {

    altrep_seq_config_t *new_cfg =
            (altrep_seq_config_t *) malloc(sizeof(altrep_seq_config_t));

    altrep_seq_config_t *old_cfg =
            (altrep_seq_config_t *) EXTPTR_PTR(R_altrep_data1(x));

    new_cfg->to = old_cfg->to;
    new_cfg->from = old_cfg->from;
    new_cfg->by = old_cfg->by;
    new_cfg->type = old_cfg->type;
    new_cfg->size = old_cfg->size;

    SEXP wrapper = allocSExp(EXTPTRSXP);
    EXTPTR_PTR(wrapper) = (void *) new_cfg;
    EXTPTR_TAG(wrapper) = Rf_install("ALTREP SEQ CFG");

    if (R_altrep_data2(x) == R_NilValue) {
        return R_new_altrep(__seq_get_class_from_type(new_cfg->type), wrapper, R_NilValue);

    } else {
        SEXP payload = PROTECT(allocVector(INTSXP, XLENGTH(R_altrep_data2(x))));

        for (int i = 0; i < XLENGTH(R_altrep_data2(x)); i++) {
            INTEGER(payload)[i] = INTEGER(R_altrep_data2(x))[i];
        }

        SEXP ans = R_new_altrep(__seq_get_class_from_type(new_cfg->type), wrapper, payload);
        UNPROTECT(1);
        return ans;
    }
}

static Rboolean seq_inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int)) {

    if (R_altrep_data1(x) == R_NilValue) {
        Rprintf(" seq_integer %s\n", type2char(TYPEOF(x)));
    } else {
        Rprintf(" seq_integer %s (materialized)\n", type2char(TYPEOF(x)));
    }

    if (R_altrep_data1(x) != R_NilValue) {
        inspect_subtree(R_altrep_data1(x), pre, deep, pvec);
    }

    if (R_altrep_data2(x) != R_NilValue) {
        inspect_subtree(R_altrep_data2(x), pre, deep, pvec);
    }

    return FALSE;
}

static R_xlen_t seq_length(SEXP x) {
    if (__get_debug_mode()) {
        Rprintf("seq_Length\n");
        Rprintf("           SEXP: %p\n", x);
    }
    if (R_altrep_data2(x) == R_NilValue) {
        altrep_seq_config_t *cfg =
                (altrep_seq_config_t *) EXTPTR_PTR(R_altrep_data1(x));
        return cfg->size;
    } else {
        return XLENGTH(R_altrep_data2(x));
    }
}

int populate_integer_seq(uint64_t startValueIdx, uint64_t endValueIdx, altrep_seq_config_t* cfg, int* target) {
    for (size_t i = 0; i < endValueIdx - startValueIdx; i++) {
        target[i] = cfg->from + cfg->by * (i + startValueIdx);
    }
    return  endValueIdx - startValueIdx + 1;
}

int populate_double_seq(uint64_t startValueIdx, uint64_t endValueIdx, altrep_seq_config_t* cfg, double* target) {
    for (size_t i = 0; i < endValueIdx - startValueIdx; i++) {
        target[i] = cfg->from + cfg->by * (i + startValueIdx);
    }
    return  endValueIdx - startValueIdx + 1;
}

int populate_seq(uint64_t startValueIdx, uint64_t endValueIdx, altrep_seq_config_t* cfg, double* target) {
    switch (cfg->type) {
    case INTSXP:
        return populate_integer_seq(startValueIdx, endValueIdx, cfg, target);
    case REALSXP:
        return populate_double_seq(startValueIdx, endValueIdx, cfg, target);      
    default:
        Rf_error("Unrecognized vector type: %d\n", cfg->type);
        break;
    }       
}

static void __materialize_integer_data(SEXP x) {
    altrep_seq_config_t *cfg =
            (altrep_seq_config_t *) EXTPTR_PTR(R_altrep_data1(x));    
    SEXP payload = allocVector(INTSXP, cfg->size);
    PROTECT(payload);
    int *data = INTEGER(payload);
    populate_integer_seq(0, cfg->size, cfg, data);
    R_set_altrep_data2(x, payload);
    UNPROTECT(1);
}

static void __materialize_double_data(SEXP x) {
    altrep_seq_config_t *cfg =
            (altrep_seq_config_t *) EXTPTR_PTR(R_altrep_data1(x));
    SEXP payload = allocVector(REALSXP, cfg->size);
    PROTECT(payload);
    double *data = REAL(payload);
    populate_double_seq(0, cfg->size, cfg, data);
    R_set_altrep_data2(x, payload);
    UNPROTECT(1);
}

static void __materialize_data(SEXP x) {
    altrep_seq_config_t *cfg =
            (altrep_seq_config_t *) EXTPTR_PTR(R_altrep_data1(x));
    switch (cfg->type)
    {
    case INTSXP:
        __materialize_integer_data(x);
        break;
    case REALSXP:
        __materialize_double_data(x);
        break;        
    default:
        Rf_error("Unrecognized vector type: %d\n", cfg->type);
        break;
    }        
}

static void *seq_dataptr(SEXP x, Rboolean writeable) {
    if (__get_debug_mode()) {
        Rprintf("seq_Dataptr\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("      writeable: %i\n", writeable);
    }
    if (writeable) {
        if (R_altrep_data2(x) == R_NilValue)
            __materialize_data(x);
        return DATAPTR(R_altrep_data2(x));
    } else {
        if (R_altrep_data2(x) == R_NilValue)
            __materialize_data(x);
        return DATAPTR(R_altrep_data2(x));
    }
}

static const void *seq_dataptr_or_null(SEXP x) {
    if (__get_debug_mode()) {
        Rprintf("seq_Dataptr_or_null\n");
        Rprintf("           SEXP: %p\n", x);
    }
    if (R_altrep_data2(x) == R_NilValue)
        __materialize_data(x);
    return DATAPTR_OR_NULL(R_altrep_data2(x));
}

// static void ufo_seq_element(SEXP x, R_xlen_t i, void *target) {
//     if (__get_debug_mode()) {
//         Rprintf("ufo_seq_element\n");
//         Rprintf("           SEXP: %p\n", x);
//         Rprintf("          index: %li\n", i);
//         Rprintf("         target: %p\n", target);
//     }
//     altrep_ufo_config_t *cfg =
//             (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));

//     populate_seq(i, i+1, cfg, (char *) target);
// }

static int seq_integer_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return INTEGER_ELT(R_altrep_data2(x), i);
    altrep_seq_config_t *cfg =
             (altrep_seq_config_t *) EXTPTR_PTR(R_altrep_data1(x));
    return cfg->from + cfg->by * i;
}

static double seq_numeric_element(SEXP x, R_xlen_t i) {
    if (R_altrep_data2(x) != R_NilValue)
        return REAL_ELT(R_altrep_data2(x), i);
    altrep_seq_config_t *cfg =
             (altrep_seq_config_t *) EXTPTR_PTR(R_altrep_data1(x));        
    return cfg->from + cfg->by * i;
}

// static R_xlen_t ufo_seq_get_region(SEXP x, R_xlen_t i, R_xlen_t n, void *buf) {
//     if (__get_debug_mode()) {
//         Rprintf("ufo_vector_get_region\n");
//         Rprintf("           SEXP: %p\n", x);
//         Rprintf("          index: %li\n", i);
//         Rprintf("         length: %li\n", n);
//         Rprintf("         target: %p\n", buf);
//     }
//     altrep_ufo_config_t *cfg =
//             (altrep_ufo_config_t *) EXTPTR_PTR(R_altrep_data1(x));
//     Rf_error("Unimplemented!");
//     return __load_from_file(__get_debug_mode(), i, i+n, cfg, (char *) buf);
// }

static R_xlen_t seq_integer_get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return INTEGER_GET_REGION(R_altrep_data2(x), i, n, buf);

    if (__get_debug_mode()) {
        Rprintf("seq_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("         length: %li\n", n);
        Rprintf("         target: %p\n", buf);
    }
    altrep_seq_config_t *cfg =
            (altrep_seq_config_t *) EXTPTR_PTR(R_altrep_data1(x));

    return populate_integer_seq(i, i+n, cfg, (char *) buf);   
}

static R_xlen_t seq_numeric_get_region(SEXP x, R_xlen_t i, R_xlen_t n, double *buf) {
    if (R_altrep_data2(x) == R_NilValue)
        return REAL_GET_REGION(R_altrep_data2(x), i, n, buf);
        if (__get_debug_mode()) {
        Rprintf("seq_get_region\n");
        Rprintf("           SEXP: %p\n", x);
        Rprintf("          index: %li\n", i);
        Rprintf("         length: %li\n", n);
        Rprintf("         target: %p\n", buf);
    }
    altrep_seq_config_t *cfg =
            (altrep_seq_config_t *) EXTPTR_PTR(R_altrep_data1(x));

    return populate_double_seq(i, i+n, cfg, (char *) buf);   
}

// static SEXP ufo_integer_extract_subset(SEXP x, SEXP indx, SEXP call) {
//     return;
// }


// UFO Inits
void init_seq_integer_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altinteger_class("seq_integer_altrep",
                                                   "seq_altrep", dll);
    seq_integer_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, seq_duplicate);
    R_set_altrep_Inspect_method(cls, seq_inspect);
    R_set_altrep_Length_method(cls, seq_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, seq_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, seq_dataptr_or_null);

    /* Override ALTINTEGER methods */
    R_set_altinteger_Elt_method(cls, seq_integer_element);
    R_set_altinteger_Get_region_method(cls, seq_integer_get_region);

    //R_set_altinteger_Sum_method(cls, ufo_integer_sum);

    //R_set_altvec_Extract_subset_method(cls, seq_integer_extract_subset);
}

void init_seq_numeric_altrep_class(DllInfo * dll) {
    R_altrep_class_t cls = R_make_altreal_class("seq_numeric_altrep",
                                                "seq_altrep", dll);
    seq_numeric_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, seq_duplicate);
    R_set_altrep_Inspect_method(cls, seq_inspect);
    R_set_altrep_Length_method(cls, seq_length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, seq_dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, seq_dataptr_or_null);

    /* Override ALTREAL methods */
    R_set_altreal_Elt_method(cls, seq_numeric_element);
    R_set_altreal_Get_region_method(cls, seq_numeric_get_region);

    //R_set_altreal_Sum_method(cls, ufo_numeric_sum);
}

// SEXP/*INTSXP*/ altrep_intsxp_seq(SEXP/*STRSXP*/ path) {
//     return ufo_vector_new_altrep(INTSXP, __extract_int_or_die);
// }
// SEXP/*REALSXP*/ altrep_realsxp_seq(SEXP/*STRSXP*/ path) {
//     return ufo_vector_new_altrep(REALSXP, __extract_path_or_die(path));
// }