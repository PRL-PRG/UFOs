#pragma once

#include "Rinternals.h"

SEXP/*NILSXP*/ create_prism(SEXP, SEXP/*INTSXP|REALSXP*/ indices);

void init_prism_altrep_class(DllInfo *dll);
