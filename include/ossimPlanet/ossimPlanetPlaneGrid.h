#ifndef ossimPlanetPlaneGrid_HEADER
#define ossimPlanetPlaneGrid_HEADER
#include <ossimPlanet/ossimPlanetGridUtility.h>

class OSSIMPLANET_DLL ossimPlanetPlaneGrid : public ossimPlanetGridUtility
{
public:
   ossimPlanetPlaneGrid(ossim_uint32 tileWidth = 256,
                        ossim_uint32 tileHeight = 256)
      :ossimPlanetGridUtility(tileWidth, tileHeight)
   {
   }
   virtual ossim_uint32 getNumberOfFaces()const;
   virtual void getPixelScale(double& dx,
                              double& dy,
                              ossimUnitType& pixelScaleUnits,
                              ossim_uint32 level,
                              ossim_uint64 row,
                              ossim_uint64 col)const;

   virtual void getGridPoint(ossimPlanetGridUtility::GridPoint& gridPoint,
                             const osg::Vec3d& latLon)const;

   virtual void getLatLon(osg::Vec3d& latLon,
                          const ossimPlanetGridUtility::GridPoint& gridPoint)const;
};

#endif
