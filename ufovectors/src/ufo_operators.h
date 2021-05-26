#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

SEXP ufo_fit_result(SEXP x, SEXP y, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_div_result(SEXP x, SEXP y, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_mod_result(SEXP x, SEXP y, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_rel_result(SEXP x, SEXP y, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_log_result(SEXP x, SEXP y, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_neg_result(SEXP x,         SEXP/*INTSXP*/ min_load_count);

SEXP ufo_subset		  (SEXP x, SEXP y,         SEXP/*INTSXP*/ min_load_count);
SEXP ufo_subset_assign(SEXP x, SEXP y, SEXP z, SEXP/*INTSXP*/ min_load_count);

SEXP ufo_subscript(SEXP vector, SEXP subscript, SEXP min_load_count);

//SEXP ufo_calculate_chunk_indices(SEXP x_length_sexp, SEXP y_length_sexp, SEXP chunk_sexp, SEXP chunk_size_sexp);
SEXP ufo_get_chunk(SEXP x, SEXP chunk, SEXP chunk_size, SEXP result_length);
