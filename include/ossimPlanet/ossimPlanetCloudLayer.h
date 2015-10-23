#ifndef ossimPlanetCloudLayer_HEADER
#define ossimPlanetCloudLayer_HEADER
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetGrid.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimUnitConversionTool.h>
#include <osg/NodeVisitor>
#include <osg/TexMat>
#include <osg/Geometry>
#include <vector>
#include <cstdlib>

class OSSIMPLANET_DLL ossimPlanetCloudLayer : public ossimPlanetLayer
{
public:
   typedef osg::ref_ptr<osg::Geometry> geometryPtr;
   typedef std::vector<geometryPtr> geometryPtrArray;
   
   class OSSIMPLANET_DLL Patch : public ossimPlanetGrid
   {
   public:
      Patch(double deltaLat=1.0, double deltaLon=1.0)
      :ossimPlanetGrid(GEODETIC_MODEL),
      theDeltaLat(deltaLat),
      theDeltaLon(deltaLon)
      {
      }
      virtual void getRootIds(TileIds &ids)const
      {
         ids.push_back(ossimPlanetTerrainTileId());
      }
      virtual void centerModel(const ossimPlanetTerrainTileId& /* tileId */,
                               ModelPoint& modelPoint)
      {
         modelPoint = ModelPoint();
      }
      virtual void globalGridToModel(const GridPoint& gridPoint, ModelPoint& modelPoint)const
      {
         modelPoint.setX(-theDeltaLon*.5 + gridPoint.x()*theDeltaLon);
         modelPoint.setY(-theDeltaLat*.5 + gridPoint.y()*theDeltaLat);
         modelPoint.setZ(gridPoint.z());
      }
      virtual void modelToGlobalGrid(const ModelPoint& modelPoint, GridPoint& gridPoint)const
      {
         gridPoint.setX((modelPoint.x()+(theDeltaLon*.5))/theDeltaLon);
         gridPoint.setY((modelPoint.y()+(theDeltaLat*.5))/theDeltaLat);
         gridPoint.setZ(modelPoint.z());
      }
      virtual void widthHeightInModelSpace(const ossimPlanetTerrainTileId& /* tileId */,
                                           osg::Vec2d& deltaXY)const
      {
         deltaXY[1] = theDeltaLat;
         deltaXY[0] = theDeltaLon;
      }
      virtual ossim_uint32 numberOfFaces()const
      {
         return 1;
      }
      virtual bool findGridBound(ossim_uint32 face,
                                 const ModelPoint& minPoint,
                                 const ModelPoint& maxPoint,
                                 GridBound& bound,
                                 ossim_uint32 numberOfPoints=3)const
      {
         return false;
      }
      virtual bool isPolar(const ossimPlanetTerrainTileId& /* id */)const
      {
         return false;
      }
      /**
       * 
       */
      virtual ossimPlanetGridUtility* newBackwardCompatableGrid(ossim_uint32 /* width */,
                                                                ossim_uint32 /* height */)const
      {
         return 0;
      }
      
      double theDeltaLat;
      double theDeltaLon;
   };
   ossimPlanetCloudLayer();
   virtual ~ossimPlanetCloudLayer()
   {
      theGeometryArray.clear();
   }
   void updateTexture(osg::Image* cloudTexture);
   void updateTexture(ossim_int64 seed = 0,
                      ossim_int32 coverage = 20,
                      ossim_float64 sharpness = .95);
   
   virtual void computeMesh(double patchAltitude,
                            ossim_uint32 patchWidth=9, 
                            ossim_uint32 patchHeight=9, 
                            ossim_uint32 level=3);
   
   /**
    * This method allows for changing the opacity of the entire cloud layer
    */
   void setAlphaValue(float alpha);
   
   virtual void traverse(osg::NodeVisitor& nv);
   void setHeading(ossim_float64 heading)
   {
      theHeading = heading;
   }
   /**
    *  The speed is in units per hour.
    */ 
   void setSpeedPerHour(ossim_float64 speed, ossimUnitType unit=OSSIM_METERS)
   {
      // get it into units per second.
      //
      theSpeed = ossimUnitConversionTool(speed, unit).getMeters()/3600.0;
   }
   void setSpeedPerSecond(ossim_float64 speed, ossimUnitType unit=OSSIM_METERS)
   {
      // get it into units per second.
      //
      theSpeed = ossimUnitConversionTool(speed, unit).getMeters();
   }
   void setScale(ossim_float64 scale)
   {
      theTextureScale[0] = scale;
      theTextureScale[1] = scale;
      theTextureScale[2] = 1.0;
      updateMetersPerPixelCoverage();
   }
   void setGrid(ossimPlanetGrid* g)
   {
      theGrid = g;
   }
   void setAutoUpdateTextureMatrixFlag(bool flag)
   {
      theAutoUpdateTextureMatrixFlag = flag;
   }
   bool autoUpdateTextureMatrixFlag()const
   {
      return theAutoUpdateTextureMatrixFlag;
   }
   void moveToLocationLatLonAltitude(const osg::Vec3d& llh);
   
   ossimPlanetGrid* grid()
   {
      return theGrid.get();
   }
   void setTextureMatrix(osg::TexMat* texMatrix);
   osg::TexMat* textureMatrixAttribute()
   {
      return theTextureMatrixAttribute.get();
   }
   osg::Matrixd& textureMatrix()
   {
      return theTextureMatrix;
   }
   /**
    *
    */
   void setMaxAltitudeToShowClouds(ossim_float64 maxAltitude);
   ossim_float64 maxAltitudeToShowClouds()const;
   
private:
   void init();
   void splitTiles(ossimPlanetGrid::TileIds& tiles, ossim_uint32 levels)const;
   void updateTextureMatrix(double timeScale=0.0);
   void updateMetersPerPixelCoverage();
  
   osg::ref_ptr<ossimPlanetGrid>    theGrid;
   osg::ref_ptr<osg::Geode>         theGeode;
   osg::ref_ptr<osg::Image>         theImage;
   osg::ref_ptr<osg::Texture2D>     theTexture;
   
   ossim_float64   theHeading;
   ossim_float64   theSpeed;
   osg::Vec3d      theTextureTranslation;
   osg::Vec3d      theTextureScale;
   osg::Matrixd    theTextureMatrix;
   osg::ref_ptr<osg::TexMat> theTextureMatrixAttribute;
   bool            theAutoUpdateTextureMatrixFlag;
   ossim_uint32    theTextureWidth;
   ossim_uint32    theTextureHeight;
   ossim_int32     theMeshLevel;
   
   ossim_float64   theApproximateMetersPerPixelCoverage;
   ossim_float64   theMaxAltitudeToShowClouds;
   osg::Vec3d      theCenterLlh;
   
   geometryPtrArray theGeometryArray;
};

/**
 * This perlin noise code was used from the GameDev site written by Huang
 */ 
class OSSIMPLANET_DLL ossimPlanetCloud : public osg::Referenced
{
public:
   enum TextureSize
   {
      TEXTURE_SIZE_256_256 = 0,
      TEXTURE_SIZE_512_512 = 1,
      TEXTURE_SIZE_1024_1024 = 2,
      TEXTURE_SIZE_2048_2048 = 3
   };
   ossimPlanetCloud(TextureSize=TEXTURE_SIZE_1024_1024);
   void makeCloud(ossim_int64 seed, 
                  ossim_int32 coverage=20, 
                  ossim_float64 sharpness=.95);//,
                  //TextureSize size=TEXTURE_SIZE_2048_2048);
   osg::Image* image();

protected:
   void makeNoise(ossim_int64 seed);
   ossim_float64 interpolate(ossim_float64 x, ossim_float64 y, ossim_float64  *map);
   double noise(ossim_int32 x, ossim_int32 y, ossim_int32 random)const;
   void overlapOctaves();
   void expFilter();
   ossim_int32 theNoiseDataWidth;
   ossim_int32 theNoiseDataHeight;
   ossim_int32 theCloudDataWidth;
   ossim_int32 theCloudDataHeight;
   std::vector<ossim_float64> theNoise;
   std::vector<ossim_float64> theCloudData;
   osg::ref_ptr<osg::Image>   theImage;
   
   ossim_int32   theCoverage;
   ossim_float64 theSharpness;
   
};

#endif
