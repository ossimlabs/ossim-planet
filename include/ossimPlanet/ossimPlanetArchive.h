
#ifndef ossimPlanetArchive_HEADER
#define ossimPlanetArchive_HEADER

#include <vector>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossim/base/ossimFilename.h>
#include <osg/Referenced>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <OpenThreads/ScopedLock>
#include <ossimPlanet/ossimPlanetArchiveMapping.h>


class OSSIMPLANET_DLL ossimPlanetArchive : public osg::Referenced
{
private:
	bool useArchiveMapping;
	std::vector<ossimPlanetArchiveMapping> mappingList;
	mutable ossimPlanetReentrantMutex theArchiveMutex;

public:
	ossimPlanetArchive();
protected:
	~ossimPlanetArchive();

public:
	void addMapping(ossimPlanetArchiveMapping &mapping);
	void removeMapping(ossimPlanetArchiveMapping &mapping);
	ossimFilename matchPath(const ossimFilename &filename);
	ossimFilename convertToDirectory(ossimFilename &filename);
	void setArchiveMappingEnabledFlag(bool enabled = false);
	bool archiveMappingEnabled();
	std::vector<ossimPlanetArchiveMapping> getMappingList();
};


#endif
