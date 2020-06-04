#pragma once

#include "Rinternals.h"

SEXP/*A*/ create_mosaic(SEXP/*A*/ source, SEXP/*INTSXP|REALSXP|LGLSXP*/ indices);

void init_mosaic_altrep_class(DllInfo *dll);
