#pragma once

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/epoll.h>

#include "sparseList.h"
#include "userfaultCore.h"
#include "oroboros.h"

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

  char*             buffer;
  uint64_t          bufferSize;

  uint64_t          nextID;
  sparseList_t      objects;

  uint16_t          concurrency;

  oroboros_t        chunkRecord;
  size_t            highWaterMarkBytes;
  size_t            lowWaterMarkBytes;
  size_t            usedMemory;
} ufInstance;

typedef struct {
  ufObjectConfig    config;
  ufInstance*       instance;
  union{
    char*           start;
    uint64_t        startI;
  };
  uint64_t          trueSize;
  sparseList_t      rangeMetadata;

  int               writebackMmapFd;
  uint8_t*          writebackMmapBase;
  uint32_t          writebackMmapBitmapLength;

  uint64_t          id;
} ufObject;

typedef uint32_t loadOrder;

typedef struct {
  loadOrder order;
  uint32_t  pageCt;
} ufRangeMeta;

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











