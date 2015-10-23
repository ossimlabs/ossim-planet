#include <ossimPlanet/ossimPlanetNodeFactory.h>
#include <ossimPlanet/ossimPlanetAnnotationLayerNode.h>

ossimPlanetNodeFactory* ossimPlanetNodeFactory::theInstance = 0;

ossimPlanetNodeFactory::ossimPlanetNodeFactory()
{
	theInstance = this;
}

ossimPlanetNodeFactory::~ossimPlanetNodeFactory()
{
	theInstance = 0;
}


ossimPlanetNodeFactory* ossimPlanetNodeFactory::instance()
{	
	if(!theInstance)
	{
		theInstance = new ossimPlanetNodeFactory;
	}
	
	return theInstance;
}

ossimPlanetNode* ossimPlanetNodeFactory::create(const ossimString& type)const
{	
	if(type == "Placemark")
	{
		return new ossimPlanetAnnotationPlacemark;
	}
	return 0;
}

