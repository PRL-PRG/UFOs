#pragma once

#include <R.h>
#include <Rinternals.h>
#include <stdbool.h>

typedef enum {
    VIEWPORT_SLICE,
    VIEWPORT_MOSAIC,
    VIEWPORT_PRISM,
    VIEWPORT_NONE,
} viewport_type_t;

viewport_type_t recommend_vieport_type_for_indices(SEXP/*INTSXP | REALSXP*/ indices);

bool are_indices_contiguous(SEXP/*INTSXP | REALSXP*/ indices);
bool are_indices_monotonic (SEXP/*INTSXP | REALSXP*/ indices);
bool do_indices_contain_NAs(SEXP/*INTSXP | REALSXP*/ indices);

R_xlen_t get_first_element_as_length(SEXP/*INTSXP | REALSXP*/ indices);
void copy_element(SEXP source, R_xlen_t source_index, SEXP target, R_xlen_t target_index);
void set_element_to_NA(SEXP target, R_xlen_t target_index);
SEXP copy_data_at_mask(SEXP source, SEXP/*LGLSXP*/ mask);
SEXP copy_data_at_indices(SEXP source, SEXP/*INTSXP | REALSXP*/ indices);
SEXP copy_data_in_range(SEXP source, R_xlen_t start, R_xlen_t size);