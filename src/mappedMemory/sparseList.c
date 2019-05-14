

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "sparseList.h"

typedef struct {
  union{
    void*    ptr;
    uint64_t ptrI;
  };
  uint64_t occupied :  1;
  uint64_t length    : 63;
} entryI;

typedef struct {
  entryI* list;
  uint64_t allocatedSlots, usedSlots, size;
} sparseList;


typedef struct {
  uint64_t contains : 1;
  uint64_t idx      : 63;
} searchResult;

sparseList_t newList(){
  sparseList* l = calloc(1, sizeof(sparseList));
  assert(NULL != l);

  l->size = 0;
  l->usedSlots = 0;
  l->list = calloc(8, sizeof(entryI));
  assert(NULL != l->list);
  l->allocatedSlots = 8;

  return (sparseList_t) l;
}

void freeList(sparseList_t list){
  sparseList* l = (sparseList*) list;
  free(l->list);
  free(l);
}

static inline int eCompare(uint64_t i, entryI* e){
  const bool ltP = i < e->ptrI;
  const bool gtE = i >= (e->ptrI + e->length);

   if(ltP)
     return -1;
   if(gtE)
     return 1;
   return 0;
}

static searchResult binarySearch(sparseList* list, uint64_t i){
  if(0 == list->size)
    return (searchResult){0, 0};
  entryI* e;
  int64_t m, l, h;
  int cmp;
  l = 0; h = list->usedSlots - 1;
  do{
    assert(l >= 0);
    assert(h < list->usedSlots);
    m = (l+h) >> 1;
    assert(m >= l);

    assert(m <= h);
    e = list->list+m;
    switch(cmp = eCompare(i, e)){
      case -1:
        h = m - 1;
        break;
      case 1:
        l = m + 1;
        break;
      default:
        assert(false);
      case 0:
        return (searchResult) {1, m};
    }
  }while(l <= h);
  return (searchResult) {0, -1 == cmp ? m : m + 1 };
}

enum {
  Lower   = -1,
  DirErr  =  0,
  Higher  =  1
};

static uint64_t shiftList(sparseList* l, uint64_t idx){
  entryI* e = NULL;
  uint64_t idxToTake;
  uint64_t actualIdx;
  int takeDir = DirErr;

  uint64_t top = l->usedSlots;

  uint64_t sz = top - idx;
  if(idx > sz)
    sz = idx;

  if(0 == l->usedSlots){
    assert(0 == idx);
    goto _grow; // empty list
  }
  assert(l->usedSlots >= idx); // idx should be less than or equal to the slots
  if(l->size == l->usedSlots)
    goto _grow; // No space left

  assert(l->size < l->usedSlots); // size should never be larger than the used slots, as if it were equal we'd jump to grow
  assert(idx == l->usedSlots || l->list[idx].occupied); // Why steal when this is already a tombstone

  const bool mayGrow = l->usedSlots < l->allocatedSlots;

  // Linear search for a free slot
  for(uint64_t i = 1; i <= sz; i++){
    #define shiftCheck(D, x)  ({    \
      e = l->list + x;              \
      if(!e->occupied){             \
        takeDir = D;                \
        idxToTake = x;              \
        goto _shift;                \
      }                             \
    })
    // When a free slot is found one of these will jump to shift
    int64_t il = idx - i;
    if(il >= 0)
      shiftCheck(Lower, il);
    int64_t iu = idx + i;
    if(iu < top)
      shiftCheck(Higher, iu);

    if(mayGrow && iu == top)
      goto _grow1;
  }

  for(uint64_t ii = 0; ii < l->usedSlots; ii++)
    assert(l->list[ii].occupied); // Are there really no free slots or did we miss one?
  assert(false); // No free slot found, even though there was free space?

  _grow:
  assert(l->allocatedSlots >= l->usedSlots);
  if(l->allocatedSlots == l->usedSlots){
    // Need to grow the list
    entryI* newList;
    // grow by 50% every time
    uint64_t newAlloc = l->allocatedSlots + (l->allocatedSlots >> 1);
    newList = realloc(l->list, newAlloc * sizeof(entryI));

    assert(NULL != newList); //TODO: some sane erreor

    l->list = newList;
    l->allocatedSlots = newAlloc;
  }

  // Update the used slot count after growing
  _grow1:
  l->usedSlots++;

  idxToTake = top;
  takeDir = Higher;

  _shift:
  switch(takeDir){
    case Lower:
      assert(idxToTake < idx);
      //Shift down
      memmove(l->list+idxToTake, l->list+idxToTake+1, sizeof(entry) * (idx - (idxToTake + 1)));
      actualIdx = idx - 1; // we opened up a slot below the target
      break;
    case Higher:
      // if idx is the end then no movement is needed
      if(idx < l->usedSlots - 1){
        assert(idx == 0 || idxToTake >= idx);
        //Shift up
        memmove(l->list+idx+1, l->list+idx, sizeof(entry) * (idxToTake - idx));
      }
      actualIdx = idx;// we moved the elements up
      break;
    default:
      assert(false);
      return -1;
  }

  assert(actualIdx < l->usedSlots);

  e = l->list + actualIdx;
  e->occupied = false; // Mark the slot we just freed as free
  return actualIdx;
}

static bool assertList(sparseList* l){
  const uint64_t u = l->usedSlots;

  assert(l->size <= u);

  for(uint64_t i = 0; i < u; i++){
    entryI* e = l->list+i;
    if(i != 0)
      assert(-1 == eCompare(l->list[i-1].ptrI, e));
    if(i+1 < u)
      assert(+1 == eCompare(l->list[i+1].ptrI, e));
  }
  return true;
}

int listAdd(sparseList_t list_t, void* ptr, uint64_t length){
  sparseList* l = (sparseList*) list_t;
  const searchResult insertionPoint = binarySearch(l, (uint64_t) ptr);
  entryI* e;

  if(insertionPoint.contains){
    //something is in the list here, if it isn't a tombstone then we can't insert
    e = l->list + insertionPoint.idx;
    if(e->occupied)
      return EntryConflict;
  }

  uint64_t actualIdx = insertionPoint.idx;

  if(l->size < l->usedSlots){
    // Space left, check if this is a tombstone. Shift if not
    e = l->list + insertionPoint.idx;
    if(actualIdx == l->usedSlots ||  e->occupied)
      actualIdx = shiftList(l, insertionPoint.idx);
  }else{ // No space left
    actualIdx = shiftList(l, insertionPoint.idx);
  }

  //Always re-read e with the new index
  e = l->list + actualIdx;

  // the entry will be marked as a tombstone upon grow, or was a tombstone
  if(e->occupied){
    assert(false);
    return AssertError;
  }

  e->occupied = true;
  e->length = length;
  e->ptr = ptr;

  l->size++;

  assert(actualIdx < l->usedSlots);
  assert(actualIdx == 0 || -1 == eCompare(l->list[actualIdx-1].ptrI, e));
  assert(actualIdx+1 == l->usedSlots || 1 == eCompare(l->list[actualIdx+1].ptrI, e));

  assert(assertList(l));

  return NoError;
}


int listRemove(sparseList_t list, void* ptr){
  sparseList* l = (sparseList*) list;
  searchResult r = binarySearch(l, (uint64_t) ptr);

  if(!r.contains)
    return NotRemoved;

  l->list[r.idx].occupied = false;
  l->size--;
  return NoError;
}

int listFind(sparseList_t list, entry* entry,  void* ptr){
  sparseList* l = (sparseList*) list;
  searchResult r = binarySearch(l, (uint64_t) ptr);

  if(r.contains){ // Something found
    entryI* e = l->list + r.idx;
    if(e->occupied){
      if(NULL != entry){
        entry->ptr = e->ptr;
        entry->length = e->length;
      }
      return NoError;
    }
  }
  if(NULL != entry){
    entry->ptr = NULL;
    entry->length = 0;
  }
  return NotFound;
}

