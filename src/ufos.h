#pragma once

#include <R.h>
#include <Rinternals.h>

// Initialization
void ufo_initialize();

// Constructor
SEXP/*INTSXP|VECSXP<INTSXP>*/ ufo_new(SEXP/*EXTPTRSXP*/ source);

// shutdown
void ufo_shutdown();