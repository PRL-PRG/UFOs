#pragma once

#include "make_sure.h"

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

/**
 * Assigns an integer value to an `INTSXP` vector at a specified index.
 * 
 * If compiled with `MAKE_SURE`, checks whether the vector is an INTSXP 
 * vector (by checking `TYPEOF`) and whether the index is within bounds (by 
 * checking `XLENGTH`).
 * 
 * Analogous to R's `SET_INTEGER_ELT`.
 */
inline static void safer_set_integer(SEXP/*INTSXP*/ vector, R_xlen_t index, int value) {

	make_sure(TYPEOF(vector) == INTSXP, 
	          Rf_error, 
	          "Attempting to set an integer value to vector %p of type %s"
			  "(expecting an integer vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
	          Rf_error, 
			  "Attempting to assign value %i to %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  value, type2char(TYPEOF(vector)), vector, index, 
			  XLENGTH(vector));

	SET_INTEGER_ELT(vector, index, value);
}

/**
 * Assigns a double value to a `REALSXP` vector at a specified index.
 * 
 * If compiled with `MAKE_SURE`, checks whether the vector is a REALSXP 
 * vector (by checking `TYPEOF`) and whether the index is within bounds (by 
 * checking `XLENGTH`).
 * 
 * Analogous to R's `SET_REAL_ELT`.
 */
inline static void safer_set_double(SEXP/*REALSXP*/ vector, R_xlen_t index, double value) {

	make_sure(TYPEOF(vector) == REALSXP, 
	          Rf_error, 
	          "Attempting to set a double value to vector %p of type %s "
			  "(expecting a double vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
	          Rf_error, 
			  "Attempting to assign value %f to %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  value, type2char(TYPEOF(vector)), vector, index, 
			  XLENGTH(vector));

	SET_REAL_ELT(vector, index, value);
}

/**
 * Assigns a `R_xlen_t` value to a `REALSXP` vector at a specified index.
 * 
 * If compiled with `MAKE_SURE`, checks whether the vector is a REALSXP 
 * vector (by checking `TYPEOF`) and whether the index is within bounds (by 
 * checking `XLENGTH`).
 * 
 * Analogous to R's `SET_REAL_ELT`.
 */
inline static void safer_set_xlen(SEXP/*REALSXP as R_xlen_t*/ vector, R_xlen_t index, R_xlen_t value) {

	make_sure(TYPEOF(vector) == REALSXP, 
	          Rf_error, 
	          "Attempting to set a R_xlen_t value to vector %p of type %s"
			  "(expecting a double vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
	          Rf_error, 
			  "Attempting to assign value %li to %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  value, type2char(TYPEOF(vector)), vector, index, 
			  XLENGTH(vector));

	SET_REAL_ELT(vector, index, value);
}

/**
 * Retrieves an integer value from an INTSXP vector at a specified index.
 * 
 * If compiled with `MAKE_SURE`, checks whether the vector is an integer 
 * vector (by checking `TYPEOF`) and whether the index is within bounds (by 
 * checking `XLENGTH`).
 * 
 * Analogous to R's `INTEGER_ELT`.
 */
inline static int safer_get_integer(SEXP/*INTSXP*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == INTSXP, 
	          Rf_error, 
	          "Attempting to set an integer value to vector %p of type %s"
			  "(expecting an integer vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
	          Rf_error, 
			  "Attempting to retrieve a value from %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  type2char(TYPEOF(vector)), vector, index, XLENGTH(vector));

	return INTEGER_ELT(vector, index);
}

/**
 * Retrieves a double value from a REALSXP vector at a specified index.
 * 
 * If compiled with `MAKE_SURE`, checks whether the vector is a REALSXP 
 * vector (by checking `TYPEOF`) and whether the index is within bounds (by 
 * checking `XLENGTH`).
 * 
 * Analogous to R's `REAL_ELT`.
 */
inline static double safer_get_double(SEXP/*REALSXP*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == REALSXP, 
	          Rf_error, 
	          "Attempting to set an integer value to vector %p of type %s"
			  "(expecting a double vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
	          Rf_error, 
			  "Attempting to retrieve a value from %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  type2char(TYPEOF(vector)), vector, index, XLENGTH(vector));

	return REAL_ELT(vector, index);
}

/**
 * Retrieves an R_xlen_t value from a REALSXP vector at a specified index.
 * 
 * If compiled with `MAKE_SURE`, checks whether the vector is a REALSXP 
 * vector (by checking `TYPEOF`) and whether the index is within bounds (by 
 * checking `XLENGTH`).
 * 
 * Analogous to R's `REAL_ELT`.
 */
inline static double safer_get_xlen(SEXP/*REALSXP as R_xlen_t*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == REALSXP, 
	          Rf_error, 
	          "Attempting to set an integer value to vector %p of type %s"
			  "(expecting a double vector)",
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) >= index, 
	          Rf_error, 
			  "Attempting to retrieve a value from %s vector %p with at an "
			  "out-of-bounds index %li (>= %li)", 
			  type2char(TYPEOF(vector)), vector, index, XLENGTH(vector));

	return REAL_ELT(vector, index);
}