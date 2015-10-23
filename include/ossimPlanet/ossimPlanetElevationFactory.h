#ifndef ossimPlanetElevationFactory_HEADER
#define ossimPlanetElevationFactory_HEADER
#include <ossimPlanet/ossimPlanetElevationRegistry.h>
class ossimPlanetElevationFactory : public ossimPlanetElevationRegistry::FactoryBase
{
public:
   ossimPlanetElevationFactory();
   virtual ossimPlanetElevationDatabase* openDatabase(const ossimString& location);
   static ossimPlanetElevationFactory* instance();
};

#endif