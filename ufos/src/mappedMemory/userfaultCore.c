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
#include <stdarg.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <errno.h>

#include <openssl/sha.h>

#include "userfaultCore.h"

#include "../unstdLib/math.h"
#include "../unstdLib/errors.h"

#include "userFaultCoreInternal.h"
#include "oroboros.h"

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

//static uint32_t loadChunkSize(ufObject* o){
//  const uint32_t atOnce = o->config.objectsAtOnce;
//  const uint32_t stride = o->config.stride;
//
//  assert(atOnce > 0);
//  assert(stride > 0);
//  assert(__builtin_clz(atOnce) + __builtin_clz(stride) >= 32);
//
//  return o->config.objectsAtOnce * o->config.stride;
//}

int check_totals (ufInstance* i) {
  uint64_t total = 0;
  void sum(size_t i, oroboros_item_t *item, void* user_data) { total += item->size; }
  oroboros_for_each(i->chunkRecord, sum, NULL);
  return i->usedMemory == total;
}

static int readMsgSlowPath(const int fd, const int sz, char* m, int readBytes){
  assert(readBytes != sz);
  int toRead = sz;
  do{
    if(readBytes == 0)
      return 2;
    if(readBytes < 0){
      if(errno == EAGAIN || errno == EWOULDBLOCK)
        return 1;
      return -1;
    }

    // Not an error, merely a partial read
    // With pipes and userfault FDs this should be pretty darn rare if not nonexistant
    toRead -= readBytes;
    assert(toRead > 0);
    if(0 == toRead)
      return 0;
    m += readBytes;

    readBytes = read(fd, m, toRead);
  }while(true);
}

static inline int readMsg(const int fd, const int sz, char* msg){
  int readBytes = read(fd, msg, sz);
  // Common case, we read the whole thing in one go and return
  // We expect readMsg to mostly be called after epoll told us there are things waiting
  if(__builtin_expect(sz == readBytes, 1))
    return 0;
  // Call out to the slow path instead of inlining it here
  // The intent is to make this function small enough to be a good inlining candidate
  return readMsgSlowPath(fd, sz, msg, readBytes);
}

static void handlerShutdown(ufInstance* i, bool selfFree){
  // don't need events anymore
  close(i->epollFd);

  // This should have been done already and we expect this to return an error
  // nobody should write to us anymore
  close(i->msgPipe[1]);

  //Nuke all the objects
  void nullObject(entry* e){
    ufObject* ufo = asUfo(e->valuePtr);
    if(NULL != ufo)
      munmap(ufo->start, ufo->trueSize);
    else
      assert(false);
  }
  listWalk(i->objects, nullObject);

  ufAsyncMsg msg;
  while(!readMsg(i->msgPipe[0], sizeof(msg), (char*)&msg)){
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
        assert(false); // Unexpected and bad!
        break;
    }
  }
  close(i->msgPipe[0]);

  free(i->buffer);

  close(i->ufFd); //Do this last. If something is still (improperly) active this will likely crash the whole program
  if(selfFree)
    free(i);
}

static int ufCopy(ufInstance* i, struct uffdio_copy* copy){
  int res;
  do{
    assert(errno == 0);
    res = ioctl(i->ufFd, UFFDIO_COPY, copy);
    if(res == 0){
      assert(copy->copy == copy->len);
      return 0;
    }else{
      assert(-1 == res);
      if(errno == EAGAIN){
        const int64_t copied = copy->copy;
        assert(copied > 0);
        assert(copied < copy->len);
        copy->len -= copied;
        copy->src += copied;
        copy->dst += copied;
        copy->copy = 0;
        errno = 0;
      }else{
        return -1;
      }
    }
  }while(1);
}

static int reclaimMemory(ufInstance* i, oroboros_item_t* chunkMetadata){
  if(0 == chunkMetadata->size)
    return 0; // this chunk was already reclaimed
  uint8_t* sha = (uint8_t*)alloca(SHA256_DIGEST_LENGTH);
  SHA256(chunkMetadata->address, chunkMetadata->size, sha);

  // Let the compiler make this fast
  bool isEqual = true;
  for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    isEqual = isEqual && sha[i] == chunkMetadata->sha[i];

  if(!isEqual){
    // When the memory changed write it out to the writeback file
    ufObject *ufo = asUfo(chunkMetadata->ufo);
    uint64_t offset = ((uint64_t)chunkMetadata->address) - (uint64_t) ufGetValuePointer(ufo);

    uint32_t bitIndex = offset / (ufo->config.stride * ufo->config.objectsAtOnce);
    ufo->writebackMmapBase[bitIndex >> 3] |= 1 << (bitIndex & 7);

    memcpy(ufo->writebackMmapBase + ufo->writebackMmapBitmapLength, chunkMetadata->address, chunkMetadata->size);
  }

  madvise(chunkMetadata->address, chunkMetadata->size, MADV_DONTNEED); // also possible: MADV_FREE

  i->usedMemory -= chunkMetadata->size;
  assert(check_totals(i));

  return 0;
}

static int ensureFreeMemory(ufInstance* i, uint32_t ensureFreeAmount){
  int res;
  if (i->usedMemory + ensureFreeAmount > i->highWaterMarkBytes) {
    while (i->usedMemory + ensureFreeAmount > i->lowWaterMarkBytes) {
      oroboros_item_t chunkMetadata;

      tryPerrInt(res, oroboros_pop(i->chunkRecord, &chunkMetadata),
          "Reclaiming all the elements from ring buffer did not free enough memory "
          "to allocate incoming chunk without breaking the high water mark memory usage threshold",
          error);
      tryPerrInt(res, reclaimMemory(i, &chunkMetadata), "error reclaiming memory", error);
    }
  }

  return 0;

  error:
  return -1;
}

static int readHandleUfEvent(ufInstance* i){
  struct uffd_msg msg;

  int res;
  tryPerrNegOne(res, readMsg(i->ufFd, sizeof(msg), (char*)&msg), "error reading from userfault", error);
  switch(res){
    case 1:
      // Huh… read nothing? Not really an error, though we should only get here when there is something to read
      perror("nothing to read");
      return 0;
    case 2:
      return 0; // File handle closed? We shouldn't see this
  }

  if(!(msg.event & UFFD_EVENT_PAGEFAULT)) {
    perror("Unknown userfault event");
    assert(false);
    goto error;
  }

  const uint64_t faultAtAddr = msg.arg.pagefault.address;
  assert(0 == (faultAtAddr % pageSize));

  entry e;
  tryPerrInt(res, listFind(i->objects, &e, (void*)faultAtAddr), "no known object for fault", error);
  ufObject* ufo = asUfo(e.valuePtr);

  const uint64_t bodyStart = (uint64_t) ufo->start + ufo->config.headerSzWithPadding;
  const uint64_t faultRelBody = faultAtAddr - bodyStart; // Translate the fault address to an address relative to the body of the object
  const uint64_t bytesAtOnce = ufo->config.objectsAtOnce * ufo->config.stride;

  const uint64_t faultAtLoadBoundaryRelBody  = (faultRelBody / bytesAtOnce) * bytesAtOnce; // Round down to the next loading boundary
  assert(0 == faultAtLoadBoundaryRelBody % bytesAtOnce);
  assert(0 == faultAtLoadBoundaryRelBody % ufo->config.stride);
  const uint64_t faultAtLoadBoundaryAbsolute = faultAtLoadBoundaryRelBody + bodyStart;

  const uint64_t idx = faultAtLoadBoundaryRelBody / ufo->config.stride;

  uint64_t actualFillCt = ufo->config.objectsAtOnce;
  if(idx + actualFillCt > ufo->config.elementCt)
    actualFillCt = ufo->config.elementCt - idx;

  const uint64_t fillSizeBytes = ceilDiv(actualFillCt * ufo->config.stride, pageSize) * pageSize;
  if (fillSizeBytes > i->highWaterMarkBytes) {
    // This is guarded against at object creation but it remains here in case there is a change in the math and the check isn't properly performed
    perror("Cannot load a chunk whose size is greater than the total number of memory dedicated "
           "to storing chunk data (high water mark).");
    goto error;
  }

  tryPerrInt(res, ensureFreeMemory(i, fillSizeBytes), "error ensuring memory capacity", error);

  if(__builtin_expect(i->bufferSize < fillSizeBytes, 0)){
    tryPerrNull(i->buffer, realloc(i->buffer, fillSizeBytes), "cannot realloc buffer", error);
    i->bufferSize = fillSizeBytes;
  }

  int callout(ufPopulateCalloutMsg* msg){
    switch(msg->cmd){
      case ufResolveRangeCmd:
        return 0; // Not yet implemented, but this is advisory only so no error
      case ufExpandRange:
        return ufWarnNoChange; // Not yet implemented, but callers have to deal with this anyway, even spuriously
      default:
        return ufBadArgs;
    }
    __builtin_unreachable();
  }

  uint32_t chunkIndex = faultAtLoadBoundaryRelBody / (ufo->config.stride * ufo->config.objectsAtOnce);
  uint8_t* copySource;
  assert((chunkIndex >> 3) <= ufo->writebackMmapBitmapLength);
  if(ufo->writebackMmapBase[chunkIndex >> 3] & (1 << (chunkIndex & 7)) ){
    // Pull in from the writeback
    copySource = ufo->writebackMmapBase + ufo->writebackMmapBitmapLength;
  }else{
    // callout to the user supplied function
    //uint64_t startValueIdx, uint64_t endValueIdx, ufPopulateCallout callout, ufUserData userData, char* target
    tryPerrInt(res,
        ufo->config.populateFunction(idx, idx + actualFillCt, callout, ufo->config.userConfig, i->buffer),
        "populate error", error);
    copySource = (uint8_t*)i->buffer;
  }

  oroboros_item_t chunkMetadata = {
          .size    = fillSizeBytes,
          .address = (void *) faultAtLoadBoundaryAbsolute,
          .ufo     = ufo
  };
  //TODO CMYK 2020.05.26 : Blake 2 or 3 would be faster, but this was handy
  SHA256(copySource, fillSizeBytes, chunkMetadata.sha);


  assert(check_totals(i));

  i->usedMemory += fillSizeBytes;
  tryPerrInt(res, oroboros_push(i->chunkRecord, chunkMetadata, true),
    "Could not push metadata onto the ring buffer. The ring buffer cannot resize.", error);  // FIXME consider adding an upper limit to size

  assert(check_totals(i));

  struct uffdio_copy copy = (struct uffdio_copy){
      .src = (uint64_t) copySource,
      .dst = faultAtLoadBoundaryAbsolute,
      .len = fillSizeBytes,
      .mode = 0};
  tryPerrNegOne(res, ufCopy(i, &copy), "error copying", error);

  return 0;

  error:
  return -1;
}

static int allocateUfo(ufInstance* i, ufAsyncMsg* msg){
  assert(ufAllocateMsg == msg->msgType);
  int res;
  ufObject* ufo = asUfo(msg->toAllocate);
  const uint64_t size = ufo->trueSize;
  assert(size > 0);

  const uint64_t fillSizeBytes = ceilDiv(ufo->config.objectsAtOnce * ufo->config.stride, pageSize) * pageSize;
  if (fillSizeBytes > i->highWaterMarkBytes) {
    perror("Cannot allocate an object whose chunk size is greater than the total number of memory dedicated "
           "to storing chunk data (high water mark).");
    goto error;
  }

  // allocate a memory region to be managed by userfaultfd
  /* With writeback files we can allow writes! */
  tryPerr(ufo->start, ufo->start == (void*)-1, mmap(NULL, ufo->trueSize, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0), "error allocating ufo memory", callerErr);

  /* With writeback files the whole mapping is already writeable */
  //  tryPerrInt(res, mprotect(ufo->start, ufo->config.headerSzWithPadding, PROT_READ|PROT_WRITE), "error changing header write protections", mprotectErr); // make the header writeable

  // register with the kernel
  struct uffdio_register ufM;
  ufM = (struct uffdio_register) {.range = {.start = ufo->startI, .len = size}, .mode = UFFDIO_REGISTER_MODE_MISSING, .ioctls = 0};
  tryPerrInt(res, ioctl(i->ufFd, UFFDIO_REGISTER, &ufM), "error registering ufo with UF", callerErr);

  if((ufM.ioctls & UFFD_API_RANGE_IOCTLS) != UFFD_API_RANGE_IOCTLS) {
    perror("unexpected userfaultfd ioctl set\n");
    goto error;
  }

  tryPerrInt(res, listAdd(i->objects, ufo->start, ufo->trueSize, ufo), "unknown UFO", callerErr);

  // zero the header area so it doesn't fault
  if(ufo->config.headerSzWithPadding > 0){
    struct uffdio_zeropage ufZ = (struct uffdio_zeropage) {.mode = 0, .range = {.start = ufo->startI, .len = ufo->config.headerSzWithPadding}};
    tryPerrInt(res, ioctl(i->ufFd, UFFDIO_ZEROPAGE, &ufZ), "error zeroing ufo header", error);
  }

  *msg->return_p = 0;

  tryPerrInt(res, sem_post(msg->completionLock_p), "error unlocking waiter", error);

  return 0;

//  mprotectErr:
//  munmap(ufo->start, size);

  callerErr:
  *msg->return_p = -2; // error
  tryPerrInt(res, sem_post(msg->completionLock_p), "error unlocking waiter for free on caller Err", error);
  return 0;

  error:
  return -1;
}

static int freeUfo(ufInstance* i, ufAsyncMsg* msg){
  assert(ufFreeMsg == msg->msgType);
  int res;
  ufObject* ufo = asUfo(msg->toFree);
  struct uffdio_register ufM;

  const uint64_t size = ufo->trueSize;
  ufM = (struct uffdio_register) {.range = {.start = ufo->startI, .len = size}};
  tryPerrInt(res, ioctl(i->ufFd, UFFDIO_UNREGISTER, &ufM), "error unregistering ufo with UF", callerErr);

  tryPerrInt(res, listRemove(i->objects, ufo->start), "unknown UFO", callerErr);

  munmap(ufo->start, size);
  *msg->return_p = 0; // Success
  tryPerrInt(res, sem_post(msg->completionLock_p), "error unlocking waiter for free", error);

  void mark (size_t index, oroboros_item_t *item, void *data) {
    if (item->ufo == ufo) {
      //item->garbage_collected = true;

      i->usedMemory -= item->size;
      // clear the structure entirely, also marks it as claimed by setting size to 0
      memset(item, 0, sizeof(oroboros_item_t));

      assert(check_totals(i));
      // We don't actually reclaim here, because it's done for the whole object.
    }
  }
  oroboros_for_each(i->chunkRecord, mark, NULL);

  munmap(ufo->writebackMmapBase, size + ufo->writebackMmapBitmapLength);
  close(ufo->writebackMmapFd); // temnp file is destroyed when the last handle to it is closed

  return 0;

  callerErr:
  *msg->return_p = -2; // error
  tryPerrInt(res, sem_post(msg->completionLock_p), "error unlocking waiter for free on caller Err", error);
  return 0;

  error:
  return -1;
}

static int readHandleMsg(ufInstance* i, bool* selfFreeP){
  ufAsyncMsg msg;
  int res;
  tryPerrNegOne(res, readMsg(i->msgPipe[0], sizeof(ufAsyncMsg), (char*)&msg), "error reading from pipe", error);
  switch(res){
    case 1:
      // Huh… read nothing? Not really an error, though we should only get here when there is something to read
      perror("nothing to read");
      return 0;
    case 2:
      goto shutdown;
  }

  assert(msg.msgType >= ufShutdownMsg && msg.msgType <= ufFreeMsg);
  // Soft errors are handled in the calls, we only worry about hard errors (ones that bring down the system)
  switch(msg.msgType){
    case ufAllocateMsg:
      tryPerrInt(res, allocateUfo(i, &msg), "error allocating ufo", error);
      break;

    case ufFreeMsg:
      tryPerrInt(res, freeUfo(i, &msg), "error freeing ufo", error);
      break;

    case ufShutdownMsg:
      *selfFreeP = msg.selfFree;
      shutdown:
      return 1;
  }
  return 0; // Success

  error:
  return -1;
}

#define MAX_EVENTS 2 // We only register 2 handles. 2 is literally the max for us

static int ePollLoop(ufInstance* i, struct epoll_event* events){
  int interruptCt = 0, nRdy;
  do{
    nRdy = epoll_wait(i->epollFd, events, MAX_EVENTS, 200);
    if(nRdy >= 0)
      return nRdy;
    if(errno != EINTR){
      perror("epoll error");
      return -1;
    }

    errno = 0;
    interruptCt++;
    if(interruptCt >= 3){
      perror("Interrupted 3 times in a row in epoll");
      return -1;
    }
  }while(true);
}

static void* handler(void* arg){
  ufInstance* i = asUfInstance(arg);
  bool selfFree = true;

  struct epoll_event events[MAX_EVENTS];
  int nRdy, res;

  do{
    tryPerrNegOne(nRdy, ePollLoop(i, events), "Error while polling for events", error);

    for(int x = 0; x < nRdy; x++){
      if(events[x].data.fd == i->ufFd){
        tryPerrInt(res, readHandleUfEvent(i), "error handling an event, shutting down", error);
      }else{
        assert(events[x].data.fd == i->msgPipe[0]);
        tryPerr(res, res < 0, readHandleMsg(i, &selfFree), "error handling an event, shutting down", error);
        switch(res){
          case 0:  continue; // No worries
          case 1:  goto shutdown; // got the shutdown signal
          default: goto error;
        }
      }
    }
  }while(true);
  shutdown:
  handlerShutdown(i, selfFree);
  return NULL;

  error:
  handlerShutdown(i, true); // On an error always self-free
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

  ioctlErr:
  return res;
}

int ufSetMemoryLimits(ufInstance_t instance, size_t highWaterMarkBytes, size_t lowWaterMarkBytes) {
  ufInstance* ins =  asUfInstance(instance);

  if (0 != ins->userfaultThread) {
    perror("Memory limits can only be set before init");
    return -1;
  }

  if (!(highWaterMarkBytes > lowWaterMarkBytes)) {
    perror("High water mark must be greater than low water mark.");
    return -2;
  }

  ins->highWaterMarkBytes = highWaterMarkBytes;
  ins->lowWaterMarkBytes = lowWaterMarkBytes;
  return 0;
}

int ufInit(ufInstance_t instance){
  ufInstance* ins =  asUfInstance(instance);
  if(0 == pageSize)
    pageSize = get_page_size();

  if(ins->concurrency <= 0)
    ins->concurrency = 1;

  tryPerrNull(ins->buffer, malloc(pageSize * 20), "malloc error", mallocErr);
  ins->bufferSize = pageSize * 20;

  int res;
  /* init the userfault FD and the pipe FDs, set uf and the read end of the pipe to be nonblocking */
  tryPerrInt(res, initUfFileDescriptor(ins), "error initializing User-Fault file descriptor", errFd);
  int flags = fcntl(ins->ufFd, F_GETFL, 0);
  tryPerrInt(res, fcntl(ins->ufFd, F_SETFL, flags | O_NONBLOCK), "error setting userfault to nonblocking", errFd);

  tryPerrInt(res, pipe(ins->msgPipe), "error creating msg pipe", errPipe);
  flags = fcntl(ins->msgPipe[0], F_GETFL, 0);
  tryPerrInt(res, fcntl(ins->msgPipe[0], F_SETFL, flags | O_NONBLOCK), "error setting userfault to nonblocking", errPipe);

  tryPerrNegOne(ins->epollFd, epoll_create1(0), "Err init epoll", errEpoll);

  /* register events with epoll for the userfault file descriptor and our message pipe */
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = ins->ufFd;
  tryPerrInt(res, epoll_ctl(ins->epollFd, EPOLL_CTL_ADD, ins->ufFd, &event), "error registering uffd with epoll", errRegUf);
  event.data.fd = ins->msgPipe[0];
  tryPerrInt(res, epoll_ctl(ins->epollFd, EPOLL_CTL_ADD, ins->msgPipe[0], &event), "error registering pipe read end with epoll", errRegPipe);

  //Everything in place? Start the handler thread
  tryPerrInt(res, pthread_create(&ins->userfaultThread, NULL, handler, ins), "error starting thread", errThread);

  // Initially, we're not using any memory, because no chunks have been populated
  ins->usedMemory = 0;

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

  mallocErr:
  return -1;
}

ufInstance_t ufMakeInstance(){
  ufInstance* i = calloc(1, sizeof(ufInstance));
  if (i == NULL) {
      perror("Instance creation");
      return NULL;
  }
  i->objects = newList();
  i->chunkRecord = oroboros_init(1024);

  i->highWaterMarkBytes = 2l * 1024l * 1024l * 1024l; // 2G
  i->lowWaterMarkBytes  = 1l * 1024l * 1024l * 1024l; // 1G

  return i;
}

/* Objects and Object config */
ufObjectConfig_t makeObjectConfig0(uint32_t headerSize, uint64_t ct, uint32_t stride, int32_t minLoadCt){
  if(stride < 1)
    return NULL;

  ufObjectConfig* conf = calloc(1, sizeof(ufObjectConfig));

  conf->stride = stride;
  conf->elementCt = ct;
  conf->headerSize = headerSize;

  // If pageSize is zero, perhaps the framework was never initialized?
  assert(pageSize > 0);

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
  assert(msg->msgType >= ufShutdownMsg && msg->msgType <= ufFreeMsg);
  int res = write(i->msgPipe[1], msg, sizeof(ufAsyncMsg));
  if(res != sizeof(ufAsyncMsg)){
    perror("write error");
    assert(false);
  }
  return 0;
}

int ufAwaitShutdown(ufInstance_t instance){
  ufInstance* i = asUfInstance(instance);
  int res;
  tryPerrInt(res, pthread_join(i->userfaultThread, NULL), "error joining thread", joinErr);

  // once we join the thread we can finally free the instance
  free(i);
  return 0;

//  lockErr:
  joinErr:
  return -1;
}

int ufShutdown(ufInstance_t instance, bool free){
  ufInstance* i = asUfInstance(instance);

  //Self free is the inverse of our argument. Our argument asks to wait for freeing, the msg argument is telling the instance if it should free itself, no waiting
  ufAsyncMsg msg = (ufAsyncMsg){.msgType = ufShutdownMsg, .selfFree = !free };
  sendMsg(i, &msg); // If this fails it was shutting down / already down anyway
  close(i->msgPipe[1]); // Close the write side promptly. May race with another writer, but the instance will clear those
  if(!free)
    return 0;
  return ufAwaitShutdown(instance);
}

////TODO CMYK 2020.05.22: make the temp folder configurable
//static int writebackFileName(char** dst, ufInstance_t instance, uint64_t ufObjectBase){
//  uint64_t myPid = (uint64_t) getpid();
//  uint64_t instAddr = (uint64_t) instance;
//
//  // the pid is printed normally and the memory addresses are hex
//  return asprintf(dst, "/tmp/%lu_%lx.%lx", myPid,
//      // Try to make the name as compact as possible, but consistent
//      instAddr     >> __builtin_ctz(sizeof(ufInstance)),
//      ufObjectBase >> __builtin_ctz(pageSize));
//}

static int createWriebackFile(ufInstance* instance, ufObject* o){
  int res;

//  char* filename;
  int fd;

//  tryPerrNegOne(res, writebackFileName(&filename, instance, o->startI), "could name create writeback file Name", nameErr);
//  fprintf(stderr, "%s\n", filename);

  tryPerrNegOne(fd, open("/tmp/", O_RDWR | O_TMPFILE, 0600), "Could not open annon writeback file", fileErr);

  uint32_t bitmapSize = ceilDiv(o->config.elementCt, o->config.objectsAtOnce); // number of slots
  bitmapSize = ceilDiv(bitmapSize, 8); // number of bytes for those slots, rounded up
  bitmapSize = ceilDiv(bitmapSize, pageSize) * pageSize; // round up to the next page boundary

  uint64_t writebackSize = bitmapSize + (o->config.stride * o->config.elementCt);

  assert(64 - __builtin_clz(writebackSize) <= (sizeof(off_t) << 3));
  tryPerrInt(res, ftruncate(fd, writebackSize), "could not truncate file to required size", errTruncate);

  void* writebackMmapPtr;
  tryPerr(writebackMmapPtr, writebackMmapPtr == (void*)-1,
    mmap(NULL, writebackSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0), "could not mmap writeback", errMMap);

  o->writebackMmapFd = fd;
  o->writebackMmapBase = writebackMmapPtr;
  o->writebackMmapBitmapLength = bitmapSize;
//  o->writebackFileName = filename;

  return 0;

  errMMap:
  errTruncate:
  close(fd);
//  unlink(filename);

  fileErr:
//  nameErr:
  return -1;
}

int ufCreateObject(ufInstance_t instance, ufObjectConfig_t objectConfig, ufObject_t* object_p){
  ufInstance*        i = asUfInstance(instance);
  ufObjectConfig* conf = asObjectConfig(objectConfig);

  int res = -1, returnVal = -1;
  ufObject* o;
  tryPerrNull(o, calloc(1, sizeof(ufObject)), "error allocating object", errAlloc);
  memcpy(&o->config, conf, sizeof(ufObjectConfig)); // objects contain a copy of the config so the original config can be reused
  o->instance = i;

  const uint64_t toAllocate = conf->headerSzWithPadding + ceilDiv(conf->stride*conf->elementCt, pageSize) * pageSize;
  o->trueSize = toAllocate;

  // Assign an ID to the object
  o->id = i->nextID++;

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

  // If we got success then the worker surely allocated our object
  assert(NULL != o->start);

  // Create a file to contain writes
  tryPerrInt(res, createWriebackFile(instance, o), "could not create writeback file", writebackErr);

  // Done and all went well
  sem_destroy(&completionLock);

  *object_p = o;
  return 0;

  writebackErr:
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

ufObject_t ufLookupObjectByMemberAddress(ufInstance_t instance, void* ptr){
  ufInstance* i = asUfInstance(instance);

  int res;
  entry e;
  res = listFind(i->objects, &e, ptr);
  if(0 != res)
    return NULL;
  return asUfo(e.valuePtr);
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
    tryPerrInt(sendErr, sendMsg(i, &msg), "instance shutting down", shuttingDown);

    tryPerrInt(res, sem_wait(&completionLock), "error waiting for object destruction", awaitErr);
    if(returnVal) goto freeErr; // thats bad… don't free the object

    // cleanup
    free(o);
    sem_destroy(&completionLock);

    return 0;

    freeErr:
    awaitErr:
    shuttingDown:

    if(sendErr) free(o); // Only need to free the struct if shutting down
    sem_destroy(&completionLock);
    semErr:

    return sendErr ? 0 : res;
  }else{
    // instance shutting down, it frees everything but the struct
    free(o);
    return 0;
  }
}






