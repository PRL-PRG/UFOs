#pragma once

#include "Rinternals.h"

SEXP/*INTSXP*/ ufo_vectors_intsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*REALSXP*/ ufo_vectors_realsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*CPLXSXP*/ ufo_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*LGLSXP*/ ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*RAWSXP*/ ufo_vectors_rawsxp_bin(SEXP/*STRSXP*/ path);

SEXP/*INTSXP*/ ufo_matrix_intsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols);
SEXP/*REALSXP*/ ufo_matrix_realsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols);
SEXP/*CPLXSXP*/ ufo_matrix_cplxsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols);
SEXP/*LGLSXP*/ ufo_matrix_lglsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols);
SEXP/*RAWSXP*/ ufo_matrix_rawsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols);

SEXP/*NILSXP*/ ufo_store_bin(SEXP/*STRSXP*/ path, SEXP vector);

//SEXP/*NILSXP*/ ufo_vectors_initialize();
SEXP/*NILSXP*/ ufo_vectors_shutdown(); // TODO pin to a weakref to automatically destroy

SEXP/*NILSXP*/ ufo_vectors_set_debug_mode(SEXP/*LGLSXP*/ debug);