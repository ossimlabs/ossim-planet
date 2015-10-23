#ifndef ossimPlanetTextureLayerRegistry_HEADER
#define ossimPlanetTextureLayerRegistry_HEADER
#include <OpenThreads/ReadWriteMutex>
#include <osg/Referenced>
#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <ossimPlanet/ossimPlanetTextureLayerFactory.h>

class OSSIMPLANET_DLL ossimPlanetTextureLayerRegistry : public osg::Referenced
{
public:
   ossimPlanetTextureLayerRegistry();
   virtual ~ossimPlanetTextureLayerRegistry();
   static ossimPlanetTextureLayerRegistry* instance();

   void registerFactory(ossimPlanetTextureLayerFactory* factory);
   void registerFactoryToFront(ossimPlanetTextureLayerFactory* factory);
   void unregisterFactory(ossimPlanetTextureLayerFactory* factory);
   /**
    * The passed in string could be a WMS http string, keyword list
    * describing archives or a local image file, .. etc.  A layer will be returned.
    *
    * Note this layer could be a grouped layer.  For instance.  An image may have multiple
    * handlers and will return a group of all images.
    *
    */ 
   osg::ref_ptr<ossimPlanetTextureLayer> createLayer(const ossimString& name, bool openAllEntriesFlag=true)const;

   
protected:
   ossimPlanetTextureLayerRegistry(const ossimPlanetTextureLayerRegistry& /*src*/):osg::Referenced(){}
   const ossimPlanetTextureLayerRegistry& operator = (const ossimPlanetTextureLayerRegistry& /*src*/){return *this;}
   bool containsFactory(ossimPlanetTextureLayerFactory* factory)const;
   
   static ossimPlanetTextureLayerRegistry* theInstance;
   std::vector<ossimPlanetTextureLayerFactory* > theFactoryList;
   mutable OpenThreads::ReadWriteMutex theFactoryListMutex;
   mutable bool destroyingFlag;
};

#endif
