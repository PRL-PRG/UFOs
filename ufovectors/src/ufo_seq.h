#pragma once
#include "Rinternals.h"

#include "../include/ufos.h"

SEXP/*INTSXP*/  ufo_intsxp_seq (                        SEXP/*INTXP*/ from, SEXP/*INTXP*/ to, SEXP/*INTXP*/ by, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);
SEXP/*REALSXP*/ ufo_realsxp_seq(                        SEXP/*INTXP*/ from, SEXP/*INTXP*/ to, SEXP/*INTXP*/ by, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);
SEXP/*<type>*/  ufo_seq        (ufo_vector_type_t type, SEXP/*INTXP*/ from, SEXP/*INTXP*/ to, SEXP/*INTXP*/ by, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);
