#include <ossimPlanet/ossimPlanetThreadPool.h>
#include <ossimPlanet/ossimPlanetThread.h>
#include <algorithm>
#include <iostream>
#include <OpenThreads/ScopedLock>
unsigned int ossimPlanetThreadPool::theMaxThreads = 32;

osg::ref_ptr<ossimPlanetThreadPool> ossimPlanetThreadPool::theInstance = 0;
osg::ref_ptr<ossimPlanetThreadPool> ossimPlanetThreadPool::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimPlanetThreadPool;
   }
   return theInstance;
}

ossimPlanetThreadPool::ossimPlanetThreadPool()
{
   theInstance = this;
}

unsigned int ossimPlanetThreadPool::totalThreads()const
{
   return theAvailableList.size()+theUnavailableList.size();
}


void ossimPlanetThreadPool::makeAvailable(osg::ref_ptr<ossimPlanetThread> thread)
{
   ossimPlanetThreadImp* imp = thread->implementation();
   bool pooled = false;
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theListMutex);
      std::vector<osg::ref_ptr<ossimPlanetThread> >::iterator iter = std::find(theUnavailableList.begin(),
                                                                             theUnavailableList.end(),
                                                                             thread);
      if(iter!=theUnavailableList.end())
      {
         pooled = true;
         theUnavailableList.erase(iter);
         thread->setImplementation(0);
         theAvailableList.push_back(thread);
      }
   }
   if(pooled)
   {
      imp->threadPooled();
   }
         
   
}

osg::ref_ptr<ossimPlanetThread> ossimPlanetThreadPool::nextAvailable()
{
   osg::ref_ptr<ossimPlanetThread> thread = NULL;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theListMutex);
   if(theAvailableList.size() < 1)
   {
      if(totalThreads() < theMaxThreads)
      {
         osg::ref_ptr<ossimPlanetThread> thread = new ossimPlanetThread;
         thread->setThreadPool(this);
         theAvailableList.push_back(thread);
      }
      else
      {
         return thread;
      }
   }
   if(theAvailableList.size() > 0)
   {
      thread = theAvailableList[theAvailableList.size()-1];
      theAvailableList.pop_back();
      theUnavailableList.push_back(thread);
   }

   return thread;
}

void ossimPlanetThreadPool::setMaxThread(int maxThreads)
{
  theMaxThreads = maxThreads;
}
