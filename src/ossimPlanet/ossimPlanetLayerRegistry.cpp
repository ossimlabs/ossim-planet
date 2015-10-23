#include <ossimPlanet/ossimPlanetLayerRegistry.h>
#include <ossimPlanet/ossimPlanetLayerFactory.h>
#include <algorithm>
ossimPlanetLayerRegistry* ossimPlanetLayerRegistry::theInstance = 0;
ossimPlanetLayerRegistry::ossimPlanetLayerRegistry()
{
   theInstance = this;
}

ossimPlanetLayerRegistry::~ossimPlanetLayerRegistry()
{
   theInstance = 0;
}

ossimPlanetLayerRegistry* ossimPlanetLayerRegistry::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimPlanetLayerRegistry;
      theInstance->registerFactory(ossimPlanetLayerFactory::instance());
   }
   
   return theInstance;
}

void ossimPlanetLayerRegistry::finalize()
{
   if(theInstance)
   {
		theFactoryList.clear();
		
      delete theInstance;
      theInstance = 0;
   }
}

ossimPlanetLayer* ossimPlanetLayerRegistry::create(const ossimString& layerName)const
{
   ossimPlanetLayer* result = 0;
   ossim_uint32 idx = 0;
   theFactoryListMutex.readLock();
   for(idx = 0; ((idx < theFactoryList.size())&&(!result)); ++idx)
   {
      result = theFactoryList[idx]->create(layerName);
   }
   theFactoryListMutex.readUnlock();
   
   return result;
}

void ossimPlanetLayerRegistry::registerFactory(ossimPlanetLayerFactoryBase* factory,
															  bool insertFrontFlag)
{
   if(!hasFactory(factory))
   {
      theFactoryListMutex.writeLock();
		if(!insertFrontFlag)
		{
			theFactoryList.push_back(factory);
		}
		else
		{
			theFactoryList.insert(theFactoryList.begin(), factory);
		}
      theFactoryListMutex.writeUnlock();
   }
}

void ossimPlanetLayerRegistry::unregisterFactory(const ossimPlanetLayerFactoryBase* factory)
{
   theFactoryListMutex.writeLock();
   FactoryListType::iterator iter = std::find(theFactoryList.begin(), 
                                              theFactoryList.end(),
                                              factory);
   if(iter != theFactoryList.end())
   {
      theFactoryList.erase(iter);
   }
   theFactoryListMutex.writeUnlock();
}

bool ossimPlanetLayerRegistry::hasFactory(const ossimPlanetLayerFactoryBase* factory)const
{
   FactoryListType::const_iterator iter = std::find(theFactoryList.begin(), 
                                                    theFactoryList.end(),
                                                    factory);
   
   return (iter!=theFactoryList.end());
}
