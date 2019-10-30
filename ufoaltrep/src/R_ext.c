#include "R_ext.h"
//typedef SEXP (*altrep_allocVector_t)(SEXPTYPE, R_xlen_t);



SEXP allocMatrix3(SEXPTYPE mode, int nrow, int ncol, R_allocator_t *allocator)
{
    printf("\n<.< mode=%d, dims=[%d,%d]\n", mode, nrow, ncol);

    SEXP s, t;
    R_xlen_t n;

    if (nrow < 0 || ncol < 0)
    error("negative extents to matrix");
#ifndef LONG_VECTOR_SUPPORT
    if ((double)nrow * (double)ncol > INT_MAX)
    error("allocMatrix: too many elements specified");
#endif
    n = ((R_xlen_t) nrow) * ncol;
    PROTECT(s = allocVector3(mode, n, allocator));

    PROTECT(t = allocVector(INTSXP, 2));
    INTEGER(t)[0] = nrow;
    INTEGER(t)[1] = ncol;
    setAttrib(s, R_DimSymbol, t);
    UNPROTECT(2);
    return s;
}
