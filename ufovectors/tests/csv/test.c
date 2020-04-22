#include <assert.h>
#include <stdio.h>
#include "../../src/ufo_csv_manipulation.h"

void test_scan(char* path) {
    uint32_t interval = 2;

    tokenizer_t tokenizer = csv_tokenizer();
    tokenizer_scan_info_t info = scan_info_init(interval, 1);
    int result = tokenizer_scan(&tokenizer, path, &info, 10);

    for (size_t i = 0; i < info.offset_row_record_size; i++) {
        printf("%lith row at offset %li\n", ((i + 1) * (interval)) + 1, info.offset_row_record[i]);
    }
};

void test_file(char* path) {
    tokenizer_t tokenizer = csv_tokenizer();
    tokenizer_state_t *state = tokenizer_state_init(path, 0, 10, 10);
    tokenizer_start(&tokenizer, state);
    tokenizer_token_t *token;

    while (true) {
        int result =
                tokenizer_next(&tokenizer, state, &token);

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
    scan_results_t *results = ufo_csv_perform_initial_scan(path);

    printf("After initial scan of %s: \n\n", path);
    printf("    rows: %li\n", results->rows);
    printf("    cols: %li\n", results->columns);
    printf("    column_types:\n\n");

    for (size_t i = 0; i < results->columns; i++) {
        printf("        [%li]: %s/%i\n", i, token_type_to_string(results->column_types[i]), results->column_types[i]);
    }
};

int main (int argc, char *argv[]) {
    test_scan("test.csv");
    test_file("test.csv");
    test_initial_scan("test.csv");
    test_initial_scan("test2.csv");
    return 0;
}