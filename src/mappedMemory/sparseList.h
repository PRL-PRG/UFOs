#pragma once

#include <stdint.h>

typedef void* any_t;

typedef any_t sparseList_t;

typedef struct {
  void*    ptr;
  uint64_t length;
} entry;


sparseList_t newList();
void freeList(sparseList_t list);


int listAdd(sparseList_t list, void* ptr, uint64_t length);
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
  AssertError
};

