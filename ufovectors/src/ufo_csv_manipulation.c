#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "ufo_csv_manipulation.h"

tokenizer_t csv_tokenizer () {
    tokenizer_t tokenizer;

    tokenizer.row_delimiter = '\n';
    tokenizer.column_delimiter = ',';
    tokenizer.escape = '\\';
    tokenizer.quote = '"';
    tokenizer.use_double_quote_escape = true;
    tokenizer.use_escape_character = true;

    return tokenizer;
}

tokenizer_state_t *tokenizer_state_init (char *path, long initial_offset, size_t buffer_size) {
    tokenizer_state_t *state = (tokenizer_state_t *) malloc(sizeof(tokenizer_state_t));
    if (state == NULL) {
        perror("Error: cannot allocate memory for state");
        return NULL;
    }

    state->file = fopen(path, "r");
    if (state->file == NULL) {
        perror("Error: cannot open file");
        return NULL;
    }

    state->initial_offset = initial_offset;
    state->read_characters = 0L;

    state->read_buffer = (char *) malloc(10 * sizeof(char)); // TODO actually use this
    state->token_buffer = tokenizer_token_buffer_init(buffer_size);

    return state;
}


char next_character (tokenizer_state_t *state) {
    // TODO buffering
    return fgetc(state->file);
}

tokenizer_token_buffer_t *tokenizer_token_buffer_init (size_t max_size) {
    tokenizer_token_buffer_t *buffer =
            (tokenizer_token_buffer_t *) malloc(sizeof(tokenizer_token_buffer_t));
    if (buffer == NULL) {
        perror("Error: cannot allocate memory for token buffer");
        return NULL;
    }

    buffer->max_size = max_size;
    buffer->size = 0;
    buffer->buffer = (char *) malloc(sizeof(char) * max_size);

    if (buffer->buffer == NULL) {
        perror("Error: cannot allocate memory for token buffer's internal buffer");
        return NULL;
    }

    return buffer;
}

tokenizer_token_t *tokenizer_token_buffer_get_token (tokenizer_token_buffer_t *buffer) {
    assert(buffer->size <= buffer->max_size);

    tokenizer_token_t *token = (tokenizer_token_t *) malloc(sizeof(tokenizer_token_t));
    token->size = buffer->size;
    token->string = (char *) malloc(sizeof(char) * buffer->size);
    if (token->string == NULL) {
        perror("Error: cannot allocate memory for token");
        return NULL;
    }

    token->string = memcpy(token->string, buffer->buffer, buffer->size);

    return token;
}

int tokenizer_token_buffer_append (tokenizer_token_buffer_t *buffer, char c) {
    assert(buffer->size <= buffer->max_size);
    if (buffer->size == buffer->max_size) {
        buffer->max_size += (buffer->max_size >> 1);
        buffer->buffer = (char *) realloc(buffer->buffer,
                                          sizeof(tokenizer_token_buffer_t) * buffer->max_size);
        if (buffer->buffer == NULL) {
            perror("Error: cannot allocate memory to expand token buffer's internal buffer");
            return -1;
        }
    }
    buffer->buffer[buffer->size++] = c;
    return 0;
}

void tokenizer_token_buffer_clear (tokenizer_token_buffer_t *buffer) {
    buffer->size = 0;
}
void tokenizer_token_buffer_free (tokenizer_token_buffer_t *buffer) {
    free(buffer->buffer);
    free(buffer);
}

void tokenizer_state_close (tokenizer_state_t *state) {
    fclose(state->file);
    free(state);
}

void pop_token (tokenizer_state_t *state, tokenizer_token_t **token) {

    *token = tokenizer_token_buffer_get_token (state->token_buffer);

    (*token)->position_start = state->end_of_last_token;
    (*token)->position_end = state->current_offset;
    state->end_of_last_token = (*token)->position_end;

    tokenizer_token_buffer_clear(state->token_buffer);
}

static inline void __transition(tokenizer_state_t *state, tokenizer_state_value_t next_state) {
    state->state = next_state;
    state->current_offset++;
}

static inline tokenizer_result_t __pop_and_yield(tokenizer_state_t *state, tokenizer_token_t **token, tokenizer_state_value_t next_state, tokenizer_result_t result) {
    __transition(state, next_state);
    pop_token(state, token);
    return result;
}

static inline int __append(tokenizer_state_t *state, char c, tokenizer_state_value_t next_state) {
    int result = tokenizer_token_buffer_append(state->token_buffer, c);
    if (result != 0) {
        perror("Error: Could not append character to token buffer.");
        return -1;
    }
    __transition(state, next_state);
    return 0;
}


int tokenizer_start(tokenizer_t *tokenizer, tokenizer_state_t *state) {
    int seek_status = fseek(state->file, state->initial_offset, SEEK_SET);
    if (seek_status < 0) {
        perror("Error: cannot seek to initial position");
        return 1;
    }

    state->state = TOKENIZER_FIELD;
    state->current_offset = state->initial_offset;
    return 0;
}

void tokenizer_close(tokenizer_t *tokenizer, tokenizer_state_t *state) {
    fclose(state->file);
}

tokenizer_result_t tokenizer_next (tokenizer_t *tokenizer, tokenizer_state_t *state, tokenizer_token_t **token) {

    while (true) {
        switch (state->state) {
            case TOKENIZER_FIELD: {
                char c = next_character(state);
                if (c == EOF)                         { return __pop_and_yield(state, token, TOKENIZER_FINAL, TOKENIZER_END_OF_FILE);               }
                if (c == tokenizer->column_delimiter) { return __pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK);                        }
                if (c == tokenizer->row_delimiter)    { return __pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW);                }
                if (c == ' ' || c == '\t')            {        __transition(state, TOKENIZER_FIELD);                                      continue; }
                if (c == tokenizer->quote)            {        __transition(state, TOKENIZER_QUOTED_FIELD);                               continue; }
                /* c is any other character */        { if   (!__append(state, c, TOKENIZER_UNQUOTED_FIELD))                              continue;
                                                        else { __transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
            }

            case TOKENIZER_UNQUOTED_FIELD: {
                char c = next_character(state);
                if (c == EOF)                         { return __pop_and_yield(state, token, TOKENIZER_FINAL, TOKENIZER_END_OF_FILE);               }
                if (c == tokenizer->column_delimiter) { return __pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK);                        }
                if (c == tokenizer->row_delimiter)    { return __pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW);                }
                /* c is any other character */        { if   (!__append(state, c, TOKENIZER_UNQUOTED_FIELD))                              continue;
                                                        else { __transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
            }

            case TOKENIZER_QUOTED_FIELD: {
                char c = next_character(state);
                if (c == tokenizer->quote)            {        __transition(state, TOKENIZER_QUOTE);                                      continue; }
                if (c == tokenizer->escape)           {        __transition(state, TOKENIZER_ESCAPE);                                     continue; }
                if (c == EOF)                         { return __pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR);             }
                /* c is any other character */        {        __append(state, c, TOKENIZER_QUOTED_FIELD);                                continue; }
            }

            case TOKENIZER_QUOTE: {
                char c = next_character(state);
                if (c == tokenizer->quote)            { if   (!__append(state, c, TOKENIZER_QUOTED_FIELD))                                continue;
                                                        else { __transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
                if (c == EOF)                         { return __pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR);             }
                if (c == ' ' || c == '\t')            {        __transition(state, TOKENIZER_TRAILING);                                   continue; }
                if (c == tokenizer->column_delimiter) { return __pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK);                        }
                if (c == tokenizer->row_delimiter)    { return __pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW);                }
                /* c is any other character */        {        __transition(state, TOKENIZER_CRASHED); return TOKENIZER_PARSE_ERROR;                }
            }

            case TOKENIZER_TRAILING: {
                char c = next_character(state);
                if (c == ' ' || c == '\t')            {        __transition(state, TOKENIZER_TRAILING);                                   continue; }
                if (c == tokenizer->column_delimiter) { return __pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK);                        }
                if (c == tokenizer->row_delimiter)    { return __pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW);                }
                if (c == EOF)                         { return __pop_and_yield(state, token, TOKENIZER_FINAL, TOKENIZER_END_OF_FILE);               }
                /* c is any other character */        { return __pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR);             }
            }

            case TOKENIZER_ESCAPE: {
                char c = next_character(state);
                if (c == tokenizer->quote)            { if   (!__append(state, c, TOKENIZER_QUOTED_FIELD))                                continue;
                                                        else { __transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
                if (c == EOF)                         { return __pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR);             }
                /* c is any other character */        { if   (!__append(state, tokenizer->escape, TOKENIZER_QUOTED_FIELD)
                                                        &&    !__append(state, c, TOKENIZER_QUOTED_FIELD))                                continue;
                                                        else { __transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
            }

            case TOKENIZER_INITIAL:                   { perror("Error: Tokenizer was now properly opened"); return TOKENIZER_ERROR;                 }
            case TOKENIZER_FINAL:                     { perror("Error: Tokenizer completed reading this file"); return TOKENIZER_ERROR;             }
            case TOKENIZER_CRASHED:                   { perror("Error: Tokenizer crashed and cannot continue"); return TOKENIZER_ERROR;             }
            default:                                  { perror("Error: unimplemented"); return TOKENIZER_ERROR;                                     }
        }

    }

}

const char *tokenizer_result_to_string (tokenizer_result_t result) {
    switch (result) {
        case TOKENIZER_OK:          return "OK";
        case TOKENIZER_END_OF_ROW:  return "END OF ROW";
        case TOKENIZER_END_OF_FILE: return "END OF FILE";
        case TOKENIZER_PARSE_ERROR: return "PARSE ERROR";
        default:                    return "ERROR";
    }
}

void perform_initial_scan(char* path) {




}

