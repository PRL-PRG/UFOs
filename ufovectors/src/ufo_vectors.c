#include <stdio.h>
#include <stdlib.h>

#include "../include/ufos.h"
#include "ufo_vectors.h"

#include "../include/mappedMemory/userfaultCore.h"

typedef struct {
    const char*         path;
    ufo_vector_type_t   vector_type;
    size_t              element_size; /* in bytes */
    size_t              vector_size;
} ufo_file_source_data_t;

int ufo_initialized = 0;
int ufo_debug_mode = 0;

int __extract_boolean_or_die(SEXP/*STRSXP*/ sexp) {
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

SEXP/*NILSXP*/ ufo_vectors_set_debug_mode(SEXP/*LGLSXP*/ debug) {
    ufo_debug_mode = __extract_boolean_or_die(debug);
    return R_NilValue;
}

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
 * Note: MappedMemory core should ensure that start and end never exceed the
 * actual size of the vector.
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
                     ufUserData user_data, char* target) {

    ufo_file_source_data_t* data = (ufo_file_source_data_t*) user_data;
    //size_t size_of_memory_fragment = end - start + 1;

    if (ufo_debug_mode) {
        Rprintf("__load_from_file\n");
        Rprintf("    start index: %li\n", start);
        Rprintf("      end index: %li\n", end);
        Rprintf("  target memory: %p\n", (void *) target);
        Rprintf("    source file: %s\n", data->path);
        Rprintf("    vector type: %d\n", data->vector_type);
        Rprintf("    vector size: %li\n", data->vector_size);
        Rprintf("   element size: %li\n", data->element_size);
    }

    // FIXME concurrency
    FILE* file = fopen(data->path, "rb");

    if (!file) {
        fprintf(stderr, "Could not open file.\n"); //TODO use proper channels
        return -1;
    }

    int initial_seek_status = fseek(file, 0L, SEEK_END);
    if (initial_seek_status < 0) {
        // Could not seek in from file.
        fprintf(stderr, "Could not seek to the end of file.\n"); //TODO use proper channels
        return 1;
    }

    long file_size_in_bytes = ftell(file);
    //fprintf(stderr, "file_size=%li\n", file_size_in_bytes);

    long start_reading_from = data->element_size * start;
    //fprintf(stderr, "start_reading_from=%li\n", start_reading_from);
    if (start_reading_from > file_size_in_bytes) {
        // Start index out of bounds of the file.
        fprintf(stderr, "Start index out of bounds of the file.\n"); //TODO use proper channels
        return 42;
    }

    long end_reading_at = data->element_size * end;
    //fprintf(stderr, "end_reading_at=%li\n", end_reading_at);
    if (end_reading_at > file_size_in_bytes) {
        // End index out of bounds of the file.
        fprintf(stderr, "End index out of bounds of the file.\n"); //TODO use proper channels
        return 43;
    }

    int rewind_seek_status = fseek(file, start_reading_from, SEEK_SET);
    if (rewind_seek_status < 0) {
        // Could not seek in the file to position at start index.
        fprintf(stderr, "Could not seek in the file to position at start index.\n"); //TODO use proper channels
        return 2;
    }

    size_t read_status = fread(target, data->element_size, end - start, file);
    if (read_status < end - start || read_status == 0) {
        // Read failed.
        fprintf(stderr, "Read failed. Read %li out of %li elements.\n", read_status, end - start); //TODO use proper channels
        return 44;
    }

    fclose(file);
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
        size_t write_status = fwrite(target, data->element_size, end-start, file);
        if (write_status < end-start || write_status == 0) {
            return 47;
        }
    }

    fclose(file);
    return 0;
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
        case UFO_RAW:
            return sizeof(Rbyte);
        default:
            Rf_error("Unrecognized vector type: %d\n", vector_type);
    }
}

long __get_vector_length_from_file_or_die(const char * path, size_t element_size) {

    // FIXME concurrency
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        Rf_error("Could not open file.\n");
    }

    int seek_status = fseek(file, 0L, SEEK_END);
    if (seek_status < 0) {
        // Could not seek in from file.
        fclose(file);
        Rf_error("Could not seek to the end of file.\n");
    }

    long file_size_in_bytes = ftell(file);
    fclose(file);

    if (file_size_in_bytes % element_size != 0) {
        Rf_error("File size not divisible by element size.\n");
    }

    return file_size_in_bytes / element_size;
}

void __check_file_path_or_die(const char * path) {

}

ufo_source_t* __make_source(ufo_vector_type_t type, const char* path) {
    __check_file_path_or_die(path);

    ufo_file_source_data_t *data = (ufo_file_source_data_t*) malloc(sizeof(ufo_file_source_data_t));

    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));
    source->population_function = &__load_from_file;
    source->data = (ufUserData*) data;
    source->vector_type = type;
    source->element_size = __get_element_size(type);
    source->vector_size = __get_vector_length_from_file_or_die(path, source->element_size);

//    fprintf(stderr, "source->vector_type=%i\n",source->vector_type);
//    fprintf(stderr, "source->element_size=%li\n",source->element_size);
//    fprintf(stderr, "source->vector_size=%li\n",source->vector_size);

    data->path = path;
    data->vector_type = source->vector_type;
    data->vector_size = source->vector_size;
    data->element_size = source->element_size;

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
    ufo_source_t *source = __make_source(type, path);
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

//SEXP ufo_vectors_strsxp_bin(SEXP/*STRSXP*/ path) {
//    Rf_error("ufo_vectors_strsxp_bin not implemented");
//    //return __make_vector(UFO_STR, path);
//}

SEXP ufo_vectors_lglsxp_bin(SEXP/*STRSXP*/ path) {
    return __make_vector(UFO_LGL, path);
}

SEXP ufo_vectors_rawsxp_bin(SEXP/*STRSXP*/ path) {
    return __make_vector(UFO_RAW, path);
}

void __write_bytes_to_disk(const char *path, size_t size, const char *bytes) {
    //fprintf(stderr, "__write_bytes_to_disk(%s,%li,...)\n", path, size);

    FILE* file = fopen(path, "wb");

    if (!file) {
        fclose(file);
        Rf_error("Error opening file '%s'.", path);
    }

    size_t write_status = fwrite(bytes, sizeof(const char), size, file);
    if (write_status < size || write_status == 0) {
        fclose(file);
        Rf_error("Error writing to file '%s'. Written: %i out of %li",
                 path, write_status, size);
    }

    fclose(file);
}

SEXP/*NILSXP*/ ufo_store_bin(SEXP/*STRSXP*/ _path, SEXP vector) {
    __write_bytes_to_disk(__extract_path_or_die(_path),
                          Rf_length(vector) * __get_element_size(TYPEOF(vector)),
                          (const char *) DATAPTR_RO(vector));
    return R_NilValue;
}