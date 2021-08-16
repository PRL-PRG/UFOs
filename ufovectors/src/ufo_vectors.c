#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

#include "../include/ufos.h"
#include "ufo_vectors.h"
#include "ufo_metadata.h"
#include "helpers.h"
#include "debug.h"
#include "bin/io.h"

#include "safety_first.h"

int ufo_initialized = 0;

void __destroy(void* user_data) {
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

ufo_source_t* __make_source_or_die(ufo_vector_type_t type, const char *path, int *dimensions, size_t dimensions_length, bool read_only, int32_t min_load_count) {

    ufo_file_source_data_t *data = (ufo_file_source_data_t*) malloc(sizeof(ufo_file_source_data_t));
    if(data == NULL) {
    	Rf_error("Cannot allocate ufo_file_source_t");
    }

    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));
    if(source == NULL) {
    	Rf_error("Cannot allocate ufo_source_t");
    }

    source->population_function = &__load_from_file;
    source->destructor_function = &__destroy;
    source->data = (void*) data;
    source->vector_type = type;
    source->element_size = __get_element_size(type);
    source->vector_size = __get_vector_length_from_file_or_die(path, source->element_size);
    source->dimensions = dimensions;
    source->dimensions_length = dimensions_length;
    source->read_only = read_only;
    source->min_load_count = __select_min_load_count(min_load_count, source->element_size);

    data->path = path;
    data->vector_type = source->vector_type;
    data->vector_size = source->vector_size;
    data->element_size = source->element_size;

    data->file_handle = __open_file_or_die(path);
    data->file_cursor = 0;

    return source;
}

SEXP __make_vector(ufo_vector_type_t type, SEXP sexp, SEXP/*LGLSXP*/ read_only_sexp, SEXP/*INTSXP*/ min_load_count_sexp) {
    const char *path = __extract_path_or_die(sexp);
    int32_t min_load_count = __extract_int_or_die(min_load_count_sexp);
    bool read_only = __extract_boolean_or_die(read_only_sexp);
    ufo_source_t *source = __make_source_or_die(type, path, NULL, 0, read_only, min_load_count);
    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    return ufo_new(source);
}

SEXP ufo_vectors_intsxp_bin(SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only_sexp, SEXP/*INTSXP*/ min_load_count_sexp) {
    return __make_vector(UFO_INT, path, read_only_sexp, min_load_count_sexp);
}

SEXP ufo_vectors_realsxp_bin(SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only_sexp, SEXP/*INTSXP*/ min_load_count_sexp) {
    return __make_vector(UFO_REAL, path, read_only_sexp, min_load_count_sexp);
}

SEXP ufo_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only_sexp, SEXP/*INTSXP*/ min_load_count_sexp) {
    return __make_vector(UFO_CPLX, path, read_only_sexp, min_load_count_sexp);
}

SEXP ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only_sexp, SEXP/*INTSXP*/ min_load_count_sexp) {
    return __make_vector(UFO_LGL, path, read_only_sexp, min_load_count_sexp);
}

SEXP ufo_vectors_rawsxp_bin(SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only_sexp, SEXP/*INTSXP*/ min_load_count_sexp) {
    return __make_vector(UFO_RAW, path, read_only_sexp, min_load_count_sexp);
}

SEXP/*NILSXP*/ ufo_store_bin(SEXP/*STRSXP*/ _path, SEXP vector) {
    __write_bytes_to_disk(__extract_path_or_die(_path),
                          Rf_length(vector) * __get_element_size(TYPEOF(vector)),
                          (const char *) DATAPTR_RO(vector));
    return R_NilValue;
}

// TODO I think we should remove this and assign dimensions to a vector in R.
SEXP __make_matrix(ufo_vector_type_t type, SEXP/*STRSXP*/ path_sexp, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*LGLSXP*/ read_only_sexp, SEXP/*INTSXP*/ min_load_count_sexp) {
    const char *path = __extract_path_or_die(path_sexp);
    bool read_only = __extract_boolean_or_die(read_only_sexp);
    int32_t min_load_count = __extract_int_or_die(min_load_count_sexp);
    int *dimensions = (int *) malloc(sizeof(int) * 2);
    dimensions[0] = __extract_int_or_die(rows);
    dimensions[1] = __extract_int_or_die(cols);
    ufo_source_t *source = __make_source_or_die(type, path, dimensions, 2, read_only, min_load_count);
    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new_multidim");

    return ufo_new(source);
}

SEXP ufo_matrix_intsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count) {
    return __make_matrix(UFO_INT, path, rows, cols, read_only, min_load_count);
}

SEXP ufo_matrix_realsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count) {
    return __make_matrix(UFO_REAL, path, rows, cols, read_only, min_load_count);
}

SEXP ufo_matrix_cplxsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count) {
    return __make_matrix(UFO_CPLX, path, rows, cols, read_only, min_load_count);
}

SEXP ufo_matrix_lglsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*LGLSXP*/ read_only, SEXP/*LGLSXP*/ min_load_count) {
    return __make_matrix(UFO_LGL, path, rows, cols, read_only, min_load_count);
}

SEXP ufo_matrix_rawsxp_bin(SEXP/*STRSXP*/ path, SEXP/*INTSXP*/ rows, SEXP/*INTSXP*/ cols, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count) {
    return __make_matrix(UFO_RAW, path, rows, cols, read_only, min_load_count);
}
