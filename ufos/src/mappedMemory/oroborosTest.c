#include "oroboros.h"
#include <assert.h>
#include <stdio.h>
#include <printf.h>

void test_oroboros_push_peek_pop(int n) {
    printf("oroboros push-peek-pop\n");

    oroboros_t oroboros = oroboros_init(n);

    for (int i = 0; i < n + 5; i++) {
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

    for (int i = 0; i < n + 5; i++) {
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

    printf("oroboros push-peek-pop ok\n");
}

void test_oroboros_resize(int n) {

    printf("oroboros resize\n");

    oroboros_t oroboros = oroboros_init(n);

    for (int i = 0; i < n + 5; i++) {
        oroboros_item_t item;
        item.stuff = i;

        int result = oroboros_push(oroboros, item);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        if (i < n) {
            assert(result == 0);
        } else {
            assert(result != 0);
        }
        printf("\t...ok\n");
    }

    printf("oroboros\told size=%i\t", oroboros_size(oroboros));
    int resize_result = oroboros_resize(oroboros, n + 5);
    printf("new size=%i\tresult=%i", oroboros_size(oroboros), resize_result);
    assert(resize_result == 0);
    assert(oroboros_size(oroboros) == n + 5);
    printf("\t...ok\n");

    for (int i = n; i < n + 10; i++) {
        oroboros_item_t item;
        item.stuff = i;

        int result = oroboros_push(oroboros, item);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        fflush(stdout);
        if (i < n + 5) {
            assert(result == 0);
        } else {
            assert(result != 0);
        }
        printf("\t...ok\n");
    }

    printf("oroboros resize ok\n");
}

void test_oroboros_resize_with_overflow(int n) {

    printf("oroboros resize\n");

    oroboros_t oroboros = oroboros_init(n);

    for (int i = 0; i < n + 10; i++) {
        oroboros_item_t item;
        item.stuff = i;

        int result = oroboros_push(oroboros, item);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        if (i < n) {
            assert(result == 0);
        } else {
            assert(result != 0);
        }
        printf("\t...ok\n");
    }

    printf("oroboros\told size=%i\t", oroboros_size(oroboros));
    int resize_result = oroboros_resize(oroboros, n + 5);
    printf("new size=%i\tresult=%i", oroboros_size(oroboros), resize_result);
    assert(resize_result == 0);
    assert(oroboros_size(oroboros) == n + 5);
    printf("\t...ok\n");

    for (int i = n; i < n + 5; i++) {
        oroboros_item_t item;
        item.stuff = i;

        int result = oroboros_push(oroboros, item);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        fflush(stdout);
        assert(result == 0);
        printf("\t...ok\n");
    }

    printf("oroboros resize ok\n");
}

int main(int argc, char **argv) {
    //test_oroboros_push_peek_pop(10);
    test_oroboros_resize(10);
    //g();
    return 0;
}