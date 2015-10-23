#ifndef ossimPlanetThread_HEADER
#define ossimPlanetThread_HEADER
#include <OpenThreads/Thread>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <osg/Referenced>
#include <osg/ref_ptr>
#include "ossimPlanetThreadImp.h"
#include "ossimPlanetExport.h"

class ossimPlanetThreadPool;
class OSSIMPLANET_DLL ossimPlanetThread : public osg::Referenced,
                                         public OpenThreads::Thread
{
public:
   ossimPlanetThread();
   virtual void run();
   virtual int cancel();
   virtual void updateThreadBlock();
   void setImplementation(ossimPlanetThreadImp* implementation);
   void setThreadPool(ossimPlanetThreadPool* threadPool);
   ossimPlanetThreadImp* implementation();
   const ossimPlanetThreadImp* implementation()const;
   void setRetainThreadFlag(bool flag);
protected:
   ossimPlanetThreadImp*  theImplementation;
   ossimPlanetThreadPool*              theThreadPool;
   bool                                theRetainThreadFlag;
};

#endif
