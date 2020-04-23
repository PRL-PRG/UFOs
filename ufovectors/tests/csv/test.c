#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../../src/csv/token.h"
#include "../../src/csv/tokenizer.h"
#include "../../src/csv/reader.h"

/*
 * gcc -o test -I../src/csv/ ../../src/csv/reader.c  ../../src/csv/tokenizer.c ../../src/csv/token.c test.c -g -O2 -Wall
 */

void test_file(char* path) {
    tokenizer_t tokenizer = csv_tokenizer();
    tokenizer_state_t *state = tokenizer_state_init(path, 0, 10, 10);
    tokenizer_start(&tokenizer, state);
    tokenizer_token_t *token;

    while (true) {
        int result = tokenizer_next(&tokenizer, state, &token, false);

        printf("Token: [size: %li, start: %li, end: %li, string: <%s>], %s\n",
               token->size, token->position_start, token->position_end, token->string,
               tokenizer_result_to_string(result));

        if (result == TOKENIZER_END_OF_FILE || result == TOKENIZER_ERROR || result == TOKENIZER_PARSE_ERROR) {
            break;
        }
    }

    tokenizer_close(&tokenizer, state);
};

void test_initial_scan(char* path) {
    //tokenizer_t tokenizer = csv_tokenizer();
    scan_results_t *results = ufo_csv_perform_initial_scan(path, 5);

    printf("After initial scan of %s: \n\n", path);
    printf("    rows: %li\n", results->rows);
    printf("    cols: %li\n", results->columns);

    printf("    column_types:\n\n");
    for (size_t i = 0; i < results->columns; i++) {
        printf("        [%li]: %s/%i\n",
                i, token_type_to_string(results->column_types[i]),
                results->column_types[i]);
    }
    printf("\n");

    printf("    row_offsets:\n\n");
    for (size_t i = 0; i < results->row_offsets->size; i++) {
        printf("        [%li] (row #%li): %li\n",
                i, offset_record_human_readable_key(results->row_offsets, i),
                results->row_offsets->offsets[i]);
    }
    printf("\n");
};

void test_read_individual_columns(char* path) {
    //tokenizer_t tokenizer = csv_tokenizer();
    scan_results_t *results = ufo_csv_perform_initial_scan(path, 5);

    for (size_t column = 0; column <= results->columns; column++) {

        tokenizer_token_t **tokens = ufo_csv_read_column(path, column, results);

        printf("After reading column %li of %s: \n\n", column, path);
        for (size_t row = 0; row < results->rows; row++) {
            printf("    [%li]: %s\n", row, tokens[row]->string);
        }
        printf("\n");
    }
}

int main (int argc, char *argv[]) {
    //test_file("test.csv");
    //test_initial_scan("test.csv");
    //test_initial_scan("test2.csv");
    test_read_individual_columns("test2.csv");
    return 0;
}