#include <stdio.h>
#include <stdlib.h>
#include <Rinternals.h>

#include "ufo_sources.h"
#include "bin_file_source.h"
#include "mappedMemory/userfaultCore.h"

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
 * @param path A SEXP containing the file path to validate.
 */
void __validate_path_or_die(SEXP/*STRSXP*/ path) {
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
                     ufUserData user_data, void* target) {
    ufo_file_source_data_t* data = (ufo_file_source_data_t*) user_data;
    FILE* file = fopen(data->path, "rb");

    if (file) {
        int seek_status = fseek(file, data->element_size * start, SEEK_SET);
        if (seek_status < 0) {
            return 42;
        }
        int read_status = fread(target, data->element_size, end-start, file);
        if (read_status < end-start || read_status == 0) {
            return 44;
        }
    }

    fclose(file);
    return 0;
}

// TODO not actually implementing anything in userfaultCore.h
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

/**
 * Create a new binary source for UFO vectors. This source reads/writes data
 * to/from a binary file.
 *
 * @param path
 * @return Returns SEXP of type EXTPTR, containing a pointer to a
 */
SEXP/*EXTPTRSXP*/ ufo_bin_file_source(SEXP/*STRSXP*/ sexp) {
    __validate_path_or_die(sexp);

    const char* path = CHAR(STRING_ELT(sexp, 0));

    ufo_file_source_data_t *data =
            (ufo_file_source_data_t*) malloc(sizeof(ufo_file_source_data_t));
    data->path = path;

    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));
    source->population_function = &__load_from_file;
    source->data = (ufUserData*) data;

    data->element_size = __get_element_size(source->vector_type);

    return R_MakeExternalPtr(source, install("ufo_source"), R_NilValue);
}