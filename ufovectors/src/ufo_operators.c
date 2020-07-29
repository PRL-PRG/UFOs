#include "ufo_operators.h"

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Itermacros.h>

#include "make_sure.h"
#include "ufo_empty.h"

//-----------------------------------------------------------------------------
// Problem children
//-----------------------------------------------------------------------------

#ifndef IS_CACHED
#ifndef CACHED_MASK
#define CACHED_MASK (1<<5)
#endif
#define IS_CACHED(x) (((x)->sxpinfo.gp) & CACHED_MASK)
#endif

#ifndef IS_BYTES
#ifndef BYTES_MASK
#define BYTES_MASK (1<<1)
#endif
#define IS_BYTES(x) ((x)->sxpinfo.gp & BYTES_MASK)
#endif

#ifndef ENC_KNOWN
#ifndef LATIN1_MASK
#define LATIN1_MASK (1<<2)
#endif
#ifndef UTF8_MASK
#define UTF8_MASK (1<<3)
#endif
#define ENC_KNOWN(x) ((x)->sxpinfo.gp & (LATIN1_MASK | UTF8_MASK))
#endif

#ifndef SET_COMPLEX_ELT
void SET_COMPLEX_ELT(SEXP x, R_xlen_t i, Rcomplex v);
#endif

#ifndef SET_RAW_ELT
void SET_RAW_ELT(SEXP x, R_xlen_t i, Rbyte v);
#endif

//-----------------------------------------------------------------------------
// Chunked binary and unary operators
//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------
// Subscript generation
//-----------------------------------------------------------------------------

typedef struct {
	R_xlen_t nas;
	R_xlen_t negatives;
	R_xlen_t positives;
	R_xlen_t zeros;
} integer_vector_stats_t;

integer_vector_stats_t integer_subscript_stats(SEXP subscript) {
	integer_vector_stats_t stats = {
			.nas           = 0,
			.negatives     = 0,
			.positives     = 0,
			.zeros         = 0
	};

	R_xlen_t subscript_length = XLENGTH(subscript);

	for (R_xlen_t i = 0; i < subscript_length; i++) {
		int value = INTEGER_ELT(subscript, i);
		if (value == NA_INTEGER) stats.nas++;
		else if (value < 0)		 stats.negatives++;
		else if (value > 0)      stats.positives++;
		else					 stats.zeros++;
	}

	return stats;
}

R_xlen_t integer_subscript_length(SEXP vector, SEXP subscript) {
	if (XLENGTH(subscript) == 0) {
		return 0;
	}

	integer_vector_stats_t stats = integer_subscript_stats(subscript);

	if (stats.negatives == 0) {
		return stats.positives + stats.nas;
	}

	if (stats.positives == 0 && stats.nas == 0) {
		return XLENGTH(vector) - stats.negatives;
	}

	Rf_error("only 0s may be mixed with negative subscripts");
	return 0; // unreachable
}

typedef struct {
	R_xlen_t nas;
	R_xlen_t negatives;
	R_xlen_t positives;
	R_xlen_t zeros;
	R_xlen_t between_zero_and_ones;
} real_vector_stats_t;

real_vector_stats_t real_subscript_stats(SEXP subscript) {
	R_xlen_t subscript_length = XLENGTH(subscript);
	real_vector_stats_t stats = {
			.nas = 0,
			.negatives = 0,
			.positives = 0,
			.between_zero_and_ones = 0
	};

	for (R_xlen_t i = 0; i < subscript_length; i++) {
		double value = REAL_ELT(subscript, i);
		if (ISNAN(value)) 	    stats.nas++;
		else if (value < 0)	    stats.negatives++;
		else if (value >= 1)    stats.positives++;
		else if (value < 1)     stats.between_zero_and_ones++;
							    // ignore zeros
	}

	return stats;
}

R_xlen_t real_subscript_length(SEXP vector, SEXP subscript) {
	real_vector_stats_t stats = real_subscript_stats(subscript);

	if (stats.negatives == 0) return stats.positives + stats.nas;
	if (stats.positives > 0 || stats.nas > 0 || stats.between_zero_and_ones > 0) {
		Rf_error("only 0s may be mixed with negative subscripts");
	}

	return XLENGTH(vector) - stats.negatives;
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

R_xlen_t ufo_subscript_dimension_length(SEXP vector, SEXP subscript, SEXP min_load_count_sexp) {
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

SEXP null_subscript(SEXP vector, SEXP subscript, int32_t min_load_count) {
	return allocVector(TYPEOF(vector), 0);
}

SEXP logical_subscript(SEXP vector, SEXP subscript, int32_t min_load_count) {

	R_xlen_t vector_length    = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);
	SEXPTYPE vector_type      = TYPEOF(vector);

	if (subscript_length == 0) {
		return allocVector(vector_type, 0);
	}

	R_xlen_t result_length         = logical_subscript_length(vector, subscript); // FIXME makes sure used only once
	bool     result_vector_is_long = result_length > R_SHORT_LEN_MAX;
	SEXP     result                = PROTECT(ufo_empty(result_vector_is_long ? REALSXP : INTSXP, result_length, min_load_count));
	R_xlen_t result_index          = 0;

	for (R_xlen_t vector_index = 0; vector_index < vector_length; vector_index++) { // XXX consider iterating by region
		Rboolean value = LOGICAL_ELT(subscript, vector_index % subscript_length);

		if (value == FALSE)		   continue;
		if (result_vector_is_long) SET_REAL_ELT   (result, result_index, value == TRUE ? vector_index + 1: NA_REAL);
		else             		   SET_INTEGER_ELT(result, result_index, value == TRUE ? vector_index + 1: NA_INTEGER);

		result_index++;
	}

	for (R_xlen_t si = vector_length; si < subscript_length; si++) {
		if (result_vector_is_long) SET_REAL_ELT(result, result_index, NA_REAL);
		else         		       SET_INTEGER_ELT(result, result_index, NA_INTEGER);

		result_index++;
	}

	UNPROTECT(1);
	return result;
}

SEXP positive_integer_subscript(SEXP vector, SEXP subscript, int32_t min_load_count, integer_vector_stats_t stats) {

	R_xlen_t result_length = stats.positives + stats.nas;
	R_xlen_t result_vector_is_long = result_length > R_SHORT_LEN_MAX;
	SEXPTYPE result_type = result_vector_is_long ? REALSXP : INTSXP;
	R_xlen_t subscript_length = XLENGTH(subscript);
	R_xlen_t vector_length = XLENGTH(vector);

	SEXP result = PROTECT(ufo_empty(result_type, result_length, min_load_count));
	for (R_xlen_t result_index = 0, subscript_index = 0; subscript_index < subscript_length; subscript_index++) {
		int value = INTEGER_ELT(subscript, subscript_index);

		if (value == 0) continue;

		bool na = (value == NA_INTEGER || value > vector_length);
		if (result_vector_is_long) SET_REAL_ELT   (result, result_index, na ? NA_REAL    : (double) value);
		else             		   SET_INTEGER_ELT(result, result_index, na ? NA_INTEGER :          value);

		result_index++;
	}
	UNPROTECT(1);
	return result;
}

// XXX There **has** to be a better way to do this.
SEXP negative_integer_subscript(SEXP vector, SEXP subscript, int32_t min_load_count, integer_vector_stats_t stats) {
	R_xlen_t vector_length = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);
	SEXP bitmap = PROTECT(ufo_empty(LGLSXP, vector_length, min_load_count));

	for (R_xlen_t vector_index = 0; vector_index < vector_length; vector_index++) {
		SET_LOGICAL_ELT(bitmap, vector_index, TRUE);
	}

	for (R_xlen_t subscript_index = 0; subscript_index < subscript_length; subscript_index++) {
		int subscript_value = INTEGER_ELT(subscript, subscript_index);
		if (subscript_value == 0) 		  	  continue;
		if (subscript_value == NA_INTEGER)    continue;
		if (-subscript_value > vector_length) continue;
		SET_LOGICAL_ELT(bitmap, -subscript_value - 1, FALSE);
	}

	SEXP indices = logical_subscript(vector, bitmap, min_load_count);
	UNPROTECT(1);
	return indices;
}

SEXP integer_subscript(SEXP vector, SEXP subscript, int32_t min_load_count) {
	integer_vector_stats_t stats = integer_subscript_stats(subscript);

	if (stats.nas + stats.positives + stats.negatives == 0) {
		return allocVector(INTSXP, 0);
	}

	if (stats.negatives > 0 && (stats.positives > 0 || stats.nas > 0)) {
		Rf_error("only 0s may be mixed with negative subscripts");
	}

	if (stats.negatives != 0) {
		return negative_integer_subscript(vector, subscript, min_load_count, stats);
	} else {
		return positive_integer_subscript(vector, subscript, min_load_count, stats);
	}
}

SEXP positive_real_subscript(SEXP vector, SEXP subscript, int32_t min_load_count, real_vector_stats_t stats) {

	R_xlen_t result_length = stats.positives + stats.nas;
	R_xlen_t result_vector_is_long = result_length > R_SHORT_LEN_MAX;
	SEXPTYPE result_type = result_vector_is_long ? REALSXP : INTSXP;
	R_xlen_t subscript_length = XLENGTH(subscript);
	R_xlen_t vector_length = XLENGTH(vector);

	SEXP result = PROTECT(ufo_empty(result_type, result_length, min_load_count));
	for (R_xlen_t result_index = 0, subscript_index = 0; subscript_index < subscript_length; subscript_index++) {
		double value = REAL_ELT(subscript, subscript_index);

		if (value >= 0 && value < 1) continue;

		bool na = (ISNAN(value) || (R_xlen_t) value > vector_length);
		if (result_vector_is_long) SET_REAL_ELT   (result, result_index, na ? NA_REAL    :       value);
		else   		               SET_INTEGER_ELT(result, result_index, na ? NA_INTEGER : (int) value);

		result_index++;
	}

	UNPROTECT(1);
	return result;
}

// XXX There **has** to be a better way to do this.
SEXP negative_real_subscript(SEXP vector, SEXP subscript, int32_t min_load_count, real_vector_stats_t stats) {

	R_xlen_t vector_length = XLENGTH(vector);
	R_xlen_t subscript_length = XLENGTH(subscript);
	SEXP bitmap = PROTECT(ufo_empty(LGLSXP, vector_length, min_load_count));

	for (R_xlen_t vector_index = 0; vector_index < vector_length; vector_index++) {
		SET_LOGICAL_ELT(bitmap, vector_index, TRUE);
	}

	for (R_xlen_t subscript_index = 0; subscript_index < subscript_length; subscript_index++) {
		double subscript_value = REAL_ELT(subscript, subscript_index);
		if (subscript_value == 0) 			  continue;
		if (ISNAN(subscript_value))           continue;
		if (-subscript_value > vector_length) continue;
		SET_LOGICAL_ELT(bitmap, (R_xlen_t) -subscript_value - 1, FALSE);
	}

	SEXP indices = logical_subscript(vector, bitmap, min_load_count);
	UNPROTECT(1);
	return indices;
}

SEXP real_subscript(SEXP vector, SEXP subscript, int32_t min_load_count) {
	real_vector_stats_t stats = real_subscript_stats(subscript);

	if (stats.nas + stats.positives + stats.negatives == 0) {
		return allocVector(INTSXP, 0);
	}

	if (stats.negatives > 0 && (stats.positives > 0 || stats.nas > 0)) {
		Rf_error("only 0s may be mixed with negative subscripts");
	}

	if (stats.negatives != 0) {
		return negative_real_subscript(vector, subscript, min_load_count, stats);
	} else {
		return positive_real_subscript(vector, subscript, min_load_count, stats);
	}
}

//struct _HashData {
//    int K;
//    hlen M;
//    R_xlen_t nmax;
//#ifdef LONG_VECTOR_SUPPORT
//    Rboolean isLong;
//#endif
//    hlen (*hash)(SEXP, R_xlen_t, HashData *);
//    int (*equal)(SEXP, R_xlen_t, SEXP, R_xlen_t);
//    SEXP HashTable;

//    int nomatch;
//    Rboolean useUTF8;
//    Rboolean useCache;
//};

bool string_equal (SEXP/*CHARSXP*/ string_a, SEXP/*CHARSXP*/ string_b) {
    if (string_a == string_b)  return true;
    if (string_a == NA_STRING) return false;
    if (string_b == NA_STRING) return false;

    if (IS_CACHED(string_a) && IS_CACHED(string_b)
        && ENC_KNOWN(string_a) == ENC_KNOWN(string_b)) {
    	return false;
    }

    const void *vmax = vmaxget();
	bool result = !strcmp(translateCharUTF8(string_a), translateCharUTF8(string_b));
	vmaxset(vmax); /* discard any memory used by translateChar */

	return result;
}

typedef struct {
	bool use_bytes;
	bool use_utf8;
	bool use_cache;
} string_vector_character_t;

size_t scatter(unsigned int key, unsigned int senior_bits_in_hash)
{
    return 3141592653U * key >> (32 - senior_bits_in_hash);
}

size_t c_string_hash(SEXP/*CHARSXP*/ c_string, string_vector_character_t character, unsigned int senior_bits_in_hash)
{
    intptr_t c_string_as_pointer = (intptr_t) c_string;
    unsigned int masked_pointer = (unsigned int)(c_string_as_pointer & 0xffffffff);
#if SIZEOF_LONG == 8
    unsigned int exponent = (unsigned int)(c_string_as_pointer/0x100000000L);
#else
    unsigned int exponent = 0;
#endif
    return scatter(masked_pointer ^ exponent, senior_bits_in_hash);
}

size_t string_hash (SEXP/*CHARSXP*/ string, string_vector_character_t character, unsigned int senior_bits_in_hash) {
    if(!character.use_utf8 && character.use_cache) {
    	return c_string_hash(string, character, senior_bits_in_hash);
    }

    const void *vmax = vmaxget();
    const char *string_contents = translateCharUTF8(string);

    unsigned int key = 0;
    while (*string_contents++) {
    	key = 11 * key + (unsigned int) *string_contents; /* was 8 but 11 isn't a power of 2 */
    }
    vmaxset(vmax); /* discard any memory used by translateChar */
    return scatter(key, senior_bits_in_hash);
}

string_vector_character_t derive_string_vector_character(SEXP vector) {
	R_xlen_t length = XLENGTH(vector);
	string_vector_character_t character;

	character.use_bytes = false;
	character.use_utf8 = false;
	character.use_cache = true;

	for (R_xlen_t index = 0; index < length; index++) {
		SEXP element = STRING_ELT(vector, index);

		if (IS_BYTES(element)) {
			character.use_bytes = true;
			character.use_utf8  = false;
			break;
		}

		if (ENC_KNOWN(element)) {
			character.use_utf8  = true;
		}

		if(!IS_CACHED(element)) {
			character.use_cache = false;
			break;
		}
	}

	return character;
}

int needle_index_in_int_haystack(SEXP/*STRSXP*/ haystack, SEXP/*INTSXP*/ hash_table, SEXP/*CHARSXP*/ needle, string_vector_character_t character, unsigned int senior_bits_in_hash) {

	size_t hash = string_hash(needle, character, senior_bits_in_hash);
	R_xlen_t hash_table_length = XLENGTH(hash_table);

	for (int haystack_index = INTEGER_ELT(hash_table, hash);
		 INTEGER_ELT(hash_table, haystack_index) != -1;
		 hash = (haystack_index + 1) % hash_table_length) {

		SEXP/*CHARSXP*/ hay = STRING_ELT(haystack, haystack_index);
	    if (string_equal(hay, needle))
		return haystack_index >= 0 ? haystack_index + 1 : NA_INTEGER;
	}

	return NA_INTEGER;
}

double needle_index_in_double_haystack(SEXP/*STRSXP*/ haystack, SEXP/*INTSXP*/ hash_table, SEXP/*CHARSXP*/ needle) {

}

// reimplements match5 but just for strings
SEXP string_lookup(SEXP haystack, SEXP needles, int32_t min_load_count) {

	SEXPTYPE haystack_type = TYPEOF(haystack);
	SEXPTYPE needles_type = TYPEOF(needles);

	make_sure(haystack_type == STRSXP, Rf_error,
			  "haystack vector is of type %s but must be of type %s",
			  type2char(haystack_type), type2char(STRSXP));

	make_sure(needles_type == STRSXP, Rf_error,
			  "needles vector is of type %s but must be of type %s",
			  type2char(needles_type), type2char(STRSXP));

	R_xlen_t needles_length = XLENGTH(needles);
	if (needles_length == 0) {
		return allocVector(INTSXP, 0);
	}

	R_xlen_t haystack_length = XLENGTH(haystack);
	if (haystack_length == 0) {
		SEXP result = PROTECT(ufo_empty(INTSXP, needles_length, min_load_count));
		for (R_xlen_t result_index = 0; result_index < needles_length; result_index++) {
			SET_INTEGER_ELT(result, result_index, NA_INTEGER);
		}
		UNPROTECT(1);
		return result;
	}

	// Derive the character of the vector.
	string_vector_character_t character = derive_string_vector_character(needles);
	if(!character.use_bytes || character.use_cache) {
		string_vector_character_t haystack_character = derive_string_vector_character(haystack);

		character.use_utf8  = haystack_character.use_utf8;
		character.use_cache = haystack_character.use_cache;
	}

	bool use_long_vector = true; // FIXME

	R_xlen_t hash_table_length = 2;
    int senior_bits_in_hash = 1;
    while (hash_table_length < 2U * haystack_length) {
    	hash_table_length *= 2;
    	senior_bits_in_hash++;
    }
    //d->nmax = haystack_length;

    // Hashing
    SEXP hash_table = PROTECT(ufo_empty(use_long_vector ? REALSXP : INTSXP, hash_table_length, min_load_count));
    if (use_long_vector) {
    	for (R_xlen_t hash_table_index = 0; hash_table_index < hash_table_length; hash_table_index++)  {
    		REAL0(hash_table)[hash_table_index] = -1;
    	}
    } else {
    	for (R_xlen_t hash_table_index = 0; hash_table_index < hash_table_length; hash_table_index++) {
    		INTEGER0(hash_table)[hash_table_index] = -1;
    	}
    }

    // FIXME something missing here
    // FIXME long vectors?
    SEXP result = PROTECT(ufo_empty(use_long_vector ? REALSXP : INTSXP, hash_table_length, min_load_count));
	for (R_xlen_t needles_index = 0; needles_index < needles_length; needles_index++) {




//	    SET_INTEGER_ELT(result, hash_table_index, sLookup(table, x, i, d));
	}

}

SEXP hash_string_subscript(SEXP vector, SEXP names, SEXP subscript, int32_t min_load_count) {

	SEXP indices = string_lookup(names, subscript, min_load_count); /**** guaranteed to be fresh???*/

//	int *pindx = INTEGER(indx);
//	for (i = 0; i < ns; i++)
//	    if(STRING_ELT(s, i) == NA_STRING || !CHAR(STRING_ELT(s, i))[0])
//		pindx[i] = 0;
//	}

	Rf_error("not implemented");
}

SEXP looped_string_subscript(SEXP vector, SEXP names, SEXP subscript, int32_t min_load_count) {
	R_xlen_t names_length = XLENGTH(names);
	R_xlen_t subscript_length = XLENGTH(subscript);

	SEXP integer_subscript = PROTECT(ufo_empty(INTSXP, subscript_length, min_load_count));
	for (R_xlen_t subscript_index = 0; subscript_index < subscript_length; subscript_index++) { // TODO hashing implementation

		SEXP subscript_element = STRING_ELT(subscript, subscript_index);
		if (subscript_element == NA_STRING) {
			SET_INTEGER_ELT(integer_subscript, subscript_index, NA_INTEGER);
			continue;
		}

		for (R_xlen_t names_index = 0; names_index < names_length; names_index++) {
			SEXP name = STRING_ELT(names, names_index);
			if (NonNullStringMatch(subscript_element, name)) {
				SET_INTEGER_ELT(integer_subscript, subscript_index, names_index + 1);
				goto _found_something;
			}
		}

		_found_nothing:
		SET_INTEGER_ELT(integer_subscript, subscript_index, NA_INTEGER); // Neither NA nor an element of `names`

		_found_something:
		continue;
	}

	return integer_subscript;
}

SEXP string_subscript(SEXP vector, SEXP subscript, int32_t min_load_count) { // XXX There's probably a better way to do this one.

	SEXP names = PROTECT(getAttrib(vector, R_NamesSymbol));
	if (TYPEOF(names) == NILSXP) return R_NilValue;

	R_xlen_t subscript_length = XLENGTH(subscript);
	R_xlen_t vector_length = XLENGTH(vector);
	bool use_hashing = (( (subscript_length > 1000 && vector_length)
			           || (vector_length > 1000 && subscript_length))
			           || (subscript_length * vector_length > 15 * vector_length + subscript_length));

	if (use_hashing) return hash_string_subscript  (vector, names, subscript, min_load_count);
	else             return looped_string_subscript(vector, names, subscript, min_load_count);
}

SEXP ufo_subscript(SEXP vector, SEXP subscript, SEXP min_load_count_sexp) {
	make_sure(isVector(vector) || isList(vector) || isLanguage(vector), Rf_error, "subscripting on non-vector");

	int32_t min_load_count = (int32_t) __extract_int_or_die(min_load_count_sexp); // XXX do value checks
	SEXPTYPE subscript_type   = TYPEOF(subscript);

	switch (subscript_type) {
	case NILSXP:  return null_subscript(vector, subscript, min_load_count);
	case LGLSXP:  return logical_subscript(vector, subscript, min_load_count);
	case INTSXP:  return integer_subscript(vector, subscript, min_load_count);
	case REALSXP: return real_subscript(vector, subscript, min_load_count);
	case STRSXP:  return string_subscript(vector, subscript, min_load_count);
	default:      Rf_error("invalid subscript type '%s'", type2char(subscript_type));
	}

	Rf_error("unreachable");
	return R_NilValue;
}

SEXP ufo_subset(SEXP x, SEXP y, SEXP min_load_count_sexp) {
	return R_NilValue;
}

SEXP ufo_subset_assign(SEXP x, SEXP y, SEXP z, SEXP min_load_count_sexp) {
	Rf_error("not implemented");
	return R_NilValue;
}

// TODO always do a normal SEXP below a certain threshold and always do a UFO above a certain threshold
// TODO rename arguments called "subscript" to "indices" and results to "subscript"
