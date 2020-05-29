#include "helpers.h"

int __extract_int_or_die(SEXP/*INTSXP*/ sexp) {
    if (TYPEOF(sexp) != INTSXP) {
        Rf_error("Invalid type for integer vector: %d\n", TYPEOF(sexp));
    }

    if (LENGTH(sexp) == 0) {
        Rf_error("Provided a zero length vector for integer vector\n");
    }

    if (LENGTH(sexp) > 1) {
        Rf_warning("Provided multiple values for integer vector, "
                           "using the first one only\n");
    }

    return INTEGER_ELT(sexp, 0);
}

int __extract_boolean_or_die(SEXP/*LGLSXP*/ sexp) {
    if (TYPEOF(sexp) != LGLSXP) {
        Rf_error("Invalid type for boolean vector: %d\n", TYPEOF(sexp));
    }

    if (LENGTH(sexp) == 0) {
        Rf_error("Provided a zero length vector for boolean vector\n");
    }

    if (LENGTH(sexp) > 1) {
        Rf_warning("Provided multiple values for boolean vector, "
                           "using the first one only\n");
    }

    int element = LOGICAL_ELT(sexp, 0);
    return element == 1;
}

const char* __extract_path_or_die(SEXP/*STRSXP*/ path) {
    if (TYPEOF(path) != STRSXP) {
        Rf_error("Invalid type for paths: %d\n", TYPEOF(path));
    }

    if (LENGTH(path) == 0) {
        Rf_error("Provided a zero length string for path\n");
    }

    if (TYPEOF(STRING_ELT(path, 0)) != CHARSXP) {
        Rf_error("Invalid type for path: %d\n", TYPEOF(STRING_ELT(path, 0)));
    }

    if (LENGTH(path) > 2) {
        Rf_warning("Provided multiple string values for path, "
                           "using the first one only\n");
    }

    return CHAR(STRING_ELT(path, 0));
}

SEXP __check_type_or_die(SEXP sexp, SEXPTYPE type) {
    if (TYPEOF(sexp) != type) {
        Rf_error("Invalid type for SEXP. Expected %d, but got %d\n", type, TYPEOF(sexp));
    }

    return sexp;
}

/**
 * Translates the type of UFO vector to the size of its element in bytes.
 *
 * @param vector_type The type of the vector as specified by the ufo_vector_type
 *                    enum.
 * @return Returns the size of the element vector or dies in case of an
 *         unrecognized vector type.
 */
size_t __get_element_size(SEXPTYPE vector_type) {
    switch (vector_type) {
        case CHARSXP:
            return sizeof(Rbyte);
        case LGLSXP:
            return sizeof(Rboolean);
        case INTSXP:
            return sizeof(int);
        case REALSXP:
            return sizeof(double);
        case CPLXSXP:
            return sizeof(Rcomplex);
        case RAWSXP:
            return sizeof(Rbyte);
        default:
            Rf_error("Unrecognized vector type: %d\n", vector_type);
    }
}




