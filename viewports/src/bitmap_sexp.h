#pragma once

#include <R.h>
#include <stdbool.h>

SEXP/*INTSXP*/  bitmap_new                  (R_xlen_t size_in_bits);
void            bitmap_set                  (SEXP/*INTSXP*/ bitmap, R_xlen_t which_bit);
void            bitmap_reset                (SEXP/*INTSXP*/ bitmap, R_xlen_t which_bit);
bool            bitmap_get                  (SEXP/*INTSXP*/ bitmap, R_xlen_t which_bit);
SEXP/*INTSXP*/  bitmap_clone                (SEXP/*INTSXP*/ source);
R_xlen_t        bitmap_count_set_bits       (SEXP/*INTSXP*/ bitmap);
R_xlen_t        bitmap_index_of_nth_set_bit (SEXP/*INTSXP*/ bitmap, R_xlen_t which_bit);
