#include <ossimPlanet/ossimPlanetElevationFactory.h>
#include <ossimPlanet/ossimPlanetSrtmElevationDatabase.h>
#include <ossimPlanet/ossimPlanetDtedElevationDatabase.h>
#include <ossimPlanet/ossimPlanetGeneralRasterElevationDatabase.h>

ossimPlanetElevationFactory::ossimPlanetElevationFactory()
{
   setId("default");
}

ossimPlanetElevationFactory* ossimPlanetElevationFactory::instance()
{
   static ossimPlanetElevationFactory* theInstance = 0;
   if(!theInstance)
   {
      theInstance = new ossimPlanetElevationFactory;
   }
   
   return theInstance;
}

ossimPlanetElevationDatabase* ossimPlanetElevationFactory::openDatabase(const ossimString& location)
{
   // check SRTM database
   osg::ref_ptr<ossimPlanetElevationDatabase> database = new ossimPlanetSrtmElevationDatabase;
   
   if(database->open(location) == ossimPlanetTextureLayer_VALID)
   {
      return database.release();
   }
   else
   {
      database = new ossimPlanetDtedElevationDatabase;
      if(database->open(location) == ossimPlanetTextureLayer_VALID)
      {
         return database.release();
      }
      else
      {
         database = new ossimPlanetGeneralRasterElevationDatabase;
         if(database->open(location) == ossimPlanetTextureLayer_VALID)
         {
            return database.release();
         }
      }
   }
   
   return 0;
}
