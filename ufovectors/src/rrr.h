#include <stdbool.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

/*
 * RRR TYPE ENCODINGS 
 *
 * nint                     integer,  may contain NA_INTEGER
 * cint                     integer,  may not contain NA_INTEGER (ie. clean)
 * nxlen                    R_xlen_t, may contain NA_INTEGER
 * cxlen                    R_xlen_t, may not contain NA_INTEGER
 *
 * zero_nint_index_t        0-based integer index,  may contain NA_INTEGER
 * zero_cint_index_t        0-based integer index,  may not contain NA_INTEGER
 * zero_nxlen_index_t       0-based R_xlen_t index, may contain NA_INTEGER
 * zero_cxlen_index_t       0-based R_xlen_t index, may not contain NA_INTEGER
 *
 * one_nint_index_t         1-based integer index,  may contain NA_INTEGER
 * one_cint_index_t         1-based integer index,  may not contain NA_INTEGER
 * one_nxlen_index_t        1-based R_xlen_t index, may contain NA_INTEGER
 * one_cxlen_index_t        1-based R_xlen_t index, may not contain NA_INTEGER
 */

typedef struct zero_nint_index_struct { int nzix; } zero_nint_index_t;
typedef struct zero_cint_index_struct { int zix;  } zero_cint_index_t;
typedef struct one_nint_index_struct  { int noix; } one_nint_index_t;
typedef struct one_cint_index_struct  { int oix;  } one_cint_index_t;

zero_nint_index_t zero_index_from_int(int);
one_nint_index_t one_index_from_int(int);


typedef struct { SEXP/*INTSXP*/  int_vector;     } integer_vector_t; /*int*/
typedef struct { SEXP/*REALSXP*/ double_vector;  } real_vector_t;    /*double*/
typedef struct { SEXP/*LGLSXP*/  boolean_vector; } logical_vector_t; /*Rboolean*/
typedef struct { SEXP/*RAWSXP*/  byte_vector;    } raw_vector_t;     /*Rbyte*/
typedef struct { SEXP/*STRSXP*/  charsxp_vector; } string_vector_t;  /*CHARSXP*/
typedef struct { SEXP/*CPLXSXP*/ complex_vector; } complex_vector_t; /*Rcomplex*/
typedef struct { SEXP/*VECSXP*/  generic_vector; } generic_vector_t; /*SEXP*/\

// REALSXP containing R_xlen_t values encoded as doubles
typedef struct { SEXP/*REALSXP*/ xlen_vector;    } xlen_vector_t;    /*double/R_xlen_t*/

typedef struct { SEXP/*INTSXP*/ one_nint_index_vector; } one_indexing_integer_vector_t; /*int/one_nint_index*/


integer_vector_t integer_vector_from(SEXP/*INTSXP*/ sexp);
R_xlen_t integer_vector_length(integer_vector_t vector);

int integer_vector_one_indexed_get(integer_vector_t vector, one_nint_index_t index);
int integer_vector_zero_indexed_get(integer_vector_t vector, zero_nint_index_t index);

one_indexing_integer_vector_t one_indexing_integer_vector_from(SEXP/*INTSXP*/ sexp);
R_xlen_t one_indexing_integer_vector_length(one_indexing_integer_vector_t vector);

one_nint_index_t one_indexing_integer_vector_one_indexed_get(one_indexing_integer_vector_t vector, one_nint_index_t index);
one_nint_index_t one_indexing_integer_vector_zero_indexed_get(one_indexing_integer_vector_t vector, zero_nint_index_t index);