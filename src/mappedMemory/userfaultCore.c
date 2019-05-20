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
#include <sys/epoll.h>
#include <stdlib.h>
#include <errno.h>1

#include "userfaultCore.h"

#include "../unstdLib/math.h"
#include "../unstdLib/errors.h"

#include "userFaultCoreInternal.h"

static int freeInstance(ufInstance* i);

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

static int readMsg(ufInstance* i, ufAsyncMsg* msg){
  int toRead = sizeof(ufAsyncMsg);
  char* m = (char*) msg;
  do{
    int readBytes = read(i->msgPipe[0], m, toRead);
    if(readBytes < 0){
      switch(errno){
        case EAGAIN:
        case EWOULDBLOCK:
          return -1;
        default:
          return -2;
      }
    }

    toRead -= readBytes;
    if(0 == toRead)
      return 0;
    m += readBytes;
  }while(true);
}

static void handlerShutdown(ufInstance* i, bool selfFree){
  // don't need events anymore
  close(i->epollFd);

  // This should have been done already and we expect this to return an error
  // nobody should write to us anymore
  close(i->msgPipe[1]);

  //Nuke all the objects
  void nullObjectInstance(entry* e){
    ufObject* ufo = asUfo(e->valuePtr);
    if(NULL != ufo)
      munmap(ufo->start, ufo->config.headerSzWithPadding + (ufo->config.elementCt * ufo->config.stride));
    else
      assert(false);
  }
  listWalk(i->instances, nullObjectInstance);

  ufAsyncMsg msg;
  while(readMsg(i, &msg)){
    switch(msg.msgType){
      case ufAllocateMsg:
        *msg.return_p = ufShuttingDown;
        sem_post(msg.completionLock_p);
        break;

      case ufFreeMsg:
        *msg.return_p = 0;
        sem_post(msg.completionLock_p);
        break;

      case ufShutdownMsg:
        continue; // we know
      default:
        assert(false); // Huh?
        break;
    }
  }
  close(i->msgPipe[0]);

  close(i->ufFd); //Do this last. If something is still (improperly) active this will likely crash the whole program
  if(selfFree)
    free(i);
}

static void* handler(void* arg){
  ufInstance* i = asUfInstance(arg);
  bool selfFree = true;

  do{

  }while(true);

  handlerShutdown(i, selfFree);
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
  int flags = fcntl(ins->ufFd, F_GETFL, 0);
  fcntl(ins->ufFd, F_SETFL, flags | O_NONBLOCK); // set nonblocking

  tryPerrInt(res, pipe(ins->msgPipe), "error creating msg pipe", errPipe);
  flags = fcntl(ins->msgPipe[0], F_GETFL, 0);
  fcntl(ins->msgPipe[0], F_SETFL, flags | O_NONBLOCK); // set the read end to nonblocking

  tryPerrNegOne(ins->epollFd, epoll_create1(0), "Err init epoll", errEpoll);

  /* register events with epoll for the userfault file descriptor and our message pipe */
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = ins->ufFd;
  tryPerrInt(res, epoll_ctl(ins->epollFd, EPOLL_CTL_ADD, 0, &event), "error registering uffd with epoll", errRegUf);
  event.data.fd = ins->msgPipe[0];
  tryPerrInt(res, epoll_ctl(ins->epollFd, EPOLL_CTL_ADD, 0, &event), "error registering pipe read end with epoll", errRegPipe);

  //Everything in place? Start the handler thread
  tryPerrInt(res, pthread_create(&ins->userfaultThread, NULL, handler, ins), "error starting thread", errThread);

  return 0; //done and all good

  errThread:
  errRegPipe:
  errRegUf:

  close(ins->epollFd);
  errEpoll:

  close(ins->msgPipe[0]);
  close(ins->msgPipe[1]);
  errPipe:

  close(ins->ufFd);
  errFd:
  return -1;
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

static int sendMsg(ufInstance* i, ufAsyncMsg* msg){
  return write(i->msgPipe[1], &msg, sizeof(ufAsyncMsg));
}

int ufAwaitShutdown(ufInstance_t instance){
  ufInstance* i = asUfInstance(instance);
  int res;
  tryPerrInt(res, pthread_join(i->userfaultThread, NULL), "error joining thread", joinErr);

  free(i);
  return 0;

//  lockErr:
  joinErr:
  return -1;
}

int ufShutdown(ufInstance_t instance, bool free){
  ufInstance* i = asUfInstance(instance);

  int res;

  //Self free is the inverse of our argument. Our argument asks to wait for freeing, the msg argument is telling the instance if it should free itself, no waiting
  ufAsyncMsg msg = (ufAsyncMsg){.msgType = ufShutdownMsg, .selfFree = !free };
  sendMsg(i, &msg); // If this fails it was shutting down / already down anyway
  close(i->msgPipe[1]); // Close the write side promptly. May race with another writer, but the instance will clear those
  if(!free)
    return 0;
  return ufAwaitShutdown(instance);
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
  tryPerrInt(res, sendMsg(i, &msg), "error sending message, instance shutting down?", sendErr);
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
  sendErr:

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

  if(NULL != i){
    int res = -1, returnVal = -1, sendErr = 0;
    sem_t completionLock;
    tryPerrInt(res, sem_init(&completionLock, 0, 0), "error initializing the completion lock", semErr);

    // Init the free request
    ufAsyncMsg msg = (ufAsyncMsg) {.msgType = ufFreeMsg, .toFree = o, .return_p = &returnVal, .completionLock_p = &completionLock};
    // send the request
    tryPerrInt(sendErr, sendMsg(i, &msg), "instance shutting down", manualFree);

    tryPerrInt(res, sem_wait(&completionLock), "error waiting for object destruction", awaitErr);
    if(returnVal) goto freeErr; // thats bad… don't free the object

    // cleanup
    free(o);
    sem_destroy(&completionLock);

    return 0;

    freeErr:
    awaitErr:
    sem_destroy(&completionLock);

    manualFree:
    if(sendErr){
      munmap(o->start, o->config.headerSzWithPadding + (o->config.elementCt * o->config.stride));
      free(o);
    }

    semErr:
    return res;


  }else{
    // instance shutting down, it frees everything
    return 0;
  }
}






