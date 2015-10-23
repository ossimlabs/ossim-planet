#ifndef ossimPlanetStandardTextureLayerFactory_HEADER
#define ossimPlanetStandardTextureLayerFactory_HEADER
#include <OpenThreads/Mutex>
#include <ossimPlanet/ossimPlanetTextureLayerFactory.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetStandardTextureLayerFactory : public ossimPlanetTextureLayerFactory
{
public:
   ossimPlanetStandardTextureLayerFactory();

   static ossimPlanetStandardTextureLayerFactory* instance();
   virtual osg::ref_ptr<ossimPlanetTextureLayer> createLayer(const ossimString& name, bool openAllEntriesFlag)const;

protected:
   ossimPlanetStandardTextureLayerFactory(const ossimPlanetStandardTextureLayerFactory& src):ossimPlanetTextureLayerFactory(src) { } 
   const ossimPlanetStandardTextureLayerFactory& operator =(const ossimPlanetStandardTextureLayerFactory& ) { return *this;}
      
   osg::ref_ptr<ossimPlanetTextureLayer> createLayerFromFilename(const ossimFilename& name, bool openAllEntriesFlag)const;
   osg::ref_ptr<ossimPlanetTextureLayer> createLayerFromKwl(const ossimKeywordlist& kwl,
                                                            const ossimString& prefix=ossimString())const;
   osg::ref_ptr<ossimPlanetTextureLayer> createLayerFromOldKwl(const ossimKeywordlist& kwl,
                                                               const ossimString& prefix=ossimString())const;
   static ossimPlanetStandardTextureLayerFactory* theInstance;
   mutable ossimPlanetReentrantMutex theMutex;
};

#endif
