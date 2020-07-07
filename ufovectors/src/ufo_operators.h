#pragma once

#include <R.h>

SEXP ufo_add (SEXP x, SEXP y);
SEXP ufo_sub (SEXP x, SEXP y);
SEXP ufo_mul (SEXP x, SEXP y);
SEXP ufo_div (SEXP x, SEXP y);
SEXP ufo_pow (SEXP x, SEXP y);
SEXP ufo_mod (SEXP x, SEXP y);
SEXP ufo_idiv(SEXP x, SEXP y);

SEXP ufo_lt  (SEXP x, SEXP y);
SEXP ufo_le  (SEXP x, SEXP y);
SEXP ufo_gt  (SEXP x, SEXP y);
SEXP ufo_ge  (SEXP x, SEXP y);
SEXP ufo_eq  (SEXP x, SEXP y);
SEXP ufo_neq (SEXP x, SEXP y);
SEXP ufo_neg (SEXP x);
SEXP ufo_bor (SEXP x, SEXP y);
SEXP ufo_band(SEXP x, SEXP y);
SEXP ufo_or  (SEXP x, SEXP y);
SEXP ufo_and (SEXP x, SEXP y);

SEXP ufo_subset		  (SEXP x, SEXP y);
SEXP ufo_subset_assign(SEXP x, SEXP y, SEXP z);
