#ifndef ossimPlanetGridUtility_HEADER
#define ossimPlanetGridUtility_HEADER
#include <ossim/base/ossimConstants.h>
#include <osg/Referenced>
#include "ossimPlanetExport.h"
#include <osg/Vec3d>
#include <vector>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimCommon.h>

class OSSIMPLANET_DLL ossimPlanetGridUtility : public osg::Referenced
{
public:
   enum ossimPlanetGridUtilityIndices
   {
      LAT = 0,
      LON = 1,
      HGT = 2,
      GRIDX = 0,
      GRIDY = 1,
      GRIDZ = 2
   };
   class GridPoint
   {
   public:
      //  the global is a 0 to 1 for the entire face.
      osg::Vec3d   theGlobalGridPoint;
      
      // the local is a 0 to 1 for the local row col patch.  This value could be used
      // for the texture coordinate
      osg::Vec3d   theLocalGridPoint;

      // theFace this grid point corresponds to
      ossim_uint32 theFace;
   };
   
   ossimPlanetGridUtility(ossim_uint32 tileWidth = 256,
                          ossim_uint32 tileHeight = 256)
   {
      theTileWidth  = tileWidth;
      theTileHeight = tileHeight;
   }
   void getGeographicLonCrossings(std::vector<osg::Vec2d>& minMaxPairs,
                                  ossim_uint32 level,
                                  ossim_uint64 row,
                                  ossim_uint64 col)const;
   virtual bool crossesGeographicBounds(ossim_uint32 level,
                                        ossim_uint64 row,
                                        ossim_uint64 col)const;
   virtual void getWidthHeightInDegrees(double& deltaX,
                                        double& deltaY,
                                        ossim_uint32 level,
                                        ossim_uint64 row,
                                        ossim_uint64 col)const;
   virtual void getGridSpacing(double& xSpacing,
                               double& ySpacing,
                               ossim_uint32 level,
                               ossim_uint64 row,
                               ossim_uint64 col)const;

   virtual void getPixelScaleAsDegrees(double& dx,
                                       double& dy,
                                       ossim_uint32 level,
                                       ossim_uint64 row,
                                       ossim_uint64 col)const;
   virtual void getPixelScaleAsMeters(double& dx,
                                      double& dy,
                                      ossim_uint32 level,
                                      ossim_uint64 row,
                                      ossim_uint64 col)const;
   virtual void getPixelScale(double& dx,
                              double& dy,
                              ossimUnitType& pixelScaleUnits,
                              ossim_uint32 level,
                              ossim_uint64 row,
                              ossim_uint64 col)const=0;
   
   virtual void getLocalRowColumn(ossim_uint64& localRow,
                                  ossim_uint64& localCol,
                                  ossim_uint32 level,
                                  ossim_uint64 row,
                                  ossim_uint64 col)const;
   virtual void mapToRowCol(ossim_uint64& targetRow,
                            ossim_uint64& targetCol,
                            ossim_uint32  targetLevel,
                            ossim_uint32  srcLevel,
                            ossim_uint64  srcRow,
                            ossim_uint64  srcCol)const;

   virtual ossim_uint32 getFace(ossim_uint32 level,
                                ossim_uint64 row,
                                ossim_uint64 col)const;


   virtual void getNumberOfTilesWideHighPerFace(ossim_uint64 &wide,
                                                ossim_uint64 &high,
                                                ossim_uint32 level)const;
   
  
   virtual ossim_uint32 getNumberOfFaces()const=0;

   virtual void getCenterGridPoint(ossimPlanetGridUtility::GridPoint& point,
                                   ossim_uint32 level,
                                   ossim_uint32 row,
                                   ossim_uint32 col)const;

   virtual void getCenterGridPoint(ossimPlanetGridUtility::GridPoint& gridPoint,
                                   ossim_uint32 level,
                                   const osg::Vec3d& latLon)const;
   
   virtual void createGridPoints(std::vector<ossimPlanetGridUtility::GridPoint>& points,
                                 ossim_uint32 level,
                                 ossim_uint64 row,
                                 ossim_uint64 col,
                                 ossim_uint32 rows,
                                 ossim_uint32 cols)const;
   
   
   virtual void getLatLon(osg::Vec3d& latLon,
                          const ossimPlanetGridUtility::GridPoint& gridPoint)const=0;
   virtual void getGridPoint(ossimPlanetGridUtility::GridPoint& gridPoint,
                             const osg::Vec3d& latLon)const=0;
   virtual void getLatLonCorners(osg::Vec3d& ul,
                                 osg::Vec3d& ur,
                                 osg::Vec3d& lr,
                                 osg::Vec3d& ll,
                                 ossim_uint32 level,
                                 ossim_uint32 row,
                                 ossim_uint32 col)const;
   virtual void getCenterLatLon(osg::Vec3d& center,
                                ossim_uint32 level,
                                ossim_uint32 row,
                                ossim_uint32 col)const;
   virtual void getLatLonBounds(double& minLat,
                                double& minLon,
                                double& maxLat,
                                double& maxLon,
                                ossim_uint32 level,
                                ossim_uint64 row,
                                ossim_uint64 col)const;
   
   virtual void getNumberOfTilesWideHigh(ossim_uint64 &wide,
                                         ossim_uint64 &high,
                                         ossim_uint32 level)const;
   
   virtual ossim_uint64 getNumberOfTiles(ossim_uint32 level)const;
   virtual ossim_uint64 getTotalNumberOfTiles(ossim_uint32 level)const;

   virtual ossim_uint64 getId(ossim_uint32 level,
                              ossim_uint64 row,
                              ossim_uint64 col)const;

   ossim_uint32 getTileWidth()const;
   ossim_uint32 getTileHeight()const;

   void setTileWidthHeight(ossim_uint32 tileWidth,
                           ossim_uint32 tileHeight);

                            
protected:
   ossim_uint32 theTileWidth;
   ossim_uint32 theTileHeight;
   
};

#endif
