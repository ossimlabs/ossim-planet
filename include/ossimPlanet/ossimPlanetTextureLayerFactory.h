#ifndef ossimPlanetTextureLayerFactory_HEADER
#define ossimPlanetTextureLayerFactory_HEADER
#include <osg/Referenced>
#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetTextureLayerFactory : public osg::Referenced
{
public:
   virtual osg::ref_ptr<ossimPlanetTextureLayer> createLayer(const ossimString& name, bool openAllEntriesFlag=true)const=0;
    
protected:
   ossimPlanetTextureLayerFactory(){}
   ossimPlanetTextureLayerFactory(const ossimPlanetTextureLayerFactory&):osg::Referenced(){}
   const ossimPlanetTextureLayerFactory& operator = (const ossimPlanetTextureLayerFactory& /*src*/){return *this;}
   
};

#endif
