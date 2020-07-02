#include "string_set.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//FIXME a tree would be better

string_set_t *string_set_new (size_t initial_size) {
    string_set_t *set = (string_set_t *) malloc(sizeof(string_set_t));
    set->allocated = initial_size;
    set->size = 0;
    set->strings = (char **) calloc(initial_size, sizeof(char *));
    return set;
}

bool string_set_contains(string_set_t *set, char *string) {
    for (size_t i = 0; i < set->size; i++) {
        if (0 == strcmp(set->strings[i], string)) {
            return true;
        }
    }
    return false;
}

size_t string_set_size(string_set_t *set) {
    return set->size;
}

bool string_set_add(string_set_t *set, char *string) {
    if (string_set_contains(set, string)) {
        return true;
    }

    if (set->size >= set->allocated) {
        size_t new_allocated = set->allocated + (set->allocated >> 1);
        set->strings = (char **) realloc(set->strings, new_allocated * sizeof(char *));

        if (NULL == set->strings) {
            return false;
        }

        set->allocated = new_allocated;
    }

    set->strings[set->size] = (char *) malloc(sizeof(char) * (strlen(string) + 1/*\0*/));
    if (NULL != set->strings[set->size]) {
        strcpy(set->strings[set->size], string);
        set->size++;
        return true;
    } else {
        return false;
    }
}

void string_set_free(string_set_t *set) {
    for (size_t i = 0; i < set->size; i++) {
        free(set->strings[i]);
    }
    free(set->strings);
    free(set);
}