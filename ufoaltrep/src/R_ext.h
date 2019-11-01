#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rallocators.h>

typedef SEXP (*altrep_constructor_t)(SEXPTYPE, R_xlen_t, void *);
SEXP allocMatrix4(SEXPTYPE mode, int nrow, int ncol,
                  altrep_constructor_t altrep_constructor,
                  void *altrep_constructor_data);
