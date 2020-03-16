#include "oroboros.h"
#include <assert.h>
#include <stdio.h>
#include <printf.h>

void test_oroboros_push_peek_pop(int n) {
    oroboros_t oroboros = oroboros_init(n);

    for (int i = 0; i < n + 10; i++) {
        oroboros_item_t item;
        item.stuff = i;

        int result = oroboros_push(oroboros, item);
        printf("push\ti=%i\tresult=%i\t", i, result);
        if (i < n) {
            assert(result == 0);
        } else {
            assert(result != 0);
        }
        printf("\t...ok\n");
    }

    for (int i = 0; i < n + 10; i++) {
        oroboros_item_t item;
        int result;

        item.stuff = 666;
        result = oroboros_peek(oroboros, &item);
        if (i < n) {
            printf("peek\ti=%i\tresult=%i\titem=%i", i, result, item.stuff);
            assert(result == 0);
            assert(item.stuff==i);
        } else {
            printf("peek\ti=%i\tresult=%i\titem=?", i, result);
            assert(result != 0);
            assert(item.stuff==666);
        }
        printf("\t...ok\n");

        item.stuff = 666;
        result = oroboros_pop(oroboros, &item);
        if (i < n) {
            printf("pop\ti=%i\tresult=%i\titem=%i", i, result, item.stuff);
            assert(result == 0);
            assert(item.stuff==i);
        } else {
            printf("pop\ti=%i\tresult=%i\titem=?", i, result);
            assert(result != 0);
            assert(item.stuff==666);
        }
        printf("\t...ok\n");
    }

    oroboros_free(oroboros);
}

int main(int argc, char **argv) {
    test_oroboros_push_peek_pop(100);
    return 0;
}