#define _GNU_SOURCE

#include <linux/userfaultfd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "userfaultCore.h"

#include "../unstdLib/math.h"
#include "../unstdLib/errors.h"

#include "userFaultCoreInternal.h"

/* System init and initial worker thread */

static size_t pageSize = 0;

static size_t get_page_size(){
  long ret = sysconf(_SC_PAGESIZE);
  if (ret == -1) {
    perror("sysconf/pagesize");
    exit(1);
  }
  assert(ret > 0);
  return ret;
}



static void* handler(void* arg){
  ufObjectConfig* conf = asObjectConfig(arg);

  return NULL;
}

static int initUfFileDescriptor(ufInstance* ins){
  int res;

  // open the userfault fd
  int uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
  if(uffd == -1){
    perror("syscall/userfaultfd");
    return -1;
  }

  ins->ufFd = uffd;

  // enable for api version and check features
  struct uffdio_api uffdio_api;
  uffdio_api.api = UFFD_API;
  uffdio_api.features = 0;
  // UFFD_FEATURE_EVENT_REMOVE; This is needed to bne notified of removals, though we will be the ones doing those...
  // | UFFD_FEATURE_EVENT_UNMAP; // Unmapping is when someone un-mmaps an area, this really shouldn't be happening by anyone but us!
  tryPerrInt(res, ioctl(uffd, UFFDIO_API, &uffdio_api), "ioctl/uffdio_api", ioctlErr);

  if (uffdio_api.api != UFFD_API) {
    perror("unsupported userfaultfd api\n");
    return -1;
  }

//  // allocate a memory region to be managed by userfaultfd
//  region = mmap(NULL, page_size * num_pages, PROT_READ|PROT_WRITE,
//      MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
//  if (region == MAP_FAILED) {
//    perror("mmap");
//    exit(1);
//  }
//
//  // register the pages in the region for missing callbacks
//  struct uffdio_register uffdio_register;
//  uffdio_register.range.start = (unsigned long)region;
//  uffdio_register.range.len = page_size * num_pages;
//  uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
//  if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1) {
//    perror("ioctl/uffdio_register");
//    exit(1);
//  }
//
//  if ((uffdio_register.ioctls & UFFD_API_RANGE_IOCTLS) !=
//      UFFD_API_RANGE_IOCTLS) {
//    fprintf(stderr, "unexpected userfaultfd ioctl set\n");
//    exit(1);
//  }

  ioctlErr:
  return res;
}

int ufInit(ufInstance_t instance){
  ufInstance* ins =  asUfInstance(instance);
  if(0 == pageSize)
    pageSize = get_page_size();

  if(ins->concurrency <= 0)
    ins->concurrency = 1;

  int res;
  tryPerrInt(res, initUfFileDescriptor(ins), "error initializing User-Fault file descriptor", errFd);

  ins->isInstanceShutdown = false;
  tryPerrInt(res, pthread_cond_init(&ins->instanceShutdownCond, NULL), "error initializing shutdown considiton", errCondition);
  tryPerrInt(res, pthread_mutex_init(&ins->instanceShutdownMutex, NULL), "error initializing shutdown considiton", errMutex);

  tryPerrInt(res, pipe(ins->msgPipe), "error creating msg pipe", errPipe);

// On Linux the pipe size is always at least one page
//  tryPerrInt(res, fcntl(ins->msgPipe[1], F_SETPIPE_SZ, pageSize), "error manipulating msg pipe", errPipe1);
//  errPipe1:
//  close(ins->msgPipe[1]);
//  close(ins->msgPipe[0]);

  //Everything in place? Start the handler thread
  tryPerrInt(res, pthread_create(&ins->userfaultThread, NULL, handler, ins), "error starting thread", errThread);

  return 0; //done and all good

  errThread:
  errPipe:

  pthread_mutex_consistent(&ins->instanceShutdownMutex);
  errMutex:

  pthread_cond_destroy(&ins->instanceShutdownCond);
  errCondition:

  errFd:
  return res;
}


/* Objects and Object config */

ufObjectConfig_t makeObjectConfig0(uint32_t headerSize, uint64_t ct, uint32_t stride, int32_t minLoadCt){
  if(stride < 1)
    return NULL;

  ufObjectConfig* conf = calloc(1, sizeof(ufObjectConfig));

  conf->stride = stride;
  conf->elementCt = ct;
  // the header starts at offset headerSzWithPadding - headerSize, the body at offset headerSzWithPadding
  conf->headerSzWithPadding = ceilDiv(headerSize, pageSize) * pageSize;

  if(minLoadCt < 1)
    minLoadCt = 1;

  //TODO: unit test this algorithm
  //the GCD of two numbers tells you how many of the larger you need for the smaller to divide evenly
  const size_t minPages = gcd(pageSize, conf->stride);
  uint32_t pMinObjects;
  if(pageSize > conf->stride){ // Common case, objects are (much) smaller than the page size, so flip minPages over into the number of objects
    assert(0 == (pageSize * minPages) % stride);
    pMinObjects = (pageSize * minPages) / stride;
  }else{
    pMinObjects = minPages; // uncommon, object is larger than a page and minPages is the number of them we need to get an even number of pages
  }

  //at least one min-objects worth, but as many as are needed to meet the requested minimum to load at once
  conf->objectsAtOnce = pMinObjects * ceilDiv(minLoadCt, pMinObjects);

  return (ufObjectConfig_t) conf;
}

static void sendMsg(ufInstance* i, ufAsyncMsg* msg){
  write(i->msgPipe[1], &msg, sizeof(ufAsyncMsg));
}

void ufAwaitShutdown(ufInstance_t instance){
  //TODO: wait, call free
}

void ufShutdown(ufInstance_t instance, bool free){
  //TODO: send the message, optionally call await
}

int ufCreateObject(ufInstance_t instance, ufObjectConfig_t objectConfig, ufObject_t* object_p){
  ufInstance*        i = asUfInstance(instance);
  ufObjectConfig* conf = asObjectConfig(objectConfig);

  int res = -1, returnVal = -1;
  ufObject* o;
  tryPerrNull(o, calloc(1, sizeof(ufObject)), "error allocating object", errAlloc);
  memcpy(&o->config, conf, sizeof(ufObjectConfig)); // objects contain a copy of the config so the original config can be reused
  o->instance = i;

  // Init the allocation vars and message
  sem_t completionLock;
  tryPerrInt(res, sem_init(&completionLock, 0, 0), "error initializing the completion lock", semErr);
  ufAsyncMsg msg = (ufAsyncMsg) {.msgType = ufAllocateMsg, .toAllocate = o, .completionLock_p = &completionLock, .return_p = &returnVal};

  // Ask the worker thread to allocate our object
  sendMsg(i, &msg);
  // And wait for it to do so
  tryPerrInt(res, sem_wait(&completionLock), "error waiting for object creation", awaitErr);

  res = returnVal;
  if(res) goto initErr;

  //If we got success then the worker surely allocated our object
  assert(NULL != o->start);

  // Done and all went well
  sem_destroy(&completionLock);

  *object_p = o;
  return 0;

  initErr:
  awaitErr:

  sem_destroy(&completionLock);
  semErr:

  assert(NULL != o);
  free(o);
  errAlloc:

  return res;
}

int ufDestroyObject(ufObject_t object_p){
  ufObject*   o = asUfo(object_p);
  ufInstance* i = o->instance;

  int res = -1, returnVal = -1;
  sem_t completionLock;
  tryPerrInt(res, sem_init(&completionLock, 0, 0), "error initializing the completion lock", semErr);

  // Init the free request
  ufAsyncMsg msg = (ufAsyncMsg) {.msgType = ufFreeMsg, .toFree = o, .return_p = &returnVal, .completionLock_p = &completionLock};
  // send the request
  sendMsg(i, &msg);

  tryPerrInt(res, sem_wait(&completionLock), "error waiting for object destruction", awaitErr);
  if(returnVal) goto freeErr; // thats bad… don't free the object

  // cleanup
  free(o);
  sem_destroy(&completionLock);

  return 0;

  freeErr:
  awaitErr:
  sem_destroy(&completionLock);

  semErr:
  return res;
}






