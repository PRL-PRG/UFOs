#include "io.h"

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

#include "../ufo_metadata.h"
#include "../debug.h"

#include <stdint.h>

int32_t __load_from_file(void* user_data, uintptr_t start, uintptr_t end, unsigned char* target) {

    ufo_file_source_data_t* cfg = (ufo_file_source_data_t*) user_data;

    if (__get_debug_mode()) {
        REprintf("__load_from_file\n");
        REprintf("    start index: %li\n", start);
        REprintf("      end index: %li\n", end);
        REprintf("  target memory: %p\n", (void *) target);
        REprintf("    source file: %s\n", cfg->path);
        REprintf("    vector type: %d\n", cfg->vector_type);
        REprintf("    vector size: %li\n", cfg->vector_size);
        REprintf("   element size: %li\n", cfg->element_size);
    }

    int initial_seek_status = fseek(cfg->file_handle, 0L, SEEK_END);
    if (initial_seek_status < 0) {
        // Could not seek in from file.
        REprintf("Could not seek to the end of file.\n");
        return 1;
    }

    long file_size_in_bytes = ftell(cfg->file_handle);

    long start_reading_from = cfg->element_size * start;
    if (start_reading_from > file_size_in_bytes) {
        // Start index out of bounds of the file.
        REprintf("Start index out of bounds of the file.\n");
        return 42;
    }

    long end_reading_at = cfg->element_size * end;
    if (end_reading_at > file_size_in_bytes) {
        // End index out of bounds of the file.
        REprintf("End index out of bounds of the file.\n");
        return 43;
    }

    int rewind_seek_status = fseek(cfg->file_handle, start_reading_from, SEEK_SET);
    if (rewind_seek_status < 0) {
        // Could not seek in the file to position at start index.
        REprintf("Could not seek in the file to position at start index.\n");
        return 2;
    }

    size_t read_status = fread(target, cfg->element_size, end - start, cfg->file_handle);
    if (read_status < end - start || read_status == 0) {
        // Read failed.
        REprintf("Read failed. Read %li out of %li elements.\n", read_status, end - start);
        return 44;
    }

    return 0;
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

FILE *__open_file_or_die(char const *path) {
    FILE * file = fopen(path, "rb");
    if (!file) {
        Rf_error("Could not open file.\n");
    }
    return file;
}
