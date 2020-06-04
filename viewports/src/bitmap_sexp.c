#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

#include <assert.h>
#include <limits.h>

#include "bitmap_sexp.h"

#define hot_many_bits_in_int (sizeof(int) * CHAR_BIT)

SEXP/*INTSXP*/ bitmap_new(R_xlen_t size_in_bits) {
    R_xlen_t size_in_ints = size_in_bits / hot_many_bits_in_int + 1;
    SEXP bitmap = PROTECT(allocVector(INTSXP, size_in_ints));
    SET_TRUELENGTH(bitmap, size_in_bits);
    for (R_xlen_t i =  0; i < size_in_ints; i++) {
        SET_INTEGER_ELT(bitmap, i, 0);
    }
    UNPROTECT(1);
    return bitmap;
}

void bitmap_set(SEXP/*INTSXP*/ bitmap, R_xlen_t which_bit) {
    assert(TYPEOF(bitmap) == INTSXP);
    int *words = (int *) INTEGER(bitmap);
    R_xlen_t which_word = ((which_bit) / hot_many_bits_in_int);
    int mask = (1 << ((which_bit) % hot_many_bits_in_int));
    words[which_word] |= mask;
}

void bitmap_reset(SEXP bitmap, R_xlen_t which_bit) {
    assert(TYPEOF(bitmap) == INTSXP);
    int *words = (int *) INTEGER(bitmap);
    R_xlen_t which_word = ((which_bit) / hot_many_bits_in_int);
    int mask = (1 << ((which_bit) % hot_many_bits_in_int));
    words[which_word] &= ~mask;
}

bool bitmap_get(SEXP bitmap, R_xlen_t which_bit) {
    assert(TYPEOF(bitmap) == INTSXP);
    int *words = (int *) INTEGER(bitmap);
    R_xlen_t which_word = ((which_bit) / hot_many_bits_in_int);
    int mask = (1 << ((which_bit) % hot_many_bits_in_int));
    return 0 != (words[which_word] & mask);
}

R_xlen_t bitmap_count_set_bits(SEXP bitmap) {
    R_xlen_t set_bits = 0;
    for (R_xlen_t i = 0; i < TRUELENGTH(bitmap); i++) {
        if (bitmap_get(bitmap, i)) {
            set_bits++;
        }
    }
    return set_bits;
}

SEXP/*INTSXP*/ bitmap_clone(SEXP/*INTSXP*/ source) {
    assert(TYPEOF(source) == INTSXP);

    R_xlen_t how_many_ints = XLENGTH(source);
    R_xlen_t how_many_bits = XTRUELENGTH(source);

    SEXP/*INTSXP*/ target = bitmap_new(how_many_bits);
    for (R_xlen_t i = 0; i < how_many_ints; i++) {
        SET_INTEGER_ELT(target, i, INTEGER_ELT(source, i));
    }
    return target;
}

R_xlen_t bitmap_index_of_nth_set_bit (SEXP/*INTSXP*/ bitmap, R_xlen_t which_bit) {
    assert(TYPEOF(bitmap) == INTSXP);

    R_xlen_t how_many_bits = XTRUELENGTH(bitmap);
    assert(which_bit < how_many_bits);

    R_xlen_t ones_encountered_so_far = 0;
    for (R_xlen_t i = 0; i < how_many_bits; i++) {
        if (bitmap_get(bitmap, i)) {
            if (ones_encountered_so_far == which_bit) {
                return i;
            }
            ones_encountered_so_far++;
        }
    }

    assert(false);
    return 0;
}