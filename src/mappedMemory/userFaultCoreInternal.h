#pragma once

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/epoll.h>

#include "sparseList.h"
#include "userfaultCore.h"

/*
 * This is an internals file. This is not meant for use by developers\
 */
#ifndef __UFOs_CoreDev__
  #error It looks like you're not a UFO core dev. Don't use this file. You have been warned.
#endif

typedef struct {
  //Calculated
  uint32_t          stride;
  uint32_t          headerSzWithPadding;

  //Provided
  void*             userConfig;
  ufPopulateRange   populateFunction;
  uint64_t          elementCt;
  uint32_t          headerSize;

  //Defaulted
  uint32_t          objectsAtOnce;
} ufObjectConfig;

typedef struct {
  int               ufFd;
  //From the docs: msgPipe[0] refers to the read end of the pipe. msgPipe[1] refers to the write end of the pipe.
  int               msgPipe[2];
  int               epollFd;
  pthread_t         userfaultThread;

  sparseList_t      instances;

  uint16_t          concurrency;
} ufInstance;

typedef struct {
  ufObjectConfig    config;
  ufInstance*       instance;
  char*             start;
} ufObject;

#define _as(t, o) ((t*)o)
#define asObjectConfig(o) _as(ufObjectConfig, o)
#define asUfo(o) _as(ufObject, o)
#define asUfInstance(o) _as(ufInstance, o)

/*
 * Internal message passing, using the msg pipe
 */

typedef enum {
  ufShutdownMsg,
  ufAllocateMsg,
  ufFreeMsg
} ufMessageType;

typedef struct {
  ufMessageType msgType;
  union{
    ufObject* toAllocate;
    ufObject* toFree;
    bool      selfFree;
  };
  sem_t*    completionLock_p;
  int*      return_p;
} ufAsyncMsg;











