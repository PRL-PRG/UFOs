
#include <stdio.h>
#include <printf.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <assert.h>

#include <unistd.h>


#include "../src/userfaultCore.h"
#include "../src/errors.h"

int testpopulate(uint64_t startValueIdx, uint64_t endValueIdx, ufPopulateCallout callout, ufUserData userData, char* target){
  uint64_t* t = (uint64_t*) target;
  for(int i = startValueIdx; i < endValueIdx; i++)
    t[i - startValueIdx] = ~i;

  return 0;
}


#define setBit(i)   ( bitmap[i >> 6] |= 1ull << (i & 0x3f) )
#define clearBit(i) ( bitmap[i >> 6] &= ~(1ull << (i & 0x3f)) )
#define testBit(i)  (0 != (bitmap[i >> 6] & (1ull << (i & 0x3f))) )

int main(int argc, char **argv) {
  int res;

  srand(13);

  ufInstance_t ufI = ufMakeInstance();
  tryPerrInt(res, ufInit(ufI), "Err Init", error);

//  uint64_t* bitmap = calloc(1024*1024*1024, 8);
  do{
    const uint64_t ct = 1024ull*1024*((rand() & 0xffull) + 1), sz = ct*8 ;

    ufObjectConfig_t config = makeObjectConfig(64, uint64_t, ct, (rand() & 0xffff) + 1);
    ufSetPopulateFunction(config, testpopulate);

    ufObject_t o;
    tryPerrInt(res, ufCreateObject(ufI, config, &o), "Err init obj", error1);
    uint64_t* ptr = (uint64_t*) ufGetValuePointer(o);

    free(config);

    printf("%lu\n", (uint64_t)ct);

    while((random() & 0xfff) != 0){
      uint64_t i = rand() % ct;
      assert(i < ct);
      assert(  (((uint64_t) &ptr[i]) - ((uint64_t)ptr)) < sz );
      uint64_t x = ptr[i];
      assert(~i == x);
    }

    tryPerrInt(res, ufDestroyObject(o), "Err destroying obj", error1);

  }while(1);

  error1:
  tryPerrInt(res, ufShutdown(ufI, false), "Err Shutdown", error);
  tryPerrInt(res, ufAwaitShutdown(ufI), "Err Await Shutdown", error);
  exit(0);

  error:
  exit(-1);
}
