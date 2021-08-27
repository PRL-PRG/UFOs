
#include <stdio.h>
#include <printf.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <assert.h>

#include <unistd.h>

#include "ufos_c.h"
#include "../unstdLib/errors.h"

int testpopulate(void* userData, uint64_t startValueIdx, uint64_t endValueIdx, unsigned char* target){
  uint64_t* t = (uint64_t*) target;
  uint64_t* requestCt = (uint64_t*) userData;
  UNUSED(requestCt);
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
  int i = 0;
  int n = (argc > 1) ? atoi(argv[1]) : -1;

  srand(31);

  UfoCore ufoCore = ufo_new_core("/tmp/", 50*1024*1024, 200*1024*1024);
  if(ufo_core_is_error(&ufoCore))
    goto error;

  UfoPrototype ufoPrototype = ufo_new_prototype(64, sizeof(uint64_t), (rand() & 0xffff) + 1, false);
  // ufo_free_prototype(ufoPrototype);
  if(ufo_prototype_is_error(&ufoPrototype))
    goto proto_error; 

//  uint64_t* bitmap = calloc(1024*1024*1024, 8);
  while(n < 0 || i < n) {
    uint64_t ct = 1024ull*1024*((rand() & 0xfffull) + 1), sz = ct*8;
    UNUSED(sz);

    UfoObj o = ufo_new_with_prototype(&ufoCore, &ufoPrototype, ct, &ct, testpopulate);
    if(ufo_is_error(&o))
      goto bad_ufo;

    uint64_t* h = (uint64_t*) ufo_header_ptr(&o);
    assert(h[0] == 0x00);
    h[0] = 0x01;
    assert(h[0] == 0x01);


    *((uint32_t*)ufo_header_ptr(&o)) = 0x5c5c;
    uint64_t* ptr = (uint64_t*) ufo_body_ptr(&o);

    printf("%lu\n", (uint64_t)ct);

    uint64_t persianFlawIdx;
    void makeFlaw(){
      persianFlawIdx = rand() % ct;
      ptr[persianFlawIdx] = persianFlawIdx;
    }
    makeFlaw();

    while((random() & 0xfff) != 0){
      if((random() & 0xfff) < 7){
        ufo_reset(&o);
        makeFlaw();
        assert(0x5c5c == *((uint32_t*)ufo_header_ptr(&o)));
      }

      uint64_t i = rand() % ct;
      assert(i < ct);
      assert(  (((uint64_t) &ptr[i]) - ((uint64_t)ptr)) < sz );


      if(persianFlawIdx == i){
        assert(ptr[i] == persianFlawIdx);
      }else{
        uint64_t x = ptr[i];
        UNUSED(x);
        assert(~i == x);
      }
    }

    ufo_free(o);

    i++;
  };

  bad_ufo:
  // ufo_core_shutdown(ufoCore);
  exit(-3);

  proto_error:
  // ufo_core_shutdown(ufoCore);
  exit(-2);

  error:
  exit(-1);
}
