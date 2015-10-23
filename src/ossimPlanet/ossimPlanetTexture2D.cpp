#include <iostream>
#include <ossimPlanet/ossimPlanetTexture2D.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <OpenThreads/ScopedLock>
#include <osg/DisplaySettings>
//#define OSGPLANET_ENABLE_ALLOCATION_COUNT
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
static OpenThreads::Mutex objectCountMutex;
static ossim_uint32 textureCount = 0;
#endif

ossimPlanetTexture2D::ossimPlanetTexture2D(const ossimPlanetTerrainTileId& id)

   :osg::Texture2D(),
   theTileId(id)
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(objectCountMutex);
   ++textureCount;
   std::cout << "ossimPlanetTexture2D count = " << textureCount << "\n";
#endif
}

ossimPlanetTexture2D::ossimPlanetTexture2D(osg::Image* image, 
                                           const ossimPlanetTerrainTileId& id)
   :osg::Texture2D(image),
   theTileId(id)
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(objectCountMutex);
   ++textureCount;
   std::cout << "ossimPlanetTexture2D count = " << textureCount << "\n";
#endif   
}
ossimPlanetTexture2D::ossimPlanetTexture2D(ossimPlanetImage* image)
:osg::Texture2D(image),
theTileId(image?image->tileId():ossimPlanetTerrainTileId())
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(objectCountMutex);
   ++textureCount;
   std::cout << "ossimPlanetTexture2D count = " << textureCount << "\n";
#endif   
}

ossimPlanetTexture2D::ossimPlanetTexture2D(const ossimPlanetTexture2D& text,const osg::CopyOp& copyop)
   :osg::Texture2D(text, copyop)
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(objectCountMutex);
   ++textureCount;
   std::cout << "ossimPlanetTexture2D count = " << textureCount << "\n";
#endif   
}

ossimPlanetTexture2D::~ossimPlanetTexture2D()
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(objectCountMutex);
  --textureCount;
   std::cout << "ossimPlanetTexture2D count = " << textureCount << "\n";
#endif
}

void ossimPlanetTexture2D::setImage(ossimPlanetImage* image)
{
   if(image)
   {
      setId(image->tileId());
   }
   
   osg::Texture2D::setImage(image);
}

void ossimPlanetTexture2D::setId(const ossimPlanetTerrainTileId& id)
{
   theTileId = id;
}

const ossimPlanetTerrainTileId& ossimPlanetTexture2D::tileId()const
{
   return theTileId;
}