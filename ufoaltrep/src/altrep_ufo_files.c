#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "altrep_ufo_vectors.h"

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


long __get_vector_length_from_file_or_die(const char *path, size_t element_size) {

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

int __load_from_file(int debug, uint64_t start, uint64_t end, char const *path, size_t element_size, size_t vector_size, char* target) {

    if (debug) {
        Rprintf("__load_from_file\n");
        Rprintf("    start index: %li\n", start);
        Rprintf("      end index: %li\n", end);
        Rprintf("  target memory: %p\n", (void *) target);
        Rprintf("    source file: %s\n", path);
//      Rprintf("    vector type: %d\n", data->vector_type);
        Rprintf("    vector size: %li\n", vector_size);
        Rprintf("   element size: %li\n", element_size);
    }

    // FIXME concurrency
    FILE* file = fopen(path, "rb");

    if (!file) {
        fprintf(stderr, "Could not open file.\n");
        return -1;
    }

    int initial_seek_status = fseek(file, 0L, SEEK_END);
    if (initial_seek_status < 0) {
        // Could not seek in from file.
        fprintf(stderr, "Could not seek to the end of file.\n");
        return 1;
    }

    long file_size_in_bytes = ftell(file);
    //fprintf(stderr, "file_size=%li\n", file_size_in_bytes);

    long start_reading_from = element_size * start;
    //fprintf(stderr, "start_reading_from=%li\n", start_reading_from);
    if (start_reading_from > file_size_in_bytes) {
        // Start index out of bounds of the file.
        fprintf(stderr, "Start index out of bounds of the file.\n");
        return 42;
    }

    long end_reading_at = element_size * end;
    //fprintf(stderr, "end_reading_at=%li\n", end_reading_at);
    if (end_reading_at > file_size_in_bytes) {
        // End index out of bounds of the file.
        fprintf(stderr, "End index out of bounds of the file.\n");
        return 43;
    }

    int rewind_seek_status = fseek(file, start_reading_from, SEEK_SET);
    if (rewind_seek_status < 0) {
        // Could not seek in the file to position at start index.
        fprintf(stderr, "Could not seek in the file to position at start index.\n");
        return 2;
    }

    size_t read_status = fread(target, element_size, end - start, file);
    if (read_status < end - start || read_status == 0) {
        // Read failed.
        fprintf(stderr, "Read failed. Read %li out of %li elements.\n", read_status, end - start);
        return 44;
    }

    fclose(file);
    return 0;
}

// TODO this does not actually implement anything in userfaultCore.h yet
//int __save_to_file(uint64_t start, uint64_t end, ufPopulateCallout cf,
//                   ufUserData user_data, void* target) {
//    ufo_file_source_data_t* data = (ufo_file_source_data_t*) user_data;
//    FILE* file = fopen(data->path, "wb");
//
//    if (file) {
//        // TODO "allocate" if necessary.
//        int seek_status = fseek(file, data->element_size * start, SEEK_SET);
//        if (seek_status < 0) {
//            return 42;
//        }
//        size_t write_status = fwrite(target, data->element_size, end-start, file);
//        if (write_status < end-start || write_status == 0) {
//            return 47;
//        }
//    }
//
//    fclose(file);
//    return 0;
//}

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