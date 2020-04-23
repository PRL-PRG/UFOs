#include "token.h"

#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#ifdef USE_R_STUFF
#include <R_ext/Utils.h>
#endif

tokenizer_token_t *tokenizer_token_empty () {
    tokenizer_token_t *token = (tokenizer_token_t *) malloc(sizeof(tokenizer_token_t));

    token->string = NULL;
    token->size = 0;
    token->position_start = 0;
    token->position_end = 0;

    return token;
}

const char *token_type_to_string(token_type_t type) {
    switch (type) {
        case TOKEN_EMPTY:                     { return "EMPTY";   }
        case TOKEN_NA:                        { return "NA";      }
        case TOKEN_BOOLEAN:                   { return "LOGICAL"; }
        case TOKEN_INTEGER:                   { return "INTEGER"; }
        case TOKEN_DOUBLE:                    { return "NUMERIC"; }
        case TOKEN_STRING:                    { return "STRING";  }
    }
    return "U.N. Owen";
}

bool deduce_token_is_empty(tokenizer_token_t *token) {
    if (1 == token->size) {
        return token->string[0] == '\0';
    }
    if (0 == token->size) {
        return true;
    }
    return false;
}

bool deduce_token_is_na(tokenizer_token_t *token) {
    return 0 == strcmp("NA", token->string);
}

bool deduce_token_is_logical(tokenizer_token_t *token) {

#ifdef USE_R_STUFF
    if (StringFalse(token->string))           { return true; }
    if (StringTrue(token->string))            { return true; }

    return false;
#else

    if (0 == strcmp("F",     token->string))  { return true; }
    if (0 == strcmp("false", token->string))  { return true; }
    if (0 == strcmp("FALSE", token->string))  { return true; }
    if (0 == strcmp("False", token->string))  { return true; }

    if (0 == strcmp("T",    token->string))   { return true; }
    if (0 == strcmp("true", token->string))   { return true; }
    if (0 == strcmp("TRUE", token->string))   { return true; }
    if (0 == strcmp("True", token->string))   { return true; }

    return false;
#endif
}

bool deduce_token_is_integer(tokenizer_token_t *token) {

    char *trailing;
    long result = strtol(token->string, &trailing, 10);

    if (*trailing != '\0')                    { return false; }
    if (result > INT_MAX || result < INT_MIN) { return false; }
    if (errno == ERANGE)                      { return false; }
    if (errno == EINVAL)                      { return false; }

    return true;
}

bool deduce_token_is_numeric(tokenizer_token_t *token) {

#ifdef USE_R_STUFF
    char *end;
    R_strtod(token->string, &end);
    return isBlankString(end);

#else

    char *trailing;
    /*double result =*/ strtod(token->string, &trailing);

    if (*trailing != '\0')                    { return false; }
    if (errno == ERANGE)                      { return false; }
    if (errno == EINVAL)                      { return false; }

    return true;
#endif
}

token_type_t deduce_token_type(tokenizer_token_t *token) {

    if (deduce_token_is_empty(token))         { return TOKEN_EMPTY;   }
    if (deduce_token_is_na(token))            { return TOKEN_NA;      }
    if (deduce_token_is_logical(token))       { return TOKEN_BOOLEAN; }
    if (deduce_token_is_integer(token))       { return TOKEN_INTEGER; }
    if (deduce_token_is_numeric(token))       { return TOKEN_DOUBLE;  }

    return TOKEN_STRING;
}

