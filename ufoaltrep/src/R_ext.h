#pragma once

#define USE_RINTERNALS

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rallocators.h>

SEXP allocMatrix3(SEXPTYPE mode, int nrow, int ncol,
                  altrep_constructor_t *altrep_constructor,
                  void *altrep_constructor_data);
