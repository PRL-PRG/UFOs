#pragma once

#define USE_RINTERNALS

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rallocators.h>

SEXP allocMatrix3(SEXPTYPE mode, int nrow, int ncol, R_allocator_t *allocator);
