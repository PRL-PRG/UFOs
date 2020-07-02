#pragma once
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    size_t allocated;
    size_t size;
    char **strings;
} string_set_t;

string_set_t *string_set_new (size_t initial_size);

bool string_set_contains(string_set_t *set, char *string);

//string must be null terminated
//string is copied
bool string_set_add(string_set_t *set, char *string);
size_t string_set_size(string_set_t *set);
void string_set_free(string_set_t *set);