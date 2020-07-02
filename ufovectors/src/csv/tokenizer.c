#include "tokenizer.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "token.h"

tokenizer_t csv_tokenizer() {
    tokenizer_t tokenizer;

    tokenizer.row_delimiter = '\n';
    tokenizer.column_delimiter = ',';
    tokenizer.escape = '\\';
    tokenizer.quote = '"';
    tokenizer.use_double_quote_escape = true;
    tokenizer.use_escape_character = true;

    return tokenizer;
}

tokenizer_t *new_csv_tokenizer() {
    tokenizer_t *tokenizer = (tokenizer_t *) malloc(sizeof(tokenizer_t));
    *tokenizer = csv_tokenizer();
    return tokenizer;
}

void tokenizer_free(tokenizer_t *tokenizer) {
    free(tokenizer);
}

tokenizer_read_buffer_t *read_buffer_init(size_t character_buffer_size) {
    tokenizer_read_buffer_t *read_buffer = (tokenizer_read_buffer_t *) malloc(sizeof(tokenizer_read_buffer_t));

    read_buffer->buffer = (char *) malloc(character_buffer_size * sizeof(char));
    read_buffer->max_size = character_buffer_size;
    read_buffer->size = character_buffer_size;                            // so that it triggers a full buffer...
    read_buffer->pointer = character_buffer_size;                         // ...check and initially populates it

    return read_buffer;
}

void read_buffer_free(tokenizer_read_buffer_t *read_buffer) {
    free(read_buffer->buffer);
    free(read_buffer);
}

tokenizer_token_buffer_t *tokenizer_token_buffer_init(size_t initial_token_size) {
    tokenizer_token_buffer_t *buffer =
            (tokenizer_token_buffer_t *) malloc(sizeof(tokenizer_token_buffer_t));

    if (buffer == NULL) {
        perror("Error: cannot allocate memory for token buffer");
        return NULL;
    }

    buffer->max_size = initial_token_size;
    buffer->size = 0;
    buffer->buffer = (char *) malloc(sizeof(char) * initial_token_size);

    if (buffer->buffer == NULL) {
        perror("Error: cannot allocate memory for token buffer's internal buffer");
        return NULL;
    }

    return buffer;
}

tokenizer_state_t *tokenizer_state_init (const char *path, long initial_offset, size_t token_buffer_size, size_t character_buffer_size) {
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

    state->read_buffer = read_buffer_init(character_buffer_size);
    state->token_buffer = tokenizer_token_buffer_init(token_buffer_size);

    return state;
}


char next_character (tokenizer_state_t *state) {
    assert(state->read_buffer->pointer <= state->read_buffer->size);
    if (state->read_buffer->pointer == state->read_buffer->size) {
        int read_characters = fread(state->read_buffer->buffer, sizeof(char), state->read_buffer->max_size, state->file);
        if (read_characters == 0) {
            return EOF;
        }
        state->read_buffer->pointer = 0;
        state->read_buffer->size = read_characters;
    }

    return state->read_buffer->buffer[state->read_buffer->pointer++];
}

tokenizer_token_t *tokenizer_token_buffer_get_token (tokenizer_token_buffer_t *buffer, bool trim_trailing) {
    assert(buffer->size <= buffer->max_size);

    size_t trim_length = 0;
    if (trim_trailing && buffer->size > 0) {
        for (size_t i = buffer->size - 1; i >= 0; i--) {
            if (!isspace(buffer->buffer[i])) {
                break;
            }
            trim_length++;
        }
    }

    tokenizer_token_t *token = (tokenizer_token_t *) malloc(sizeof(tokenizer_token_t));
    token->size = buffer->size - trim_length + 1;
    token->string = (char *) malloc(sizeof(char) * (token->size));
    if (token->string == NULL) {
        perror("Error: cannot allocate memory for token");
        return NULL;
    }

    token->string = memcpy(token->string, buffer->buffer, token->size - 1);
    token->string[token->size - 1] = '\0';

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
    tokenizer_token_buffer_free(state->token_buffer);
    read_buffer_free(state->read_buffer);
    free(state);
}

void pop_token (tokenizer_state_t *state, tokenizer_token_t **token, bool trim_trailing, bool fake) {

    if (fake) {
        *token = NULL;
        tokenizer_token_buffer_clear(state->token_buffer);
    } else {
        *token = tokenizer_token_buffer_get_token(state->token_buffer, trim_trailing);
        (*token)->position_start = state->end_of_last_token;
        (*token)->position_end = state->current_offset;
        state->end_of_last_token = (*token)->position_end;
        tokenizer_token_buffer_clear(state->token_buffer);
    }
}

int tokenizer_start(tokenizer_t *tokenizer, tokenizer_state_t *state) {
    int seek_status = fseek(state->file, state->initial_offset, SEEK_SET);
    if (seek_status < 0) {
        perror("Error: cannot seek to initial position");
        return 1;
    }

    state->state = TOKENIZER_FIELD;
    state->current_offset = state->initial_offset;
    state->end_of_last_token = 0;
    return 0;
}

void tokenizer_close(tokenizer_t *tokenizer, tokenizer_state_t *state) { // TODO remove tokenizer
    fclose(state->file);
}

static int is_whitespace(tokenizer_t* tokenizer, char c) {
    return c == ' ' || c == '\t';
}

static int is_escape(tokenizer_t* tokenizer, char c) {
    return c == tokenizer->escape && tokenizer->use_escape_character;
}

static int is_quote_escape(tokenizer_t* tokenizer, char c) {
    return c == tokenizer->quote && tokenizer->use_double_quote_escape;
}

static inline void transition(tokenizer_state_t *state, tokenizer_state_value_t next_state) {
    //printf("%s -> %s\n", tokenizer_state_to_string(state->state), tokenizer_state_to_string(next_state));
    state->state = next_state;
    state->current_offset++;
}

static inline tokenizer_result_t pop_and_yield(tokenizer_state_t *state, tokenizer_token_t **token, tokenizer_state_value_t next_state, tokenizer_result_t result, bool skip) {
    transition(state, next_state);
    pop_token(state, token, false, skip);
    return result;
}

static inline tokenizer_result_t trim_pop_and_yield(tokenizer_state_t *state, tokenizer_token_t **token, tokenizer_state_value_t next_state, tokenizer_result_t result, bool skip) {
    transition(state, next_state);
    pop_token(state, token, true, skip);
    return result;
}

static inline int append(tokenizer_state_t *state, char c, tokenizer_state_value_t next_state) {
    int result = tokenizer_token_buffer_append(state->token_buffer, c);
    if (result != 0) {
        perror("Error: Could not append character to token buffer.");
        return -1;
    }
    transition(state, next_state);
    return 0;
}

tokenizer_result_t tokenizer_next (tokenizer_t *tokenizer, tokenizer_state_t *state, tokenizer_token_t **token, bool skip) {
    while (true) {
        switch (state->state) {
            case TOKENIZER_FIELD: {
                char c = next_character(state);
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_FINAL, TOKENIZER_END_OF_FILE, skip);         }
                if (c == tokenizer->column_delimiter) { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK, skip);                  }
                if (c == tokenizer->row_delimiter)    { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW, skip);          }
                if (is_whitespace(tokenizer, c))      {        transition(state, TOKENIZER_FIELD);                                      continue; }
                if (c == tokenizer->quote)            {        transition(state, TOKENIZER_QUOTED_FIELD);                               continue; }
                /* c is any other character */        { if   (!append(state, c, TOKENIZER_UNQUOTED_FIELD))                              continue;
                                                        else { transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
            }

            case TOKENIZER_UNQUOTED_FIELD: {
                char c = next_character(state);
                if (c == EOF)                         { return trim_pop_and_yield(state, token, TOKENIZER_FINAL, TOKENIZER_END_OF_FILE, skip);    }
                if (c == tokenizer->column_delimiter) { return trim_pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK, skip);             }
                if (c == tokenizer->row_delimiter)    { return trim_pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW, skip);     }
                /* c is any other character */        { if   (!append(state, c, TOKENIZER_UNQUOTED_FIELD))                              continue;
                                                        else { transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
            }

            case TOKENIZER_QUOTED_FIELD: {
                char c = next_character(state);
                if (is_quote_escape(tokenizer, c))    {        transition(state, TOKENIZER_QUOTE);                                      continue; }
                if (is_escape(tokenizer, c))          {        transition(state, TOKENIZER_ESCAPE);                                     continue; }
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR, skip);       }
                /* c is any other character */        {        append(state, c, TOKENIZER_QUOTED_FIELD);                                continue; }
            }

            case TOKENIZER_QUOTE: {
                char c = next_character(state);
                if (c == tokenizer->quote)            { if   (!append(state, c, TOKENIZER_QUOTED_FIELD))                                continue;
                                                        else { transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR, skip);       }
                if (is_whitespace(tokenizer, c))      {        transition(state, TOKENIZER_TRAILING);                                   continue; }
                if (c == tokenizer->column_delimiter) { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK, skip);                  }
                if (c == tokenizer->row_delimiter)    { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW, skip);          }
                /* c is any other character */        {        transition(state, TOKENIZER_CRASHED); return TOKENIZER_PARSE_ERROR;                }
            }

            case TOKENIZER_TRAILING: {
                char c = next_character(state);
                if (is_whitespace(tokenizer, c))      {        transition(state, TOKENIZER_TRAILING);                                   continue; }
                if (c == tokenizer->column_delimiter) { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK, skip);                  }
                if (c == tokenizer->row_delimiter)    { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW, skip);          }
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_FINAL, TOKENIZER_END_OF_FILE, skip);         }
                /* c is any other character */        { return pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR, skip);       }
            }

            case TOKENIZER_ESCAPE: {
                char c = next_character(state);
                if (c == tokenizer->escape)           { if   (!append(state, c, TOKENIZER_QUOTED_FIELD))                                continue;
                                                        else { transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR, skip);       }
                /* c is any other character */        { if   (!append(state, tokenizer->escape, TOKENIZER_QUOTED_FIELD)
                                                        &&    !append(state, c, TOKENIZER_QUOTED_FIELD))                                continue;
                                                        else { transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
            }

            case TOKENIZER_INITIAL:                   { perror("Error: Tokenizer was now properly opened"); return TOKENIZER_ERROR;               }
            case TOKENIZER_FINAL:                     { perror("Error: Tokenizer completed reading this file"); return TOKENIZER_ERROR;           }
            case TOKENIZER_CRASHED:                   { perror("Error: Tokenizer crashed and cannot continue"); return TOKENIZER_ERROR;           }
            default:                                  { perror("Error: unimplemented"); return TOKENIZER_ERROR;                                   }
        }
    }
}

const char *tokenizer_result_to_string (tokenizer_result_t result) {
    switch (result) {
        case TOKENIZER_OK:              return "OK";
        case TOKENIZER_END_OF_ROW:      return "END OF ROW";
        case TOKENIZER_END_OF_FILE:     return "END OF FILE";
        case TOKENIZER_PARSE_ERROR:     return "PARSE ERROR";
        default:                        return "ERROR";
    }
}

const char *tokenizer_state_to_string (tokenizer_state_value_t state) {
    switch (state) {
        case TOKENIZER_INITIAL:         return "initial";
        case TOKENIZER_FIELD:           return "field";
        case TOKENIZER_UNQUOTED_FIELD:  return "unquoted field";
        case TOKENIZER_QUOTED_FIELD:    return "initial";
        case TOKENIZER_QUOTE:           return "quote";
        case TOKENIZER_TRAILING:        return "trailing";
        case TOKENIZER_ESCAPE:          return "escape";
        case TOKENIZER_FINAL:           return "final";
        case TOKENIZER_CRASHED:         return "crashed";
        default:                        return "?";
    }
}