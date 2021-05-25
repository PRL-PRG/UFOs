#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include "../include/ufos.h"

int __extract_int_or_die(SEXP/*INTSXP*/ sexp);
int __extract_boolean_or_die(SEXP/*LGLSXP*/ sexp);
const char* __extract_path_or_die(SEXP/*STRSXP*/ path);
size_t __get_element_size(SEXPTYPE vector_type);
int32_t __select_min_load_count(int32_t min_load_count, size_t element_size);
int32_t __1MB_of_elements(size_t element_size);
R_xlen_t __extract_R_xlen_t_or_die(SEXP/*REALSXP*/ sexp);