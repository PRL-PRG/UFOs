#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

SEXP ufo_fit_result(SEXP x, SEXP y, SEXP min_load_count);
SEXP ufo_div_result(SEXP x, SEXP y, SEXP min_load_count);
SEXP ufo_mod_result(SEXP x, SEXP y, SEXP min_load_count);
SEXP ufo_rel_result(SEXP x, SEXP y, SEXP min_load_count);
SEXP ufo_log_result(SEXP x, SEXP y, SEXP min_load_count);
SEXP ufo_neg_result(SEXP x,         SEXP min_load_count);

SEXP ufo_subset		  (SEXP x, SEXP y);
SEXP ufo_subset_assign(SEXP x, SEXP y, SEXP z);
