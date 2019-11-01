#include "R_ext.h"

SEXP allocMatrix4(SEXPTYPE mode, int nrow, int ncol,
                  altrep_constructor_t altrep_constructor,
                  void *altrep_constructor_data) {
    SEXP s, t;
    R_xlen_t n;

    if (nrow < 0 || ncol < 0)
        error("negative extents to matrix");
#ifndef LONG_VECTOR_SUPPORT
    if ((double)nrow * (double)ncol > INT_MAX)
    error("allocMatrix3: too many elements specified");
#endif
    n = ((R_xlen_t) nrow) * ncol;
    PROTECT(s = altrep_constructor(mode, n, altrep_constructor_data));
    PROTECT(t = allocVector(INTSXP, 2));
    INTEGER(t)[0] = nrow;
    INTEGER(t)[1] = ncol;
    setAttrib(s, R_DimSymbol, t);
    UNPROTECT(2);
    return s;
}
