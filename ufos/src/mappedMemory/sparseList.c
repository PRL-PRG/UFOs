

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <string.h>

#include <assert.h>


#include "sparseList.h"

#define incGetModCt(l) (l->modCt = (l->modCt + 1) & ((1 << 30) - 1))

typedef struct {
  union{
    void*    ptr;
    uint64_t ptrI;
  };
  uint64_t occupied  :  1;
  uint64_t length    : 63;
  void*    valuePtr;
} entryI;

typedef struct {
  entryI* list;
  uint64_t allocatedSlots, usedSlots, size;
  uint32_t modCt;
} sparseList;


typedef struct {
  uint64_t contains : 1;
  uint64_t idx      : 63;
} searchResult;

sparseList_t newList(){
  sparseList* l = calloc(1, sizeof(sparseList));
  if(NULL == l)
    goto nomem0;

  l->size = 0;
  l->usedSlots = 0;
  l->list = calloc(8, sizeof(entryI));
  if(NULL == l)
    goto nomem1;

  l->allocatedSlots = 8;

  return (sparseList_t) l;

  nomem1:
  assert(NULL != l); // Should only be reachable after l is allocated
  free(l);

  nomem0:
  errno = ENOMEM;
  return NULL;
}

void freeList(sparseList_t list){
  sparseList* l = (sparseList*) list;
  free(l->list);
  free(l);
}

static inline int eCompare(bool overlapEq, uint64_t i, entryI* e){
  const bool ltP = i < e->ptrI;
  const bool gt  = overlapEq ? i >= (e->ptrI + e->length) : i > e->ptrI;

   if(ltP) // less than the pointer
     return -1;
   if(gt) // greater than either the pointer or the end
     return 1;
   return 0; // between ptr and the end or exactly eqal to ptr
}

static searchResult binarySearch(sparseList* list, uint64_t i){
  if(0 == list->usedSlots)
    return (searchResult){0, 0};
  entryI* e;
  // (M)edian, (L)ow, (H)igh
  int64_t m, l, h;
  int cmp;
  l = 0; h = list->usedSlots - 1;
  do{
    assert(l >= 0);
    assert(h < list->usedSlots);
    // https://web.archive.org/web/20190513121937/https://ai.googleblog.com/2006/06/extra-extra-read-all-about-it-nearly.html
    m = (l+h) >> 1; // find median
    assert(m >= l);
    assert(m <= h);

    e = list->list+m;
    switch(cmp = eCompare(e->occupied, i, e)){
      case -1:
        h = m - 1;
        break;
      case 1:
        l = m + 1;
        break;
      case 0:
        return (searchResult) {1, m};

      default:
        assert(false); // for debugger
        exit(EXIT_FAILURE); // This literally can't happen
    }
  }while(l <= h);
  return (searchResult) {0, -1 == cmp ? m : m + 1 };
}

enum {
  Lower   = -1,
  DirErr  =  0,
  Higher  =  1
};

/**
 * Shift elements of the list up or down to make room at or one below idx
 * This may also choose to expand the list
 *
 * Of note is that if the element we are trying to add is the new largest element
 *  then this will almost always expand the list if there is spare allocated space
 * If there is no extra allocated space then this will shift
 */
static int makeRoomAtIdx(sparseList* l, uint64_t* newIdx, uint64_t idx){
  entryI* e = NULL;
  uint64_t idxToTake;
  uint64_t actualIdx;
  int takeDir = DirErr;

  uint64_t top = l->usedSlots;

  // (S)i(z)e is large enough to guarantee that we explore every slot
  // starting with the ones adjacent to the intended index growing up and down
  const uint64_t sz = idx > (top-idx) ? idx : top - idx;

  if(0 == l->usedSlots){
    assert(0 == idx);
    goto _grow; // empty list
  }

  assert(l->usedSlots >= idx); // idx should be less than or equal to the slots
  assert(l->size <= l->usedSlots); // size should never be larger than the used slots
  if(l->size == l->usedSlots)
    goto _grow; // No space left, must grow

  assert(idx == l->usedSlots || l->list[idx].occupied); // Why are we stealing when this idx isn't occupied?

  // the top of the list may be closer than an empty slot, but only if there is free space
  const bool mayGrow = l->usedSlots < l->allocatedSlots;

  // Linear search for a free slot, which MUST exist because size < used
  // starting at 1 because we already know idx isn't available
  for(uint64_t i = 1; i <= sz; i++){
    //Only used / useful locally, so defined locally
    #define shiftCheck(D, x)  ({    \
      e = l->list + x;              \
      if(!e->occupied){             \
        takeDir = D;                \
        idxToTake = x;              \
        goto _shift;                \
      }                             \
    })
    // When a free slot is found one of these will jump to _shift
    int64_t il = idx - i;
    if(il >= 0)
      shiftCheck(Lower, il);
    int64_t iu = idx + i;
    if(iu < top)
      shiftCheck(Higher, iu);

    if(mayGrow && iu == top)
      goto _grow1; // the top of the list is closest and there is space, so just grow by 1
  }

  /*
   * This code should be unreachable
   * No free slot found, even though there was free space?
   * Should not be able to get here. Run some asserts to help discover the root cause
   */
  assert(({
    for(uint64_t ii = 0; ii < l->usedSlots; ii++)
        assert(l->list[ii].occupied); // Are there really no free slots or did we miss one?
    false; // trigger the enclosing assert
  }));
  // Crash and burn
  return AssertError;

  _grow:
  assert(l->allocatedSlots >= l->usedSlots); // Can't have more used slots than allocated slots
  // Only need to reallocate the list if there aren't any spare allocated slots
  if(l->allocatedSlots == l->usedSlots){
    // Need to grow the list
    entryI* newList;
    // grow by 50% every time
    const uint64_t newAlloc = l->allocatedSlots + (l->allocatedSlots >> 1);
    newList = realloc(l->list, newAlloc * sizeof(entryI));

    if(NULL == newList){
      errno = ENOMEM;
      return NoMemory; // Out of memory, can't do much
    }

    l->list = newList;
    l->allocatedSlots = newAlloc;
  }

  // Update the used slot count
  _grow1:
  l->usedSlots++;
  // and indicate that the free index is the new highest one
  idxToTake = top;
  takeDir = Higher;

  _shift:
  switch(takeDir){
    case Lower: // The free slot is below the target idx
      assert(idxToTake < idx);
      //Shift down
      memmove(l->list+idxToTake, l->list+idxToTake+1, sizeof(entry) * (idx - (idxToTake + 1)));
      actualIdx = idx - 1; // we opened up a slot below the target
      break;
    case Higher: // The free slot is above the target idx
      // if idx and take are the same at this point in timme then that implies that we are putting this element at the end of the list
      if(idx != idxToTake){
        assert(idx == 0 || idxToTake >= idx);
        //Shift up
        memmove(l->list+idx+1, l->list+idx, sizeof(entry) * (idxToTake - idx));
      }else{
        // make sure that if idx and to-take are the same that we are, in fact, adding to the end of the list
        assert(idx == top);
      }
      actualIdx = idx; // Target index unchanged, slot is now available
      break;
    default:
      // This should be impossible, there must be absolutely no path through the code that fails to set a valid direction
      assert(false);
      exit(EXIT_FAILURE);
  }

  // at this point in time even if we added to the end the used slots will have been incremented
  assert(actualIdx < l->usedSlots);

  // Mark the slot we just freed as free
  e = l->list + actualIdx;
  e->occupied = false;

  *newIdx = actualIdx;

  return NoError;
}

#ifdef ExpensiveAsserts
static void assertListInvariants(sparseList* l){
  const uint64_t u = l->usedSlots;
  assert(l->size <= u);

  uint64_t occupied = 0;

  for(uint64_t i = 0; i < u; i++){
    entryI* e = l->list+i;

    if(e->occupied)
      occupied++;

    // Make sure that the element below i is smaller
    if(i != 0)
      assert(-1 == eCompare(true, l->list[i-1].ptrI, e));
    // and the element above is higher
    if(i+1 < u)
      assert(+1 == eCompare(l->list[i+1].occupied, l->list[i+1].ptrI, e));
  }

  // We saw the correct number of elements, right?
  assert(l->size == occupied);
}
#else
#define assertListInvariants(l) ({})
#endif

int listAdd(sparseList_t list_t, void* ptr, uint64_t length, dataPtr value){
  sparseList* l = (sparseList*) list_t;
  const uint32_t modCt = incGetModCt(l);

  /* this return one of
   *  • the first indx in the list where the e@idx is larger than the elemnt we are trying to add
   *  • the idx of an overlaping element, and will also flag the return as "contains"
   *  • give us the index one past the end of the list because we are adding an element larger than the rest of the list
   */
  const searchResult insertionPoint = binarySearch(l, (uint64_t) ptr);
  entryI* e;

  if(insertionPoint.contains){
    //something is in the list here, if it occupied then we can't insert
    e = l->list + insertionPoint.idx;
    if(e->occupied)
      return EntryConflict;
  }

  // try to insert at the insertion point
  uint64_t actualIdx = insertionPoint.idx;

  if(l->size < l->usedSlots){
    /* Space left
     * That means that there is at least one tombstone
     * We may, in fact, be pointing right at it
     */
    e = l->list + insertionPoint.idx;
    //if the index is one larger than the list OR the element we ar targeting is occupied then make room
    if(actualIdx == l->usedSlots ||  e->occupied){
      int err = makeRoomAtIdx(l, &actualIdx, insertionPoint.idx);
      if(err) return err;
    }
  }else{ // No space left, the list will have to grow
    int err = makeRoomAtIdx(l, &actualIdx, insertionPoint.idx);
    if(err) return err;
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
  e->valuePtr = value;

  l->size++;

  // Ensure the element just added is greater than its predecessor and less than its sucessor
  // This holds true even if those slots are no longer "occupied"
  assert(actualIdx < l->usedSlots);
  assert(actualIdx == 0 || -1 == eCompare(true, l->list[actualIdx-1].ptrI, e));
  // Check if the next slot it strictly larger than us. If it is occupied check for overlap too
  if(actualIdx+1 < l->usedSlots)
    assert(1 ==  eCompare(l->list[actualIdx+1].occupied, l->list[actualIdx+1].ptrI, e));

  // Expensive assertion of all list invariants that we can think of
  // Will be a nop if turned off
  assertListInvariants(l);

  if(modCt != l->modCt){
    assert(false);
    return ConcurrentModificationError;
  }

  return NoError;
}


int listRemove(sparseList_t list, void* ptr){
  sparseList* l = (sparseList*) list;
  const uint32_t modCt = incGetModCt(l);

  searchResult r = binarySearch(l, (uint64_t) ptr);

  if(!r.contains)
    return NotRemoved;

  l->list[r.idx].occupied = false;
  l->list[r.idx].length   = 0;
  l->list[r.idx].valuePtr = NULL; // bonus safety
  l->size--;

  // Expensive assertion of all list invariants that we can think of
  // Will be a nop when expensive assertions are not enabled
  assertListInvariants(l);

  if(modCt != l->modCt){
    assert(false);
    return ConcurrentModificationError;
  }

  return NoError;
}

int listFind(sparseList_t list, entry* entry,  void* ptr){
  sparseList* l = (sparseList*) list;
  searchResult r = binarySearch(l, (uint64_t) ptr);

  if(r.contains){ // Something found
    entryI* e = l->list + r.idx;
    if(e->occupied){ // Slot had something, but that something was removed
      if(NULL != entry){
        entry->ptr = e->ptr;
        entry->valuePtr = e->valuePtr;
        entry->length = e->length;
      }
      return NoError;
    }
  }
  if(NULL != entry){
    entry->ptr = NULL;
    entry->valuePtr = NULL;
    entry->length = 0;
  }
  return NotFound;
}

void listWalk(sparseList_t list, walkCallback callback){
  assert(NULL != list);
  sparseList* l = (sparseList*) list;

  entry e;
  for(uint64_t x = 0; x < l->usedSlots; x++){
    entryI* en = l->list + x;
    if(!en->occupied)
      continue;
    e.length   = en->length;
    e.ptr      = en->ptr;
    e.valuePtr = en->valuePtr;
    callback(&e);
  }
}

