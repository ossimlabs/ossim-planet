#ifndef ossimPlanetLayerFactory_HEADER
#define ossimPlanetLayerFactory_HEADER
#include "ossimPlanetLayerFactoryBase.h"
#include <osg/ref_ptr>

class ossimPlanetLayerFactory : public ossimPlanetLayerFactoryBase
{
public:
   ossimPlanetLayerFactory();
   virtual ~ossimPlanetLayerFactory();
   
   static ossimPlanetLayerFactory* instance();
   
   virtual ossimPlanetLayer* create(const ossimString& type)const;
protected:
   static ossimPlanetLayerFactory* theInstance;
   
};


#endif
