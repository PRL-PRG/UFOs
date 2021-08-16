#include "ufo_coerce.h"

#define USE_RINTERNALS
#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <R_ext/PrtUtil.h>
#include <R_ext/Itermacros.h>

#include "safety_first.h"

//extern char* OutDec;
//extern const char *EncodeRealDrop0(double x, int w, int d, int e, const char *dec);

#include <assert.h>

int element_as_integer(SEXP source, R_xlen_t index) {
	switch (TYPEOF(source))	{
	case INTSXP:  return INTEGER_ELT(source, index);
	case REALSXP: return real_as_integer(REAL_ELT(source, index));
	case CPLXSXP: return complex_as_integer(COMPLEX_ELT(source, index));		
	case STRSXP:  return string_as_integer(STRING_ELT(source, index));
	case LGLSXP:  return logical_as_integer(LOGICAL_ELT(source, index));
	case RAWSXP:  // We don't do that here.	
	default:
		Rf_error("Cannot coerce values from vector of type %i to integer", 
		         TYPEOF(source)); // TODO pretty print
	}
}

double element_as_real(SEXP source, R_xlen_t index) {
	switch (TYPEOF(source))	{
	case INTSXP:  return integer_as_real(INTEGER_ELT(source, index));
	case REALSXP: return REAL_ELT(source, index);
	case CPLXSXP: return complex_as_real(COMPLEX_ELT(source, index));		
	case STRSXP:  return string_as_real(STRING_ELT(source, index));
	case LGLSXP:  return logical_as_real(LOGICAL_ELT(source, index));
	case RAWSXP:  // We don't do that here.	
	default:
		Rf_error("Cannot coerce values from vector of type %i to real", 
		         TYPEOF(source)); // TODO pretty print
	}
}


Rcomplex element_as_complex(SEXP source, R_xlen_t index) {
	switch (TYPEOF(source))	{
	case INTSXP:  return integer_as_complex(INTEGER_ELT(source, index));
	case REALSXP: return real_as_complex(REAL_ELT(source, index));
    case CPLXSXP: return COMPLEX_ELT(source, index);
	case STRSXP:  return string_as_complex(STRING_ELT(source, index));
	case LGLSXP:  return logical_as_complex(LOGICAL_ELT(source, index));
	case RAWSXP:  // We don't do that here.	
	default:
		Rf_error("Cannot coerce values from vector of type %i to complex", 
		         TYPEOF(source)); // TODO pretty print
	}
}

SEXP/*CHARSXP*/ element_as_string(SEXP source, R_xlen_t index) {
	switch (TYPEOF(source))	{
	case INTSXP:  return integer_as_string(INTEGER_ELT(source, index));
	case REALSXP: return real_as_string(REAL_ELT(source, index));
	case CPLXSXP: return complex_as_string(COMPLEX_ELT(source, index));		
	case STRSXP:  return STRING_ELT(source, index);
	case LGLSXP:  return logical_as_string(LOGICAL_ELT(source, index));
	case RAWSXP:  return raw_as_string(RAW_ELT(source, index));
	default:
		Rf_error("Cannot coerce values from vector of type %i to int", 
		         TYPEOF(source)); // TODO pretty print
	}
}

Rboolean element_as_logical(SEXP source, R_xlen_t index) {
	switch (TYPEOF(source))	{
	case INTSXP:  return integer_as_logical(INTEGER_ELT(source, index));		
	case REALSXP: return real_as_logical(REAL_ELT(source, index));		
	case CPLXSXP: return complex_as_logical(COMPLEX_ELT(source, index));		
	case STRSXP:  return string_as_logical(STRING_ELT(source, index));
	case LGLSXP:  return LOGICAL_ELT(source, index);
	case RAWSXP:  // We don't do that here.	
	default:
		Rf_error("Cannot coerce values from vector of type %i to logical", 
		         TYPEOF(source)); // TODO pretty print
	}
}

Rbyte element_as_raw(SEXP source, R_xlen_t index) {
	switch (TYPEOF(source))	{
    case RAWSXP: return RAW_ELT(source, index);
	default:
		Rf_error("Cannot coerce values from vector of type %i to raw", 
		         TYPEOF(source)); // TODO pretty print
	}
}

Rboolean integer_as_logical(int value) {
    return (value == NA_INTEGER) ? NA_LOGICAL : value != 0;
}

Rboolean real_as_logical(double value) {
    return ISNAN(value) ? NA_LOGICAL : (value != 0);
}

Rboolean complex_as_logical(Rcomplex value) {
    return ISNAN(value.r) || ISNAN(value.i) ? NA_LOGICAL : (value.r != 0 || value.i != 0);
}

Rboolean string_as_logical(SEXP value) {
    if (StringTrue(CHAR(value))) {
        return 1;
    }
    if (StringFalse(CHAR(value))) {
        return 0;
    }
    return NA_LOGICAL;
}

int real_as_integer(double value) {
    if (ISNAN(value)) {
        return NA_INTEGER;
    }
    if (value >= INT_MAX + 1. || value <= INT_MIN ) {
        //*warn |= WARN_INT_NA; // FIXME implement warnings
        return NA_INTEGER;
    }
    return (int) value;
}

int complex_as_integer(Rcomplex value) {
    if (ISNAN(value.r) || ISNAN(value.i)) {
	    return NA_INTEGER;
    }    
    if (value.r > INT_MAX + 1. || value.r <= INT_MIN ) {
	    // *warn |= WARN_INT_NA; // FIXME implement warnings
	    return NA_INTEGER;
    }
    if (value.i != 0) {
	    //*warn |= WARN_IMAG; // FIXME implement warnings
    }
    return (int) value.r;
}

int string_as_integer(SEXP/*CHARSXP*/ value) { // This could use careful cleanup.
    if (value == R_NaString || isBlankString(CHAR(value))) { // ASCII
        return NA_INTEGER;
    }
           
    char *trailing_characters;
    double value_as_double = R_strtod(CHAR(value), &trailing_characters);  // ASCII
    if (!isBlankString(trailing_characters)) {
        //*warn |= WARN_NA; // FIXME implement warnings
        return NA_INTEGER;
    }
    if (value_as_double >= INT_MAX + 1. || value_as_double <= INT_MIN) {
        //*warn |= WARN_INT_NA; // FIXME implement warnings
        return NA_INTEGER;
    }
    return (int) value_as_double;
}

int logical_as_integer(Rboolean value) {
    return (value == NA_LOGICAL) ? NA_INTEGER : value;
}

double integer_as_real(int value) {
    return (value == NA_INTEGER) ? NA_REAL : value;
}

double complex_as_real(Rcomplex value) {
    if (ISNAN(value.r) || ISNAN(value.i)) {
	    return NA_REAL;
    }
    if (ISNAN(value.r)) { // FIXME ???
        return value.r; 
    }
    if (ISNAN(value.i)) {
        return NA_REAL;
    }
    if (value.i != 0) {
	    //*warn |= WARN_IMAG; // FIXME
    }
    return value.r;
}

double string_as_real(SEXP/*CHARSXP*/ value) {
    if (value == R_NaString || isBlankString(CHAR(value))) { /* ASCII */
        return NA_REAL;
    }
    
    char *trailing_characters;
	double value_as_double = R_strtod(CHAR(value), &trailing_characters); /* ASCII */
	if (!isBlankString(trailing_characters)) {
	    //*warn |= WARN_NA;
        return NA_REAL;
    }
    return value_as_double;
}

double logical_as_real(Rboolean value) {
    return (value == NA_LOGICAL) ? NA_REAL : value;
}

Rcomplex complex(double real, double imaginary) {
    Rcomplex complex;
    complex.r = real;
    complex.i = imaginary;
    return complex;
}

Rcomplex integer_as_complex(int value) {
    return (value == NA_INTEGER) ? complex(NA_REAL, NA_REAL) : complex(value, 0);
}

Rcomplex real_as_complex(double value) {
    return complex(value, 0);
}

Rcomplex string_as_complex(SEXP/*CHARSXP*/ value) {
    if (value == R_NaString) {
        return complex(NA_REAL, NA_REAL);
    }

    const char *characters = CHAR(value); /* ASCII */
    if (isBlankString(characters)) {
        return complex(NA_REAL, NA_REAL);
    }

    char *imaginary_component_characters;
	double real_component = R_strtod(characters, &imaginary_component_characters);

	if (isBlankString(imaginary_component_characters)) {
	    return complex(real_component, 0);
	}
	if (*imaginary_component_characters != '+' && *imaginary_component_characters != '-') {
	    // *warn |= WARN_NA; // FIXME
        return complex(NA_REAL, NA_REAL);
	}

    char *trailing_characters;
    double imaginary_component = R_strtod(imaginary_component_characters, &trailing_characters);

    if (*trailing_characters++ == 'i' && isBlankString(imaginary_component_characters)) {
        return complex(real_component, imaginary_component);
    }

    // *warn |= WARN_NA; // FIXME
    return complex(NA_REAL, NA_REAL);
}

Rcomplex logical_as_complex(Rboolean value) {
    return (value == NA_LOGICAL) ? complex(NA_REAL, NA_REAL) : complex(value, 0);
}

SEXP/*CHARSXP*/ integer_as_string(int value) {
    if (value == NA_INTEGER) {
        return NA_STRING;
    }    
    // TODO implement SFI caching, maybe
    int field_width;
	formatInteger(&value, 1, &field_width);
	return mkChar(EncodeInteger(value, field_width));	
}

SEXP/*CHARSXP*/ real_as_string(double value) {
    if (ISNA(value)) {
        return NA_STRING;
    }

    int field_width, decimal_width, scientific_notation;
    formatReal(&value, 1, 
               &field_width, &decimal_width, &scientific_notation, 
               0);

    return mkChar(EncodeReal(value, // FIXME Originally this was `EncodeRealDrop0` but it's not exported.
                             field_width, 
                             decimal_width, 
                             scientific_notation, 
                             '.')); // FIXME This should be `OutDec` from `Defn.h`
}

SEXP/*CHARSXP*/ complex_as_string(Rcomplex value) {
     // "NA" if Re or Im is (but not if they're just NaN)
    if (ISNA(value.r) || ISNA(value.i)) {
	    return NA_STRING;
    } 

    int real_field_width;
    int real_decimal_width;
    int real_scientific_notation;
    int imaginary_field_width;
    int imaginary_decimal_width;
    int imaginary_scientific_notation;
    // Have you heard of structs?

    formatComplex(&value, 1, 
                  &real_field_width, 
                  &real_decimal_width, 
                  &real_scientific_notation,
                  &imaginary_field_width, 
                  &imaginary_decimal_width, 
                  &imaginary_scientific_notation, 
                  0);

    // EncodeComplex has its own anti-trailing-0 care
	return mkChar(EncodeComplex(value, 
                                real_field_width, 
                                real_decimal_width, 
                                real_scientific_notation,
                                imaginary_field_width, 
                                imaginary_decimal_width, 
                                imaginary_scientific_notation, 
                                ".")); // FIXME This should be `OutDec` from `Defn.h`
}

SEXP/*CHARSXP*/ logical_as_string(Rboolean value) {
    int field_width;
    int value_as_int = (int) value;
    formatLogical(&value_as_int, 1, &field_width);
    if (value == NA_LOGICAL) {
        return NA_STRING;
    }
    return mkChar(EncodeLogical(value, field_width));
}

SEXP/*CHARSXP*/ raw_as_string(Rbyte value) {
    char buffer[3];
    sprintf(buffer, "%02x", value);
    return mkChar(buffer);
}