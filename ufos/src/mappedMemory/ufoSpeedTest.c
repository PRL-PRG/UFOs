
#include <stdio.h>
#include <printf.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <assert.h>
#include <time.h>
#include <unistd.h>


#include "ufos_c.h"

int testpopulate(void* userData, uint64_t startValueIdx, uint64_t endValueIdx, unsigned char* target){
  uint64_t* t = (uint64_t*) target;
  uint64_t* requestCt = (uint64_t*) userData;
  assert(startValueIdx >= 0);
  assert(endValueIdx <= *requestCt);
  for(int i = startValueIdx; i < endValueIdx; i++)
    t[i - startValueIdx] = i;

  return 0;
}

static inline uint64_t getns(void)
{
  struct timespec ts;
  int ret = clock_gettime(CLOCK_MONOTONIC, &ts);
  assert(ret == 0);
  return (((uint64_t)ts.tv_sec) * 1000000000ULL) + ts.tv_nsec;
}


int main(int argc, char **argv) {
  int res;

  // begin_ufo_log();

  UfoCore ufoCore = ufo_new_core("/tmp/", 50l*1024*1024, 100l*1024*1024);

  UfoPrototype prototype = ufo_new_prototype(64, sizeof(uint64_t), 1024*1024);

  uint64_t ct = 1024ull*1024*1024*2 + 1, sz = ct*8 ;
  // uint64_t ct = 1ull*1024*1024 + 1, sz = ct*8 ;
  UfoObj o = ufo_new(&ufoCore, &prototype, ct, &ct, testpopulate);

  uint64_t* h = (uint64_t*) ufo_header_ptr(&o);

  assert(h[0] == 0x00);
  h[0] = 0x01;
  assert(h[0] == 0x01);

  uint64_t* ptr = (uint64_t*) ufo_body_ptr(&o);

  uint64_t sum = 0;

  uint64_t start = getns();
  for(uint64_t x = 0; x < ct; x++){
    sum ^= ptr[x];
  }
  uint64_t dur = getns() - start;

  fprintf(stdout, "%lx\n%lu in %lu\n", sum, sz, dur);

  ufo_free(o);
  ufo_core_shutdown(ufoCore);

  exit(0);
}
