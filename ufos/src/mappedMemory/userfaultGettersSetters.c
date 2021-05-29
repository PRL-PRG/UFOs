
#include "userfaultCore.h"
#include "userFaultCoreInternal.h"


void ufSetPopulateFunction(ufObjectConfig_t config, ufPopulateRange populateF){
  asObjectConfig(config)->populateFunction = populateF;
}

void ufSetUserConfig(ufObjectConfig_t config, ufUserData userData){
  asObjectConfig(config)->userConfig = userData;
}

void ufSetReadOnly(ufObjectConfig_t config){
  asObjectConfig(config)->readOnly = true;
}

char* ufGetHeaderPointer(ufObject_t object){
  ufObject* o = asUfo(object);
  return o->start + (o->config.headerSzWithPadding - o->config.headerSize);
}

char* ufGetValuePointer(ufObject_t object){
  ufObject* o = asUfo(object);
  return o->start + o->config.headerSzWithPadding;
}
