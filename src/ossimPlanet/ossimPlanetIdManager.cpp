#include <ossimPlanet/ossimPlanetIdManager.h>
#include <OpenThreads/ScopedLock>

ossimPlanetId ossimPlanetIdManager::theCurrentId = 0;
ossimPlanetReentrantMutex ossimPlanetIdManager::theMutex;

ossimPlanetId ossimPlanetIdManager::nextId()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   
   return ++theCurrentId;
}
