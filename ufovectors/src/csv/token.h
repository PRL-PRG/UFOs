#pragma once
#include <stdlib.h>

#ifdef USE_R_STUFF
#include <Rinternals.h>
#include <R_ext/Utils.h>
typedef Rboolean trinary_t;

#else
#include <math.h>
#include <limits.h>
#define NA_INTEGER INT_MIN
#define NA_REAL NAN
typedef enum {
    FALSE = 0, TRUE = 1, NA_LOGICAL = INT_MIN
} trinary_t;

#endif

typedef struct {
    /*const*/ char  *string; //const?
    size_t size;
    long   position_start;
    long   position_end;
} tokenizer_token_t;

typedef enum {
    TOKEN_NOTHING         = 0,
    TOKEN_EMPTY           = 1,
    TOKEN_NA              = 2,
    TOKEN_BOOLEAN         = 4,
    TOKEN_INTEGER         = 8,
    TOKEN_DOUBLE          = 16,
    TOKEN_STRING          = 32,
    TOKEN_INTERNED_STRING = 64,
    TOKEN_FREE_STRING     = 128,
} token_type_t;

tokenizer_token_t  *tokenizer_token_empty();
token_type_t        deduce_token_type(tokenizer_token_t *token);
char               *token_into_string(tokenizer_token_t *token);
const char         *token_type_to_string(token_type_t type);

trinary_t token_to_logical(tokenizer_token_t *token);
int token_to_integer(tokenizer_token_t *token);
double token_to_numeric(tokenizer_token_t *token);

size_t token_type_size(token_type_t type);