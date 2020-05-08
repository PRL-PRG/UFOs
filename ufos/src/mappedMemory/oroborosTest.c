#include "oroboros.h"
#include <assert.h>
#include <stdio.h>
#include <printf.h>

void print_item_and_check(size_t index, oroboros_item_t *item, void *data) {
    int *expected_values = (int *) data;
    printf("item\t%li -> %li == %i\n", index, item->owner_id, expected_values[index]);
    assert(item->owner_id == expected_values[index]);
}

void print_item(size_t index, oroboros_item_t *item, void *data) {
    printf("item\t%li -> %li\n", index, item->owner_id);
}

void test_oroboros_push_peek_pop(int n) {
    printf("oroboros push-peek-pop\n");

    oroboros_t oroboros = oroboros_init(n);

    for (int i = 0; i < n + 5; i++) {
        oroboros_item_t item;
        item.owner_id = i;

        int result = oroboros_push(oroboros, item, 0);
        printf("push\ti=%i\tresult=%i\t", i, result);
        if (i < n) {
            assert(result == 0);
        } else {
            assert(result != 0);
        }
        printf("\t...ok\n");
    }

    int expected_values[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    oroboros_for_each(oroboros, &print_item_and_check, expected_values);

    for (int i = 0; i < n + 5; i++) {
        oroboros_item_t item;
        int result;

        item.owner_id = 666;
        result = oroboros_peek(oroboros, &item);
        if (i < n) {
            printf("peek\ti=%i\tresult=%i\titem=%li", i, result, item.owner_id);
            assert(result == 0);
            assert(item.owner_id==i);
        } else {
            printf("peek\ti=%i\tresult=%i\titem=?", i, result);
            assert(result != 0);
            assert(item.owner_id==666);
        }
        printf("\t...ok\n");

        item.owner_id = 666;
        result = oroboros_pop(oroboros, &item);
        if (i < n) {
            printf("pop\ti=%i\tresult=%i\titem=%li", i, result, item.owner_id);
            assert(result == 0);
            assert(item.owner_id==i);
        } else {
            printf("pop\ti=%i\tresult=%i\titem=?", i, result);
            assert(result != 0);
            assert(item.owner_id==666);
        }
        printf("\t...ok\n");
    }

    oroboros_free(oroboros);

    printf("oroboros push-peek-pop ok\n");
}

void test_oroboros_simple_resize(int n) {

    printf("oroboros simple resize\n");

    oroboros_t oroboros = oroboros_init(n + 2);

    for (int i = 0; i < n; i++) {
        oroboros_item_t item;
        item.owner_id = i;

        int result = oroboros_push(oroboros, item, 0);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        assert(result == 0);
        printf("\t...ok\n");
    }

    int expected_values[n];
    for (size_t i = 0; i < n; i++)
        expected_values[i] = i;
    oroboros_for_each(oroboros, &print_item_and_check, expected_values);

    printf("resize\told size=%li\t", oroboros_size(oroboros));
    int resize_result = oroboros_resize(oroboros, n + 5);
    printf("new size=%li\tresult=%i", oroboros_size(oroboros), resize_result);
    assert(resize_result == 0);
    assert(oroboros_size(oroboros) == n + 5);
    printf("\t...ok\n");

    oroboros_for_each(oroboros, &print_item_and_check, expected_values);

    for (int i = n; i < n + 10; i++) {
        oroboros_item_t item;
        item.owner_id = i;

        int result = oroboros_push(oroboros, item, 0);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        fflush(stdout);
        if (i < n + 5) {
            assert(result == 0);
        } else {
            assert(result != 0);
        }
        printf("\t...ok\n");
    }


    int expected_values2[n+5];
    for (size_t i = 0; i < n+5; i++)
        expected_values2[i] = i;
    oroboros_for_each(oroboros, &print_item_and_check, expected_values2);

    oroboros_free(oroboros);

    printf("oroboros resize ok\n");
}

void test_oroboros_resize_with_overflow(int n) {

    printf("oroboros resize with overflow\n");

    oroboros_t oroboros = oroboros_init(n);

    for (int i = 0; i < n; i++) {
        oroboros_item_t item;
        item.owner_id = i;

        int result = oroboros_push(oroboros, item, 0);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        assert(result == 0);
        printf("\t...ok\n");
    }

    int expected_values[n];
    for (size_t i = 0; i < n; i++)
        expected_values[i] = i;
    oroboros_for_each(oroboros, &print_item_and_check, expected_values);

    printf("resize\told size=%li\t", oroboros_size(oroboros));
    int resize_result = oroboros_resize(oroboros, n + 5);
    printf("new size=%li\tresult=%i", oroboros_size(oroboros), resize_result);
    assert(resize_result == 0);
    assert(oroboros_size(oroboros) == n + 5);
    printf("\t...ok\n");

    oroboros_for_each(oroboros, &print_item_and_check, expected_values);

    for (int i = n; i < n + 10; i++) {
        oroboros_item_t item;
        item.owner_id = i;

        int result = oroboros_push(oroboros, item, 0);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        fflush(stdout);
        if (i < n + 5) {
            assert(result == 0);
        } else {
            assert(result != 0);
        }
        printf("\t...ok\n");
    }

    int expected_values2[n+5];
    for (size_t i = 0; i < n+5; i++)
        expected_values2[i] = i;
    oroboros_for_each(oroboros, &print_item_and_check, expected_values2);

    printf("oroboros resize with overflow ok\n");
}

void test_oroboros_resize_with_complex_overflow(int n) {

    printf("oroboros resize with complex overflow\n");

    oroboros_t oroboros = oroboros_init(n);

    for (int i = 0; i < n; i++) {
        oroboros_item_t item;
        item.owner_id = i;

        int result = oroboros_push(oroboros, item, 0);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        if (i < n) {
            assert(result == 0);
        } else {
            assert(result != 0);
        }
        printf("\t...ok\n");
    }

    int expected_values[n];
    for (size_t i = 0; i < n; i++)
        expected_values[i] = i;
    oroboros_for_each(oroboros, &print_item_and_check, expected_values);

    for (int i = 0; i < 8; i++) {
        oroboros_item_t item;
        item.owner_id = 666;
        int result = oroboros_pop(oroboros, &item);
        printf("pop\tinto=%p\ti=%i\tresult=%i\titem=%li", oroboros, i, result, item.owner_id);
        assert(result == 0);
        assert(item.owner_id==i);
        printf("\t...ok\n");
    }

    int expected_values2[n-8];
    for (size_t i = 0; i < n - 8; i++)
        expected_values2[i] = 8 + i;
    oroboros_for_each(oroboros, &print_item_and_check, expected_values2);

    for (int i = n; i < n + 8; i++) {
        oroboros_item_t item;
        item.owner_id = i;
        int result = oroboros_push(oroboros, item, 0);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        assert(result == 0);
        printf("\t...ok\n");
    }

    int expected_values3[n+5];
    for (size_t i = 0; i < n + 5; i++)
        expected_values3[i] = 8 + i;
    oroboros_for_each(oroboros, &print_item_and_check, expected_values3);

    printf("resize\told size=%li\t", oroboros_size(oroboros));
    int resize_result = oroboros_resize(oroboros, n + 5);
    printf("new size=%li\tresult=%i", oroboros_size(oroboros), resize_result);
    assert(resize_result == 0);
    assert(oroboros_size(oroboros) == n + 5);
    printf("\t...ok\n");

    oroboros_for_each(oroboros, &print_item_and_check, expected_values3);

    for (int i = n + 8; i < n + 8 + 5; i++) {
        oroboros_item_t item;
        item.owner_id = i;
        int result = oroboros_push(oroboros, item, 0);
        printf("push\tinto=%p\ti=%i\tresult=%i\t", oroboros, i, result);
        fflush(stdout);
        assert(result == 0);
        printf("\t...ok\n");
    }

    int expected_values4[n+8];
    for (size_t i = 0; i < n + 8; i++)
        expected_values4[i] = 8 + i;
    oroboros_for_each(oroboros, &print_item_and_check, expected_values4);

    oroboros_free(oroboros);

    printf("oroboros resize with complex overflow ok\n");
}

int main(int argc, char **argv) {
    test_oroboros_push_peek_pop(10);
    test_oroboros_simple_resize(10);
    test_oroboros_resize_with_overflow(10);
    test_oroboros_resize_with_complex_overflow(10);
    return 0;
}