// Circular buffer implementation. It holds pointers.

#include <malloc.h>
#include "oroboros.h"

typedef struct {                                                                // oroboros is a struct

  size_t head;                                                                  // oroboros has a head
  size_t tail;                                                                  // oroboros has a tail
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
  oroboros->size = initial_size;                                                // oroboros travels a path of arbitrary length

  return (oroboros_t) oroboros;                                                 // oroboros hungers
}

// CMYK 05.03.2020 TODO: Should we also have a variant that resizes if needed?
int oroboros_push(oroboros_t an_oroboros, oroboros_item_t item) {               // oroboros moves its head to consume an item

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true form
                                                                                // oroboros is secretly a struct pointer

  size_t head_after_push = (oroboros->head + 1) % oroboros->size;               // oroboros will move its head in a circular path

  if (head_after_push == oroboros->tail) return -1;                             // oroboros has caught up with its tail
                                                                                // oroboros is full
                                                                                // perhaps it is time to make oroboros bigger

  oroboros->buffer[oroboros->head] = item;                                      // oroboros fills its head with ideas about item
  oroboros->head = head_after_push;                                             // oroboros moves its head to consume the item

  return 0;                                                                     // oroboros is happy
}

// CMYK 05.03.2020 TODO: This should return the element pop'd
int oroboros_pop(oroboros_t an_oroboros) {                                      // oroboros moves its tail to discard an item

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true form
                                                                                // oroboros is secretly a struct pointer

  if (oroboros->head == oroboros->tail) return -1;                              // oroboros is too small
                                                                                // oroboros has nothing left to give
                                                                                // do not ask more of oroboros

  oroboros->tail = oroboros->tail + 1 % oroboros->size;                         // oroboros moves its tail in a circular path

  return 0;                                                                     // oroboros is content
}


oroboros_t oroboros_resize(oroboros_t an_oroboros, size_t size) {               // oroboros adapts to follow a new path
                                                                                // the path will be bigger or smaller
                                                                                // oroboros can now grow to be bigger or not as big

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true shape
                                                                                // oroboros is secretly a struct pointer
// CMYK 05.03.2020 TODO: this should realloc the internal buffer instead, and not return a new snekfriemb
  oroboros_internal_t *new_oroboros =                                           // oroboros friend is born
          (oroboros_internal_t *) oroboros_init(size);

  if (new_oroboros == NULL) return NULL;                                        // oroboros cannot change size without a friend
                                                                                // why is there no room in the world for oroboros friend?

  new_oroboros->tail = oroboros->tail - oroboros->head;                         // oroboros friend kept its head is at the beginning of the path
                                                                                // but oroboros friend moved its tail
                                                                                // oroboros and oroboros friend are not equal length

  for (size_t i = 0; i < oroboros->size; ++i) {                                 // oroboros looks at each item in its path carefully
                                                                                // be it within or without oroboros

    size_t j = i + oroboros->head % oroboros->size;                             // oroboros thinks where each item should be on the path
                                                                                // oroboros wants oroboros friend to have the same items within
                                                                                // and the same items without as oroboros

    new_oroboros->buffer[i] = oroboros->buffer[j];                              // oroboros puts an element on the path of oroboros friend
  }                                                                             // oroboros friend is the same as oroboros

  oroboros_free(oroboros);                                                      // oroboros retires
  return (oroboros_t) new_oroboros;                                             // oroboros friend becomes oroboros
}


void oroboros_free(oroboros_t an_oroboros) {                                    // oroboros relinquishes its life

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true shape
                                                                                // oroboros is secretly a struct pointer

  free(oroboros);                                                               // oroboros expires
}

// CMYK 05.03.2020 TODO: This would normally be called peek, and it should peek at the tail
const oroboros_item_t *oroboros_get(oroboros_t an_oroboros) {                   // oroboros illuminates its deep inner life

  oroboros_internal_t *oroboros = (oroboros_internal_t *) an_oroboros;          // oroboros reveals its true shape
                                                                                // oroboros is secretly a struct pointer

  return &(oroboros->buffer[oroboros->head]);                                   // oroboros shows the contents of its head
                                                                                // ::hiss::
}


// CMYK 05.03.2020 TODO: Note on resizing https://github.com/unofficial-openjdk/openjdk/blob/jdk8u/jdk8u/jdk/src/share/classes/java/util/ArrayList.java
// Grow by 50% of the current size (s + (s >> 1)) for a nice amortization on the cost of growing with fairly well bounded waste
