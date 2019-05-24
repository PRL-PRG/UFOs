//#pragma once

#include <stdint.h>
#include <stdbool.h>

#define bMaxKeys 3
#define bMaxChildren (bMaxKeys+1)

#define bMinKeys (bMaxKeys >> 1)
#define bMinChildren (bMinKeys + 1)

struct bNode;

typedef union {
  void*    ptr;
  uint64_t i;
} ptrInt;


//TODO: move value out of key to compress the key area of the node
typedef struct{
  ptrInt start;
  ptrInt end;
  void*  value;
} bKey; // 24 bytes

typedef struct{
  struct bNode*    parent;
  uint64_t         occupancy;
  uint64_t         __padding; // Keep in reserve
} bMeta; // 24

typedef struct __attribute__((aligned(64))) bNode { // Align with cache lines, we are 2 lines large
  bKey*             keys[bMaxKeys];          // 72 bytes = 24 * 3
  struct bNode*     children[bMaxChildren];  // 32 = 8 * 4
  bMeta             metadata;                // 24
} bNode;                                     // Total 128

typedef struct{
  int     err;
  bKey*  element;
}bInsertResult;

#define BInsertConflict 1

bInsertResult bInsert(bNode* node, bKey* key);
void* bLookup(bNode* node, uint64_t intInRange);
bKey* bRemove(bNode* node, bKey* key);
