#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef USE_R_STUFF
#include <R_ext/Utils.h>
#endif

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
    read_buffer.pointer = character_buffer_size;                         // ...check and initially populates it
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

    /*char *tmp = (char *) malloc(sizeof(char) * (buffer->size + 1));
    memcpy(tmp, buffer->buffer, buffer->size);
    tmp[buffer->size] = '\0';
    printf("trimming %li off of |%s|\n", trim_length, tmp);
    free(tmp); FIXME clean up after DEBUG */

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
    free(state);
}

void pop_token (tokenizer_state_t *state, tokenizer_token_t **token, bool trim_trailing) {

    *token = tokenizer_token_buffer_get_token(state->token_buffer, trim_trailing);

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
    state->end_of_last_token = 0;
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
    pop_token(state, token, false);
    return result;
}

static inline tokenizer_result_t trim_pop_and_yield(tokenizer_state_t *state, tokenizer_token_t **token, tokenizer_state_value_t next_state, tokenizer_result_t result) {
    transition(state, next_state);
    pop_token(state, token, true);
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
        if (info->offset_row_record_size >= info->offset_row_record_max_size) {
            info->offset_row_record_max_size += (info->offset_row_record_max_size >> 1);
            info->offset_row_record = (long *) realloc(info->offset_row_record,
                                                       sizeof(long) * info->offset_row_record_max_size);
            if (info->offset_row_record == NULL) {
                perror("Error: cannot allocate memory to expand offset record");
                return -1;
            }
        }
        info->offset_row_record[info->offset_row_record_size++] = info->offset;
    }
    info->columns = 0;
    return 0;
}

tokenizer_scan_info_t scan_info_init (size_t row_record_interval, size_t initial_record_size) {
    tokenizer_scan_info_t info;
    info.columns = 0;
    info.max_columns = 0;
    info.rows = 0;
    info.offset = 0;

    info.offset_row_record_size = 0;
    info.offset_row_record_max_size = initial_record_size;
    info.offset_row_record = malloc(info.offset_row_record_max_size * sizeof(long));

    info.offset_row_record_interval = row_record_interval;

    return info;
}

int tokenizer_scan (tokenizer_t *tokenizer, const char* path, tokenizer_scan_info_t *info, size_t buffer_size) {
    tokenizer_read_buffer_t buffer = read_buffer_init(buffer_size);
    FILE *source = fopen(path, "r");
    tokenizer_state_value_t state = TOKENIZER_FIELD;

    while (true) {
        char c = next_character(&buffer, source);
        info->offset++;

        switch (state) {
            case TOKENIZER_FIELD: {
                if (c == EOF)                         { scanned_row(info); goto good; }
                if (c == tokenizer->column_delimiter) { scanned_column(info); continue; }
                if (c == tokenizer->row_delimiter)    { scanned_row(info); continue; }
                if (is_whitespace(tokenizer, c))      { continue; }
                if (c == tokenizer->quote)            { state = TOKENIZER_QUOTED_FIELD; continue; }
                /* c is any other character */        { state = TOKENIZER_UNQUOTED_FIELD; continue; }
            }

            case TOKENIZER_UNQUOTED_FIELD: {
                if (c == EOF)                         { scanned_row(info); goto good; }
                if (c == tokenizer->column_delimiter) { state = TOKENIZER_FIELD; scanned_column(info); continue; }
                if (c == tokenizer->row_delimiter)    { state = TOKENIZER_FIELD; scanned_row(info); continue;  }
                /* c is any other character */        { continue;                      }
            }

            case TOKENIZER_QUOTED_FIELD: {
                if (is_quote_escape(tokenizer, c))    { state = TOKENIZER_QUOTE; continue; }
                if (is_escape(tokenizer, c))          { state = TOKENIZER_ESCAPE; continue; }
                if (c == EOF)                         { goto bad;            }
                /* c is any other character */        { continue; }
            }

            case TOKENIZER_QUOTE: {
                if (c == tokenizer->quote)            { state = TOKENIZER_QUOTED_FIELD; continue; }
                if (c == EOF)                         { goto bad;            }
                if (is_whitespace(tokenizer, c))      { state = TOKENIZER_TRAILING;     continue; }
                if (c == tokenizer->column_delimiter) { state = TOKENIZER_FIELD; scanned_column(info); continue;      }
                if (c == tokenizer->row_delimiter)    { state = TOKENIZER_FIELD; scanned_row(info); continue;         }
                /* c is any other character */        { goto bad;                }
            }

            case TOKENIZER_TRAILING: {
                if (is_whitespace(tokenizer, c))      { state = TOKENIZER_TRAILING; continue; }
                if (c == tokenizer->column_delimiter) { state = TOKENIZER_FIELD; scanned_column(info); continue;      }
                if (c == tokenizer->row_delimiter)    { state = TOKENIZER_FIELD; scanned_row(info); continue;         }
                if (c == EOF)                         { scanned_row(info); goto good;               }
                /* c is any other character */        { goto bad;             }
            }

            case TOKENIZER_ESCAPE: {
                if (c == tokenizer->escape)           { state = TOKENIZER_QUOTED_FIELD; continue; }
                if (c == EOF)                         { goto bad;             }
                /* c is any other character */        { state = TOKENIZER_QUOTED_FIELD; continue; }
            }

            default:                                  { perror("Error: unimplemented"); goto bad;        }
        }
    }

    good:
        fclose(source);
        return 0;

    bad:
        fclose(source);
        return 1;
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
                if (c == EOF)                         { return trim_pop_and_yield(state, token, TOKENIZER_FINAL, TOKENIZER_END_OF_FILE);          }
                if (c == tokenizer->column_delimiter) { return trim_pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_OK);                   }
                if (c == tokenizer->row_delimiter)    { return trim_pop_and_yield(state, token, TOKENIZER_FIELD, TOKENIZER_END_OF_ROW);           }
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

const char *token_type_to_string(token_type_t type) {
    switch (type) {
        case TOKEN_EMPTY:               return "EMPTY";
        case TOKEN_NA:                  return "NA";
        case TOKEN_BOOLEAN:             return "LOGICAL";
        case TOKEN_INTEGER:             return "INTEGER";
        case TOKEN_DOUBLE:              return "NUMERIC";
        case TOKEN_STRING:              return "STRING";
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

    if (0 == strcmp("F",     token->string)) { return true; }
    if (0 == strcmp("false", token->string)) { return true; }
    if (0 == strcmp("FALSE", token->string)) { return true; }
    if (0 == strcmp("False", token->string)) { return true; }

    if (0 == strcmp("T",    token->string))  { return true; }
    if (0 == strcmp("true", token->string))  { return true; }
    if (0 == strcmp("TRUE", token->string))  { return true; }
    if (0 == strcmp("True", token->string))  { return true; }

    // could use: StringFalse?(const char*) and StringTrue(const char*) from Utils.h

    return false;
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

    if (deduce_token_is_empty(token))   { return TOKEN_EMPTY;   }
    if (deduce_token_is_na(token))      { return TOKEN_NA;      }
    if (deduce_token_is_logical(token)) { return TOKEN_BOOLEAN; }
    if (deduce_token_is_integer(token)) { return TOKEN_INTEGER; }
    if (deduce_token_is_numeric(token)) { return TOKEN_DOUBLE; }

    return TOKEN_STRING;
}

typedef union {
    struct {
        unsigned int empty: 1;
        unsigned int na: 1;
        unsigned int boolean: 1;
        unsigned int integer: 1;
        unsigned int numeric: 1;
        unsigned int string: 1;
        unsigned int _unused: 26;
    } flags;
    token_type_t value;
} token_type_map_t;

token_type_t type_from_type_map (token_type_map_t map) {

    if (map.flags.string  == 1) { return TOKEN_STRING;  }
    if (map.flags.numeric == 1) { return TOKEN_DOUBLE; }
    if (map.flags.integer == 1) { return TOKEN_INTEGER; }
    if (map.flags.boolean == 1) { return TOKEN_BOOLEAN; }
    if (map.flags.na      == 1) { return TOKEN_NA;      }

    return TOKEN_EMPTY;
}

typedef struct {
    token_type_map_t *types;
    size_t size;
    size_t allocated;
} token_type_vector_t;

token_type_vector_t *token_type_vector_new(size_t initial_size) {
    token_type_vector_t *vector = (token_type_vector_t *) malloc(sizeof(token_type_vector_t));
    vector->allocated = initial_size;
    vector->size = 0;
    vector->types = (token_type_map_t *) calloc(initial_size, sizeof(token_type_map_t));
    return vector;
}

int token_type_vector_add_type(token_type_vector_t *vector, size_t index, token_type_t type) {

    if (index >= vector->allocated) {
        size_t new_allocated = vector->allocated + (vector->allocated >> 1);
        vector->types = (token_type_map_t *) realloc(vector->types, new_allocated * sizeof(vector));

        if (vector->types == NULL) {
            return -1;
        }

        for (size_t i = vector->allocated; i < new_allocated; i++) {
            vector->types[i].value = TOKEN_EMPTY;
        }

        vector->allocated = new_allocated;
    }

    if (index >= vector->size) {
        vector->size = index + 1;
    }

    vector->types[index].value |= type;

    return 0;
}

void token_type_vector_free(token_type_vector_t *vector) {
    free(vector->types);
    free(vector);
}

scan_results_t *scan_results_new(size_t rows, size_t columns) {
    scan_results_t *results = (scan_results_t *) malloc(sizeof(scan_results_t));
    results->rows = rows;
    results->columns = columns;
    results->column_types = (token_type_t *) malloc(sizeof(token_type_t) * columns);
    return results;
}

void scan_results_free(scan_results_t *results) {
    free(results->column_types);
    free(results);
}

scan_results_t *ufo_csv_perform_initial_scan(char* path) {

    tokenizer_t tokenizer = csv_tokenizer();
    tokenizer_state_t *state = tokenizer_state_init(path, 0, 10, 10);
    tokenizer_start(&tokenizer, state);

    size_t row = 0;
    size_t column = 0;

    token_type_vector_t *column_types = token_type_vector_new(32);

    while (true) {
        tokenizer_token_t *token = NULL;
        tokenizer_result_t result = tokenizer_next(&tokenizer, state, &token);

        switch (result) {
            case TOKENIZER_PARSE_ERROR:
            case TOKENIZER_ERROR:
                tokenizer_close(&tokenizer, state);
                return NULL;
            default: ;
        }

        token_type_t token_type = deduce_token_type(token);
        printf("(row: %li, column: %li) [size: %li, start: %li, end: %li, string: <%s> type: [%s/%i]], %s\n",
               row, column,
               token->size, token->position_start, token->position_end, token->string,
               token_type_to_string(token_type), token_type, tokenizer_result_to_string(result));

        token_type_vector_add_type(column_types, column, token_type);

        switch (result) {
            case TOKENIZER_OK:
                column++;
                break;

            case TOKENIZER_END_OF_ROW:
                column = 0;
                row++;
                break;

            case TOKENIZER_END_OF_FILE: {
                scan_results_t *results = scan_results_new(row + 1, column_types->size);
                for (size_t i = 0; i < column_types->size; i++) {
                    results->column_types[i] = type_from_type_map(column_types->types[i]);
                }

                token_type_vector_free(column_types);
                tokenizer_close(&tokenizer, state);
                return results;
            }

            default: ;
        }
    }
}



