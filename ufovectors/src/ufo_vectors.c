#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../include/ufos.h"
#include "ufo_vectors.h"
#include "ufo_metadata.h"
#include "helpers.h"
#include "debug.h"
#include "bin/io.h"

#include "../include/mappedMemory/userfaultCore.h"

int ufo_initialized = 0;

/**
 * Translates the type of UFO vector to the size of its element in bytes.
 *
 * @param vector_type The type of the vector as specified by the ufo_vector_type
 *                    enum.
 * @return Returns the size of the element vector or dies in case of an
 *         unrecognized vector type.
 */
size_t __get_ufo_element_size(ufo_vector_type_t vector_type) {
    switch (vector_type) {
        case UFO_CHAR:
            return sizeof(Rbyte);
        case UFO_LGL:
            return sizeof(Rboolean);
        case UFO_INT:
            return sizeof(int);
        case UFO_REAL:
            return sizeof(double);
        case UFO_CPLX:
            return sizeof(Rcomplex);
        case UFO_RAW:
            return sizeof(Rbyte);
        case UFO_STR:
            return sizeof(SEXP/*STRSXP*/);
        default:
            Rf_error("Unrecognized vector type: %d\n", vector_type);
    }
}

void __destroy(ufUserData *user_data) {
    ufo_file_source_data_t *data = (ufo_file_source_data_t*) user_data;
    if (__get_debug_mode()) {
        REprintf("__destroy\n");
        REprintf("    source file: %s\n", data->path);
        REprintf("    vector type: %d\n", data->vector_type);
        REprintf("    vector size: %li\n", data->vector_size);
        REprintf("   element size: %li\n", data->element_size);
    }
    fclose(data->file_handle);
    free((char *) data->path);
    free(data);
}



ufo_source_t* __make_source_or_die(ufo_vector_type_t type, const char *path, int *dimensions, size_t dimensions_length, int32_t min_load_count) {

    ufo_file_source_data_t *data = (ufo_file_source_data_t*) malloc(sizeof(ufo_file_source_data_t));

    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));
    source->population_function = &__load_from_file;
    source->destructor_function = &__destroy;
    source->data = (ufUserData*) data;
    source->vector_type = type;
    source->element_size = __get_ufo_element_size(type);
    source->vector_size = __get_vector_length_from_file_or_die(path, source->element_size);
    source->dimensions = dimensions;
    source->dimensions_length = dimensions_length;
    source->min_load_count =
            min_load_count > 0 ? min_load_count
                               : __1MB_of_elements(source->element_size);

    data->path = path;
    data->vector_type = source->vector_type;
    data->vector_size = source->vector_size;
    data->element_size = source->element_size;

    data->file_handle = __open_file_or_die(path);
    data->file_cursor = 0;

    return source;
}

SEXP __make_vector(ufo_vector_type_t type, SEXP sexp, SEXP/*INTSXP*/ min_load_count_sexp) {
    const char *path = __extract_path_or_die(sexp);
    int32_t min_load_count = __extract_int_or_die(min_load_count_sexp);
    ufo_source_t *source = __make_source_or_die(type, path,
                                                NULL, 0,
                                                min_load_count);
    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    return ufo_new(source);
}

SEXP ufo_vectors_intsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ min_load_count_sexp) {
    return __make_vector(UFO_INT, path, min_load_count_sexp);
}

SEXP ufo_vectors_realsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ min_load_count_sexp) {
    return __make_vector(UFO_REAL, path, min_load_count_sexp);
}

SEXP ufo_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ min_load_count_sexp) {
    return __make_vector(UFO_CPLX, path, min_load_count_sexp);
}

SEXP ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ min_load_count_sexp) {
    return __make_vector(UFO_LGL, path, min_load_count_sexp);
}

SEXP ufo_vectors_rawsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ min_load_count_sexp) {
    return __make_vector(UFO_RAW, path, min_load_count_sexp);
}

SEXP/*NILSXP*/ ufo_store_bin(SEXP/*STRSXP*/ _path, SEXP vector) {
    __write_bytes_to_disk(__extract_path_or_die(_path),
                          Rf_length(vector) * __get_element_size(TYPEOF(vector)),
                          (const char *) DATAPTR_RO(vector));
    return R_NilValue;
}

SEXP __make_matrix(ufo_vector_type_t type, SEXP/*STRSXP*/ path_sexp, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*INTSXP*/ min_load_count_sexp) {
    const char *path = __extract_path_or_die(path_sexp);
    int32_t min_load_count = __extract_int_or_die(min_load_count_sexp);
    int *dimensions = (int *) malloc(sizeof(int) * 2);
    dimensions[0] = __extract_int_or_die(rows);
    dimensions[1] = __extract_int_or_die(cols);
    ufo_source_t *source = __make_source_or_die(type, path,
                                                dimensions, 2,
                                                min_load_count);
    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new_multidim");

    return ufo_new(source);
}

SEXP ufo_matrix_intsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*INTSXP*/ min_load_count) {
    return __make_matrix(UFO_INT, path, rows, cols, min_load_count);
}

SEXP ufo_matrix_realsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*INTSXP*/ min_load_count) {
    return __make_matrix(UFO_REAL, path, rows, cols, min_load_count);
}

SEXP ufo_matrix_cplxsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*INTSXP*/ min_load_count) {
    return __make_matrix(UFO_CPLX, path, rows, cols, min_load_count);
}

SEXP ufo_matrix_lglsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*INTSXP*/ min_load_count) {
    return __make_matrix(UFO_LGL, path, rows, cols, min_load_count);
}

SEXP ufo_matrix_rawsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*INTSXP*/ min_load_count) {
    return __make_matrix(UFO_RAW, path, rows, cols, min_load_count);
}