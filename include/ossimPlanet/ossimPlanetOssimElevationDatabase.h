#ifndef ossimPlanetDtedElevationDatabase_HEADER
#define ossimPlanetDtedElevationDatabase_HEADER
#include <ossimPlanet/ossimPlanetElevationDatabase.h>
#include <ossim/elevation/ossimGeneralRasterElevationDatabase.h>
#include <ossim/elevation/ossimImageElevationDatabase.h>
#include <osg/Timer>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossim/elevation/ossimElevationDatabase.h>

class OSSIMPLANET_DLL ossimPlanetOssimElevationDatabase : public ossimPlanetElevationDatabase
{
public:
   ossimPlanetOssimElevationDatabase();
   ossimPlanetOssimElevationDatabase(const ossimPlanetOssimElevationDatabase& src);
   virtual ~ossimPlanetOssimElevationDatabase();
   virtual ossimPlanetTextureLayer* dup()const;
   virtual ossimPlanetTextureLayer* dupType()const;
   virtual ossimPlanetTextureLayerStateCode updateExtents();
   virtual void updateStats()const;
   virtual void resetStats()const;
   
   virtual ossimPlanetTextureLayerStateCode open(const std::string& location);
   virtual void setDatabase(ossimElevationDatabase* database)
   {
      m_database = database;
      m_cellDatabaseFlag = ((!dynamic_cast<ossimGeneralRasterElevationDatabase*>(database) &&
                             !dynamic_cast<ossimImageElevationDatabase*>(database))&&
                            ((dynamic_cast<ossimElevationCellDatabase*>(database) != 0)));
      updateExtents();
   }
   virtual bool hasTexture(ossim_uint32 width,
                           ossim_uint32 height,
                           const ossimPlanetTerrainTileId& tileId,
                           const ossimPlanetGrid& grid);
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 width,
                                                     ossim_uint32 height,
                                                     const ossimPlanetTerrainTileId& tileId,
                                                     const ossimPlanetGrid& grid,
                                                     ossim_int32 padding=0);
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 /* level */,
                                                     ossim_uint64 /* row */,
                                                     ossim_uint64 /* col */,
                                                     const ossimPlanetGridUtility& /* utility */)
   {
      return 0;
   }
protected:
   virtual osg::ref_ptr<ossimPlanetImage> getTextureCellDatabase(ossim_uint32 width,
                                                     ossim_uint32 height,
                                                     const ossimPlanetTerrainTileId& tileId,
                                                     const ossimPlanetGrid& grid,
                                                     ossim_int32 padding=0);
   bool                                m_cellDatabaseFlag;
   ossimRefPtr<ossimElevationDatabase> m_database;
};
#endif
