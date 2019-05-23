#include "mappedMemory/userfaultCore_dummy.h"
ufInstance_t ufMakeInstance(){}
int ufInit(ufInstance_t instance){}
void ufShutdown(ufInstance_t instance, bool free){}
void ufAwaitShutdown(ufInstance_t instance){}
uint32_t ufGetPageSize(ufInstance_t instance){}
ufObjectConfig_t _makeObjectConfig(uint64_t ct, uint32_t stride, int32_t minLoadCt){}
void ufSetPopulateFunction(ufObjectConfig_t config, ufPopulateRange populateF){}
void ufSetUserConfig(ufObjectConfig_t config, ufUserData userData){}
int ufCreateObject(ufInstance_t instance, ufObjectConfig_t objectConfig, ufObject_t* object_p){}
int ufDestroyObject(ufObject_t object_p){}
void* ufGetHeaderPointer(ufObject_t object){}
void* ufGetValuePointer(ufObject_t object){}



