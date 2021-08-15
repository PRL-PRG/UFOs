#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

#include "../include/ufos.h"
#include "ufo_metadata.h"
#include "helpers.h"
#include "debug.h"

SEXP ufo_empty(ufo_vector_type_t type, R_xlen_t size, bool populate_with_na, int32_t min_load_count);

SEXP ufo_intsxp_empty (SEXP/*REALSXP*/ size, SEXP/*LGLSXP*/ populate_with_na, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_realsxp_empty(SEXP/*REALSXP*/ size, SEXP/*LGLSXP*/ populate_with_na, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_rawsxp_empty (SEXP/*REALSXP*/ size,                                  SEXP/*INTSXP*/ min_load_count);
SEXP ufo_cplxsxp_empty(SEXP/*REALSXP*/ size, SEXP/*LGLSXP*/ populate_with_na, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_vecsxp_empty (SEXP/*REALSXP*/ size,                                  SEXP/*INTSXP*/ min_load_count);
SEXP ufo_strsxp_empty (SEXP/*REALSXP*/ size, SEXP/*LGLSXP*/ populate_with_na, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_lglsxp_empty (SEXP/*REALSXP*/ size, SEXP/*LGLSXP*/ populate_with_na, SEXP/*INTSXP*/ min_load_count);
