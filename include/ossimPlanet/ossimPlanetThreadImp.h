#ifndef ossimPlanetThreadImp_HEADER
#define ossimPlanetThreadImp_HEADER
#include <osg/Referenced>
#include <osg/ref_ptr>
#include "ossimPlanetExport.h"
#include "ossimPlanetRefBlock.h"
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
class ossimPlanetThread;
class OSSIMPLANET_DLL ossimPlanetThreadImp
{
public:
   ossimPlanetThreadImp()
      :theThread(0),
       theDoneFlag(false) 
   {
   }
   virtual ~ossimPlanetThreadImp();
   virtual void setThread(ossimPlanetThread* thread)
   {
      theThread = thread;
   }
   ossimPlanetThread* thread()
   {
      return theThread;
   }
   virtual void run()=0;
   virtual int cancel();
   virtual void threadPooled();
   virtual void setDoneFlag(bool flag);
   virtual bool doneFlag()const;
   virtual void updateThreadBlock();
protected:
   mutable ossimPlanetReentrantMutex theImpMutex;
   ossimPlanetThread* theThread;
   bool theDoneFlag;
};

#endif
