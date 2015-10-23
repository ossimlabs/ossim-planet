#include <ossimPlanet/ossimPlanetArchiveMapping.h>


ossimPlanetArchiveMapping::ossimPlanetArchiveMapping()
{
}

ossimPlanetArchiveMapping::ossimPlanetArchiveMapping(const ossimFilename &source, const ossimFilename &destination)
{
	src = source;
	dest = destination;
}

ossimPlanetArchiveMapping::~ossimPlanetArchiveMapping()
{
}

ossimFilename ossimPlanetArchiveMapping::getSource()
{ 
	return src; 
}

void ossimPlanetArchiveMapping::setSource(const ossimFilename &source)
{ 
	src = source; 
}

ossimFilename ossimPlanetArchiveMapping::getDestination()
{ 
	return dest; 
}

void ossimPlanetArchiveMapping::setDestination(const ossimFilename &destination)
{ 
	dest = destination; 
}
