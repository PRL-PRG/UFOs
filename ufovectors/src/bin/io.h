#pragma once

#include <stdio.h>
#include <stdint.h>

/**
 * Load a range of values from a binary file.
 *
 * Note: MappedMemory core should ensure that start and end never exceed the
 * actual size of the vector.
 *
 * @param user_data Structure containing configuration information for the.
 *                  loader (path, element size), must be ufo_file_source_data_t.
 * @param start First index of the range.
 * @param end Last index within the range.
 * @param target The area of memory where the data from the file will be loaded.
 * @return 0 on success, 42 if seek failed to find the required index in the
 *         file, 44 if reading failed.
 */
int32_t __load_from_file(
    void* user_data,
    uintptr_t start, uintptr_t end,
    unsigned char* target);

// int __save_to_file(
//     UfoPopulateData user_data,
//     uint64_t start, uint64_t end,
//     void* target);
void __write_bytes_to_disk(const char *path, size_t size, const char *bytes);
long __get_vector_length_from_file_or_die(const char * path, size_t element_size);
FILE *__open_file_or_die(char const *path);