#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <stdint.h>
#include "metadata.h"

long __get_vector_length_from_file_or_die(const char *path, size_t element_size);
int __load_from_file(int debug, uint64_t start, uint64_t end, altrep_ufo_config_t* cfg, char* target);
void __write_bytes_to_disk(const char *path, size_t size, const char *bytes);
FILE *__open_file_or_die(char const *path);