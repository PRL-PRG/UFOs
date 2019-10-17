#pragma once

#include "Rinternals.h"

SEXP/*INTSXP*/ ufo_matrix_intsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*REALSXP*/ ufo_matrix_realsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*CPLXSXP*/ ufo_matrix_cplxsxp_bin(SEXP/*STRSXP*/ path);
//SEXP/*STRSXP*/ ufo_vectors_strsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*LGLSXP*/ ufo_matrix_lglsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*RAWSXP*/ ufo_matrix_rawsxp_bin(SEXP/*STRSXP*/ path);

SEXP/*NILSXP*/ ufo_store_bin(SEXP/*STRSXP*/ path, SEXP vector);

//SEXP/*NILSXP*/ ufo_vectors_initialize();
SEXP/*NILSXP*/ ufo_matrix_shutdown();

SEXP/*NILSXP*/ ufo_matrix_set_debug_mode(SEXP/*LGLSXP*/ debug);
