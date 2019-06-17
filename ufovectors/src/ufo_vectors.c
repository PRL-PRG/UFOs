#include <stdio.h>
#include <stdlib.h>

#include "../include/ufos.h"
#include "ufo_vectors.h"

#include "../include/userfaultCore.h"

typedef struct {
    const char* path;
    size_t element_size; /* in bytes */
} ufo_file_source_data_t;

/**
 * Check if the path provided via this SEXP makes sense:
 *  - it contains strings,
 *  - it's has at least one element,
 *  - preferably it has only one element.
 *
 * If the number of elements in the path vector is too large the function exits
 * but prints a warning in the R interpreter. If the other requirements are not
 * met the function dies and shows a message to the R interpreter.
 *
 * If the path makes sense, it is is extracted into a C string.
 *
 * @param path A SEXP containing the file path to validate.
 * @return
 */
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

/**
 * Load a range of values from a binary file.
 *
 * @param start First index of the range.
 * @param end Last index within the range.
 * @param cf Callout function (not used).
 * @param user_data Structure containing configuration information for the.
 *                  loader (path, element size), must be ufo_file_source_data_t.
 * @param target The area of memory where the data from the file will be loaded.
 * @return 0 on success, 42 if seek failed to find the required index in the
 *         file, 44 if reading failed.
 */
int __load_from_file(uint64_t start, uint64_t end, ufPopulateCallout cf,
                     ufUserData user_data, char* target) { // FIXME ASK COLETTE
//    ufo_file_source_data_t* data = (ufo_file_source_data_t*) user_data;
//    FILE* file = fopen(data->path, "rb");
//
//    if (file) {
//        int seek_status = fseek(file, data->element_size * start, SEEK_SET);
//        if (seek_status < 0) {
//            return 42;
//        }
//        int read_status = fread(target, data->element_size, end-start, file);
//        if (read_status < end-start || read_status == 0) {
//            return 44;
//        }
//    }
//
//    fclose(file);

    int *x = (int *) target;
    for(int i = 0; i < end - start; i++) {
        x[i] = start + i;
    }

    return 0;
}

// TODO this does not actually implement anything in userfaultCore.h yet
int __save_to_file(uint64_t start, uint64_t end, ufPopulateCallout cf,
                   ufUserData user_data, void* target) {
    ufo_file_source_data_t* data = (ufo_file_source_data_t*) user_data;
    FILE* file = fopen(data->path, "wb");

    if (file) {
        // TODO "allocate" if necessary.
        int seek_status = fseek(file, data->element_size * start, SEEK_SET);
        if (seek_status < 0) {
            return 42;
        }
        int write_status = fwrite(target, data->element_size, end-start, file);
        if (write_status < end-start || write_status == 0) {
            return 47;
        }
    }

    fclose(file);
}

/**
 * Translates the type of UFO vector to the size of its element in bytes.
 *
 * @param vector_type The type of the vector as specified by the ufo_vector_type
 *                    enum.
 * @return Returns the size of the element vector or dies in case of an
 *         unrecognized vector type.
 */
size_t __get_element_size(ufo_vector_type_t vector_type) {
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
        default:
            Rf_error("Unrecognized vector type: %d\n", vector_type);
    }
}

ufo_source_t* __make_source(ufo_vector_type_t type, const char* path) {
    ufo_file_source_data_t *data =
            (ufo_file_source_data_t*) malloc(sizeof(ufo_file_source_data_t));
    data->path = path;

    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));
    source->population_function = &__load_from_file;
    source->data = (ufUserData*) data;
    source->vector_type = type;
    source->length = 256; //FIXME this should be arbitrary or discovered

    data->element_size = __get_element_size(source->vector_type);

    return source;
}

SEXP __make_vector(ufo_vector_type_t type, SEXP sexp) {
    const char * path = __extract_path_or_die(sexp);
    ufo_source_t* source = __make_source(type, path);
    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    return ufo_new(source);
}

SEXP ufo_vectors_intsxp_bin(SEXP/*STRSXP*/ path) {
    return __make_vector(UFO_INT, path);
}

//SEXP ufo_vectors_realsxp_bin(SEXP/*STRSXP*/ path) {
//    return __make_vector(UFO_REAL, path);
//}
//
//SEXP ufo_vectors_cplxsxp_bin(SEXP/*STRSXP*/ path) {
//    return __make_vector(UFO_CPLX, path);
//}
//
//SEXP ufo_vectors_strsxp_bin(SEXP/*STRSXP*/ path) {
//    Rf_error("ufo_vectors_strsxp_bin not implemented");
//    //return __make_vector(UFO_STR, path);
//}
//
//SEXP ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path) {
//    return __make_vector(UFO_LGL, path);
//}

SEXP ufo_vectors_initialize() {
    ufo_initialize_t ufo_initialize = (ufo_initialize_t) R_GetCCallable("ufos", "ufo_initialize");
    ufo_initialize();
    return R_NilValue;
}

SEXP ufo_vectors_shutdown() {
    ufo_shutdown_t ufo_shutdown = (ufo_shutdown_t) R_GetCCallable("ufos", "ufo_shutdown");
    ufo_shutdown();
    return R_NilValue;
}