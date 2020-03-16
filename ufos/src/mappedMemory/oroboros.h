#pragma once

#include <stddef.h>

typedef void *oroboros_t;

typedef struct {
  int stuff;
} oroboros_item_t;

oroboros_t       oroboros_init   (size_t initial_size);
int              oroboros_push   (oroboros_t, oroboros_item_t  item);
int              oroboros_pop    (oroboros_t, oroboros_item_t* item);
int              oroboros_peek   (oroboros_t, oroboros_item_t* item);
oroboros_t       oroboros_resize (oroboros_t, size_t size);
void             oroboros_free   (oroboros_t);
