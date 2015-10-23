#ifndef ossimPlanetCubeGrid_HEADER
#define ossimPlanetCubeGrid_HEADER
#include <ossimPlanet/ossimPlanetGridUtility.h>
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetCubeGrid : public ossimPlanetGridUtility
{
public:
   ossimPlanetCubeGrid(ossim_uint32 tileWidth = 256,
                        ossim_uint32 tileHeight = 256)
      :ossimPlanetGridUtility(tileWidth, tileHeight)
   {
   }
   virtual void getPixelScale(double& dx,
                              double& dy,
                              ossimUnitType& pixelScaleUnits,
                              ossim_uint32 level,
                              ossim_uint64 row,
                              ossim_uint64 col)const;
   virtual void getWidthHeightInDegrees(double& deltaX,
                                        double& deltaY,
                                        ossim_uint32 level,
                                        ossim_uint64 row,
                                        ossim_uint64 col)const;
   // this is from GeoFusion SPEC
   virtual void getLatLon(osg::Vec3d& latLon,
                          const ossimPlanetGridUtility::GridPoint& gridPoint)const;

   // this is from Geofusion SPEC
   virtual void getGridPoint(ossimPlanetGridUtility::GridPoint& gridPoint,
                             const osg::Vec3d& latLon)const;
   
   virtual ossim_uint32 getNumberOfFaces()const;
   
};

#endif
