#include "ring_buffer.h"

SEXP ring_buffer_new(R_xlen_t size) {
    SEXP ring = PROTECT(allocSExp(LISTSXP));                                    // This is the ring buffer used for caching.
    CAR(ring) = PROTECT(allocVector(VECSXP, size));                             // This is where the data will go.
    TAG(ring) = PROTECT(allocVector(INTSXP, 1));                                // This will point to the head.
    CDR(ring) = PROTECT(allocVector(INTSXP, size * 2));                         // This is where cache boundaries will live.

    ring_buffer_set_head(ring, 0);                                              // Initially head is the first element.

    UNPROTECT(3);
    return ring;
}

R_xlen_t ring_buffer_get_size(SEXP ring) {
    return XLENGTH(CAR(ring));
}

int ring_buffer_get_head(SEXP ring) {
    return INTEGER_ELT(TAG(ring), 0);
}

void ring_buffer_set_head(SEXP ring, int new_head) {
    SET_INTEGER_ELT(TAG(ring), 0, new_head);
}

void ring_buffer_set_boundary_start_at(SEXP ring, R_len_t position, int value) {
    SET_INTEGER_ELT(CDR(ring), 2 * position, value);
}

void ring_buffer_set_boundary_end_at(SEXP ring, R_len_t position, int value) {
    SET_INTEGER_ELT(CDR(ring), 2 * position + 1, value);
}

int ring_buffer_get_boundary_start_at(SEXP ring, R_len_t position) {
    return INTEGER_ELT(CDR(ring), 2 * position);
}

int ring_buffer_get_boundary_end_at(SEXP ring, R_len_t position) {
    return INTEGER_ELT(CDR(ring), 2 * position + 1);
}

int ring_buffer_bump_head(SEXP ring) {
    R_xlen_t buffer_size = ring_buffer_get_size(ring);
    int old_head = ring_buffer_get_head(ring);
    int new_head = (old_head + 1) % buffer_size;
    ring_buffer_set_head(ring, new_head);
    return new_head;
}

SEXP/*VECSXP*/ ring_buffer_get_cache(SEXP ring) {
    return CAR(ring);
}

SEXP/*VECSXP*/ ring_buffet_get_cache_vector_at(SEXP ring, R_xlen_t position) {
    return VECTOR_ELT(CAR(ring), position);
}

void ring_buffer_set_boundaries(SEXP ring, int start_index, int end_index) {
    R_xlen_t head = ring_buffer_get_head(ring);
    ring_buffer_set_boundary_start_at(ring, head, start_index);
    ring_buffer_set_boundary_end_at(ring, head, end_index);
}

void ring_buffer_add_vector_to_cache(SEXP ring, SEXP any_vector, int start_index, R_xlen_t length) {
    SEXP cache = ring_buffer_get_cache(ring);
    int new_head = ring_buffer_bump_head(ring);
    ring_buffer_set_boundaries(ring, start_index, start_index + length);
    SET_VECTOR_ELT(cache, new_head, any_vector);
}

SEXP ring_buffer_make_cache_vector(SEXP ring, SEXPTYPE type, int start_index, R_xlen_t length) {
    SEXP vector = PROTECT(allocVector(type, length));
    UNPROTECT(1);
}

int ring_buffer_find_cache(SEXP ring, int element_index) {
    for (int chunk_index = 0;
         chunk_index < ring_buffer_get_size(ring);
         ++chunk_index) {

        if (element_index < ring_buffer_get_boundary_start_at(ring, chunk_index))
            continue;

        if (element_index >= ring_buffer_get_boundary_end_at(ring, chunk_index))
            continue;

        return chunk_index;
    }

    return NA_INTEGER;
}

int ring_buffer_get_element_at_cache(SEXP ring, int element_index, int cache_index, void *element) {
    SEXP cache = ring_buffet_get_cache_vector_at(ring, cache_index);
    int cache_start_index = ring_buffer_get_boundary_start_at(ring, cache_index);

    switch (TYPEOF(cache)) {
        case INTSXP:
            int value = INTEGER_ELT(cache_index, element_index - cache_start_index);
            *((int *) element) = value;
            break;
        case REALSXP:
            double value = REAL_ELT(cache_index, element_index - cache_start_index);
            *((double *) element) = value;
            break;
        case LGLSXP:
            Rboolean value = LOGICAL_ELT(cache_index, element_index - cache_start_index);
            *((Rboolean *) element) = value;
            break;
        case RAWSXP:
            Rbyte value = RAW_ELT(cache_index, element_index - cache_start_index);
            *((Rbyte *) element) = value;
            break;
        case CPLXSXP:
            Rcomplex value = COMPLEX_ELT(cache_index, element_index - cache_start_index);
            *((Rcomplex *) element) = value;
            break;
        default: 
            Rf_error("Unknown vector type");
    }
    return 0;
}

int ring_buffer_get_element_at(SEXP ring, int element_index, int *element) {
    for (int cache_index = 0;
         cache_index < ring_buffer_get_size(ring);
         ++cache_index) {

        int cache_start_index = ring_buffer_get_boundary_start_at(ring, cache_index);
        if (element_index < cache_start_index)
            continue;

        if (element_index >= ring_buffer_get_boundary_end_at(ring, cache_index))
            continue;

        SEXP cache = ring_buffet_get_cache_vector_at(ring, cache_index);

        switch (TYPEOF(cache)) {
            case INTSXP:
                int value = INTEGER_ELT(cache_index, element_index - cache_start_index);
                *((int *) element) = value;
                break;
            case REALSXP:
                double value = REAL_ELT(cache_index, element_index - cache_start_index);
                *((double *) element) = value;
                break;
            case LGLSXP:
                Rboolean value = LOGICAL_ELT(cache_index, element_index - cache_start_index);
                *((Rboolean *) element) = value;
                break;
            case RAWSXP:
                Rbyte value = RAW_ELT(cache_index, element_index - cache_start_index);
                *((Rbyte *) element) = value;
                break;
            case CPLXSXP:
                Rcomplex value = COMPLEX_ELT(cache_index, element_index - cache_start_index);
                *((Rcomplex *) element) = value;
                break;
            default: 
                Rf_error("Unknown vector type");
        }
    }

    return 0;
}
