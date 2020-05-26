// Circular buffer implementation. It holds pointers.

#include <malloc.h>
#include <string.h>
#include "oroboros.h"

typedef struct {                                                                // oroboros is a struct

  size_t head;                                                                  // oroboros has a head
  size_t tail;                                                                  // oroboros has a tail
  size_t elements;                                                              // oroboros knows its length
  size_t size;                                                                  // oroboros follows a path of a specific circumference

  oroboros_item_t *buffer;                                                      // oroboros swallows items along the path
                                                                                // oroboros moves its head to consume items
                                                                                // oroboros moves its tail to relinquish them

} oroboros_internal_t;                                                          // this is the true form of oroboros

oroboros_t oroboros_init(size_t initial_size) {                                 // oroboros is born

  oroboros_internal_t *oroboros =                                               // oroboros comes into existence without form
          (oroboros_internal_t *) malloc(sizeof(oroboros_internal_t));

  oroboros->head = 0;                                                           // oroboros starts at the beginning of the path
  oroboros->tail = 0;                                                           // oroboros is initially small
  oroboros->elements = 0;                                                       // oroboros know that it is small
  oroboros->size = initial_size;                                                // oroboros travels a path of arbitrary length

  oroboros->buffer =                                                            // oroboros becomes a chasm waiting to be filled
          (oroboros_item_t *) malloc(sizeof(oroboros_item_t) * oroboros->size);

  return (oroboros_t) oroboros;                                                 // oroboros hungers
}

int oroboros_push(oroboros_t an_oroboros, oroboros_item_t item, int resize) {   // oroboros moves its head to consume an item
                                                                                // but oroboros does not trust
                                                                                // item is provided but it is not what oroboros consumes
                                                                                // oroboros makes appear another a simulacrum of item

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true form
                                                                                // oroboros is secretly a struct pointer

  if ((oroboros->elements != 0) && (oroboros->head == oroboros->tail)) {        // oroboros has caught up with its tail
    if (!resize) {                                                              // sometimes it is not time to make oroboros bigger
      return -1;                                                                // but oroboros is full
    } else {                                                                    // perhaps it is time to make oroboros bigger
      if(oroboros_resize(an_oroboros,                                           // oroboros will try to grow
          oroboros->size + (oroboros->size >> 1))){
        return -1;                                                              // oroboros discovered resizing was unwise
      }
    }
  }

  size_t head_after_push = (oroboros->head + 1) % oroboros->size;               // oroboros will move its head in a circular path

  oroboros->buffer[oroboros->head] = item;                                      // oroboros fills its head with ideas about item
  oroboros->head = head_after_push;                                             // oroboros moves its head to consume the item
  oroboros->elements++;                                                         // oroboros grows

  return 0;                                                                     // oroboros is happy
}

int oroboros_pop(oroboros_t an_oroboros, oroboros_item_t *item) {               // oroboros moves its tail to discard an item

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true form
                                                                                // oroboros is secretly a struct pointer

  if (oroboros->elements == 0) return -1;                                       // oroboros is too small
                                                                                // oroboros has nothing left to give
                                                                                // do not ask more of oroboros

  *item = oroboros->buffer[oroboros->tail];                                     // oroboros makes apparent the content of its bowels
  oroboros->tail = (oroboros->tail + 1) % oroboros->size;                       // oroboros moves its tail in a circular path
  oroboros->elements--;                                                         // oroboros diminishes

  return 0;                                                                     // oroboros is content
}

int oroboros_resize(oroboros_t an_oroboros, size_t size) {                      // oroboros adapts to follow a new path
                                                                                // the path will be bigger or smaller
                                                                                // oroboros can now grow to be bigger or not as big

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals itself
                                                                                // oroboros is secretly a struct pointer

  if (oroboros->size > size) return -1;                                         // oroboros refuses to limit its ambition
  if (oroboros->size == size) return 0;                                         // oroboros changes naught

  oroboros->buffer =                                                            // oroboros sees the path stretching before it
          (oroboros_item_t *) realloc(oroboros->buffer,                         // oroboros will follow the new path
                                      size * sizeof(oroboros_item_t));          // a grander journey will begin

  if (oroboros->buffer == NULL) return -2;                                      // oroboros is set in its ways
                                                                                // and the world has no new paths for it

  if (oroboros->elements == 0 || oroboros->tail < oroboros->head) {             // oroboros already has its head on the new path
    oroboros->size = size;                                                      // oroboros counts the steps
    return 0;                                                                   // oroboros sets out on a new journey
  }

  oroboros_item_t *added_region = oroboros->buffer + oroboros->size;
  size_t size_of_added_region = size - oroboros->size;

  memcpy(added_region, oroboros->buffer, sizeof(oroboros_item_t) * size_of_added_region);

  if (oroboros->head > size_of_added_region) {
    oroboros_item_t *overflow_region = oroboros->buffer + size_of_added_region;
    size_t size_of_overflow_region = oroboros->head - size_of_added_region;
    memcpy(oroboros->buffer, overflow_region, sizeof(oroboros_item_t) * size_of_overflow_region);
  }

  oroboros->size = size;                                                        // oroboros counts the steps
  size_t head_after_resize =                                                    // oroboros will move its head to follow the new path
          (oroboros->head + oroboros->elements) % oroboros->size;
  oroboros->head = head_after_resize;                                           // oroboros adjusts its head

  return 0;
}

size_t oroboros_size(oroboros_t an_oroboros) {                                  // oroboros reveals the size of its path

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals itself
                                                                                // oroboros is secretly a struct pointer

  return oroboros->size;                                                        // oroboros counts the steps
}

void oroboros_free(oroboros_t an_oroboros) {                                    // oroboros relinquishes its life

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true shape
                                                                                // oroboros is secretly a struct pointer

  free(oroboros->buffer);                                                       // oroboros discorporates its contents
  free(oroboros);                                                               // oroboros expires
}

int oroboros_peek(oroboros_t an_oroboros, oroboros_item_t *item) {              // oroboros illuminates its deep inner life

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true shape
                                                                                // oroboros is secretly a struct pointer

  if (oroboros->elements == 0) return -1;                                       // oroboros is empty
                                                                                // oroboros has nothing to reveal
                                                                                // oroboros cries

  *item = oroboros->buffer[oroboros->tail];                                     // oroboros reveals its oldest item
  return 0;                                                                     // this gives oroboros satisfaction
}

void oroboros_for_each (oroboros_t an_oroboros, oroboros_fun_t f, void *data) {
  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true shape

  for (size_t i = 0; i < oroboros->elements; i++) {
    size_t index = (i + oroboros->tail) % oroboros->size;
    f(i, &oroboros->buffer[index], data);
  }
}
