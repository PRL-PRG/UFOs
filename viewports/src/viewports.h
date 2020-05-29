#pragma once

#include "Rinternals.h"

SEXP/*NILSXP*/ set_debug_mode(SEXP/*LGLSXP*/);
SEXP/*NILSXP*/ create_viewport(SEXP, SEXP/*INTSXP|REALSXP*/ start, SEXP/*INTSXP|REALSXP*/ size);

void init_viewport_altrep_class(DllInfo *dll);
