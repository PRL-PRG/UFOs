
#include <stdio.h>
#include <printf.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <assert.h>

#include "../src/sparseList.h"

int main(int argc, char **argv) {

  uint64_t* bitmap = calloc(256, 8);
  #define setBit(i)   ( bitmap[i >> 6] |= 1ull << (i & 0x3f) )
  #define clearBit(i) ( bitmap[i >> 6] &= ~(1ull << (i & 0x3f)) )
  #define testBit(i)  (0 != (bitmap[i >> 6] & (1ull << (i & 0x3f))) )

  #define iPtr ( (void*)((i << 8) + (random() & 0x0f)) )
  srand(12);

  sparseList_t l = newList();
  do{
    const int r = random();
    const uint64_t i = random() & 0xfff;
    if(r & 1){
      // Add something or make sure it is already there
      const bool exists = testBit(i);
      int ret;
      if(exists){
        // already in the list, make sure that re-adding is a conflict
        ret = listAdd(l, (void*)(i << 8), 128, NULL);
        assert(EntryConflict == ret);
      }else{
        // add to the list
        ret = listAdd(l, (void*)(i << 8), 128, (void*)i);
        assert(!ret);
        // mark it as in the list
        setBit(i);
      }

      // make sure it is marked as in the list
      assert(testBit(i));

      entry e;
      ret = listFind(l, &e, iPtr);
      assert(!ret);
      assert((uint64_t)e.ptr == i << 8);
      assert((uint64_t)e.valuePtr == i);
    }else{
      // Remove something or ensure it isn't there
      if(testBit(i)){
        // Make sure it is in the list
        int ret = listFind(l, NULL, iPtr );
        assert(!ret);

        // Then remove it
        ret = listRemove(l, iPtr );
        assert(!ret);

        // Make sure it is gone
        ret = listFind(l, NULL, iPtr );
        assert(ret == NotFound);

        // Then mark it as gone
        clearBit(i);
        assert(!testBit(i));
      }else{
        // make sure it really isn't in the list
        int ret = listFind(l, NULL, iPtr );
        assert(!!ret);
      }
    }
  }while(1);
}
