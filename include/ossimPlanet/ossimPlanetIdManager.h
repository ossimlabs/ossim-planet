#ifndef ossimPlanetIdManager_HEADER
#define ossimPlanetIdManager_HEADER
#include "ossimPlanetId.h"
#include "ossimPlanetExport.h"
#include <mutex>

class OSSIMPLANET_DLL ossimPlanetIdManager
{
public:
   static ossimPlanetId nextId();
   
protected:
   static ossimPlanetId theCurrentId;
   static std::recursive_mutex theMutex;
};
#endif
