#include "string_vector.h"

#include <assert.h>
#include <stdio.h>

string_vector_t *string_vector_new(size_t initial_size) {
    assert(initial_size > 0);
    string_vector_t *vector = (string_vector_t *) malloc(sizeof(string_vector_t));
    if (vector == NULL) {
        return NULL;
    }
    vector->size = 0;
    vector->allocated = initial_size;
    vector->strings = (char**) calloc(initial_size, sizeof(char *) * initial_size);
    if (vector->strings == NULL) {
        free(vector);
        return NULL;
    }
    return vector;
}

int string_vector_append(string_vector_t * vector, char *string) {
    if (vector->size == vector->allocated) {
        vector->allocated += (vector->allocated >> 1);
        vector->strings = (char **) realloc(vector->strings, sizeof(char *) * vector->allocated);
        if (vector->strings == NULL) {
            perror("Error: cannot allocate memory to expand string vector");
            return -1;
        }
    }
    vector->strings[vector->size++] = string;
    return 0;
}

int string_vector_set(string_vector_t *vector, size_t index, char *string) {
    if (index >= vector->size) {
        return -1;
    } else {
        vector->strings[index] = string;
        return 0;
    }
}

char *string_vector_get(string_vector_t *vector, size_t index) {
    if (index >= vector->size) {
        return NULL;
    } else {
        return vector->strings[index];
    }
}

void string_vector_free(string_vector_t *vector) {
    free(vector->strings);
    free(vector);
}