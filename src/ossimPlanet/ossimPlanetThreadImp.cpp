#include <ossimPlanet/ossimPlanetThreadImp.h>
#include <ossimPlanet/ossimPlanetThread.h>
ossimPlanetThreadImp::~ossimPlanetThreadImp()
{
   
}

int ossimPlanetThreadImp::cancel()
{
   setDoneFlag(true);
   updateThreadBlock();
   return 0;
}

void ossimPlanetThreadImp::setDoneFlag(bool flag)
{
   std::lock_guard<std::recursive_mutex> lock(theImpMutex);
   theDoneFlag = flag;
}

bool ossimPlanetThreadImp::doneFlag()const
{
   std::lock_guard<std::recursive_mutex> lock(theImpMutex);
   return theDoneFlag;
}

void ossimPlanetThreadImp::threadPooled()
{
}

void ossimPlanetThreadImp::updateThreadBlock()
{
}
