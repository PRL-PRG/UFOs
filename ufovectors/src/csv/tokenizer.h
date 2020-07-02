#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "token.h"

typedef struct {
    char row_delimiter;
    char column_delimiter;
    char escape;
    char quote;
    bool use_double_quote_escape;
    bool use_escape_character;
    //bool strip_whitespace;
} tokenizer_t;

typedef enum {
    TOKENIZER_OK,
    TOKENIZER_END_OF_ROW,
    TOKENIZER_END_OF_FILE,
    TOKENIZER_PARSE_ERROR,
    TOKENIZER_ERROR,
} tokenizer_result_t;

typedef enum {
    TOKENIZER_INITIAL,
    TOKENIZER_FIELD,
    TOKENIZER_UNQUOTED_FIELD,
    TOKENIZER_QUOTED_FIELD,
    TOKENIZER_QUOTE,
    TOKENIZER_TRAILING,
    TOKENIZER_ESCAPE,
    TOKENIZER_FINAL,
    TOKENIZER_CRASHED,
}  tokenizer_state_value_t;

typedef struct { // Maybe I should conceal state here.
    char  *buffer;
    size_t max_size;
    size_t size;
} tokenizer_token_buffer_t;

typedef struct {
    char  *buffer;
    size_t max_size;
    size_t size;
    size_t pointer;
} tokenizer_read_buffer_t;

typedef struct {
    FILE                     *file;
    tokenizer_state_value_t   state;
    long                      initial_offset; // long because fseek offset uses long
    long                      read_characters;
    long                      current_offset;
    long                      end_of_last_token;
    tokenizer_read_buffer_t  *read_buffer;
    tokenizer_token_buffer_t *token_buffer;
} tokenizer_state_t;

tokenizer_t               csv_tokenizer ();
tokenizer_t              *new_csv_tokenizer ();
void                      tokenizer_free(tokenizer_t*);

tokenizer_result_t        tokenizer_next (tokenizer_t *tokenizer, tokenizer_state_t *state, tokenizer_token_t **token, bool skip);
int                       tokenizer_start(tokenizer_t *tokenizer, tokenizer_state_t *state);
void                      tokenizer_close(tokenizer_t *tokenizer, tokenizer_state_t *state);

tokenizer_state_t        *tokenizer_state_init (const char *path, long initial_offset, size_t token_buffer_size, size_t character_buffer_size);
void                      tokenizer_state_close (tokenizer_state_t *);
const char               *tokenizer_state_to_string (tokenizer_state_value_t);

const char               *tokenizer_result_to_string (tokenizer_result_t);