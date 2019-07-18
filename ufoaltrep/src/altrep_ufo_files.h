#pragma once

#include "Rinternals.h"

size_t __get_element_size(SEXPTYPE vector_type);
long __get_vector_length_from_file_or_die(const char * path, size_t element_size);
int __extract_boolean_or_die(SEXP/*LGLSXP*/ sexp);
const char* __extract_path_or_die(SEXP/*STRSXP*/ path);
int __load_from_file(int debug, uint64_t start, uint64_t end, char const *path, size_t element_size, size_t vector_size, char* target);