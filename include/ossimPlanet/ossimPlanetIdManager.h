#ifndef ossimPlanetIdManager_HEADER
#define ossimPlanetIdManager_HEADER
#include "ossimPlanetId.h"
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include "ossimPlanetExport.h"
class OSSIMPLANET_DLL ossimPlanetIdManager
{
public:
   static ossimPlanetId nextId();
   
protected:
   static ossimPlanetId theCurrentId;
   static ossimPlanetReentrantMutex theMutex;
};
#endif
