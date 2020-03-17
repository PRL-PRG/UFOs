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

// CMYK 05.03.2020 TODO: Should we also have a variant that resizes if needed?
int oroboros_push(oroboros_t an_oroboros, oroboros_item_t item) {               // oroboros moves its head to consume an item
                                                                                // but oroboros does not trust
                                                                                // item is provided but it is not what oroboros consumes
                                                                                // oroboros makes appear another a simulacrum of item

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true form
                                                                                // oroboros is secretly a struct pointer

  if ((oroboros->elements != 0) && (oroboros->head == oroboros->tail))          // oroboros has caught up with its tail
      return -1;                                                                // oroboros is full
                                                                                // perhaps it is time to make oroboros bigger

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
  oroboros->tail = oroboros->tail + 1 % oroboros->size;                         // oroboros moves its tail in a circular path
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

  memcpy(added_region, oroboros->buffer, oroboros->size);

  if (oroboros->head > size_of_added_region) {
    oroboros_item_t *overflow_region = oroboros->buffer + size_of_added_region;
    size_t size_of_overflow_region = size_of_added_region - oroboros->head;
    memcpy(oroboros->buffer, overflow_region, size_of_overflow_region);
  }

  size_t head_after_resize = (oroboros->head + 1) % oroboros->size;             // oroboros will move its head to follow the new path
  oroboros->head = head_after_resize;                                           // oroboros adjusts its head
  oroboros->size = size;                                                        // oroboros counts the steps

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


// CMYK 05.03.2020 TODO: Note on resizing https://github.com/unofficial-openjdk/openjdk/blob/jdk8u/jdk8u/jdk/src/share/classes/java/util/ArrayList.java
// Grow by 50% of the current size (s + (s >> 1)) for a nice amortization on the cost of growing with fairly well bounded waste
