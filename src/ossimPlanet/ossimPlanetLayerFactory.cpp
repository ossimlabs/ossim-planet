#include <ossimPlanet/ossimPlanetLayerFactory.h>
#include <ossimPlanet/ossimPlanetLayerRegistry.h>
#include <ossimPlanet/ossimPlanetLand.h>
#include <ossimPlanet/ossimPlanetLatLonHud.h>
#include <ossimPlanet/ossimPlanetSousaLayer.h>
#include <ossimPlanet/ossimPlanetKmlLayer.h>
#include <ossimPlanet/ossimPlanetAnnotationLayer.h>
#include <ossimPlanet/ossimPlanetTerrain.h>

ossimPlanetLayerFactory* ossimPlanetLayerFactory::theInstance = 0;

ossimPlanetLayerFactory::ossimPlanetLayerFactory()
{
   theInstance = this;
}

ossimPlanetLayerFactory::~ossimPlanetLayerFactory()
{
   theInstance = 0;
}

ossimPlanetLayerFactory* ossimPlanetLayerFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimPlanetLayerFactory;
   }
   
   return theInstance;
}

ossimPlanetLayer* ossimPlanetLayerFactory::create(const ossimString& type)const
{
   if(type == "ossimPlanetLand")
   {
      return new ossimPlanetLand;
   }
   else if(type == "osismPlanetTerrain")
   {
      return new ossimPlanetTerrain;
   }
   else if(type == "ossimPlanetVideoLayer")
   {
   }
   else if(type == "ossimPlanetLatLonHud")
   {
      return new ossimPlanetLatLonHud;
   }
   else if(type == "ossimPlanetSousaLayer")
   {
      return new ossimPlanetSousaLayer;
   }
   else if(type == "ossimPlanetKmlLayer")
   {
      return new ossimPlanetKmlLayer;
   }
	else if(type == "ossimPlanetAnnotationLayer")
	{
		return new ossimPlanetAnnotationLayer;
	}
   
   return 0;
}
