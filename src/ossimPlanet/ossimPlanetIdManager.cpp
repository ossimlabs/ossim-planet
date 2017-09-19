#include <ossimPlanet/ossimPlanetIdManager.h>
#include <mutex>

ossimPlanetId ossimPlanetIdManager::theCurrentId = 0;
std::recursive_mutex ossimPlanetIdManager::theMutex;

ossimPlanetId ossimPlanetIdManager::nextId()
{
   std::lock_guard<std::recursive_mutex> lock(theMutex);
   
   return ++theCurrentId;
}
