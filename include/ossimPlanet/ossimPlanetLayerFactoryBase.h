#ifndef ossimPlanetLayerFactoryBase_HEADER
#define ossimPlanetLayerFactoryBase_HEADER
#include <osg/Referenced>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/ossimPlanetExport.h>
class ossimPlanetLayer;
class OSSIMPLANET_DLL ossimPlanetLayerFactoryBase : public osg::Referenced
{
public:
   ossimPlanetLayerFactoryBase(){}
   
   virtual ossimPlanetLayer* create(const ossimString& type)const=0;
};

#endif
