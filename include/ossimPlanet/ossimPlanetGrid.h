#ifndef ossimPlanetGrid_HEADER
#define ossimPlanetGrid_HEADER
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetTerrainTileId.h>
#include <ossimPlanet/ossimPlanetExtents.h>
#include <osg/Referenced>
#include <osg/Vec3d>
#include <osg/Vec2d>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimDrect.h>
#include <vector>
#include <iostream>

// this is here temporarily until everyone uses the new grid interface.  This will be used to
// crate a backward comapatable grid type
#include <ossimPlanet/ossimPlanetGridUtility.h>

class OSSIMPLANET_DLL ossimPlanetGrid : public osg::Referenced
{
public:
   enum IndexValues
   {
      LAT_IDX = 0,
      LON_IDX = 1,
      HGT_IDX = 2,
      X_IDX = 0,
      Y_IDX = 1,
      Z_IDX = 2
   };
   enum ModelType
   {
      UNKNOWN        = 0,
      GEODETIC_MODEL = 1
   };
   class LocalNdcPoint
      {
      public:
         LocalNdcPoint(ossim_float64 xValue=0.0,
                       ossim_float64 yValue=0.0,
                       ossim_float64 zValue=0.0)
         {
            setX(xValue);
            setY(yValue);
            setZ(zValue);
         }
         void setX(ossim_float64 value){theXYZ[ossimPlanetGrid::X_IDX] = value;}
         void setY(ossim_float64 value){theXYZ[ossimPlanetGrid::Y_IDX] = value;}
         void setZ(ossim_float64 value){theXYZ[ossimPlanetGrid::Z_IDX] = value;}
         ossim_float64 x()const{return theXYZ[ossimPlanetGrid::X_IDX];}
         ossim_float64 y()const{return theXYZ[ossimPlanetGrid::Y_IDX];}
         ossim_float64 z()const{return theXYZ[ossimPlanetGrid::Z_IDX];}
         osg::Vec3d theXYZ;
      };
   class GridPoint
      {
      public:
         friend std::ostream& operator <<(std::ostream& out, const ossimPlanetGrid::GridPoint& point)
         {
            return out << "<" << point.theFace << ", " << point.theXYZ[ossimPlanetGrid::X_IDX] << ", " << point.theXYZ[ossimPlanetGrid::Y_IDX] << ", " << point.theXYZ[ossimPlanetGrid::Z_IDX] << ">";
         }
         GridPoint()
         :theFace(0)
         {
            theXYZ[0]=0.0;
            theXYZ[1]=0.0;
            theXYZ[2]=0.0;
         }
         GridPoint(ossim_uint32 face,
                   ossim_float64 x, ossim_float64 y, ossim_float64 z=0.0)
         :theFace(face)
         {
            theXYZ[ossimPlanetGrid::X_IDX]=x;
            theXYZ[ossimPlanetGrid::Y_IDX]=y;
            theXYZ[ossimPlanetGrid::Z_IDX]=z;
         }
         GridPoint(ossim_uint32 face,
                   osg::Vec3d& xyz)
         :theFace(face),
         theXYZ(xyz)
         {
         }
         void setX(ossim_float64 value){theXYZ[ossimPlanetGrid::X_IDX] = value;}
         void setY(ossim_float64 value){theXYZ[ossimPlanetGrid::Y_IDX] = value;}
         void setZ(ossim_float64 value){theXYZ[ossimPlanetGrid::Z_IDX] = value;}
         ossim_float64 x()const{return theXYZ[ossimPlanetGrid::X_IDX];}
         ossim_float64 y()const{return theXYZ[ossimPlanetGrid::Y_IDX];}
         ossim_float64 z()const{return theXYZ[ossimPlanetGrid::Z_IDX];}
         ossim_uint32 face()const{return theFace;}
         void setFace(ossim_uint32 face){theFace = face;}
       
         ossim_uint32 theFace;
         osg::Vec3d theXYZ;
      };
   class GridBound
      {
      public:
         friend std::ostream& operator <<(std::ostream& out, const ossimPlanetGrid::GridBound& bound)
         {
            return out << "<" << bound.face() << "," 
                       << bound.minx() << ", " 
                       << bound.miny() << ", " 
                       << bound.maxx() << ","
                       << bound.maxy() << ">";
         }
         ossim_float64 minx()const{return theMinx;}
         ossim_float64 miny()const{return theMiny;}
         ossim_float64 maxx()const{return theMinx+theWidth;}
         ossim_float64 maxy()const{return theMiny+theHeight;}
         ossim_float64 width()const{return theWidth;}
         ossim_float64 height()const{return theHeight;}
         void setFace(ossim_uint32 face){theFace = face;}
         ossim_uint32 face()const{return theFace;}
         ossimDrect toDrect()const
         {
            return ossimDrect(theMinx, 
                              theMiny+theHeight, 
                              theMinx+theWidth, 
                              theMiny, OSSIM_RIGHT_HANDED);
         }
         ossim_uint32  theFace;
         ossim_float64 theMinx;
         ossim_float64 theMiny;
         ossim_float64 theWidth;
         ossim_float64 theHeight;
      };
   class ModelPoint
      {
      public:
         friend std::ostream& operator <<(std::ostream& out, const ossimPlanetGrid::ModelPoint& point)
         {
            return out << "<" << point.theXYZ[ossimPlanetGrid::X_IDX] << ", " << point.theXYZ[ossimPlanetGrid::Y_IDX] << ", " << point.theXYZ[ossimPlanetGrid::Z_IDX] << ">";
         }
         ModelPoint()
         {
            theXYZ[0]=0.0;
            theXYZ[1]=0.0;
            theXYZ[2]=0.0;
         }
         ModelPoint(double x, double y, double z=0)
         {
            theXYZ[0] = x;
            theXYZ[1] = y;
            theXYZ[2] = z;
         }
         void setX(ossim_float64 value){theXYZ[ossimPlanetGrid::X_IDX] = value;}
         void setY(ossim_float64 value){theXYZ[ossimPlanetGrid::Y_IDX] = value;}
         void setZ(ossim_float64 value){theXYZ[ossimPlanetGrid::Z_IDX] = value;}
         ossim_float64 x()const{return theXYZ[ossimPlanetGrid::X_IDX];}
         ossim_float64 y()const{return theXYZ[ossimPlanetGrid::Y_IDX];}
         ossim_float64 z()const{return theXYZ[ossimPlanetGrid::Z_IDX];}
         
         osg::Vec3d theXYZ;
      };
   typedef std::vector<ossimPlanetTerrainTileId> TileIds;
   typedef std::vector<ModelPoint> ModelPoints;
   typedef std::vector<GridPoint> GridPoints;
   typedef std::vector<GridBound> GridBounds;
   
   ossimPlanetGrid(ossimPlanetGrid::ModelType type)
   :theModelType(type)
   {
   }
   ossimPlanetGrid::ModelType modelType()const
   {
      return theModelType;
   }
   virtual void getRootIds(TileIds &ids) const=0;
   void getInternationalDateLineCrossings(const ossimPlanetTerrainTileId& tileid,
                                          std::vector<osg::Vec2d>& minMaxPairs)const;
   virtual bool crossesInternationalDateLine(const ossimPlanetTerrainTileId& tileId)const;
   virtual void createModelPoints(const ossimPlanetTerrainTileId& tileId,
                                  ossim_uint32 w,
                                  ossim_uint32 h,
                                  ModelPoints& modelPoints,
                                  ossim_uint32 padding=0)const;
   virtual void globalGridToModel(const GridPoint& gridPoint, ModelPoint& modelPoint)const=0;
   virtual void modelToGlobalGrid(const ModelPoint& modelPoint, GridPoint& gridPoint)const=0;
   virtual void modelBound(const ossimPlanetTerrainTileId& tileId, 
                           ModelPoint& minPoint, ModelPoint& maxPoint)const;
   virtual void localNdcToGlobalGrid(const ossimPlanetTerrainTileId& tileId, const LocalNdcPoint& localNdc, GridPoint& globalGrid)const;
   virtual void localNdcToModel(const ossimPlanetTerrainTileId& tileId, const LocalNdcPoint& localNdc, ModelPoint& model)const;
   virtual void numberOfTilesPerFace(ossim_uint32 lod, ossim_uint64& tilesWide, ossim_uint64& tilesHigh) const;
   virtual void bounds(const ossimPlanetTerrainTileId& tileId, GridBound& bound)const;
   virtual void boundsToModel(const ossimPlanetTerrainTileId& tileId, 
                              ModelPoint& p0, 
                              ModelPoint& p1, 
                              ModelPoint& p2,
                              ModelPoint& p3)const;
   virtual void centerGrid(const ossimPlanetTerrainTileId& tileId, GridPoint& gridPoint);
   virtual void centerModel(const ossimPlanetTerrainTileId& tileId, ModelPoint& modelPoint);
   virtual void widthHeight(const ossimPlanetTerrainTileId& tileId, ossim_float64& width, ossim_float64& height)const;
   virtual void origin(const ossimPlanetTerrainTileId& tileId, GridPoint& gridPoint)const;
   
   virtual void getUnitsPerPixel(osg::Vec2d& unitsPerPixel,
                                 const ossimPlanetTerrainTileId& tileId, ossim_uint32 w, ossim_uint32 h,
                                 const ossimUnitType unitType=OSSIM_METERS)const;
   virtual bool convertToGeographicExtents(const ossimPlanetTerrainTileId& tileId,
                                           ossimPlanetExtents& extents, 
                                           ossim_uint32 w, ossim_uint32 h)const;
   virtual void widthHeightInModelSpace(const ossimPlanetTerrainTileId& tileId, osg::Vec2d& deltaXY)const=0;
   virtual ossim_uint32 numberOfFaces()const=0;
   virtual bool findGridBound(ossim_uint32 face,
                              const ModelPoint& minPoint,
                              const ModelPoint& maxPoint,
                              GridBound& bound,
                              ossim_uint32 numberOfPoints=3)const=0;
   virtual bool isPolar(const ossimPlanetTerrainTileId& id)const=0;
   /**
    * 
    */
   virtual ossimPlanetGridUtility* newBackwardCompatableGrid(ossim_uint32 width,
                                                             ossim_uint32 height)const=0;
protected:
   ModelType theModelType;
};

class OSSIMPLANET_DLL ossimPlanetCubeGrid2 : public ossimPlanetGrid
{
public:
   ossimPlanetCubeGrid2()
   :ossimPlanetGrid(ossimPlanetGrid::GEODETIC_MODEL),
   theEquatorialCols(4),
   theEquatorialRows(1) // will specify the number of bands
   {
   }
   virtual void getRootIds(TileIds &ids) const;
   virtual void globalGridToModel(const GridPoint& gridPoint, ModelPoint& modelPoint)const;
   virtual void modelToGlobalGrid(const ModelPoint& modelPoint, 
                                  GridPoint& gridPoint)const;
   virtual void widthHeightInModelSpace(const ossimPlanetTerrainTileId& tileId, osg::Vec2d& deltaXY)const;
   virtual bool findGridBound(ossim_uint32 face,
                              const ModelPoint& minPoint,
                              const ModelPoint& maxPoint,
                              GridBound& bound,
                              ossim_uint32 numberOfPoints=3)const;
   virtual ossim_uint32 numberOfFaces()const;
   virtual ossimPlanetGridUtility* newBackwardCompatableGrid(ossim_uint32 width,
                                                             ossim_uint32 height)const;
   virtual bool isPolar(const ossimPlanetTerrainTileId& id)const;
   
protected:
   virtual void globalGridToModelLat45(const GridPoint& gridPoint, ModelPoint& modelPoint)const;
   virtual void globalGridToModelLat67_5(const GridPoint& gridPoint, ModelPoint& modelPoint)const;
   
   ossim_uint32 theEquatorialCols;
   ossim_uint32 theEquatorialRows;
};

class OSSIMPLANET_DLL ossimPlanetPlaneGrid2 : public ossimPlanetGrid
{
public:
   ossimPlanetPlaneGrid2()
   :ossimPlanetGrid(ossimPlanetGrid::GEODETIC_MODEL)
   {
   }
   virtual void getRootIds(TileIds &ids) const;
   virtual void globalGridToModel(const GridPoint& gridPoint, ModelPoint& modelPoint)const;
   virtual void modelToGlobalGrid(const ModelPoint& modelPoint, 
                                  GridPoint& gridPoint)const;
   virtual void widthHeightInModelSpace(const ossimPlanetTerrainTileId& tileId, osg::Vec2d& deltaXY)const;
   virtual bool findGridBound(ossim_uint32 face,
                              const ModelPoint& minPoint,
                              const ModelPoint& maxPoint,
                              GridBound& bound,
                              ossim_uint32 numberOfPoints=3)const;
   virtual ossim_uint32 numberOfFaces()const;
   virtual ossimPlanetGridUtility* newBackwardCompatableGrid(ossim_uint32 width,
                                                             ossim_uint32 height)const;
   virtual bool isPolar(const ossimPlanetTerrainTileId& /*id*/)const
   {
      return false;
   }
  
};

class OSSIMPLANET_DLL ossimPlanetAdjustableCubeGrid : public ossimPlanetCubeGrid2
{
public:
   enum CapLocation
   {
      LOW_CAP = 0,     // 45 degree lat, default to Cube map 
      MEDIUM_LOW_CAP,  // 67.5 degree lat
      MEDIUM_CAP,      // 78.75 degree lat
      MEDIUM_HIGH_CAP, // 84.375 degree lat
      HIGH_CAP // 87.1875 degree lat
   };
   
   ossimPlanetAdjustableCubeGrid(CapLocation location = MEDIUM_CAP);
   virtual void setCapLocation(CapLocation location);
   virtual void getRootIds(TileIds &ids) const;
   virtual void globalGridToModel(const GridPoint& gridPoint, ModelPoint& modelPoint)const;
   virtual void modelToGlobalGrid(const ModelPoint& modelPoint, 
                                  GridPoint& gridPoint)const;
   virtual void widthHeightInModelSpace(const ossimPlanetTerrainTileId& tileId, osg::Vec2d& deltaXY)const;
   virtual bool findGridBound(ossim_uint32 face,
                              const ModelPoint& minPoint,
                              const ModelPoint& maxPoint,
                              GridBound& bound,
                              ossim_uint32 numberOfPoints=3)const;
   virtual ossim_uint32 numberOfFaces()const;
   virtual bool isPolar(const ossimPlanetTerrainTileId& id)const;
   
protected:
   CapLocation theCapLocation;
   ossim_float64 thePolarLat;
   ossim_float64 theUpperEquatorialBandLatDelta;
   ossim_float64 thePolarWidth;
};
#endif
