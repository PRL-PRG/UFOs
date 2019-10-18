#pragma once

#include "Rinternals.h"

void init_ufo_logical_altrep_class(DllInfo * dll);
void init_ufo_integer_altrep_class(DllInfo * dll);
void init_ufo_numeric_altrep_class(DllInfo * dll);
void init_ufo_complex_altrep_class(DllInfo * dll);
void init_ufo_raw_altrep_class(DllInfo * dll);

SEXP/*INTSXP*/ altrep_ufo_vectors_intsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*REALSXP*/ altrep_ufo_vectors_realsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*CPLXSXP*/ altrep_ufo_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path);
//SEXP/*STRSXP*/ altrep_ufo_vectors_strsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*LGLSXP*/ altrep_ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*RAWSXP*/ altrep_ufo_vectors_rawsxp_bin(SEXP/*STRSXP*/ path);

SEXP/*NILSXP*/ altrep_ufo_vectors_set_debug_mode(SEXP/*LGLSXP*/ debug);

