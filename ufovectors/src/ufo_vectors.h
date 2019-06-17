#pragma once

#include "Rinternals.h"

SEXP ufo_vectors_intsxp_bin(SEXP/*STRSXP*/ path);
//SEXP ufo_vectors_realsxp_bin(SEXP/*STRSXP*/ path);
//SEXP ufo_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path);
//SEXP ufo_vectors_strsxp_bin(SEXP/*STRSXP*/ path);
//SEXP ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path);

SEXP ufo_vectors_initialize();
SEXP ufo_vectors_shutdown();