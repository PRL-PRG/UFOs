#include "reader.h"

#include <assert.h>

#include "token.h"
#include "tokenizer.h"
#include "string_vector.h"

typedef union {
    struct {
        unsigned int empty: 1;
        unsigned int na: 1;
        unsigned int boolean: 1;
        unsigned int integer: 1;
        unsigned int numeric: 1;
        unsigned int string: 1;
        unsigned int _unused: 26; // todo remove
    } flags;
    token_type_t value;
} token_type_map_t;

token_type_t type_from_type_map (token_type_map_t map) {

    if (map.flags.string  == 1) { return TOKEN_STRING;  }
    if (map.flags.numeric == 1) { return TOKEN_DOUBLE;  }
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
        vector->types = (token_type_map_t *) realloc(vector->types, new_allocated * sizeof(token_type_map_t));

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


bool token_type_vector_is_string(token_type_vector_t *vector, size_t index) {
    if (index >= vector->size) {
        return false;
    } else {
        return 1 == vector->types[index].flags.string;
    }
}

void token_type_vector_free(token_type_vector_t *vector) {
    free(vector->types);
    free(vector);
}

offset_record_t *offset_record_new(long interval, size_t initial_size) {
    assert(initial_size > 0);

    offset_record_t *record = (offset_record_t *) malloc(sizeof(offset_record_t));
    record->allocated = initial_size;
    record->size = 0;
    record->interval = interval;

    record->offsets = (long *) malloc(sizeof(long) * initial_size);
    if (record->offsets == NULL) {
        perror("Error: cannot allocate memory to create offset record");
    }
    return record;
}

int offset_record_is_interesting(offset_record_t *record, long value) {
    return 0 == value % record->interval;
}

int offset_record_add(offset_record_t *record, long offset) {
    if (record->size >= record->allocated) {
        record->allocated += (record->allocated >> 1);
        record->offsets = (long *) realloc(record->offsets, sizeof(long) * record->allocated);
        if (record->offsets == NULL) {
            perror("Error: cannot allocate memory to expand offset record");
            return -1;
        }
    }
    record->offsets[record->size++] = offset;
    return 0;
}

size_t offset_record_human_readable_key(offset_record_t *record, size_t i) {
    return (i * (record->interval)) + 1;
}

size_t offset_record_key_at(offset_record_t *record, size_t i) {
    return (i * (record->interval));
}

void offset_record_get_value_closest_to_this_key(offset_record_t *record, size_t target, long* offset, size_t* key_at_offset) {
    size_t target_index = target / record->interval; // floor

    assert(record->size > target_index);
    *offset = record->offsets[target_index];
    *key_at_offset = offset_record_key_at(record, target_index);

    //printf("target: %li -> target_index: %li (key: %li) -> %li\n", target, target_index, *key_at_offset, *offset);
}

void offset_record_free(offset_record_t *record) {
    free(record->offsets);
    free(record);
}

scan_results_t *scan_results_new(size_t rows, size_t columns, offset_record_t *row_offset_record) {
    scan_results_t *results = (scan_results_t *) malloc(sizeof(scan_results_t));
    results->rows = rows;
    results->columns = columns;
    results->column_types = (token_type_t *) malloc(sizeof(token_type_t) * columns); // FIXME check
    results->column_names = (char **) malloc(sizeof(char *) * columns); // FIXME check
    results->row_offsets = row_offset_record;
    return results;
}

void scan_results_free(scan_results_t *results) {
    offset_record_free(results->row_offsets);
    free(results->column_types);
    free(results);
}

scan_results_t *ufo_csv_perform_initial_scan(tokenizer_t *tokenizer, const char* path, long record_row_offsets_at_interval, bool header, size_t initial_buffer_size) {

    tokenizer_state_t *state = tokenizer_state_init(path, 0, initial_buffer_size, initial_buffer_size);
    tokenizer_start(tokenizer, state);

    offset_record_t *row_offsets = offset_record_new(record_row_offsets_at_interval, initial_buffer_size);

    size_t row = 0;
    size_t column = 0;

    token_type_vector_t *column_types = token_type_vector_new(32);
    string_vector_t *column_names = string_vector_new(32);

    if (header) {
        bool loop = true;
        while (loop) {
            tokenizer_token_t *token = NULL;
            tokenizer_result_t result = tokenizer_next(tokenizer, state, &token, false);

            switch (result) {
                case TOKENIZER_PARSE_ERROR:
                case TOKENIZER_ERROR:
                    goto bad;
                default:;
            }

            token_type_vector_add_type(column_types, column, TOKEN_NOTHING);

            int append_ok = string_vector_append(column_names, token_into_string(token)); // token consumed here
            if (0 != append_ok) {
                goto bad;
            }

            switch (result) {
                case TOKENIZER_OK:
                    column++;
                    break;

                case TOKENIZER_END_OF_ROW:
                    column = 0;
                    loop = false;
                    break;

                case TOKENIZER_END_OF_FILE: {
                    scan_results_t *results = scan_results_new(0, column + 1, NULL);
                    for (size_t i = 0; i < column + 1; i++) {
                        results->column_types[i] = TOKEN_EMPTY;
                        results->column_names[i] = column_names->strings[i];
                    }

                    string_vector_free(column_names);
                    token_type_vector_free(column_types);
                    tokenizer_close(tokenizer, state);
                    return results;
                }

                default:;
            }
        }
    }

    while (true) {
        bool column_type_is_string = token_type_vector_is_string(column_types, column);

        tokenizer_token_t *token = NULL;
        tokenizer_result_t result = tokenizer_next(tokenizer, state, &token, column_type_is_string);

        switch (result) {
            case TOKENIZER_PARSE_ERROR:
            case TOKENIZER_ERROR:
                goto bad;
            default:;
        }

        if (!column_type_is_string) {
            token_type_t token_type = deduce_token_type(token);
            token_type_vector_add_type(column_types, column, token_type);
        }

        if (column == 0 && offset_record_is_interesting(row_offsets, row)) {
            offset_record_add(row_offsets, token->position_start);
        }

        switch (result) {
            case TOKENIZER_OK:
                column++;
                break;

            case TOKENIZER_END_OF_ROW:
                column = 0;
                row++;
                break;

            case TOKENIZER_END_OF_FILE: {
                scan_results_t *results = scan_results_new(row + (0 == column ? 0 : 1), column_types->size, row_offsets);
                for (size_t i = 0; i < column_types->size; i++) {
                    results->column_types[i] = type_from_type_map(column_types->types[i]);
                    results->column_names[i] = (i < column_names->size) ? column_names->strings[i] : "";
                }

                string_vector_free(column_names);
                token_type_vector_free(column_types);
                tokenizer_close(tokenizer, state);
                return results;
            }

            default:;
        }
    }

    bad:
    perror("merde");
    string_vector_free(column_names);
    token_type_vector_free(column_types);
    tokenizer_close(tokenizer, state);
    return NULL;
}



string_set_t *ufo_csv_read_column_unique_values(tokenizer_t *tokenizer, const char *path, size_t target_column, scan_results_t *scan_results, size_t limit, size_t initial_buffer_size) {
    string_set_t *unique_tokens = string_set_new(10);

    if (scan_results->rows == 0) {
        return unique_tokens;
    }

    long offset = 0;
    size_t row_at_offset;
    offset_record_get_value_closest_to_this_key(scan_results->row_offsets, 0, &offset, &row_at_offset);

    tokenizer_state_t *state = tokenizer_state_init(path, offset, initial_buffer_size, initial_buffer_size);
    tokenizer_start(tokenizer, state);

    size_t row = row_at_offset;
    size_t column = 0;

    while (true) {
        tokenizer_token_t *token = NULL;
        tokenizer_result_t result = tokenizer_next(tokenizer, state, &token, column != target_column);

        if (TOKENIZER_ERROR == result || TOKENIZER_PARSE_ERROR == result) {
            goto bad;
        }

        if (column == target_column) {
            assert(token != NULL);

            if (string_set_size(unique_tokens) >= limit) {
                goto good;
            }

            bool ok = string_set_add(unique_tokens, token_into_string(token));
            if (!ok) { goto bad; }
        }

        switch (result) {
            case TOKENIZER_OK:
                column++;
                break;

            case TOKENIZER_END_OF_ROW:
                column = 0;
                row++;
                break;

            case TOKENIZER_END_OF_FILE:
                goto good;

            default:
                goto bad;
        }

    }

    good:
    tokenizer_close(tokenizer, state);
    return unique_tokens;

    bad:
    perror("merde");
    string_set_free(unique_tokens);
    tokenizer_close(tokenizer, state);
    return NULL;

}


read_results_t ufo_csv_read_column(tokenizer_t *tokenizer, const char *path, size_t target_column, scan_results_t *scan_results, size_t first_row, size_t last_row, size_t initial_buffer_size) {

    assert(first_row <= last_row);
    assert(last_row < scan_results->rows);

    if (scan_results->rows == 0) {
        read_results_t result = {.tokens = NULL, .size = 0};
        return result;
    }

    long offset = 0;
    size_t row_at_offset;
    offset_record_get_value_closest_to_this_key(scan_results->row_offsets, first_row, &offset, &row_at_offset);

    tokenizer_state_t *state = tokenizer_state_init(path, offset, initial_buffer_size, initial_buffer_size);
    tokenizer_start(tokenizer, state);

    if (last_row == 0) {
        last_row = scan_results->rows - 1;
    }

    size_t row = row_at_offset;
    size_t column = 0;
    bool found_column = false;

    size_t expected_tokens = last_row - first_row + 1;

    tokenizer_token_t **tokens = (tokenizer_token_t **) malloc(expected_tokens * sizeof(tokenizer_token_t *));
    if (tokens == NULL) {
        perror("Cannot allocate token array.");
    }

    while (true) {
        tokenizer_token_t *token = NULL;
        tokenizer_result_t result = tokenizer_next(tokenizer, state, &token, column != target_column);

        switch (result) {
            case TOKENIZER_PARSE_ERROR:
            case TOKENIZER_ERROR:
                tokenizer_close(tokenizer, state);

                free(tokens);
                read_results_t result = {.tokens = NULL, .size = row};
                return result;
            default: ;
        }

        if (column == target_column && row >= first_row && (last_row == 0 || row <= last_row)) {
            assert(token != NULL);
            assert(row - first_row >= 0);
            tokens[row - first_row] = token;
            found_column = true;
        }

        switch (result) {
            case TOKENIZER_OK:
                column++;
                break;

            case TOKENIZER_END_OF_ROW:
            case TOKENIZER_END_OF_FILE: {

                column = 0;
                if (!found_column && row >= first_row && (last_row == 0 || row <= last_row)) {
                        tokenizer_token_t *token = tokenizer_token_empty();
                        assert(row - first_row >= 0);
                        assert(row - first_row < expected_tokens);
                        tokens[row - first_row] = token;
                } else {
                    found_column = false;
                }

                if (result == TOKENIZER_END_OF_FILE || (last_row != 0 && row >= last_row)) {
                    tokenizer_close(tokenizer, state);

                    read_results_t result = {.tokens = tokens, .size = row - first_row + 1};
                    return result;
                }

                row++;
            }

            default: ;
        }
    }
}