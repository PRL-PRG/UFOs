#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

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

typedef enum {
    TOKENIZER_OK,
    TOKENIZER_END_OF_ROW,
    TOKENIZER_END_OF_FILE,
    TOKENIZER_PARSE_ERROR,
    TOKENIZER_ERROR,
} tokenizer_result_t;

typedef struct {
    char  *string; //const?
    size_t size;
    long   position_start;
    long   position_end;
} tokenizer_token_t;

typedef struct {
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
    uint64_t rows;
    uint32_t columns;
    uint32_t max_columns;
    long offset;
    long offset_row_record_interval;
    long *offset_row_record;
    size_t offset_row_record_size;
    size_t offset_row_record_max_size;
} tokenizer_scan_info_t;

typedef struct {
    FILE                     *file;
    tokenizer_state_value_t   state;
    long                      initial_offset; // long because fseek offset uses long
    long                      read_characters;
    long                      current_offset;
    long                      end_of_last_token;
    tokenizer_read_buffer_t   read_buffer;
    tokenizer_token_buffer_t *token_buffer;
} tokenizer_state_t;

tokenizer_t               csv_tokenizer ();

tokenizer_state_t        *tokenizer_state_init (char *path, long initial_offset, size_t max_token_size, size_t character_buffer_size);
void                      tokenizer_state_close (tokenizer_state_t *);

tokenizer_token_buffer_t *tokenizer_token_buffer_init (size_t max_size);
tokenizer_token_t        *tokenizer_token_buffer_get_token (tokenizer_token_buffer_t *buffer, bool trim_trailing);
int                       tokenizer_token_buffer_append (tokenizer_token_buffer_t *buffer, char c);
void                      tokenizer_token_buffer_clear (tokenizer_token_buffer_t *buffer);
void                      tokenizer_token_buffer_free (tokenizer_token_buffer_t *buffer);

tokenizer_token_t        *tokenizer_token_empty ();

const char               *tokenizer_result_to_string (tokenizer_result_t);
const char               *tokenizer_state_to_string (tokenizer_state_value_t);

tokenizer_result_t tokenizer_next (tokenizer_t *tokenizer, tokenizer_state_t *state, tokenizer_token_t **token);
int                tokenizer_start(tokenizer_t *tokenizer, tokenizer_state_t *state);
void               tokenizer_close(tokenizer_t *tokenizer, tokenizer_state_t *state);

tokenizer_scan_info_t scan_info_init (size_t row_record_interval, size_t initial_record_size);
int tokenizer_scan (tokenizer_t *tokenizer, const char *path, tokenizer_scan_info_t *info, size_t buffer_size);


typedef enum {
    TOKEN_EMPTY   = 1,
    TOKEN_NA      = 2,
    TOKEN_BOOLEAN = 4,
    TOKEN_INTEGER = 8,
    TOKEN_DOUBLE  = 16,
    TOKEN_STRING  = 32,
} token_type_t;

const char *token_type_to_string(token_type_t);

typedef struct {
    size_t rows;
    size_t columns;
    token_type_t *column_types;
} scan_results_t;

scan_results_t *ufo_csv_perform_initial_scan(char* path);