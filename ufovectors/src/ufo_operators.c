#include "ufo_operators.h"

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Itermacros.h>

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
	if (x == LGLSXP)	return LGLSXP;
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

#define MAX(x, y) (x >= y ? x : y)
#define MIN(x, y) (x >= y ? y : x)

SEXP uncompact_intrange(R_xlen_t start, R_xlen_t end, R_xlen_t size) {
	make_sure(start > 0, Rf_error, "Uncompact intrange start must be > 0");
	make_sure(end >= start, Rf_error, "Uncompact intrange end must be >= start");

	R_xlen_t length = end - start + 1;
	make_sure(size >= length, Rf_error, "Uncompact intrange size must be >= end - start + 1");

	SEXP vector = PROTECT(allocVector(INTSXP, size));
	for (R_xlen_t i = 0; i < length; i++) {
		SET_INTEGER_ELT(vector, i, start + i);
	}

	UNPROTECT(1);
	return vector;
}

SEXP uncompact_intrange_fill(SEXP vector, R_xlen_t start, R_xlen_t end) {
	make_sure(start > 0, Rf_error, "Uncompact intrange start must be > 0");
	make_sure(end >= start, Rf_error, "Uncompact intrange end must be >= start");

	R_xlen_t length = XLENGTH(vector);
	R_xlen_t fill_length = end - start + 1;
	make_sure(fill_length <= length, Rf_error, "Uncompact intrange size must be >= end - start + 1");

	for (R_xlen_t i = 0; i < fill_length; i++) {
		SET_INTEGER_ELT(vector, length - fill_length + i, start + i);
	}

	return vector;
}

SEXP ufo_get_chunk(SEXP x, SEXP chunk_sexp, SEXP chunk_size_sexp, SEXP result_length_sexp) {
	R_xlen_t x_length          = XLENGTH(x);
	R_xlen_t result_length     = __extract_R_xlen_t_or_die(result_length_sexp);
	R_xlen_t chunk_number      = __extract_R_xlen_t_or_die(chunk_sexp);
	R_xlen_t chunk_size        = __extract_R_xlen_t_or_die(chunk_size_sexp);

	R_xlen_t chunk_start_index = chunk_number * chunk_size;
	R_xlen_t actual_chunk_size = chunk_start_index + chunk_size >= result_length
			                   ? result_length - chunk_start_index
			                   : chunk_size;

	SEXP chunk = PROTECT(allocVector(TYPEOF(x), actual_chunk_size));
	for (R_xlen_t i = 0; i < actual_chunk_size; i++) { // TODO maybe regions and/or memcpy
		R_xlen_t ti = (chunk_start_index + i) % x_length;

		switch(TYPEOF(x)) {
		case INTSXP:  SET_INTEGER_ELT(chunk, i, INTEGER_ELT(x, ti)); break;
		case REALSXP: SET_REAL_ELT   (chunk, i, REAL_ELT   (x, ti)); break;
		case LGLSXP:  SET_LOGICAL_ELT(chunk, i, LOGICAL_ELT(x, ti)); break;
		case CPLXSXP: SET_COMPLEX_ELT(chunk, i, COMPLEX_ELT(x, ti)); break;
		case STRSXP:  SET_STRING_ELT (chunk, i, STRING_ELT (x, ti)); break;
		case RAWSXP:  SET_RAW_ELT    (chunk, i, RAW_ELT    (x, ti)); break;
		case VECSXP:  SET_VECTOR_ELT (chunk, i, VECTOR_ELT (x, ti)); break;
		default:      Rf_error("Cannot get a chunk of SEXP of this type");
		}
	}

	R_xlen_t R_chunk_start_index = chunk_start_index + 1;
	R_xlen_t R_chunk_end_index   = chunk_start_index + actual_chunk_size;

	bool R_chunk_start_index_fits_in_int = R_chunk_start_index > INT_MAX;
	bool R_chunk_end_index_fits_in_int   = R_chunk_end_index   > INT_MAX;

	SEXP start_value;
	if (R_chunk_start_index_fits_in_int) {
		start_value = PROTECT(allocVector(INTSXP , 1));
		SET_INTEGER_ELT(start_value, 0, R_chunk_start_index);
	} else {
		start_value = PROTECT(allocVector(REALSXP , 1));
		SET_REAL_ELT(start_value, 0, R_chunk_start_index);
	}

	SEXP end_value;
	if (R_chunk_end_index_fits_in_int) {
		end_value = PROTECT(allocVector(INTSXP , 1));
		SET_INTEGER_ELT(end_value, 0, R_chunk_end_index);
	} else {
		end_value = PROTECT(allocVector(REALSXP , 1));
		SET_REAL_ELT(end_value, 0, R_chunk_end_index);
	}

	setAttrib(chunk, install("start_index"), start_value);
	setAttrib(chunk, install("end_index"),   end_value);

	UNPROTECT(3);
	return chunk;
}

SEXP __ufo_calculate_chunk_indices(SEXP x_length_sexp, SEXP y_length_sexp, SEXP chunk_sexp, SEXP chunk_size_sexp) {

	R_xlen_t x_length   = XLENGTH(x_length_sexp);
	R_xlen_t y_length   = XLENGTH(y_length_sexp);
	R_xlen_t chunk      = __extract_R_xlen_t_or_die(chunk_sexp);
	R_xlen_t chunk_size = __extract_R_xlen_t_or_die(chunk_size_sexp);

	bool x_is_bigger = x_length >= y_length ? x_length : y_length;
	bool y_is_bigger = !x_is_bigger;

	R_xlen_t bigger_vector_size  			= x_is_bigger ? x_length : y_length;
	R_xlen_t smaller_vector_size 			= y_is_bigger ? x_length : y_length;

	R_xlen_t bigger_chunk_beginning         = (chunk - 1) * chunk_size + 1;
	R_xlen_t bigger_chunk_expected_ending   = bigger_chunk_beginning + chunk_size - 1;
	R_xlen_t bigger_chunk_ending            = MIN(bigger_chunk_expected_ending, bigger_vector_size);
	R_xlen_t actual_chunk_size              = bigger_chunk_ending - bigger_chunk_beginning + 1;

	R_xlen_t smaller_chunk_beginning        = (bigger_chunk_beginning - 1) % smaller_vector_size + 1;
	R_xlen_t smaller_chunk_expected_ending  = smaller_chunk_beginning + actual_chunk_size - 1;
	R_xlen_t smaller_chunk_ending           = MIN(smaller_chunk_expected_ending, smaller_vector_size);

	R_xlen_t smaller_chunk_has_runoff       = smaller_chunk_expected_ending > smaller_vector_size;
	R_xlen_t smaller_chunk_runoff_beginning = 1;
	R_xlen_t smaller_chunk_runoff_ending    = smaller_chunk_expected_ending - smaller_vector_size;

	SEXP bigger_range   = PROTECT(uncompact_intrange(bigger_chunk_beginning, bigger_chunk_ending, actual_chunk_size));
	SEXP smaller_range  = PROTECT(uncompact_intrange(smaller_chunk_beginning, smaller_chunk_ending, actual_chunk_size));

	if (smaller_chunk_has_runoff) {
		smaller_range = uncompact_intrange_fill(smaller_range, smaller_chunk_runoff_beginning, smaller_chunk_runoff_ending);
	}

	SEXP result = PROTECT(allocVector(VECSXP, 3));
	SET_VECTOR_ELT(result, 0, x_is_bigger ? bigger_range  : smaller_range);
	SET_VECTOR_ELT(result, 1, y_is_bigger ? bigger_range  : smaller_range);
	SET_VECTOR_ELT(result, 2, bigger_range);

	UNPROTECT(3);
	return result;
}

R_xlen_t integer_subscript_length(SEXP vector, SEXP subscript) {
	R_xlen_t nas = 0, negatives = 0, positives = 0;
	R_xlen_t vector_length = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);

	for (R_xlen_t i = 0; i < subscript_length; i++) {
		int value = INTEGER_ELT(subscript, i);
		if (value == NA_INTEGER) nas++;
		else if (value < 0)		 negatives++;
		else if (value > 0)      positives++;
								 // ignore zeros
	}

	if (positives > 0) return positives + nas;
	if (positives > 0 || nas > 0) {
		Rf_error("only 0s may be mixed with negative subscripts");
	}
	return vector_length - negatives;
}

R_xlen_t real_subscript_length(SEXP vector, SEXP subscript) {
	R_xlen_t nas = 0;
	R_xlen_t negatives = 0;
	R_xlen_t positives = 0;
	R_xlen_t between_zero_and_ones = 0;
	R_xlen_t vector_length = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);

	for (R_xlen_t i = 0; i < subscript_length; i++) {
		double value = REAL_ELT(subscript, i);
		if (ISNAN(value)) 	    nas++;
		else if (value < 0)	    negatives++;
		else if (value >= 1)    positives++;
		else if (value < 1)     between_zero_and_ones++;
							    // ignore zeros
	}

	if (positives > 0) return positives + nas;
	if (positives > 0 || nas > 0 || between_zero_and_ones > 0) {
		Rf_error("only 0s may be mixed with negative subscripts");
	}
	return vector_length - negatives;
}

R_xlen_t null_subscript_length(SEXP vector, SEXP subscript) {
	return 0;
}

R_xlen_t logical_subscript_length(SEXP vector, SEXP subscript) {
	R_xlen_t vector_length = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);

	if (vector_length <= subscript_length) {
		R_xlen_t elements = subscript_length;
		for (R_xlen_t i = 0; i < subscript_length; i++) {
			if (LOGICAL_ELT(subscript, i) == 0) {
				elements--;
			}
		}
		return elements;
	}

	if (subscript_length == 1) return LOGICAL_ELT(subscript, 0) == 0 ? 0 : 1;
	if (subscript_length == 0) return 0;

	R_xlen_t whole_fits_this_many_times = vector_length / subscript_length;
	R_xlen_t remainder_fits_this_many_elements = vector_length % subscript_length;
	R_xlen_t elements_in_whole = subscript_length;
	R_xlen_t elements_in_remainder = remainder_fits_this_many_elements;

	for (R_xlen_t i = 0; i < subscript_length; i++) {
		if (LOGICAL_ELT(subscript, i) == 0) {
			elements_in_whole--;
			if (i < remainder_fits_this_many_elements) {
				elements_in_remainder--;
			}
		}
	}

	return whole_fits_this_many_times * elements_in_whole + elements_in_remainder;
}

R_xlen_t string_subscript_length(SEXP vector, SEXP subscript) {
	Rf_error("Not implemented yet.");
}

R_xlen_t ufo_subscript_dimension_length(SEXP vector, SEXP subscript) {

	make_sure(isVector(vector) || isList(vector) || isLanguage(vector), Rf_error, "subscripting on non-vector");

	SEXPTYPE subscript_type = TYPEOF(subscript);

	switch(subscript_type) {
	case INTSXP:  return integer_subscript_length(vector, subscript);
	case REALSXP: return real_subscript_length(vector, subscript);
	case NILSXP:  return null_subscript_length(vector, subscript);
	case LGLSXP:  return logical_subscript_length(vector, subscript);
	case STRSXP:  return string_subscript_length(vector, subscript);
	default:      Rf_error("invalid subscript type '%s'", type2char(subscript_type));
	}

	Rf_error("unreachable");
	return 0;
}

SEXP null_subscript(SEXP vector, SEXP subscript) {
	return allocVector(TYPEOF(vector), 0);
}

SEXP logical_subscript(SEXP vector, SEXP subscript) {

	R_xlen_t vector_length = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);
	SEXPTYPE vector_type = TYPEOF(vector);

	if (subscript_length == 0) {
		return allocVector(vector_type, 0);
	}

	R_xlen_t result_length = logical_subscript_length(vector, subscript); // FIXME makes sure used only once
	bool result_vector_is_long = result_length > R_SHORT_LEN_MAX;
	SEXP result = PROTECT(allocVector(result_vector_is_long ? REALSXP : INTSXP, result_length));

	// Subscript is equal to or larger than vector.
	if (vector_length <= subscript_length) {
		R_xlen_t ri = 0;

		for (R_xlen_t vi = 0; vi < vector_length; vi++) { // XXX consider iterating by region
			Rboolean value = LOGICAL_ELT(subscript, vi);

			if (value == FALSE)	       continue;
			if (result_vector_is_long) SET_REAL_ELT(result, ri, value == TRUE ? vi : NA_REAL);
			else         		       SET_INTEGER_ELT(result, ri, value == TRUE ? vi : NA_INTEGER);

			ri++;
		}

		for (R_xlen_t si = vector_length; si < subscript_length; si++) {
			if (result_vector_is_long) SET_REAL_ELT(result, ri, NA_REAL);
			else         		       SET_INTEGER_ELT(result, ri, NA_INTEGER);

			ri++;
		}

		UNPROTECT(1);
		return result;
	}

	// Subscript is shorter than the vector, recycling occurs (nested or otherwise)
	if (vector_length > subscript_length) {
		for (R_xlen_t si = 0, ri = 0; si < vector_length; si++) { // XXX consider iterating by region
			Rboolean value = LOGICAL_ELT(subscript, si % subscript_length);

			if (value == FALSE)		   continue;
			if (result_vector_is_long) SET_REAL_ELT(result, ri, value == TRUE ? si : NA_REAL);
			else         		       SET_INTEGER_ELT(result, ri, value == TRUE ? si : NA_INTEGER);

			ri++;
		}

		UNPROTECT(1);
		return result;
	}

	Rf_error("unreachable");
	return R_NilValue;
}

SEXP integer_subscript(SEXP vector, SEXP subscript) {

	R_xlen_t vector_length = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);

	if (subscript_length == 0) {
		return allocVector(INTSXP, 0);
	}

	Rf_error("not implemented");
	return R_NilValue;
}

SEXP real_subscript(SEXP vector, SEXP subscript) {

	R_xlen_t vector_length = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);

	if (subscript_length == 0) {
		return allocVector(INTSXP, 0);
	}

	Rf_error("not implemented");
	return R_NilValue;
}

SEXP string_subscript(SEXP vector, SEXP subscript) {

	R_xlen_t vector_length = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);

	if (subscript_length == 0) {
		return allocVector(INTSXP, 0);
	}

	Rf_error("not implemented");
	return R_NilValue;
}


SEXP ufo_subscript(SEXP vector, SEXP subscript) {

	make_sure(isVector(vector) || isList(vector) || isLanguage(vector), Rf_error, "subscripting on non-vector");

	SEXPTYPE subscript_type = TYPEOF(subscript);
	R_xlen_t vector_length = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);

	switch (subscript_type) {
	case NILSXP:  return null_subscript(vector, subscript);
	case LGLSXP:  return logical_subscript(vector, subscript);
	case INTSXP:  return integer_subscript(vector, subscript);
	case REALSXP: return real_subscript(vector, subscript);
	case STRSXP:  return string_subscript(vector, subscript);
	default:      Rf_error("invalid subscript type '%s'", type2char(subscript_type));
	}

	Rf_error("unreachable");
	return R_NilValue;
}

SEXP ufo_subset(SEXP x, SEXP y) {
	return R_NilValue;
}

SEXP ufo_subset_assign(SEXP x, SEXP y, SEXP z) {
	Rf_error("not implemented");
	return R_NilValue;
}

