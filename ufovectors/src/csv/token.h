#pragma once
#include <stdlib.h>

typedef struct {
    /*const*/ char  *string; //const?
    size_t size;
    long   position_start;
    long   position_end;
} tokenizer_token_t;

typedef enum {
    TOKEN_EMPTY   = 1,
    TOKEN_NA      = 2,
    TOKEN_BOOLEAN = 4,
    TOKEN_INTEGER = 8,
    TOKEN_DOUBLE  = 16,
    TOKEN_STRING  = 32,
} token_type_t;

tokenizer_token_t        *tokenizer_token_empty();
token_type_t              deduce_token_type(tokenizer_token_t *token);
const char               *token_type_to_string(token_type_t type);
