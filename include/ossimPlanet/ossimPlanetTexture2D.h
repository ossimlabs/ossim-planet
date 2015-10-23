#ifndef ossimPlanetTexture2D_HEADER
#define ossimPlanetTexture2D_HEADER
#include <osg/Texture2D>
#include <osg/State>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossim/base/ossimConstants.h>
#include <ossimPlanet/ossimPlanetTerrainTileId.h>

class ossimPlanetImage;
class OSSIMPLANET_DLL ossimPlanetTexture2D : public osg::Texture2D
{
public:
   ossimPlanetTexture2D(const ossimPlanetTerrainTileId& id=ossimPlanetTerrainTileId(0,0,0,0));
   ossimPlanetTexture2D(osg::Image* image, 
                        const ossimPlanetTerrainTileId& id=ossimPlanetTerrainTileId(0,0,0,0));
   ossimPlanetTexture2D(ossimPlanetImage* image);

   /** Copy constructor using CopyOp to manage deep vs shallow copy. */
   ossimPlanetTexture2D(const ossimPlanetTexture2D& text,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
   virtual ~ossimPlanetTexture2D();

   void setImage(ossimPlanetImage* image);
   const ossimPlanetTerrainTileId& tileId()const;
   void setId(const ossimPlanetTerrainTileId& id);

protected:
   ossimPlanetTerrainTileId theTileId;
   ossim_uint32 theFace;
   ossim_uint32 theLevel;
   ossim_uint64 theRow;
   ossim_uint64 theCol;
};

#endif
