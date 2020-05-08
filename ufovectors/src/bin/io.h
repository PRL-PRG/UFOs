#pragma once

#include <stdio.h>
#include <stdint.h>

#include "../../include/mappedMemory/userfaultCore.h"

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
                     ufUserData user_data, char* target);

int __save_to_file(uint64_t start, uint64_t end, ufPopulateCallout cf,
                   ufUserData user_data, void* target);
void __write_bytes_to_disk(const char *path, size_t size, const char *bytes);
long __get_vector_length_from_file_or_die(const char * path, size_t element_size);
FILE *__open_file_or_die(char const *path);