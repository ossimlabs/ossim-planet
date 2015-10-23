#include <algorithm>

#include <ossimPlanet/ossimPlanetTextureLayerRegistry.h>
#include <ossimPlanet/ossimPlanetStandardTextureLayerFactory.h>
#include <OpenThreads/ScopedLock>
#include <iostream>

ossimPlanetTextureLayerRegistry* ossimPlanetTextureLayerRegistry::theInstance = 0;

ossimPlanetTextureLayerRegistry::ossimPlanetTextureLayerRegistry()
{
   theInstance    = this;
   destroyingFlag = true;
}

ossimPlanetTextureLayerRegistry::~ossimPlanetTextureLayerRegistry()
{
   destroyingFlag = true;

   ossim_uint32 idx = 0;

   for(idx = 0; idx < theFactoryList.size(); ++idx)
   {
      delete theFactoryList[idx];
      theFactoryList[idx] = 0;
   }
   
   theInstance = 0;
   
}

ossimPlanetTextureLayerRegistry* ossimPlanetTextureLayerRegistry::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimPlanetTextureLayerRegistry;
      theInstance->registerFactory(ossimPlanetStandardTextureLayerFactory::instance());
   }

   return theInstance;
}

void ossimPlanetTextureLayerRegistry::registerFactory(ossimPlanetTextureLayerFactory* factory)
{
   theFactoryListMutex.writeLock();
   if(!containsFactory(factory))
   {
      theFactoryList.push_back(factory);
   }
   theFactoryListMutex.writeUnlock();
}

void ossimPlanetTextureLayerRegistry::registerFactoryToFront(ossimPlanetTextureLayerFactory* factory)
{
   theFactoryListMutex.writeLock();
   if(!containsFactory(factory))
   {
      theFactoryList.insert(theFactoryList.begin(), factory);
   }
   theFactoryListMutex.writeUnlock();
}

void ossimPlanetTextureLayerRegistry::unregisterFactory(ossimPlanetTextureLayerFactory* factory)
{
   if(destroyingFlag) return;
   theFactoryListMutex.writeLock();
   
   std::vector<ossimPlanetTextureLayerFactory*>::iterator iter = std::find(theFactoryList.begin(),
                                                                                         theFactoryList.end(),
                                                                                         factory);

   if(iter != theFactoryList.end())
   {
      theFactoryList.erase(iter);
   }
   theFactoryListMutex.writeUnlock();
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetTextureLayerRegistry::createLayer(const ossimString& name, bool openAllEntriesFlag)const
{
   ossim_uint32 idx=0;
   osg::ref_ptr<ossimPlanetTextureLayer>   result;
   theFactoryListMutex.readLock();
   for(idx = 0; ((idx < theFactoryList.size())&&(!result.valid()));++idx)
   {
      result = theFactoryList[idx]->createLayer(name, openAllEntriesFlag);
   }
   theFactoryListMutex.readUnlock();
   return result;
}

bool ossimPlanetTextureLayerRegistry::containsFactory(ossimPlanetTextureLayerFactory* factory)const
{
   ossim_uint32 idx = 0;

   for(idx = 0; idx < theFactoryList.size(); ++idx)
   {
      if(theFactoryList[idx] == factory)
      {
         return true;
      }
   }

   return false;
}
