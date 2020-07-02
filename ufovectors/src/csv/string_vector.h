#pragma once

#include <stdlib.h>

typedef struct {
    size_t size;
    size_t allocated;
    char **strings;
} string_vector_t;

string_vector_t *string_vector_new   (size_t initial_size);
int              string_vector_append(string_vector_t *, char *);
int              string_vector_set   (string_vector_t *, size_t, char *);
char            *string_vector_get   (string_vector_t *, size_t);
void             string_vector_free  (string_vector_t *);