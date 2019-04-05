#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Altrep.h>

#include <stdlib.h>

#include "ufos_altrep.h"

static R_altrep_class_t ufo_int_altrep;

/*
 * The ALTREP class is the most general abstract class. It specifies the methods
 * Duplicate, Coerce, Inspect, and Length.
 *
 * The ALTVEC class specifies methods common to all concrete vector types. These
 * are Dataptr, Dataptr_or_null, and Extract_subset.
 *
 * Vector classes for specific element types, such as ALTINTEGER, ALTREAL, or
 * ALTSTRING will implement Elt methods. For atomic types they will also
 * implement Get_region methods for copying a contiguous region of elements to
 * a buffer.
 *
 * A class that wants to handle serializing and unserializing its objects should
 * define Serialized_state and Unserialize methods.
 *
 * The class creation and registration functions, e.g. R_make_altreal_class,
 * create a class object in an R session corresponding to a specified class name
 * and package name.
 *
 * See also: https://github.com/wch/r-source/blob/ALTREP/ALTREP.md
 */

// ALTREP Methods
static SEXP ufo_Duplicate(SEXP x, Rboolean deep);
static Rboolean ufo_Inspect(SEXP x, int pre, int deep, int pvec,
                             void (*inspect_subtree)(SEXP, int, int, int));
static R_xlen_t ufo_Length(SEXP x);

// ALTVEC Methods
static void *ufo_Dataptr(SEXP x, Rboolean writeable);
static const void *ufo_Dataptr_or_null(SEXP x);

// ALTINTEGER Methods
static int ufo_integer_Elt(SEXP x, R_xlen_t i);
static R_xlen_t ufo_integer_Get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf);

void InitUFOAltRepClass(DllInfo * dll) {

    R_altrep_class_t cls =
            R_make_altinteger_class("ufo_integer", "ufos", dll);
    ufo_int_altrep = cls;

    /* Override ALTREP methods */
    R_set_altrep_Duplicate_method(cls, ufo_Duplicate);
    R_set_altrep_Inspect_method(cls, ufo_Inspect);
    R_set_altrep_Length_method(cls, ufo_Length);

    /* Override ALTVEC methods */
    R_set_altvec_Dataptr_method(cls, ufo_Dataptr);
    R_set_altvec_Dataptr_or_null_method(cls, ufo_Dataptr_or_null);

    /* Override ALTINTEGER methods */
    R_set_altinteger_Elt_method(cls, ufo_integer_Elt);
    R_set_altinteger_Get_region_method(cls, ufo_integer_Get_region);
}

// Function that creates an altrep vector.
SEXP/*INTSXP|VECSXP<INTSXP>*/ ufo_new_altrep(SEXP/*INTSXP*/ lengths) {

    if (LENGTH(lengths) == 1) {
        //SEXP meta = allocSExp();
        return R_new_altrep(ufo_int_altrep, R_NilValue, R_NilValue);
    }

    SEXP/*VECSXP<INTSXP>*/ results = allocVector(VECSXP, LENGTH(lengths));
    for (int i = 0; i < LENGTH(lengths); i++) {
        SET_VECTOR_ELT(results, i, R_new_altrep(ufo_int_altrep, R_NilValue, R_NilValue));
    }
}

static SEXP ufo_Duplicate(SEXP x, Rboolean deep) {

    if (R_altrep_data1(x) == NULL) {
        return R_new_altrep(ufo_int_altrep, R_NilValue, R_NilValue);
    }

    SEXP payload = PROTECT(allocVector(INTSXP, XLENGTH(R_altrep_data1(x))));

    for (int i = 0; i < XLENGTH(R_altrep_data1(x)); i++)
        INTEGER(payload)[i] = INTEGER(R_altrep_data1(x))[i];

    R_set_altrep_data1(x, payload);
    UNPROTECT(1);

    return R_new_altrep(ufo_int_altrep, payload, R_NilValue);
}

static Rboolean ufo_Inspect(SEXP x, int pre, int deep, int pvec,
                             void (*inspect_subtree)(SEXP, int, int, int)) {

    if (R_altrep_data1(x) == R_NilValue)
        Rprintf(" ufo %s\n", type2char(TYPEOF(x)));
    else
        Rprintf(" ufo %s (materialized)\n", type2char(TYPEOF(x)));

    if (R_altrep_data1(x) != R_NilValue)
        inspect_subtree(R_altrep_data1(x), pre, deep, pvec);

    if (R_altrep_data2(x) != R_NilValue)
        inspect_subtree(R_altrep_data2(x), pre, deep, pvec);

    return FALSE;
}

static R_xlen_t ufo_Length(SEXP x) {

    if (R_altrep_data1(x) == R_NilValue)
        return 42;
    else
        XLENGTH(R_altrep_data1(x));
}

static void ufo_materialize_data1(SEXP x) {

    PROTECT(x);
    SEXP payload = allocVector(INTSXP, 42);
    int *data = INTEGER(payload);
    for (int i = 0; i < 42; i++) {
        data[i] = 42;
    }
    R_set_altrep_data1(x, payload);
    UNPROTECT(1);
}

static void *ufo_Dataptr(SEXP x, Rboolean writeable) {

    if (writeable) {
        if (R_altrep_data1(x) == R_NilValue)
            ufo_materialize_data1(x);
        return DATAPTR(R_altrep_data1(x));

    } else {

        if (R_altrep_data1(x) == R_NilValue)
            ufo_materialize_data1(x);
        return DATAPTR(R_altrep_data1(x));
    }
}

static const void *ufo_Dataptr_or_null(SEXP x) {

    if (R_altrep_data1(x) == R_NilValue)
        ufo_materialize_data1(x);

    return DATAPTR_OR_NULL(R_altrep_data1(x));
}

static int ufo_integer_Elt(SEXP x, R_xlen_t i) {

    if (R_altrep_data1(x) != R_NilValue)
        return INTEGER(R_altrep_data1(x))[i];

    return 42;
}

static R_xlen_t ufo_integer_Get_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    return INTEGER_GET_REGION(R_altrep_data1(x), i, n, buf);
}