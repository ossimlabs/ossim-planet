#ifndef ossimPlanetLandReaderWriter_HEADER
#define ossimPlanetLandReaderWriter_HEADER
#include <string>
#include <ossim/base/ossimKeywordlist.h>
#include <osgDB/ReaderWriter>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/Texture2D>
#include "ossimPlanetGridUtility.h"
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimFilename.h>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetConstants.h>
#include <ossimPlanet/ossimPlanetLandNormalType.h>
/* #include <ossimPlanet/ossimPlanetElevationGrid.h> */
#include <ossimPlanet/ossimPlanetBoundingBox.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/ossimPlanetLandCache.h>
#include <ossimPlanet/ossimPlanetTextureLayerGroup.h>
#include <ossimPlanet/ossimPlanetShaderProgramSetup.h>
#include <ossimPlanet/ossimPlanetElevationDatabaseGroup.h>
#include <ossimPlanet/ossimPlanetLandCullCallback.h>
class ossimPlanetLand;
class ossimPlanetPagedLandLod;
class ossimPlanetPagedLandLodCullCallback;
class ossimPlanetTexture2D;
class ossimPlanetLandReaderWriter : public osgDB::ReaderWriter
{
public:
   typedef std::map<std::string, char> requestStringMapType;
   ossimPlanetLandReaderWriter();
   
   virtual const char* className()const;
   virtual ReadResult readNode(const std::string& fileName, const Options*)const;

   ossim_uint32 getId()const
   {
      return theId;
   }
   
   std::string createDbString(ossim_uint32 level,
                              ossim_uint64 row,
                              ossim_uint64 col)const;

   void setModel(ossimPlanetGeoRefModel* geoModel);
   const ossimPlanetGeoRefModel* model()const{return theModel.get();};
   ossimPlanetGeoRefModel* model(){return theModel.get();};
   void setLandNodeCullCallback(ossimPlanetLandCullCallback* callback);
   bool getElevationEnabledFlag()const;
   void setElevationEnabledFlag(bool elevationFlag);

   ossim_float64 getHeightExag()const;
   void setHeightExag(ossim_float64 exag);

   ossim_uint32 getElevationPatchSize()const;
   void setElevationPatchSize(ossim_uint32 densityLevel);

   ossim_uint32 getMaxLevelDetail()const;
   void setMaxLevelDetail(ossim_uint32 maxLevelDetail);

   ossimFilename getElevationCacheDir()const;
   void setElevationCacheDir(const ossimFilename& cacheDir);

   const ossimPlanetGridUtility* gridUtility()const;
   void setGridUtility(ossimPlanetGridUtility* utility);
   
//   void setMultiTextureLayers(osg::ref_ptr<ossimPlanetTextureLayerGroup> textureLayers);
   void setReferenceLayer(osg::ref_ptr<ossimPlanetTextureLayerGroup> reference);
   void setOverlayLayers(osg::ref_ptr<ossimPlanetTextureLayerGroup> overlays);
   void setMipMappingFlag(bool flag);
   bool getMipMappingFlag()const;

   void setMultiTextureEnableFlag(bool flag);
   bool getMultiTextureEnableFlag()const;

   void setLandCache(osg::ref_ptr<ossimPlanetLandCache> landCache);

   void setElevationDatabase(osg::ref_ptr<ossimPlanetElevationDatabaseGroup> databaseGroup);
   osg::ref_ptr<ossimPlanetElevationDatabaseGroup> getElevationGroup();
   const osg::ref_ptr<ossimPlanetElevationDatabaseGroup> getElevationGroup()const;
   
protected:
   enum ossimPlanetLandReaderWriterGeomType
   {
      ossimPlanetLandReaderWriterGeomType_NONE    = 0,
      ossimPlanetLandReaderWriterGeomType_TEXTURE = 1,
      ossimPlanetLandReaderWriterGeomType_GEOM    = 2,
      ossimPlanetLandReaderWriterGeomType_LOD     = 4
   };
   mutable ossimPlanetReentrantMutex theMutex;
   ossim_uint32 theId;
   osg::ref_ptr<ossimPlanetGeoRefModel> theModel;
   ossimPlanetLandType theLandType;
   ossim_uint32 theMaxLevelDetail;
   double theHeightExag;
/*    ossim_uint32 theElevationDensityLevel; */
   ossim_uint32 theElevationPatchSize;
   bool theElevationEnabledFlag;
   ossimFilename theElevationCacheDir;
   bool          theMipMappingFlag;
   osg::ref_ptr<ossimPlanetGridUtility>  theGrid;
   osg::ref_ptr<ossimPlanetGridUtility>  theElevationGrid;
   osg::ref_ptr<ossimPlanetTextureLayerGroup> theReferenceLayer;
   osg::ref_ptr<ossimPlanetTextureLayerGroup> theOverlayLayers;
/*    osg::ref_ptr<ossimPlanetTextureLayer> theTextureLayer; */
   osg::ref_ptr<ossimPlanetElevationDatabaseGroup> theElevationDatabase;
   bool                                        theMultiTextureEnableFlag;
   osg::ref_ptr<ossimPlanetLandCache> theLandCache;
   osg::ref_ptr<ossimPlanetLandCullCallback> theCullNodeCallback;
   mutable osg::ref_ptr<osg::Texture2D> theBlankTexture;
   /*    virtual osgDB::ReaderWriter::ReadResult local_readNode(const std::string& filename, */
/*                                                           const Options* options)const; */
   
   virtual bool extractValues(const std::string& filename,
                              ossim_uint32 &level,
                              ossim_uint32 &row,
                              ossim_uint32 &col,
                              ossim_uint32 &id,
                              ossimPlanetLandReaderWriterGeomType &geomType)const;
   
   void newTexture(std::vector<osg::ref_ptr<osg::Texture2D> >& textures,
                   ossim_uint32 level,
                   ossim_uint32 row,
                   ossim_uint32 col,
                   ossimPlanetImage::ossimPlanetImageStateType& textureState)const;
   
   osg::ref_ptr<osg::MatrixTransform> newGeometry(ossim_uint32 level,
                                                  ossim_uint32 row,
                                                  ossim_uint32 col,
                                                  std::vector<osg::ref_ptr<osg::Texture2D> >& textures,
                                                  osg::ref_ptr<ossimPlanetImage> elevationGrid,
/*                                                   osg::ref_ptr<ossimPlanetElevationGrid> elevationGrid, */
                                                  ossimPlanetBoundingBox& box,
                                                  bool& useClusterCullingCallback,
                                                  osg::Vec3d& clusterControlPoint,
                                                  osg::Vec3d& clusterCenterNormal,
                                                  double& minDotProduct,
                                                  double& maxClusterCullingRadius)const;
   virtual void createPoints(ossim_uint32 level,
                             ossim_uint32 row,
                             ossim_uint32 col,
                             osg::ref_ptr<ossimPlanetImage> elevationGrid,
/*                              osg::ref_ptr<ossimPlanetElevationGrid> elevationGrid, */
                             osg::Vec3Array *verts,
                             osg::Vec3Array *norms,
                             osg::Vec2Array *tcoords,
                             osg::Matrixd& localToWorld,
                             ossimPlanetBoundingBox& box,
                             bool& useClusterCullingCallback,
                             osg::Vec3d& clusterControlPoint,
                             osg::Vec3d& clusterCenterNormal,
                             double& minDotProduct,
                             double& maxClusterCullingRadius)const;

  
   void initSupportAttributes(ossim_uint32 level,
                              ossim_uint32 row,
                              ossim_uint32 col,
                              osg::Vec3d& centerPoint,
                              osg::Vec3d& ulPoint,
                              osg::Vec3d& urPoint,
                              osg::Vec3d& lrPoint,
                              osg::Vec3d& llPoint,
                              osg::Vec3d& centerNormal,
                              osg::Vec3d& ulNormal,
                              osg::Vec3d& urNormal,
                              osg::Vec3d& lrNormal,
                              osg::Vec3d& llNormal,
                              ossimPlanetBoundingBox& box,
                              std::vector<ossimPlanetBoundingBox>& childrenBounds,
                              const osg::MatrixTransform& transform)const;
   void createBounds(ossim_uint32 level,
                     ossim_uint32 row,
                     ossim_uint32 col,
                     ossimPlanetBoundingBox& bounds,
                     const osg::MatrixTransform& transform)const;
   osg::Vec3d normal(double x, double y, double z)const;
   
/*    osg::ref_ptr<ossimPlanetElevationGrid> newElevation(ossim_uint32 level, */
/*                                                      ossim_uint32 row, */
/*                                                      ossim_uint32 col)const; */
/*    osg::ref_ptr<ossimPlanetElevationGrid> getCachedElevation(ossim_uint32 level, */
/*                                                            ossim_uint32 row, */
/*                                                            ossim_uint32 col)const; */
   osg::ref_ptr<ossimPlanetImage> newElevation(ossim_uint32 level,
                                               ossim_uint32 row,
                                               ossim_uint32 col)const;
   osg::ref_ptr<ossimPlanetImage> getCachedElevation(ossim_uint32 level,
                                                     ossim_uint32 row,
                                                     ossim_uint32 col)const;
/*    void writeElevationToCache(ossim_uint32 level, */
/*                               ossim_uint32 row, */
/*                               ossim_uint32 col, */
/*                               osg::ref_ptr<ossimPlanetElevationGrid> elevation)const; */
   void writeElevationToCache(ossim_uint32 level,
                              ossim_uint32 row,
                              ossim_uint32 col,
                              osg::ref_ptr<ossimPlanetImage> elevation)const;
   
};
#endif
