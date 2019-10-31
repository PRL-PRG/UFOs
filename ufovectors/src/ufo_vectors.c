#include <stdio.h>
#include <stdlib.h>

#include "../include/ufos.h"
#include "ufo_vectors.h"
#include "ufo_metadata.h"
#include "helpers.h"
#include "debug.h"
#include "ufo_file_manipulation.h"

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
        default:
            Rf_error("Unrecognized vector type: %d\n", vector_type);
    }
}

void __check_file_path_or_die(const char * path) {

}

void __destroy(ufUserData *user_data) {
    ufo_file_source_data_t *data = (ufo_file_source_data_t*) user_data;
    if (__get_debug_mode()) {
        Rprintf("__destroy\n");
        Rprintf("    source file: %s\n", data->path);
        Rprintf("    vector type: %d\n", data->vector_type);
        Rprintf("    vector size: %li\n", data->vector_size);
        Rprintf("   element size: %li\n", data->element_size);
    }
    fclose(data->file_handle);
}

ufo_source_t* __make_source_or_die(ufo_vector_type_t type, const char* path) {
    __check_file_path_or_die(path);

    ufo_file_source_data_t *data = (ufo_file_source_data_t*) malloc(sizeof(ufo_file_source_data_t));

    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));
    source->population_function = &__load_from_file;
    source->destructor_function = &__destroy;
    source->data = (ufUserData*) data;
    source->vector_type = type;
    source->element_size = __get_ufo_element_size(type);
    source->vector_size = __get_vector_length_from_file_or_die(path, source->element_size);
    source->dimensions = NULL;

    data->path = path;
    data->vector_type = source->vector_type;
    data->vector_size = source->vector_size;
    data->element_size = source->element_size;

    data->file_handle = __open_file_or_die(path);
    data->file_cursor = 0;

    return source;
}

SEXP/*NILSXP*/ ufo_vectors_shutdown() {
    if (ufo_initialized != 0) {
        ufo_shutdown_t ufo_shutdown =
                (ufo_shutdown_t) R_GetCCallable("ufos", "ufo_shutdown");
        ufo_shutdown();
        ufo_initialized = 0;
    }
    return R_NilValue;
}

void ufo_vectors_initialize_if_necessary() {
    if (ufo_initialized == 0) {
        ufo_initialize_t ufo_initialize =
                (ufo_initialize_t) R_GetCCallable("ufos", "ufo_initialize");
        ufo_initialize();
        ufo_initialized = 1;
    }
}

SEXP __make_vector(ufo_vector_type_t type, SEXP sexp) {
    const char *path = __extract_path_or_die(sexp);
    ufo_source_t *source = __make_source_or_die(type, path);
    ufo_vectors_initialize_if_necessary();
    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    return ufo_new(source);
}

SEXP ufo_vectors_intsxp_bin(SEXP/*STRSXP*/ path) {
    return __make_vector(UFO_INT, path);
}

SEXP ufo_vectors_realsxp_bin(SEXP/*STRSXP*/ path) {
    return __make_vector(UFO_REAL, path);
}

SEXP ufo_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path) {
    return __make_vector(UFO_CPLX, path);
}

SEXP ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path) {
    return __make_vector(UFO_LGL, path);
}

SEXP ufo_vectors_rawsxp_bin(SEXP/*STRSXP*/ path) {
    return __make_vector(UFO_RAW, path);
}

SEXP/*NILSXP*/ ufo_store_bin(SEXP/*STRSXP*/ _path, SEXP vector) {
    __write_bytes_to_disk(__extract_path_or_die(_path),
                          Rf_length(vector) * __get_element_size(TYPEOF(vector)),
                          (const char *) DATAPTR_RO(vector));
    return R_NilValue;
}