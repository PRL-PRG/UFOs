#pragma once

#include <stddef.h>

typedef void *oroboros_t;

typedef struct {
  int stuff;
} oroboros_item_t;

typedef void(*oroboros_fun_t)(size_t /*index*/, oroboros_item_t /*element*/, void* user_data);

oroboros_t oroboros_init     (size_t initial_size);
int        oroboros_push     (oroboros_t, oroboros_item_t  item, int resize_if_full);
int        oroboros_pop      (oroboros_t, oroboros_item_t* item);
int        oroboros_peek     (oroboros_t, oroboros_item_t* item);
int        oroboros_resize   (oroboros_t, size_t size);
size_t     oroboros_size     (oroboros_t);
void       oroboros_free     (oroboros_t);
void       oroboros_for_each (oroboros_t, oroboros_fun_t, void* user_data);
