#include "ufo_operators.h"

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

#include "make_sure.h"
#include "ufo_empty.h"

// Good for: + - * / ^ < > <= >= == != | &
R_xlen_t ufo_vector_size_to_fit_both(SEXPTYPE x_type, SEXPTYPE y_type, R_xlen_t x_length, R_xlen_t y_length) {
	if (x_type == NILSXP || y_type == NILSXP) {
		return 0;
	}

	if ((x_type == REALSXP || x_type == INTSXP || x_type == LGLSXP || x_type == NILSXP || x_type == CPLXSXP)
		&& (y_type == REALSXP || y_type == INTSXP || y_type == LGLSXP || y_type == NILSXP || y_type == CPLXSXP))  {
		return x_length >= y_length ? x_length : y_length;
	}

	Rf_error("operation is not supported");
	return 0; // Mollifies linters.
}

// Good for: %% %/%
R_xlen_t ufo_vector_size_to_mod_both(SEXPTYPE x_type, SEXPTYPE y_type, R_xlen_t x_length, R_xlen_t y_length) {
	if (x_type == NILSXP || y_type == NILSXP) {
		return 0;
	}

	if ((x_type == REALSXP || x_type == INTSXP || x_type == LGLSXP || x_type == NILSXP)
		&& (y_type == REALSXP || y_type == INTSXP || y_type == LGLSXP || y_type == NILSXP))  {
		return x_length >= y_length ? x_length : y_length;
	}

	if (x_type == CPLXSXP && y_type == CPLXSXP) {
		return x_length >= y_length ? x_length : y_length;
	}

	Rf_error("operation is not supported");
	return 0; // Mollifies linters.
}

// Good for: + - *
SEXPTYPE ufo_vector_type_to_fit_both(SEXPTYPE x, SEXPTYPE y) {
	if (x == REALSXP) {
		if (y == REALSXP) 	return REALSXP;
		if (y == INTSXP) 	return REALSXP;
		if (y == LGLSXP)	return REALSXP;
		if (y == NILSXP)	return REALSXP;
		if (y == CPLXSXP)	return CPLXSXP;

		Rf_error("operation is not supported");
	}

	if (x == INTSXP || x == LGLSXP || x == NILSXP) {
		if (y == REALSXP) 	return REALSXP;
		if (y == INTSXP) 	return INTSXP;
		if (y == LGLSXP)	return INTSXP;
		if (y == NILSXP)	return INTSXP;
		if (y == CPLXSXP)	return CPLXSXP;

		Rf_error("operation is not supported");
	}

	if (x == CPLXSXP) {
		if (y == REALSXP) 	return CPLXSXP;
		if (y == INTSXP) 	return CPLXSXP;
		if (y == LGLSXP)	return CPLXSXP;
		if (y == NILSXP)	return CPLXSXP;
		if (y == CPLXSXP)	return CPLXSXP;

		Rf_error("operation is not supported");
	}

	Rf_error("operation is not supported");
	return 0; // Mollifies linters.
}

// Good for: + - *
SEXPTYPE ufo_vector_type_to_neg(SEXPTYPE x) {
	if (x == REALSXP) 	return REALSXP;
	if (x == INTSXP) 	return INTSXP;
	if (x == LGLSXP)	return INTSXP;
	if (x == CPLXSXP)	return CPLXSXP;

	Rf_error("operation is not supported");
	return 0; // Mollifies linters.
}

// Good for: / ^
SEXPTYPE ufo_vector_type_to_div_both(SEXPTYPE x, SEXPTYPE y) {
	if (x == REALSXP || x == INTSXP || x == LGLSXP || x == NILSXP) {
		if (y == REALSXP) 	return REALSXP;
		if (y == INTSXP) 	return REALSXP;
		if (y == LGLSXP)	return REALSXP;
		if (y == NILSXP)	return REALSXP;
		if (y == CPLXSXP)	return CPLXSXP;

		Rf_error("operation is not supported");
	}

	if (x == CPLXSXP) {
		if (y == REALSXP) 	return CPLXSXP;
		if (y == INTSXP) 	return CPLXSXP;
		if (y == LGLSXP)	return CPLXSXP;
		if (y == NILSXP)	return CPLXSXP;
		if (y == CPLXSXP)	return CPLXSXP;

		Rf_error("operation is not supported");
	}

	Rf_error("operation is not supported");
	return 0; // Mollifies linters.
}

// Good for: %% %/%
SEXPTYPE ufo_vector_type_to_mod_both(SEXPTYPE x, SEXPTYPE y) {
	if (x == REALSXP) {
		if (y == REALSXP) 	return REALSXP;
		if (y == INTSXP) 	return REALSXP;
		if (y == LGLSXP)	return REALSXP;
		if (y == NILSXP)	return REALSXP;

		Rf_error("operation is not supported");
	}

	if (x == INTSXP || x == LGLSXP) {
		if (y == REALSXP) 	return REALSXP;
		if (y == INTSXP) 	return INTSXP;
		if (y == LGLSXP)	return INTSXP;
		if (y == NILSXP)	return INTSXP;

		Rf_error("operation is not supported");
	}

	if (x == NILSXP) {
		if (y == REALSXP) 	return REALSXP;
		if (y == INTSXP) 	return INTSXP;
		if (y == LGLSXP)	return INTSXP;
		if (y == NILSXP)	return INTSXP;
		if (y == CPLXSXP)	return CPLXSXP;

		Rf_error("operation is not supported");
	}

	if (x == CPLXSXP) {
		if (y == NILSXP)	return CPLXSXP;

		Rf_error("operation is not supported");
	}

	Rf_error("operation is not supported");
	return 0; // Mollifies linters.
}

// Good for: < > <= >=
SEXPTYPE ufo_vector_type_to_rel_both(SEXPTYPE x, SEXPTYPE y) {
	if (x == REALSXP || x == INTSXP || x == LGLSXP) {
		if (y == REALSXP) 	return LGLSXP;
		if (y == INTSXP) 	return LGLSXP;
		if (y == LGLSXP)	return LGLSXP;
		if (y == NILSXP)	return LGLSXP;

		Rf_error("operation is not supported");
	}

	if (x == NILSXP) {
		if (y == REALSXP) 	return LGLSXP;
		if (y == INTSXP) 	return LGLSXP;
		if (y == LGLSXP)	return LGLSXP;
		if (y == NILSXP)	return LGLSXP;
		if (y == CPLXSXP)	return LGLSXP;

		Rf_error("operation is not supported");
	}

	if (x == CPLXSXP) {
		if (y == NILSXP)	return LGLSXP;

		Rf_error("operation is not supported");
	}

	Rf_error("operation is not supported");
	return 0; // Mollifies linters.
}

// Good for: == != | &
SEXPTYPE ufo_vector_type_to_log_both(SEXPTYPE x, SEXPTYPE y) {
	if (x == REALSXP || x == INTSXP || x == LGLSXP || x == NILSXP || x == CPLXSXP) {
		if (y == REALSXP) 	return LGLSXP;
		if (y == INTSXP) 	return LGLSXP;
		if (y == LGLSXP)	return LGLSXP;
		if (y == NILSXP)	return LGLSXP;
		if (y == CPLXSXP)	return LGLSXP;

		Rf_error("operation is not supported");
	}

	Rf_error("operation is not supported");
	return 0; // Mollifies linters.
}

// Good for: + - *
SEXP ufo_fit_result (SEXP x, SEXP y, SEXP min_load_count) {
	SEXPTYPE x_type = TYPEOF(x);
	SEXPTYPE y_type = TYPEOF(y);
	R_xlen_t x_size = XLENGTH(x);
	R_xlen_t y_size = XLENGTH(y);

	SEXPTYPE result_type = ufo_vector_type_to_fit_both(x_type, y_type);
	R_xlen_t result_size = ufo_vector_size_to_fit_both(x_type, y_type, x_size, y_size);

	return ufo_empty(result_type, result_size, __extract_int_or_die(min_load_count));
}

// Good for: / ^
SEXP ufo_div_result (SEXP x, SEXP y, SEXP min_load_count) {
	SEXPTYPE x_type = TYPEOF(x);
	SEXPTYPE y_type = TYPEOF(y);
	R_xlen_t x_size = XLENGTH(x);
	R_xlen_t y_size = XLENGTH(y);

	SEXPTYPE result_type = ufo_vector_type_to_div_both(x_type, y_type);
	R_xlen_t result_size = ufo_vector_size_to_fit_both(x_type, y_type, x_size, y_size);

	return ufo_empty(result_type, result_size, __extract_int_or_die(min_load_count));
}

// Good for: %% %/%
SEXP ufo_mod_result (SEXP x, SEXP y, SEXP min_load_count) {
	SEXPTYPE x_type = TYPEOF(x);
	SEXPTYPE y_type = TYPEOF(y);
	R_xlen_t x_size = XLENGTH(x);
	R_xlen_t y_size = XLENGTH(y);

	SEXPTYPE result_type = ufo_vector_type_to_mod_both(x_type, y_type);
	R_xlen_t result_size = ufo_vector_size_to_mod_both(x_type, y_type, x_size, y_size);

	return ufo_empty(result_type, result_size, __extract_int_or_die(min_load_count));
}

// Good for: < > <= >=
SEXP ufo_rel_result (SEXP x, SEXP y, SEXP min_load_count) {
	SEXPTYPE x_type = TYPEOF(x);
	SEXPTYPE y_type = TYPEOF(y);
	R_xlen_t x_size = XLENGTH(x);
	R_xlen_t y_size = XLENGTH(y);

	SEXPTYPE result_type = ufo_vector_type_to_rel_both(x_type, y_type);
	R_xlen_t result_size = ufo_vector_size_to_fit_both(x_type, y_type, x_size, y_size);

	return ufo_empty(result_type, result_size, __extract_int_or_die(min_load_count));
}

// Good for: == != | &
SEXP ufo_log_result (SEXP x, SEXP y, SEXP min_load_count) {
	SEXPTYPE x_type = TYPEOF(x);
	SEXPTYPE y_type = TYPEOF(y);
	R_xlen_t x_size = XLENGTH(x);
	R_xlen_t y_size = XLENGTH(y);

	SEXPTYPE result_type = ufo_vector_type_to_log_both(x_type, y_type);
	R_xlen_t result_size = ufo_vector_size_to_fit_both(x_type, y_type, x_size, y_size);

	return ufo_empty(result_type, result_size, __extract_int_or_die(min_load_count));
}

// Good for: unary + and -
SEXP ufo_neg_result (SEXP x, SEXP min_load_count) {
	SEXPTYPE x_type = TYPEOF(x);
	R_xlen_t x_size = XLENGTH(x);

	SEXPTYPE result_type = ufo_vector_type_to_neg(x_type);

	return ufo_empty(result_type, x_size, __extract_int_or_die(min_load_count));
}

SEXP ufo_subset(SEXP x, SEXP y) {
	Rf_error("not implemented");
	return R_NilValue;
}

SEXP ufo_subset_assign(SEXP x, SEXP y, SEXP z) {
	Rf_error("not implemented");
	return R_NilValue;
}
