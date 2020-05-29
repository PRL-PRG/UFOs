#pragma once

#include "Rinternals.h"

SEXP/*NILSXP*/ create_slice(SEXP, SEXP/*INTSXP|REALSXP*/ start, SEXP/*INTSXP|REALSXP*/ size);

void init_slice_altrep_class(DllInfo *dll);
