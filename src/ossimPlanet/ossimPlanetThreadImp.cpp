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
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theImpMutex);
   theDoneFlag = flag;
}

bool ossimPlanetThreadImp::doneFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theImpMutex);
   return theDoneFlag;
}

void ossimPlanetThreadImp::threadPooled()
{
}

void ossimPlanetThreadImp::updateThreadBlock()
{
}
