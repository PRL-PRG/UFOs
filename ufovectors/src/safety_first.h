#pragma once

// Standard lib
#include <stdbool.h>

// R 
#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

// Missing bits of Rinternals.h patched in:
extern void SET_COMPLEX_ELT(SEXP vector, R_xlen_t index, Rcomplex value);
extern void SET_RAW_ELT(SEXP vector, R_xlen_t index, Rbyte value);

/**
 * General assert statement. Fails if the condition evaluates to false. On 
 * failure calls Rf_error with the message provided by an agument, which cases
 * the current execution to fail and return to the interpreter. The error is 
 * displayed within the interpreter.
 */
#ifdef SAFETY_FIRST
#define make_sure(condition, ...) do { \
			if(!(condition)) { \
				Rf_error(__VA_ARGS__); \
			} \
		} while(0)
#else
#define make_sure(condition, ...)
#endif

/**
 * Soft assert statement. Fails if the condition is not true. On failure 
 * calls Rf_warning with the message provided by an agument. This does not 
 * make the current execution fail, but an error message to be displayed within
 * the interpreter when the execution completes.
 */
#ifdef SAFETY_FIRST
#define take_a_look(condition, ...) do { \
			if(!(condition)) { \
				Rf_warn(__VA_ARGS__); \
			} \
		} while(0)
#else
#define take_a_look(condition, ...)
#endif

/**
 * Critical assert statement. Fails if the condition is not true. On failure
 * calls Rf_stop with the message provided by an agument. This causes the 
 * interpreter to (gracefully) halt and the error message to be displayed.
 * 
 * This feature is currently NOT IMPLEMENETED.
 */
#ifdef SAFETY_FIRST
#define make_absolutely_sure(condition, ...) do { \
			if(!(condition)) { \
				UNIMPLEMENTED(__VA_ARGS__); \
			} \
		} while(0)
#else
#define make_absolutely_sure(condition, ...)
#endif

/**
 * Checks if an integer is an NA value.
 * 
 * Analogous to comaparing with R's `NA_INTEGER` value.
 */
inline static bool safely_is_na_integer(int value) {
	return NA_INTEGER == value;
}

/**
 * Checks if a double is an NA value.
 * 
 * Analogous to checking with R's `ISNAN` function.
 */
inline static bool safely_is_na_double(double value) {
	return ISNAN(value);
}

/**
 * Checks if a R_xlen_t value is an NA value.
 * 
 * Analogous to checking with R's `ISNAN` function.
 */
inline static bool safely_is_na_xlen(R_xlen_t value) {
	return ISNAN((double) value);
}

/**
 * Checks if an R boolean is an NA value.
 * 
 * Analogous to comaparing with R's `NA_LOGICAL` value.
 */
inline static bool safely_is_na_logical(Rboolean value) {
	return NA_LOGICAL == value;
}

/**
 * Checks if an R character vector is an NA value.
 * 
 * If compiled with `SAFETY_FIRST`, checks if the elements is a character 
 * vector.
 * 
 * Analogous to comaparing pointers with R's `NA_STRING` value.
 */
inline static bool safely_is_na_character(SEXP/*CHARSXP*/ value) {

	make_sure(TYPEOF(value) == CHARSXP,
			  "Attempting to check if %p is the \"NA\" string, but found %s " 
			  "(expecting character vector)",
			  value, type2char(TYPEOF(value)));

	return NA_STRING == value;
}

/**
 * Checks if an R complex is an NA value.
 * 
 * Analogous to comparing with R's `NA_REAL` value for both the immaginary and
 * real components of the complex value. A complex value is considered NA if
 * either of its components is NA.
 */
inline static bool safely_is_na_complex(Rcomplex value) {
	return ISNAN(value.r) || ISNAN(value.i);
}

// Note: RAWSXP does not have an NA value.

/**
 * Retrieves the length of an R vector.
 * 
 * Checks whether the vector has a length field by checking if it is
 * a VECSEXPREC type: either a vector (via `isVector`), a list (via `isList`),
 * or a language expression (via `isExpression`).
 * 
 * Analogous to `XLENGTH`.
 */
inline static bool safely_get_length(SEXP/*any vector*/ vector) {

	make_sure(isVector(vector) || isList(vector) || isLanguage(vector),
	          "Attempting to retrieve the length of %p, but it is not a "
			  "vector type (SEXP type: %s)",
			  vector, type2char(TYPEOF(vector)));

	return XLENGTH(vector);
}

/**
 * Assigns an integer value to an `INTSXP` vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is an INTSXP 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `SET_INTEGER_ELT`.
 */
inline static void safely_set_integer(SEXP/*INTSXP*/ vector, R_xlen_t index, int value) {

	make_sure(TYPEOF(vector) == INTSXP, 
	          "Attempting to set an integer value to vector %p of type %s"
			  "(expecting an integer vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to assign value %i to %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  value, type2char(TYPEOF(vector)), vector, index, 
			  XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to assign value %i to %s vector %p with an NA index "
			  "(== %li)", 
			  value, type2char(TYPEOF(vector)), vector, index);			  

	SET_INTEGER_ELT(vector, index, value);
}

/**
 * Assigns a double value to a `REALSXP` vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a REALSXP 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `SET_REAL_ELT`.
 */
inline static void safely_set_real(SEXP/*REALSXP*/ vector, R_xlen_t index, double value) {

	make_sure(TYPEOF(vector) == REALSXP, 
	          "Attempting to set a double value to vector %p of type %s "
			  "(expecting a double vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to assign value %f to %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  value, type2char(TYPEOF(vector)), vector, index, 
			  XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to assign value %i to %s vector %p with an NA index "
			  "(== %li)", 
			  value, type2char(TYPEOF(vector)), vector, index);				  

	SET_REAL_ELT(vector, index, value);
}

/**
 * Assigns a `R_xlen_t` value to a `REALSXP` vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a REALSXP 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `SET_REAL_ELT`.
 */
inline static void safely_set_xlen(SEXP/*REALSXP as R_xlen_t*/ vector, R_xlen_t index, R_xlen_t value) {

	make_sure(TYPEOF(vector) == REALSXP, 
	          "Attempting to set a R_xlen_t value to vector %p of type %s"
			  "(expecting a double vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to assign value %li to %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  value, type2char(TYPEOF(vector)), vector, index, 
			  XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to assign value %i to %s vector %p with an NA index "
			  "(== %li)", 
			  value, type2char(TYPEOF(vector)), vector, index);				  

	SET_REAL_ELT(vector, index, value);
}

/**
 * Assigns an `Rboolean` value to a `LGLSXP` vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a LGLSXP 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `SET_LOGICAL_ELT`.
 */
inline static void safely_set_logical(SEXP/*LGLSXP*/ vector, R_xlen_t index, Rboolean value) {

	make_sure(TYPEOF(vector) == LGLSXP, 
	          "Attempting to set a logical value to vector %p of type %s"
			  "(expecting a logical vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to assign value %i to %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  value, type2char(TYPEOF(vector)), vector, index, 
			  XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to assign value %i to %s vector %p with an NA index "
			  "(== %li)", 
			  value, type2char(TYPEOF(vector)), vector, index);			  

	SET_LOGICAL_ELT(vector, index, value);
}

/**
 * Assigns an `Rcomplex` value to a `CPLXSXP` vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a CPLXSXP 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `SET_COMPLEX_ELT`.
 */
inline static void safely_set_complex(SEXP/*CPLXSXP*/ vector, R_xlen_t index, Rcomplex value) {

	make_sure(TYPEOF(vector) == CPLXSXP, 
	          "Attempting to set a complex value to vector %p of type %s"
			  "(expecting a complex vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to assign value %i to %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  value, type2char(TYPEOF(vector)), vector, index, 
			  XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to assign value %i to %s vector %p with an NA index "
			  "(== %li)", 
			  value, type2char(TYPEOF(vector)), vector, index);			  

	//SET_COMPLEX_ELT(vector, index, value);
    if (ALTREP(vector)) {
		ALTCOMPLEX_SET_ELT(vector, index, value);
	} else {
		COMPLEX0(vector)[index] = value;
	}
}

/**
 * Assigns an `Rbyte` value to a `RAWSXP` vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a CPLXSXP 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `SET_RAW_ELT`.
 */
inline static void safely_set_raw(SEXP/*RAWSXP*/ vector, R_xlen_t index, Rbyte value) {

	make_sure(TYPEOF(vector) == RAWSXP, 
	          "Attempting to set a raw byte value to vector %p of type %s"
			  "(expecting a raw byte vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to assign value %i to %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  value, type2char(TYPEOF(vector)), vector, index, 
			  XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to assign value %i to %s vector %p with an NA index "
			  "(== %li)", 
			  value, type2char(TYPEOF(vector)), vector, index);			  

	SET_RAW_ELT(vector, index, value);
}

/**
 * Assigns a character vector (`CHARSXP`) to a string vector (`STRSXP`) at a 
 * specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a STRSXP
 * vector and the assigned value is a CHARSXP vector (by checking `TYPEOF`), 
 * whether the index is within bounds (by checking `XLENGTH`), and whether the
 * index is not the NA value.
 * 
 * Analogous to R's `SET_STRING_ELT`.
 */
inline static void safely_set_string(SEXP/*STRSXP*/ vector, R_xlen_t index, SEXP/*CHARSXP*/ value) {

	make_sure(TYPEOF(vector) == STRSXP && TYPEOF(value) == CHARSXP,
	          "Attempting to set a %s vector %p as an element of vector %p of "
			  "type %s (expecting a character vector to be set as an element "
			  "of a string vector)",
			  type2char(TYPEOF(value)), value, vector, 
			  type2char(TYPEOF(vector)));		  

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to assign value %i to %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  value, type2char(TYPEOF(vector)), vector, index, 
			  XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to assign value %i to %s vector %p with an NA index "
			  "(== %li)", 
			  value, type2char(TYPEOF(vector)), vector, index);			  

	SET_STRING_ELT(vector, index, value);
}

/**
 * Retrieves an integer value from an INTSXP vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is an integer 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `INTEGER_ELT`.
 */
inline static int safely_get_integer(SEXP/*INTSXP*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == INTSXP, 
	          "Attempting to set an integer value to vector %p of type %s"
			  "(expecting an integer vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to retrieve a value from %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  type2char(TYPEOF(vector)), vector, index, XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to retrieve a value from %s vector %p with an NA "
			  "index (== %li)", 
			  type2char(TYPEOF(vector)), vector, index);

	return INTEGER_ELT(vector, index);
}

/**
 * Retrieves a double value from a REALSXP vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a REALSXP 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `REAL_ELT`.
 */
inline static double safely_get_real(SEXP/*REALSXP*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == REALSXP, 
	          "Attempting to retrieve a double value from vector %p of type %s"
			  "(expecting a double vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to retrieve a value from %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  type2char(TYPEOF(vector)), vector, index, XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to retrieve a value from %s vector %p with an NA "
			  "index (== %li)", 
			  type2char(TYPEOF(vector)), vector, index);			  

	return REAL_ELT(vector, index);
}

/**
 * Retrieves an R_xlen_t value from a REALSXP vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a REALSXP 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `REAL_ELT`.
 */
inline static R_xlen_t safely_get_xlen(SEXP/*REALSXP as R_xlen_t*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == REALSXP, 
	          "Attempting to retrieve an R_xlen_t value from vector %p of "
			  "type %s (expecting a double vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to retrieve a value from %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  type2char(TYPEOF(vector)), vector, index, XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to retrieve a value from %s vector %p with an NA "
			  "index (== %li)", 
			  type2char(TYPEOF(vector)), vector, index);			  

	return REAL_ELT(vector, index);
}

/**
 * Retrieves a logical value from a LGLSXP vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a logical 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `LOGICAL_ELT`.
 */
inline static Rboolean safely_get_logical(SEXP/*LGLSXP*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == LGLSXP, 
	          "Attempting to retrieve an Rboolean value from vector %p of "
			  "type %s (expecting a logical vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to retrieve a value from %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  type2char(TYPEOF(vector)), vector, index, XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to retrieve a value from %s vector %p with an NA "
			  "index (== %li)", 
			  type2char(TYPEOF(vector)), vector, index);

	return LOGICAL_ELT(vector, index);
}

/**
 * Retrieves R's complex value from a CPLXSXP vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a raw byte 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `COMPLEX_ELT`.
 */
inline static Rcomplex safely_get_complex(SEXP/*CPLXSXP*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == CPLXSXP, 
	          "Attempting to retrieve an Rcomplex value from vector %p of "
			  "type %s (expecting a complex vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to retrieve a value from %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  type2char(TYPEOF(vector)), vector, index, XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to retrieve a value from %s vector %p with an NA "
			  "index (== %li)", 
			  type2char(TYPEOF(vector)), vector, index);

	return COMPLEX_ELT(vector, index);
}

/**
 * Retrieves R's raw byte value from a RAWSXP vector at a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a raw byte 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value.
 * 
 * Analogous to R's `RAW_ELT`.
 */
inline static Rbyte safely_get_raw(SEXP/*RAWSXP*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == RAWSXP, 
	          "Attempting to retrieve an Rbyte value from vector %p of "
			  "type %s (expecting a raw vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to retrieve a value from %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  type2char(TYPEOF(vector)), vector, index, XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to retrieve a value from %s vector %p with an NA "
			  "index (== %li)", 
			  type2char(TYPEOF(vector)), vector, index);

	return RAW_ELT(vector, index);
}

/**
 * Retrieves a character vector (`CHARSXP`) from a string vector (`STRSXP`) at
 * a specified index.
 * 
 * If compiled with `SAFETY_FIRST`, checks whether the vector is a string 
 * vector (by checking `TYPEOF`), whether the index is within bounds (by 
 * checking `XLENGTH`), and whether the index is not the NA value. It also
 * checks whether the returned vector is actually a `CHARSXP` vector.
 * 
 * Analogous to R's `STRING_ELT`.
 */
inline static SEXP/*CHARSXP*/ safely_get_string(SEXP/*STRSXP*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == STRSXP, 
	          "Attempting to retrieve a character vector from vector %p of "
			  "type %s (expecting a string vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
			  "Attempting to retrieve a value from %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  type2char(TYPEOF(vector)), vector, index, XLENGTH(vector));

	make_sure(!safely_is_na_xlen(index), 
			  "Attempting to retrieve a value from %s vector %p with an NA "
			  "index (== %li)", 
			  type2char(TYPEOF(vector)), vector, index);

	SEXP/*CHARSXP*/ retrieved_element = STRING_ELT(vector, index);

	make_sure(TYPEOF(retrieved_element) == CHARSXP,
			  "Attemptiong to retrieve a value from %s vector %p at index "
			  "%li, but the retrieved element %p has type %s (expecting "
			  "result to be a character vector)",
			  type2char(TYPEOF(vector)), vector, index, retrieved_element,
			  type2char(TYPEOF(retrieved_element)));

	return retrieved_element;
}