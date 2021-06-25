#pragma once

#include "make_sure.h"

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

/**
 * Assigns an integer value to an integer SEXP vector at a specified index.
 * 
 * If compiled with `MAKE_SURE`, checks whether the vector is an integer 
 * vector (by checking `TYPEOF`) and whether the index is within bounds (by 
 * checking `XLENGTH`).
 * 
 * Analogous to R's `SET_INTEGER_ELT`.
 */
inline static void safer_set_integer(SEXP/*INTSXP*/ vector, R_xlen_t index, int value) {

	make_sure(TYPEOF(vector) == INTSXP, 
	          Rf_error, 
	          "Attempting to set an int value to vector %p of type SEXP<%s>", 
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) < index, 
	          Rf_error, 
			  "Attempting to assign value %i to int vector %p of type "
			  "SEXP<%s> with at an out-of-bounds index %l (>= %l)", 
			  value, vector, type2char(TYPEOF(vector)), index, 
			  XLENGTH(vector));

	SET_INTEGER_ELT(vector, index, value);
}

/**
 * Retrieves an integer value from an integer SEXP vector at a specified index.
 * 
 * If compiled with `MAKE_SURE`, checks whether the vector is an integer 
 * vector (by checking `TYPEOF`) and whether the index is within bounds (by 
 * checking `XLENGTH`).
 * 
 * Analogous to R's `SET_INTEGER_ELT`.
 */
inline static int safer_get_integer(SEXP/*INTSXP*/ vector, R_xlen_t index) {

	make_sure(TYPEOF(vector) == INTSXP, 
	          Rf_error, 
	          "Attempting to set an int value to vector %p of type SEXP<%s>", 
			  vector, type2char(TYPEOF(vector)));

	make_sure(XLENGTH(vector) < index, 
	          Rf_error, 
			  "Attempting to index int vector %p of type SEXP<%s> with at an "
			  "out-of-bounds index %l (>= %l)", 
			  vector, type2char(TYPEOF(vector)), index, XLENGTH(vector));

	return INTEGER_ELT(vector, index);
}