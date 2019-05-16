#pragma once

#include <stdint.h>

typedef void* any_t;

typedef any_t sparseList_t;
typedef any_t dataPtr;

typedef struct {
  void*    ptr;
  uint64_t length;
  void* valuePtr;
} entry;


sparseList_t newList();
void freeList(sparseList_t list);


int listAdd(sparseList_t list, void* ptr, uint64_t length, dataPtr value);
int listRemove(sparseList_t list, void* ptr);
int listFind(sparseList_t list, entry* result, void* ptr);

/* returns */

enum{
  NoError = 0,

  //Non errors
  NotRemoved,
  NotFound,

  // Errors
  EntryConflict,

  // Critical errors, the program should not continue
  NoMemory,
  ConcurrentModificationError,
  AssertError
};
