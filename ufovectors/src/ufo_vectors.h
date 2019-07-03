#pragma once

#include "Rinternals.h"

SEXP/*INTSXP*/ ufo_vectors_intsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*REALSXP*/ ufo_vectors_realsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*CPLXSXP*/ ufo_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path);
//SEXP/*STRSXP*/ ufo_vectors_strsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*LGLSXP*/ ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path);
SEXP/*RAWSXP*/ ufo_vectors_rawsxp_bin(SEXP/*STRSXP*/ path);

SEXP/*NILSXP*/ ufo_store_bin(SEXP/*STRSXP*/ path, SEXP vector);

//SEXP/*NILSXP*/ ufo_vectors_initialize();
SEXP/*NILSXP*/ ufo_vectors_shutdown();

SEXP/*NILSXP*/ ufo_vectors_set_debug_mode(SEXP/*LGLSXP*/ debug);