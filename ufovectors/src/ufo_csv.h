#pragma once

#include "Rinternals.h"

SEXP ufo_csv(SEXP/*STRSXP*/ path,
             SEXP/*INTSXP*/ min_load_count,
             SEXP/*LGLSXP*/ headers,
             SEXP/*INTSXP*/ record_row_offsets_at_interval,
             SEXP/*INTSXP*/ initial_buffer_size);