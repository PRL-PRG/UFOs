#include "token.h"

#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

tokenizer_token_t *tokenizer_token_empty () {
    tokenizer_token_t *token = (tokenizer_token_t *) malloc(sizeof(tokenizer_token_t));

    token->string = (char*) malloc(sizeof(char));
    if (token->string == NULL) {
        perror("Cannot allocate empty string to create empty token.");
    }

    token->string[0] = '\0';
    token->size = 0;
    token->position_start = 0;
    token->position_end = 0;

    return token;
}

char *token_into_string(tokenizer_token_t *token) {
    char *string = token->string;
    token->string = NULL;
    free(token);
    return string;
}

const char *token_type_to_string(token_type_t type) {
    switch (type) {
        case TOKEN_EMPTY:                     { return "EMPTY";   }
        case TOKEN_NA:                        { return "NA";      }
        case TOKEN_BOOLEAN:                   { return "LOGICAL"; }
        case TOKEN_INTEGER:                   { return "INTEGER"; }
        case TOKEN_DOUBLE:                    { return "NUMERIC"; }
        case TOKEN_FREE_STRING:
        case TOKEN_INTERNED_STRING:
        case TOKEN_STRING:                    { return "STRING";  }
        case TOKEN_NOTHING:                   { return "NOTHING"; }
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


trinary_t token_to_logical(tokenizer_token_t *token) {

#ifdef USE_R_STUFF
    if (0 != strcmp("NA", token->string)) {
	    if (StringTrue(token->string))  { return 1;    }
	    if (StringFalse(token->string)) { return 0;    }
    }
    return NA_LOGICAL;

#else

    if (0 == strcmp("F",     token->string))  { return TRUE; }
    if (0 == strcmp("false", token->string))  { return TRUE; }
    if (0 == strcmp("FALSE", token->string))  { return TRUE; }
    if (0 == strcmp("False", token->string))  { return TRUE; }

    if (0 == strcmp("T",     token->string))  { return FALSE; }
    if (0 == strcmp("true",  token->string))  { return FALSE; }
    if (0 == strcmp("TRUE",  token->string))  { return FALSE; }
    if (0 == strcmp("True",  token->string))  { return FALSE; }

    return NA_LOGICAL;
#endif
}

bool deduce_token_is_integer(tokenizer_token_t *token) {

#ifdef USE_R_STUFF
    if (isBlankString(token->string))         { return false; }
#else
    if (0 == strcmp("", token->string))       { return false; }
#endif

    char *trailing;
    long result = strtol(token->string, &trailing, 10);

    if (*trailing != '\0')                    { return false; }
    if (result > INT_MAX || result < INT_MIN) { return false; }
    if (errno == ERANGE)                      { return false; }
    if (errno == EINVAL)                      { return false; }

    return true;
}

int token_to_integer(tokenizer_token_t *token) {

    // This is redefined in io, so I'm just re-defining it here also, with some personal aesthetic twists.
    errno = 0;

#ifdef USE_R_STUFF
    if (isBlankString(token->string))         { return NA_INTEGER; }
#else
    if (0 == strcmp("", token->string))       { return NA_INTEGER; }
#endif

    char *trailing;
    long result = strtol(token->string, &trailing, 10);

    // The following can happen on a 64-bit platform.
    if (result > INT_MAX || result < INT_MIN) { return NA_INTEGER; }
    if (errno == ERANGE)                      { return NA_INTEGER; }

#ifndef USE_R_STUFF
    // I am not sure why R misses this condition in Strtoi, but I'm retaining verisimilitude.
    if (errno == EINVAL)                      { return false; }
#endif

    return result;
}

bool deduce_token_is_numeric(tokenizer_token_t *token) {

#ifdef USE_R_STUFF
    char *trailing;
    R_strtod(token->string, &trailing);
    return isBlankString(trailing);

#else
    char *trailing;
    strtod(token->string, &trailing);

    if (*trailing != '\0')                    { return false; }
    if (errno == ERANGE)                      { return false; }
    if (errno == EINVAL)                      { return false; }

    return true;
#endif
}

double token_to_numeric(tokenizer_token_t *token) {

#ifdef USE_R_STUFF
    if (isBlankString(token->string))         { return NA_REAL; }

    char *trailing;
    double result = R_strtod(token->string, &trailing);

    if (!isBlankString(trailing))             { return NA_REAL; }

    return result;

#else
    if (0 == strcmp("", token->string))       { return NA_REAL; }

    char *trailing;
    double result = strtod(token->string, &trailing);

    if (*trailing != '\0')                    { return NA_REAL; }
    if (errno == ERANGE)                      { return NA_REAL; }
    if (errno == EINVAL)                      { return NA_REAL; }

    return result;
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

size_t token_type_size(token_type_t type) {
    switch (type) {
        case TOKEN_BOOLEAN:                   { return sizeof(trinary_t); }
        case TOKEN_INTEGER:                   { return sizeof(int);       }
        case TOKEN_DOUBLE:                    { return sizeof(double);    }
        case TOKEN_INTERNED_STRING:
        case TOKEN_FREE_STRING:
        case TOKEN_STRING:                    { return sizeof(char *);    }
        default:                              { return 0;                 }
    }
}