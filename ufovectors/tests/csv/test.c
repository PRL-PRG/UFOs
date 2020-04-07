#include <assert.h>
#include <stdio.h>
#include "../../src/ufo_csv_manipulation.h"

void test_file(char* path) {
    tokenizer_t tokenizer = csv_tokenizer();
    tokenizer_state_t *state = tokenizer_state_init(path, 0, 10);
    tokenizer_token_t *token;
    bool end_row;

    while (true) {
        int result =
                next(&tokenizer, state, &token, &end_row);

        printf("Token: [size: %li, start: %li, end: %li, string: \"%s\"], Row: %s, Result: %s\n",
               token->size, token->position_start, token->position_end, token->string,
               end_row ? "true" : "false",
               tokenizer_result_to_string(result));

        if (result == TOKENIZER_END_OF_FILE || result == TOKENIZER_ERROR) {
            break;
        }
    }
};


void main () {
    test_file("test.csv");
}