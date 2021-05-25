
#include <stdio.h>
#include <printf.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <assert.h>
#include <time.h>
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

static inline uint64_t getns(void){
  struct timespec ts;
  int ret = clock_gettime(CLOCK_MONOTONIC, &ts);
  assert(ret == 0);
  return (((uint64_t)ts.tv_sec) * 1000000000ULL) + ts.tv_nsec;
}


int main(int argc, char **argv){
  int res;

  uint64_t ct = 1024ull*1024*1024*2 + 1, sz = ct*8 ;

  ufInstance_t ufI = ufMakeInstance();
  ufSetMemoryLimits(ufI, 1000*1024*1024, 500*1024*1024);
  tryPerrInt(res, ufInit(ufI), "Err Init", error);

  // ufObjectConfig_t config = makeObjectConfig(uint64_t, 64, ct, (rand() & 0xffff) + 1);
  ufObjectConfig_t config = makeObjectConfig(uint64_t, 64, ct, 4*1024*1024);
  ufSetPopulateFunction(config, testpopulate);
  ufSetUserConfig(config, &ct);
  // ufSetReadOnly(config);

  ufObject_t o;
  tryPerrInt(res, ufCreateObject(ufI, config, &o), "Err init obj", error1);
  uint64_t* h = (uint64_t*) ufGetHeaderPointer(o);
  assert(h[0] == 0x00);
  h[0] = 0x01;
  assert(h[0] == 0x01);

  uint64_t* ptr = (uint64_t*) ufGetValuePointer(o);

  uint64_t sum = 0;

  uint64_t start = getns();
  for(uint64_t x = 0; x < ct; x++){
    sum ^= ptr[x];
  }
  uint64_t dur = getns() - start;

  fprintf(stdout, "%lx\n%lu in %lu\n", sum, sz, dur);

  tryPerrInt(res, ufDestroyObject(o), "Err destroying obj", error1);

  error1:
  tryPerrInt(res, ufShutdown(ufI, false), "Err Shutdown", error);
  tryPerrInt(res, ufAwaitShutdown(ufI), "Err Await Shutdown", error);
  exit(0);

  error:
  exit(-1);
}
