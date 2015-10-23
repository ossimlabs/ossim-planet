#ifndef ossimPlanetThreadPool_HEADER
#define ossimPlanetThreadPool_HEADER
#include <vector>
#include <OpenThreads/Thread>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <osg/Referenced>
#include <osg/ref_ptr>
#include "ossimPlanetExport.h"

class ossimPlanetThread;

class OSSIMPLANET_DLL ossimPlanetThreadPool : public osg::Referenced
{
public:
   static osg::ref_ptr<ossimPlanetThreadPool> instance();
   void makeAvailable(osg::ref_ptr<ossimPlanetThread> thread);
   osg::ref_ptr<ossimPlanetThread> nextAvailable();
   static void setMaxThread(int maxThreads);
   
protected:
   ossimPlanetThreadPool();

   unsigned int totalThreads()const;
   std::vector<osg::ref_ptr<ossimPlanetThread> > theAvailableList;
   std::vector<osg::ref_ptr<ossimPlanetThread> > theUnavailableList;
   ossimPlanetReentrantMutex            theListMutex;
   static unsigned int theMaxThreads;
   static osg::ref_ptr<ossimPlanetThreadPool> theInstance;

};

#endif
