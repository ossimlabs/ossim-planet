#ifndef ossimPlanetArchiveMapping_HEADER
#define ossimPlanetArchiveMapping_HEADER

#include <ossimPlanet/ossimPlanetExport.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimFilename.h>


class OSSIMPLANET_DLL ossimPlanetArchiveMapping
{
private:
	ossimFilename src;
	ossimFilename dest;

public:
	ossimPlanetArchiveMapping();
	ossimPlanetArchiveMapping(const ossimFilename &source, const ossimFilename &destination);
	~ossimPlanetArchiveMapping();

	ossimFilename getSource();
	void setSource(const ossimFilename &source);

	ossimFilename getDestination();
	void setDestination(const ossimFilename &destination);
};


#endif
