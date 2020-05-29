#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <openssl/sha.h>

#include "userfaultCore.h"

typedef void *oroboros_t; // TODO undercasicide

typedef struct {
  ufObject_t ufo;     // the UFO to which this item belongs // TODO CMYK for KMS 2020.05.26: Is there a reason for this to be the ID instead of a pointer?
  void*      address; // Location in memory inside some UFO
  size_t     size;    // in bytes, set to zero as a flag that the object itself was GC'd
  uint8_t    sha[SHA256_DIGEST_LENGTH];  // checksum on load, used by the writes system
} oroboros_item_t;

typedef void(*oroboros_fun_t)(size_t /*index*/, oroboros_item_t* /*element*/, void* user_data);

oroboros_t oroboros_init     (size_t initial_size);
int        oroboros_push     (oroboros_t, oroboros_item_t  item, int resize_if_full);
int        oroboros_pop      (oroboros_t, oroboros_item_t* item);
int        oroboros_peek     (oroboros_t, oroboros_item_t* item);
int        oroboros_resize   (oroboros_t, size_t size);
size_t     oroboros_size     (oroboros_t);
void       oroboros_free     (oroboros_t);
void       oroboros_for_each (oroboros_t, oroboros_fun_t, void* user_data);
