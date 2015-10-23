#include <ossimPlanet/ossimPlanetThread.h>
#include <ossimPlanet/ossimPlanetThreadPool.h>

ossimPlanetThread::ossimPlanetThread()
{
   theImplementation = 0;
   theThreadPool     = 0;
   theRetainThreadFlag = false;
}

void ossimPlanetThread::run()
{
   if(theImplementation)
   {
      theImplementation->run();
      if(theThreadPool&&!theRetainThreadFlag)
      {
         theThreadPool->makeAvailable(this);
      }
   }
}

int ossimPlanetThread::cancel()
{
   if(theImplementation)
   {
      theImplementation->cancel();
      while(isRunning())
      {
         OpenThreads::Thread::YieldCurrentThread();
      }
   }
   
   return 0;
}

void ossimPlanetThread::updateThreadBlock()
{
   if(theImplementation)
   {
      theImplementation->updateThreadBlock();
   }
}

void ossimPlanetThread::setImplementation(ossimPlanetThreadImp* implementation)
{
   if(theImplementation)
   {
      theImplementation->setThread(0);
      theImplementation->cancel();
   }
   theImplementation = implementation;
   if(theImplementation)
   {
      theImplementation->setThread(this);
   }
}

void ossimPlanetThread::setThreadPool(ossimPlanetThreadPool* threadPool)
{
   theThreadPool = threadPool;
}

ossimPlanetThreadImp* ossimPlanetThread::implementation()
{
   return theImplementation;
}

const ossimPlanetThreadImp* ossimPlanetThread::implementation()const
{
   return theImplementation;
}

void ossimPlanetThread::setRetainThreadFlag(bool flag)
{
   theRetainThreadFlag = flag;
}
