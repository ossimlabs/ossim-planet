#ifndef ossimPlanetElevationRegistry_HEADER
#define ossimPlanetElevationRegistry_HEADER
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetElevationDatabase.h>
#include <osg/Referenced>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>

class OSSIMPLANET_DLL ossimPlanetElevationRegistry
{
public:
   class OSSIMPLANET_DLL FactoryBase
      {
      public:
         virtual ~FactoryBase(){}
         virtual ossimPlanetElevationDatabase* openDatabase(const ossimString& location)=0;
         
         ossimString id(){return theId;}
         void setId(const ossimString& value){theId = value;}
      protected:
         ossimString theId;
      };
   typedef std::vector<FactoryBase*> FactoryList;
   
   ossimPlanetElevationRegistry();
   bool registerFactory(FactoryBase* factory);
   void unregisterFactory(FactoryBase* factory);
   ossimPlanetElevationDatabase* openDatabase(const ossimString& location);
   
   static ossimPlanetElevationRegistry* instance();
   
protected:
   ossimPlanetReentrantMutex theMutex;
   FactoryList theFactoryList;
};

#endif
