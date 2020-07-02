
#include <stdio.h>
#include <printf.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <assert.h>

#include <unistd.h>


#include "userfaultCore.h"
#include "../unstdLib/errors.h"

int testpopulate(uint64_t startValueIdx, uint64_t endValueIdx, ufPopulateCallout callout, ufUserData userData, char* target){
  uint64_t* t = (uint64_t*) target;
  uint64_t* requestCt = (uint64_t*) userData;
  assert(startValueIdx >= 0);
  assert(endValueIdx <= *requestCt);
  for(int i = startValueIdx; i < endValueIdx; i++)
    t[i - startValueIdx] = ~i;

  return 0;
}


#define setBit(i)   ( bitmap[i >> 6] |= 1ull << (i & 0x3f) )
#define clearBit(i) ( bitmap[i >> 6] &= ~(1ull << (i & 0x3f)) )
#define testBit(i)  (0 != (bitmap[i >> 6] & (1ull << (i & 0x3f))) )

int main(int argc, char **argv) {
  int res;
  int i = 0;
  int n = (argc > 1) ? atoi(argv[1]) : -1;

  srand(13);

  ufInstance_t ufI = ufMakeInstance();
  ufSetMemoryLimits(ufI, 100*1024*1024, 50*1024*1024);
  tryPerrInt(res, ufInit(ufI), "Err Init", error);

//  uint64_t* bitmap = calloc(1024*1024*1024, 8);
  while(n < 0 || i < n) {
    uint64_t ct = 1024ull*1024*((rand() & 0xffull) + 1), sz = ct*8 ;

    ufObjectConfig_t config = makeObjectConfig(uint64_t, 64, ct, (rand() & 0xffff) + 1);
    ufSetPopulateFunction(config, testpopulate);
    ufSetUserConfig(config, &ct);

    ufObject_t o;
    tryPerrInt(res, ufCreateObject(ufI, config, &o), "Err init obj", error1);
    uint64_t* h = (uint64_t*) ufGetHeaderPointer(o);
    assert(h[0] == 0x00);
    h[0] = 0x01;
    assert(h[0] == 0x01);


    uint64_t* ptr = (uint64_t*) ufGetValuePointer(o);

    free(config);

    printf("%lu\n", (uint64_t)ct);

    const uint64_t persianFlawIdx = rand() % ct;
    ptr[persianFlawIdx] = persianFlawIdx;

    while((random() & 0xfff) != 0){
      uint64_t i = rand() % ct;
      assert(i < ct);
      assert(  (((uint64_t) &ptr[i]) - ((uint64_t)ptr)) < sz );

      if(persianFlawIdx == i){
        assert(ptr[i] == persianFlawIdx);
      }else{
        uint64_t x = ptr[i];
        assert(~i == x);
      }
    }

    tryPerrInt(res, ufDestroyObject(o), "Err destroying obj", error1);

    i++;
  };

  error1:
  tryPerrInt(res, ufShutdown(ufI, false), "Err Shutdown", error);
  tryPerrInt(res, ufAwaitShutdown(ufI), "Err Await Shutdown", error);
  exit(0);

  error:
  exit(-1);
}
