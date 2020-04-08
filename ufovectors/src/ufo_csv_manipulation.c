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

tokenizer_read_buffer_t read_buffer_init(size_t character_buffer_size) {
    tokenizer_read_buffer_t read_buffer;
    read_buffer.buffer = (char *) malloc(10 * sizeof(char));
    read_buffer.max_size = character_buffer_size;
    read_buffer.size = character_buffer_size;                            // so that it triggers a full buffer...
    read_buffer.pointer = character_buffer_size - 1;                     // ...check and initially populates it // FIXME
    return read_buffer;
}

tokenizer_state_t *tokenizer_state_init (char *path, long initial_offset, size_t max_token_size, size_t character_buffer_size) {
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
    state->token_buffer = tokenizer_token_buffer_init(max_token_size);

    return state;
}


char next_character (tokenizer_read_buffer_t *read_buffer, FILE *source) {
    //printf("next character %li %li %li [%s]\n", state->read_buffer.pointer, state->read_buffer.size, state->read_buffer.max_size, state->read_buffer.buffer);
    assert(read_buffer->pointer <= read_buffer->size);
    if (read_buffer->pointer == read_buffer->size) {
        int read_characters = fread(read_buffer->buffer, sizeof(char), read_buffer->max_size, source);
        if (read_characters == 0) {
            return EOF;
        }
        read_buffer->pointer = 0;
        read_buffer->size = read_characters;
        //printf("loading %li %li %li [%s]\n", state->read_buffer.pointer, state->read_buffer.size, state->read_buffer.max_size, state->read_buffer.buffer);
    }

    //printf("returning %c\n", state->read_buffer.buffer[state->read_buffer.pointer]);
    return read_buffer->buffer[read_buffer->pointer++];
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

static inline tokenizer_result_t pop_and_yield(tokenizer_state_t *state, tokenizer_token_t **token, tokenizer_state_value_t next_state, tokenizer_result_t result) {
    transition(state, next_state);
    pop_token(state, token);
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

static inline int scanned_column (tokenizer_scan_info_t *info) {
    info->columns++;
    return 0;
}

static inline int scanned_row (tokenizer_scan_info_t *info) {
    info->rows++;
    info->columns++;
    if (info->columns > info->max_columns) {
        info->max_columns = info->columns;
    }
    if (info->rows % info->offset_row_record_interval == 0) {
        if (info->offset_row_record_cursor >= info->offset_row_record_size) {
            info->offset_row_record_size += (info->offset_row_record_size >> 1);
            info->offset_row_record = (long *) realloc(info->offset_row_record,
                                                       sizeof(long) * info->offset_row_record_size);
            if (info->offset_row_record == NULL) {
                perror("Error: cannot allocate memory to expand offset record");
                return -1;
            }
        }
        info->offset_row_record[info->offset_row_record_cursor++] = info->offset;
    }
    info->columns = 0;
    return 0;
}

tokenizer_scan_info_t scan_info_init (size_t row_record_interval) {
    tokenizer_scan_info_t info;
    info.columns = 0;
    info.max_columns = 0;
    info.rows = 0;

    info.offset_row_record_cursor = 0;
    info.offset_row_record_size = 4;
    info.offset_row_record = malloc(info.offset_row_record_size * sizeof(long));

    info.offset_row_record_interval = row_record_interval;

    return info;
}

int scan (tokenizer_t *tokenizer, FILE* source, size_t buffer_size, size_t row_record_interval) {
    tokenizer_read_buffer_t buffer = read_buffer_init(buffer_size);
    tokenizer_scan_info_t info = scan_info_init(row_record_interval);
    tokenizer_state_value_t state = TOKENIZER_FIELD;

    while (true) {
        char c = next_character(&buffer, source);
        info.offset++;

        switch (state) {
            case TOKENIZER_FIELD: {
                if (c == EOF)                         { scanned_row(&info); return 0; }
                if (c == tokenizer->column_delimiter) { scanned_column(&info); continue; }
                if (c == tokenizer->row_delimiter)    { scanned_row(&info); continue; }
                if (is_whitespace(tokenizer, c))      { continue; }
                if (c == tokenizer->quote)            { state = TOKENIZER_QUOTED_FIELD; continue; }
                /* c is any other character */        { state = TOKENIZER_UNQUOTED_FIELD; continue; }
            }

            case TOKENIZER_UNQUOTED_FIELD: {
                if (c == EOF)                         { scanned_row(&info); return 0; }
                if (c == tokenizer->column_delimiter) { state = TOKENIZER_FIELD; scanned_column(&info); continue; }
                if (c == tokenizer->row_delimiter)    { state = TOKENIZER_FIELD; scanned_row(&info); continue;  }
                /* c is any other character */        { continue;                      }
            }

            case TOKENIZER_QUOTED_FIELD: {
                if (is_quote_escape(tokenizer, c))    { state = TOKENIZER_QUOTE; continue; }
                if (is_escape(tokenizer, c))          { state = TOKENIZER_ESCAPE; continue; }
                if (c == EOF)                         { return 1;            }
                /* c is any other character */        { continue; }
            }

            case TOKENIZER_QUOTE: {
                if (c == tokenizer->quote)            { state = TOKENIZER_QUOTED_FIELD; continue; }
                if (c == EOF)                         { return 1;             }
                if (is_whitespace(tokenizer, c))      { state = TOKENIZER_TRAILING;     continue; }
                if (c == tokenizer->column_delimiter) { state = TOKENIZER_FIELD; scanned_column(&info); continue;      }
                if (c == tokenizer->row_delimiter)    { state = TOKENIZER_FIELD; scanned_row(&info); continue;         }
                /* c is any other character */        { return 1;                }
            }

            case TOKENIZER_TRAILING: {
                if (is_whitespace(tokenizer, c))      { state = TOKENIZER_TRAILING; continue; }
                if (c == tokenizer->column_delimiter) { state = TOKENIZER_FIELD; scanned_column(&info); continue;      }
                if (c == tokenizer->row_delimiter)    { state = TOKENIZER_FIELD; scanned_row(&info); continue;         }
                if (c == EOF)                         { scanned_row(&info); return 0;               }
                /* c is any other character */        { return 1;             }
            }

            case TOKENIZER_ESCAPE: {
                if (c == tokenizer->escape)           { state = TOKENIZER_QUOTED_FIELD; continue; }
                if (c == EOF)                         { return 1;             }
                /* c is any other character */        { state = TOKENIZER_QUOTED_FIELD; continue; }
            }

            default:                                  { perror("Error: unimplemented"); return TOKENIZER_ERROR;        }
        }
    }
}

tokenizer_result_t tokenizer_next (tokenizer_t *tokenizer, tokenizer_state_t *state, tokenizer_token_t **token) {
    while (true) {
        switch (state->state) {
            case TOKENIZER_FIELD: {
                char c = next_character(&state->read_buffer, state->file);
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_FINAL, TOKENIZER_END_OF_FILE);               }
                if (c == tokenizer->column_delimiter) { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK);                        }
                if (c == tokenizer->row_delimiter)    { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW);                }
                if (is_whitespace(tokenizer, c))      {        transition(state, TOKENIZER_FIELD);                                      continue; }
                if (c == tokenizer->quote)            {        transition(state, TOKENIZER_QUOTED_FIELD);                               continue; }
                /* c is any other character */        { if   (!append(state, c, TOKENIZER_UNQUOTED_FIELD))                              continue;
                                                        else { transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
            }

            case TOKENIZER_UNQUOTED_FIELD: {
                char c = next_character(&state->read_buffer, state->file);
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_FINAL, TOKENIZER_END_OF_FILE);               }
                if (c == tokenizer->column_delimiter) { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK);                        }
                if (c == tokenizer->row_delimiter)    { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW);                }
                /* c is any other character */        { if   (!append(state, c, TOKENIZER_UNQUOTED_FIELD))                              continue;
                                                        else { transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
            }

            case TOKENIZER_QUOTED_FIELD: {
                char c = next_character(&state->read_buffer, state->file);
                if (is_quote_escape(tokenizer, c))    {        transition(state, TOKENIZER_QUOTE);                                      continue; }
                if (is_escape(tokenizer, c))          {        transition(state, TOKENIZER_ESCAPE);                                     continue; }
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR);             }
                /* c is any other character */        {        append(state, c, TOKENIZER_QUOTED_FIELD);                                continue; }
            }

            case TOKENIZER_QUOTE: {
                char c = next_character(&state->read_buffer, state->file);
                if (c == tokenizer->quote)            { if   (!append(state, c, TOKENIZER_QUOTED_FIELD))                                continue;
                                                        else { transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR);             }
                if (is_whitespace(tokenizer, c))      {        transition(state, TOKENIZER_TRAILING);                                   continue; }
                if (c == tokenizer->column_delimiter) { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK);                        }
                if (c == tokenizer->row_delimiter)    { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW);                }
                /* c is any other character */        {        transition(state, TOKENIZER_CRASHED); return TOKENIZER_PARSE_ERROR;                }
            }

            case TOKENIZER_TRAILING: {
                char c = next_character(&state->read_buffer, state->file);
                if (is_whitespace(tokenizer, c))      {        transition(state, TOKENIZER_TRAILING);                                   continue; }
                if (c == tokenizer->column_delimiter) { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK);                        }
                if (c == tokenizer->row_delimiter)    { return pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW);                }
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_FINAL, TOKENIZER_END_OF_FILE);               }
                /* c is any other character */        { return pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR);             }
            }

            case TOKENIZER_ESCAPE: {
                char c = next_character(&state->read_buffer, state->file);
                if (c == tokenizer->escape)           { if   (!append(state, c, TOKENIZER_QUOTED_FIELD))                                continue;
                                                        else { transition(state, TOKENIZER_CRASHED); return TOKENIZER_ERROR; }                    }
                if (c == EOF)                         { return pop_and_yield(state, token, TOKENIZER_CRASHED, TOKENIZER_PARSE_ERROR);             }
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

void perform_initial_scan(char* path) {




}
