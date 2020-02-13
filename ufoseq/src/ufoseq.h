#pragma once
#include "Rinternals.h"

typedef struct {
    int from;
    int to;
    int by;
} ufo_seq_data_t;

SEXP/*INTXP*/ ufo_seq(SEXP/*INTXP*/ from, SEXP/*INTXP*/ to, SEXP/*INTXP*/ by);
