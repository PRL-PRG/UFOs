#pragma once

#include "Rinternals.h"
#include "debug.h"

void init_ufo_cached_logical_altrep_class(DllInfo *dll);
void init_ufo_cached_integer_altrep_class(DllInfo *dll);
void init_ufo_cached_numeric_altrep_class(DllInfo *dll);
void init_ufo_cached_complex_altrep_class(DllInfo *dll);

void init_ufo_cached_raw_altrep_class(DllInfo *dll);

SEXP/*INTSXP*/ altrep_ufo_cached_vectors_intsxp_bin(SEXP/*STRSXP*/ path,
                                                    SEXP/*INTSXP*/ cache_size,
                                                    SEXP/*INTSXP*/ cache_chunk_size);
SEXP/*REALSXP*/ altrep_ufo_cached_vectors_realsxp_bin(SEXP/*STRSXP*/ path,
                                                      SEXP/*INTSXP*/ cache_size,
                                                      SEXP/*INTSXP*/ cache_chunk_size);
SEXP/*CPLXSXP*/ altrep_ufo_cached_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path,
                                                      SEXP/*INTSXP*/ cache_size,
                                                      SEXP/*INTSXP*/ cache_chunk_size);
SEXP/*LGLSXP*/ altrep_ufo_cached_vectors_lglsxp_bin(SEXP/*STRSXP*/ path,
                                                    SEXP/*INTSXP*/ cache_size,
                                                    SEXP/*INTSXP*/ cache_chunk_size);
SEXP/*RAWSXP*/ altrep_ufo_cached_vectors_rawsxp_bin(SEXP/*STRSXP*/ path,
                                                    SEXP/*INTSXP*/ cache_size,
                                                    SEXP/*INTSXP*/ cache_chunk_size);

//SEXP/*NILSXP*/ ufo_vectors_set_debug_mode(SEXP/*LGLSXP*/ debug);


