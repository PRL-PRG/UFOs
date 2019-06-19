
#include "userfaultCore.h"
#include "userFaultCoreInternal.h"


void ufSetPopulateFunction(ufObjectConfig_t config, ufPopulateRange populateF){
  asObjectConfig(config)->populateFunction = populateF;
}

void ufSetUserConfig(ufObjectConfig_t config, ufUserData userData){
  asObjectConfig(config)->userConfig = userData;
}


char* ufGetHeaderPointer(ufObject_t object){
  ufObject* o = asUfo(object);
  return o->start + (o->config.headerSzWithPadding - o->config.headerSize);
}

char* ufGetValuePointer(ufObject_t object){
  ufObject* o = asUfo(object);
  return o->start + o->config.headerSzWithPadding;
}
