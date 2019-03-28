//#pragma once

#include <stdint.h>
#include <stdbool.h>

struct bNode;

#define bMaxKeys 3
#define bMaxChildren (bMaxKeys+1)

#define bMinKeys (bMaxKeys >> 1)
#define bMinChildren (bMinKeys + 1)

typedef union {
  void*    ptr;
  uint64_t i;
} ptrInt;

typedef struct{
  ptrInt start;
  ptrInt end;
  void*  value;
} bKey; // 24 bytes

typedef struct{
  bNode*    parent;
  uint64_t  occupancy;
  uint64_t  __padding; // Keep in reserve
} bMeta; // 24

typedef struct __attribute__((align(64))){ // Align with cache lines, we are 2 lies large but don't mind aligning on even or odd
  bKey*     keys[bMaxKeys];          // 72 bytes = 24 * 3
  bNode*    children[bMaxChildren];  // 32 = 8 * 4
  bMeta     metadata;                // 24
} bNode;                             // Total 128

typedef struct{
  int     err;
  bKey*  element;
}bInsertResult;

#define BInsertConflict 1

bInsertResult bInsert(bNode*, bKey*);
void* bLookup(bNode*, uint64_t);
bKey* bRemove(bNode*, bKey*);
