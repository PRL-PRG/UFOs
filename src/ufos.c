#define USE_RINTERNALS
#define UFOS_DEBUG

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rallocators.h>
#include <stdlib.h>
#include "ufos.h"

ufInstance_t ufInstance;
void initializeUFOs () {
    ufInstance = ufMakeInstance();
    ufInit(ufInstance);
}

typedef SEXP (*__ufo_specific_vector_constructor)(size_t length);

SEXP __ufo_new_any(SEXP/*INTSXP*/ vector_lengths, SEXP/*EXTPTRSXP*/ source,
                   __ufo_specific_vector_constructor constructor) {

    if (TYPEOF(lengths) != INTSXP) {
        Rf_error("Lengths have to be an integer vector (INTSXP).\n");
        return R_NilValue;
    }

    if (LENGTH(lengths) == 0) {
        return R_NilValue;
    }

    if (LENGTH(lengths) == 1) {
        size_t length = INTEGER_ELT(lengths, 0);
        return constructor(length);
    }

    SEXP/*VECSXP<INTSXP>*/ results;
    PROTECT(results = allocVector(VECSXP, LENGTH(lengths)));
    for (int i = 0; i < LENGTH(lengths); i++) {
        size_t length = INTEGER_ELT(lengths, i);
        SET_VECTOR_ELT(results, i, constructor(length));
    }
    UNPROTECT(1);
    return results;
}

SEXP/*INTSXP*/ __ufo_new_intsxp(size_t length) {
    return R_NilValue; //TODO
}

SEXP/*INTSXP*/ __ufo_new_lglsxp(size_t length) {
    return R_NilValue; //TODO
}

SEXP/*INTSXP|VECSXP<INTSXP>*/ ufo_new_intsxp(SEXP/*INTSXP*/ vector_lengths,
                                             SEXP/*EXTPTRSXP*/ source) {
    return __ufo_new_any(vector_lengths, source, &__ufo_new_intsxp);
}

SEXP/*INTSXP|VECSXP<INTSXP>*/ ufo_new_lglsxp(SEXP/*INTSXP*/ vector_lengths,
                                             SEXP/*EXTPTRSXP*/ source) {
    return __ufo_new_any(vector_lengths, source, &__ufo_new_lglsxp);
}

SEXP/*EXTPTRSXP*/ ufo_make_bin_file_source(SEXP/*STRSXP*/ path) {
    return R_NilValue; //TODO
}