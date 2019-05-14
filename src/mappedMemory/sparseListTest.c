
#include <stdio.h>
#include <printf.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <assert.h>

#include "sparseList.h"

int main(int argc, char **argv) {

  uint64_t* bitmap = calloc(256, 8);
  #define setBit(i) ( bitmap[i >> 6] |= 1ull << (i & 0x3f) )
  #define clearBit(i) ( bitmap[i >> 6] &= ~(1ull << (i & 0x3f)) )
  #define testBit(i) (0 != (bitmap[i >> 6] & (1ull << (i & 0x3f))) )

  #define iPtr ( (void*)((i << 8) + (random() & 0x0f)) )

  sparseList_t l = newList();
  do{
    int r = random();
    uint64_t i = random() & 0xfff;
    if(r & 1){
      // Add something
      bool exists = testBit(i);
      int ret;
      if(!exists){
        ret = listAdd(l, (void*)(i << 8), 128);
        assert(!ret);
        setBit(i);
      }

      assert(testBit(i));

      entry e;
      ret = listFind(l, &e, iPtr );
      assert(!ret);
      assert((uint64_t)e.ptr == i << 8);
    }else{
      // Remove something
      if(testBit(i)){
        int ret = listFind(l, NULL, iPtr );
        assert(!ret);

        ret = listRemove(l, iPtr );
        assert(!ret);
        ret = listFind(l, NULL, iPtr );
        assert(ret == NotFound);
        clearBit(i);
        assert(!testBit(i));
      }
    }
  }while(1);
}
