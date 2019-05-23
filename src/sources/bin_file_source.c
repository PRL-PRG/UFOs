#include <stdio>
#include <stdlib>
#include <Rinternals.h>

#include "sources/sources.h"
#include "mappedMemory/userfaultCore.h"

typedef struct {
    const char* path;
    size_t element_size; /* in bytes */
} ufo_file_source_data;

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

int __load_from_file(uint64_t start, uint64_t end, ufPopulateCallout cf,
                     ufUserData user_data, void* target) {
    ufo_source_data* data = (ufo_source_data*) user_data;
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
}

// TODO not actually implementing anything in userfaultCore.h
int __save_to_file(uint64_t start, uint64_t end, ufPopulateCallout cf,
                   ufUserData user_data, void* target) {
    ufo_source_data* data = (ufo_source_data*) user_data;
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

size_t __get_element_size(enum vector_type_t vector_type) {
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
 * @return SEXP of type EXTPTR,
 */
SEXP/*EXTPTRSXP*/ ufo_make_bin_file_source(SEXP/*STRSXP*/ path) {
    __validate_path_or_die(path);

    const char* path = CHAR(STRING_ELT(path, 0));

    ufo_file_source_data *data =
            (ufo_file_source_data*) malloc(sizeof(ufo_file_source_data));
    data->path = path;

    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));
    source->population_function = &__load_from_file;
    source->data = (ufo_source_data*) data;

    data->element_size = __get_element_size(source->vector_type);

    return R_MakeExternalPtr(source, install("ufo_source"), R_NilValue);
}