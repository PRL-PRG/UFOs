#include "reader.h"

#include <assert.h>

#include "token.h"
#include "tokenizer.h"

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

void offset_record_free(offset_record_t *record) {
    free(record->offsets);
    free(record);
}

scan_results_t *scan_results_new(size_t rows, size_t columns, offset_record_t *row_offset_record) {
    scan_results_t *results = (scan_results_t *) malloc(sizeof(scan_results_t));
    results->rows = rows;
    results->columns = columns;
    results->column_types = (token_type_t *) malloc(sizeof(token_type_t) * columns);
    results->row_offsets = row_offset_record;
    return results;
}

void scan_results_free(scan_results_t *results) {
    offset_record_free(results->row_offsets);
    free(results->column_types);
    free(results);
}

scan_results_t *ufo_csv_perform_initial_scan(char* path, long record_row_offsets_at_interval) {

    tokenizer_t tokenizer = csv_tokenizer(); // TODO pass as argument
    tokenizer_state_t *state = tokenizer_state_init(path, 0, 10, 10);
    tokenizer_start(&tokenizer, state);

    offset_record_t *row_offsets = offset_record_new(record_row_offsets_at_interval, 100);

    size_t row = 0;
    size_t column = 0;

    token_type_vector_t *column_types = token_type_vector_new(32);

    while (true) {
        tokenizer_token_t *token = NULL;
        tokenizer_result_t result = tokenizer_next(&tokenizer, state, &token, false);

        switch (result) {
            case TOKENIZER_PARSE_ERROR:
            case TOKENIZER_ERROR:
                tokenizer_close(&tokenizer, state);
                return NULL;
            default:;
        }

        token_type_t token_type = deduce_token_type(token);
//        printf("(row: %li, column: %li) [size: %li, start: %li, end: %li, string: <%s> type: [%s/%i]], %s\n",
//               row, column,
//               token->size, token->position_start, token->position_end, token->string,
//               token_type_to_string(token_type), token_type, tokenizer_result_to_string(result));

        if (column == 0 && offset_record_is_interesting(row_offsets, row)) {
            offset_record_add(row_offsets, token->position_start);
        }

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
                scan_results_t *results = scan_results_new(row + (0 == column ? 0 : 1), column_types->size, row_offsets);
                for (size_t i = 0; i < column_types->size; i++) {
                    results->column_types[i] = type_from_type_map(column_types->types[i]);
                }

                token_type_vector_free(column_types);
                tokenizer_close(&tokenizer, state);
                return results;
            }

            default:;
        }
    }
}

read_results_t ufo_csv_read_column(char *path, size_t target_column, scan_results_t *scan_results, size_t first_row, size_t last_row) {

    assert(first_row <= last_row);
    assert(last_row < scan_results->rows);

    if (scan_results->rows == 0) {
        read_results_t result = {.tokens = NULL, .size = 0};
        return result;
    }

    tokenizer_t tokenizer = csv_tokenizer(); // TODO pass as argument

    tokenizer_state_t *state = tokenizer_state_init(path, 0, 10, 10);
    tokenizer_start(&tokenizer, state);

    if (last_row == 0) {
        last_row = scan_results->rows - 1;
    }

    size_t row = 0;
    size_t column = 0;
    bool found_column = false;

    size_t expected_tokens = last_row - first_row + 1;

    tokenizer_token_t **tokens = (tokenizer_token_t **) malloc(expected_tokens * sizeof(tokenizer_token_t *));
    if (tokens == NULL) {
        perror("Cannot allocate token array.");
    }

    while (true) {
        tokenizer_token_t *token = NULL;
        tokenizer_result_t result = tokenizer_next(&tokenizer, state, &token, column != target_column);

        switch (result) {
            case TOKENIZER_PARSE_ERROR:
            case TOKENIZER_ERROR:
                tokenizer_close(&tokenizer, state);

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

//            printf("(row: %li, column: %li) [size: %li, start: %li, end: %li, string: <%s>] is_target: %i, is_within_range: %i %s\n",
//                   row, column,
//                   token->size, token->position_start, token->position_end, token->string,
//                   column != target_column,
//                   row >= first_row && (last_row == 0 || row <= last_row),
//                   tokenizer_result_to_string(result));

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

                if (result == TOKENIZER_END_OF_FILE || (last_row != 0 && row > last_row)) {
                    tokenizer_close(&tokenizer, state);

                    read_results_t result = {.tokens = tokens, .size = row - first_row};
                    return result;
                }

                row++;
            }

            default: ;
        }
    }
}
