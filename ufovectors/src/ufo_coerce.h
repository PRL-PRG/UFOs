#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

int element_as_integer(SEXP source, R_xlen_t index);
double element_as_real(SEXP source, R_xlen_t index);
Rcomplex element_as_complex(SEXP source, R_xlen_t index);
SEXP/*CHARSXP*/ element_as_string(SEXP source, R_xlen_t index);
Rboolean element_as_logical(SEXP source, R_xlen_t index);
Rbyte element_as_raw(SEXP source, R_xlen_t index); // Doesn't really work.

Rcomplex complex(double real, double imaginary);

Rboolean integer_as_logical(int value);
Rboolean real_as_logical(double value);
Rboolean complex_as_logical(Rcomplex value);
Rboolean string_as_logical(SEXP value);

int real_as_integer(double value);
int complex_as_integer(Rcomplex value);		
int string_as_integer(SEXP/*CHARSXP*/ value);
int logical_as_integer(Rboolean value);

double integer_as_real(int value);	
double complex_as_real(Rcomplex value);		
double string_as_real(SEXP/*CHARSXP*/ value);
double logical_as_real(Rboolean value);

Rcomplex integer_as_complex(int value);
Rcomplex real_as_complex(double value);
Rcomplex string_as_complex(SEXP/*CHARSXP*/ value);
Rcomplex logical_as_complex(Rboolean value);

SEXP/*CHARSXP*/ integer_as_string(int value);
SEXP/*CHARSXP*/ real_as_string(double value);
SEXP/*CHARSXP*/ complex_as_string(Rcomplex value);		
SEXP/*CHARSXP*/ logical_as_string(Rboolean value);
SEXP/*CHARSXP*/ raw_as_string(Rbyte value);