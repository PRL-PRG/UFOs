#pragma once

#include "Rinternals.h"
#include "debug.h"

void init_seq_integer_altrep_class(DllInfo * dll);
void init_seq_numeric_altrep_class(DllInfo * dll);

SEXP/*INTSXP*/  seq_intsxp_new (SEXP/*INTXP*/ from, SEXP/*INTXP*/ to, SEXP/*INTXP*/ by);
SEXP/*REALSXP*/ seq_realsxp_new(SEXP/*INTXP*/ from, SEXP/*INTXP*/ to, SEXP/*INTXP*/ by);

