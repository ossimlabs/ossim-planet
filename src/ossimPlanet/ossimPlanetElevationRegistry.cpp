#include <ossimPlanet/ossimPlanetElevationRegistry.h>
#include <ossimPlanet/ossimPlanetElevationFactory.h>
#include <algorithm>
#include <mutex>

ossimPlanetElevationRegistry::ossimPlanetElevationRegistry()
{
}

bool ossimPlanetElevationRegistry::registerFactory(FactoryBase* factory)
{
   std::lock_guard<std::recursive_mutex> lock(theMutex);
   bool result = false;
   FactoryList::iterator iter = std::find(theFactoryList.begin(), theFactoryList.end(), factory);
   
   if(iter == theFactoryList.end())
   {
      theFactoryList.push_back(factory);
      result = true;
   }
   
   return result;
}

void ossimPlanetElevationRegistry::unregisterFactory(FactoryBase* factory)
{
   std::lock_guard<std::recursive_mutex> lock(theMutex);
   FactoryList::iterator iter = std::find(theFactoryList.begin(), theFactoryList.end(), factory);
   
   if(iter != theFactoryList.end())
   {
      theFactoryList.erase(iter);
   }
}

ossimPlanetElevationDatabase* ossimPlanetElevationRegistry::openDatabase(const ossimString& location)
{
   std::lock_guard<std::recursive_mutex> lock(theMutex);
   ossimPlanetElevationDatabase* result = 0;
   
   FactoryList::iterator iter = theFactoryList.begin();
   while(!result&&(iter != theFactoryList.end()))
   {
      result = (*iter)->openDatabase(location);
      ++iter;
   }
   
   return result;
}

ossimPlanetElevationRegistry* ossimPlanetElevationRegistry::instance()
{
   static ossimPlanetElevationRegistry* theInstance = 0;
   if(!theInstance)
   {
      theInstance = new ossimPlanetElevationRegistry();
      theInstance->registerFactory(ossimPlanetElevationFactory::instance());
   }
   
   return theInstance;
}
