#include <assert.h>

#include "common.h"

bool are_integer_indices_monotonic(SEXP/*INTSXP*/ indices) {
    assert(TYPEOF(indices) == INTSXP);

    R_xlen_t size = XLENGTH(indices);
    int previous = NA_INTEGER;

    for (int i = 0; i < size; i++) {
        int current = INTEGER_ELT(indices, i);
        if (current == NA_INTEGER) {
            return false;
        }
        if (previous != NA_INTEGER && previous >= current) {
            return false;
        }
        previous = current;
    }
    return true;
}

bool are_numeric_indices_monotonic(SEXP/*REALSXP*/ indices) {
    assert(TYPEOF(indices) == REALSXP);

    R_xlen_t size = XLENGTH(indices);
    double previous = NA_REAL;

    for (int i = 0; i < size; i++) {
        double current = REAL_ELT(indices, i);
        if (current == NA_REAL) {
            return false;
        }
        if (previous != NA_REAL && previous >= current) {
            return false;
        }
        previous = current;
    }
    return true;
}

bool are_indices_monotonic (SEXP/*INTSXP | REALSXP*/ indices) {
    SEXPTYPE type = TYPEOF(indices);
    assert(type == INTSXP || type == REALSXP);

    switch (type) {
        case INTSXP:  return are_integer_indices_monotonic(indices);
        case REALSXP: return are_numeric_indices_monotonic(indices);
        default:      Rf_error("Slices can be indexed by integer or numeric vectors but found: %d\n", type);
    }
}

bool are_integer_indices_contiguous(SEXP/*INTSXP*/ indices) {
    assert(TYPEOF(indices) == INTSXP);

    R_xlen_t size = XLENGTH(indices);

    int previous = NA_INTEGER;

    for (int i = 0; i < size; i++) {
        int current = INTEGER_ELT(indices, i);
        if (current == NA_INTEGER) {
            return false;
        }
        if (previous != NA_INTEGER && previous != current + 1) {
            return false;
        }
        previous = current;
    }

    return true;
}

bool are_numeric_indices_contiguous(SEXP/*REALSXP*/ indices) {
    assert(TYPEOF(indices) == REALSXP);

    R_xlen_t size = XLENGTH(indices);

    double previous = NA_REAL;

    for (int i = 0; i < size; i++) {
        double current = REAL_ELT(indices, i);
        if (current == NA_REAL) {
            return false;
        }
        if (previous != NA_REAL && previous != current + 1) {
            return false;
        }
        previous = current;
    }

    return true;
}

bool are_indices_contiguous(SEXP/*INTSXP | REALSXP*/ indices) {
    SEXPTYPE type = TYPEOF(indices);
    assert(type == INTSXP || type == REALSXP);

    switch (type) {
        case INTSXP:  return are_integer_indices_contiguous(indices);
        case REALSXP: return are_numeric_indices_contiguous(indices);
        default:      Rf_error("Slices can be indexed by integer or numeric vectors but found: %d\n", type);
    }
}

R_xlen_t get_first_element_as_length(SEXP/*INTSXP | REALSXP*/ indices) {
    SEXPTYPE type = TYPEOF(indices);
    assert(type == INTSXP || type == REALSXP);
    assert(XLENGTH(indices) > 0);

    switch (type) {
        case INTSXP:  return (R_xlen_t) INTEGER_ELT(indices, 0);
        case REALSXP: return (R_xlen_t) REAL_ELT(indices, 0);
        default:      Rf_error("Slices can be indexed by integer or numeric vectors but found: %d\n", type);
    }
}

void copy_element(SEXP source, R_xlen_t source_index, SEXP target, R_xlen_t target_index) {
    assert(TYPEOF(source) == TYPEOF(target));
    assert(TYPEOF(source) == INTSXP
           || TYPEOF(source) == REALSXP
           || TYPEOF(source) == CPLXSXP
           || TYPEOF(source) == LGLSXP
           || TYPEOF(target) == RAWSXP
           || TYPEOF(source) == VECSXP
           || TYPEOF(source) == STRSXP);

    switch(TYPEOF(source)) {
        case INTSXP:  SET_INTEGER_ELT (target, target_index, INTEGER_ELT (source, source_index)); break;
        case REALSXP: SET_REAL_ELT    (target, target_index, REAL_ELT    (source, source_index)); break;
        case LGLSXP:  SET_LOGICAL_ELT (target, target_index, LOGICAL_ELT (source, source_index)); break;
        case CPLXSXP: SET_COMPLEX_ELT (target, target_index, COMPLEX_ELT (source, source_index)); break;
        case RAWSXP:  SET_RAW_ELT     (target, target_index, RAW_ELT     (source, source_index)); break;
        case STRSXP:  SET_VECTOR_ELT  (target, target_index, VECTOR_ELT  (source, source_index)); break;
        case VECSXP:  SET_VECTOR_ELT  (target, target_index, VECTOR_ELT  (source, source_index)); break;
        default:      Rf_error("Unsupported vector type: %d\n", TYPEOF(source));
    }
}

void set_element_to_NA(SEXP target, R_xlen_t target_index) {
    assert(TYPEOF(target) == INTSXP
           || TYPEOF(target) == REALSXP
           || TYPEOF(target) == CPLXSXP
           || TYPEOF(target) == LGLSXP
         //|| TYPEOF(target) == VECSXP
         //|| TYPEOF(target) == RAWSXP
           || TYPEOF(source) == STRSXP);

    switch(TYPEOF(target)) {
        case INTSXP:  SET_INTEGER_ELT (target, target_index, NA_INTEGER); break;
        case REALSXP: SET_REAL_ELT    (target, target_index, NA_REAL);    break;
        case LGLSXP:  SET_LOGICAL_ELT (target, target_index, NA_LOGICAL); break;
        case CPLXSXP: {
                      Rcomplex NA_CPLX = { NA_REAL, NA_REAL };
                      SET_COMPLEX_ELT (target, target_index, NA_CPLX);    break;
        }
        case STRSXP:  SET_STRING_ELT  (target, target_index, NA_STRING);  break;
        default:      Rf_error("Unsupported vector type: %d\n", TYPEOF(target));
    }
}

SEXP copy_data_in_range(SEXP source, R_xlen_t start, R_xlen_t size) {

    SEXPTYPE type = TYPEOF(source);
    SEXP target = allocVector(type, size);

    for (R_xlen_t i = 0; i < size; i++) {
        copy_element(source, start + i, target, i);
    }

    return target;
}

SEXP copy_data_at_indices(SEXP source, SEXP/*INTSXP | REALSXP*/ indices) {

    SEXPTYPE type = TYPEOF(indices);
    assert(type == INTSXP || type == REALSXP);

    R_xlen_t size = XLENGTH(indices);
    SEXP target = allocVector(type, size);

    switch (type) {
        case INTSXP:  {
            for (R_xlen_t i = 0; i < size; i++) {
                int index = INTEGER_ELT(indices, i);
                copy_element(source, (R_xlen_t) index, target, i);
            }
            break;
        }
        case REALSXP: {
            for (R_xlen_t i = 0; i < size; i++) {
                double index = REAL_ELT(indices, i);
                copy_element(source, (R_xlen_t) index, target, i);
            }
            break;
        }
        default:      Rf_error("Unsupported vector type: %d\n", type);
    }

    return target;
}

SEXP copy_data_at_mask(SEXP source, SEXP/*LGLSXP*/ mask) {

    SEXPTYPE type = TYPEOF(mask);
    assert(type == LGLSXP);

    R_xlen_t mask_size = XLENGTH(mask);
    R_xlen_t target_size = mask_size;
    for (R_xlen_t i = 0; i < mask_size; i++) {
        if (LOGICAL_ELT(mask, i) == FALSE) {
            target_size--;
        }
    }

    SEXP target = allocVector(type, target_size);
    R_xlen_t copied_elements = 0;

    for (R_xlen_t index = 0; index < mask_size; index++) {
        Rboolean current = LOGICAL_ELT(mask, index);

        if (current == NA_LOGICAL) {
            set_element_to_NA(target, copied_elements);
            copied_elements++;
            continue;
        }

        if (current == TRUE) {
            copy_element(source, index, target, copied_elements);
            copied_elements++;
        }
    }

    assert(XLENGTH(target) == copied_elements);
    return target;
}

viewport_type_t recommend_vieport_type_for_indices(SEXP/*INTSXP | REALSXP*/ indices) {
    assert(false); //FIXME
    return VIEWPORT_NONE;
}