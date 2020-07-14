#include "ufo_operators.h"

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

#include "make_sure.h"

typedef enum {UFO_ADDITION, UFO_SUBTRACTION, UFO_MULTIPLICATION, UFO_DIVISION} ufo_operations_t;

SEXP make_scalar_vector_for_result(SEXPTYPE type, SEXP x, SEXP y) {
    if (NO_REFERENCES(x)) return x;
    if (NO_REFERENCES(y)) return y;

	return allocVector(type, 1);
}

//// This is not optimized for scalars under the assumption that length-1 UFOs are impossible.
//SEXP ufo_binary_arithmetic(SEXP x, SEXP y) {
//	make_sure((TYPEOF(x) == INTSXP || TYPEOF(x) == REALSXP) &&
//			  (TYPEOF(y) == INTSXP || TYPEOF(y) == REALSXP),
//			  Rf_error, "Both arguments must be either INTSXPs or REALSXPs.");
//
//	if (IS_SCALAR(x, REALSXP) && IS_SCALAR(y, REALSXP)) {
//		// This should not ever happen, since length-1 UFOs cannot be created.
//		SEXP result = make_scalar_vector_for_result(REALSXP, x, y);
//
//		double x_value = SCALAR_DVAL(x);
//		double y_value = SCALAR_DVAL(y);
//
//
//	}
//
//	if (IS_SCALAR(x, REALSXP) || IS_SCALAR(y, REALSXP)) {
//		SEXP scalar = IS_SCALAR(x, REALSXP) ? x : y;
//		SEXP vector = IS_SCALAR(x, REALSXP) ? x : y;
//
//
//	}
//
//	return R_NilValue;
//}
//
//SEXP ufo_real_binary(ufo_operations_t operation, SEXP x, SEXP y, R_alloc_t result_allocator) {
//	R_xlen_t x_length = XLENGTH(x);
//	R_xlen_t y_length = XLENGTH(y);
//	R_xlen_t length = (x_length > y_length) ? x_length : y_length;
//
//	if (x_length == 0 || y_length == 0) {
//		return(allocVector3(REALSXP, 0, ));
//	}
//
//
//
//}

SEXP ufo_add (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_sub (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_mul (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_div (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_pow (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_mod (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_idiv(SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_lt  (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_le  (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_gt  (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_ge  (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_eq  (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_neq (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_neg (SEXP x) {
	return R_NilValue;
}

SEXP ufo_bor (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_band(SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_or  (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_and (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_subset		  (SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_subset_assign(SEXP x, SEXP y, SEXP z) {
	return R_NilValue;
}
