#pragma once

#include <R.h>
#include <Rinternals.h>

// Initialization
void initializeUFOs();

// Vector constructors
SEXP/*INTSXP|VECSXP<INTSXP>*/ ufo_new_intsxp(SEXP/*INTSXP*/ vector_lengths,
                                             SEXP/*EXTPTRSXP*/ source);
SEXP/*INTSXP|VECSXP<INTSXP>*/ ufo_new_lglsxp(SEXP/*INTSXP*/ vector_lengths,
                                             SEXP/*EXTPTRSXP*/ source);

// Source constructors
SEXP/*EXTPTRSXP*/ ufo_make_bin_file_source(SEXP/*STRSXP*/ path);