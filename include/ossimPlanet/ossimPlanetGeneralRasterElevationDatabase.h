#ifndef ossimPlanetGeneralRasterElevationDatabase_HEADER
#define ossimPlanetGeneralRasterElevationDatabase_HEADER
#include <ossimPlanet/ossimPlanetElevationDatabase.h>
#include <osg/Timer>
#include <ossim/elevation/ossimGeneralRasterElevHandler.h>
class OSSIMPLANET_DLL ossimPlanetGeneralRasterElevationDatabase : public ossimPlanetElevationDatabase
{
public:
   ossimPlanetGeneralRasterElevationDatabase();
   ossimPlanetGeneralRasterElevationDatabase(const ossimPlanetGeneralRasterElevationDatabase& src);
   virtual ~ossimPlanetGeneralRasterElevationDatabase();
   virtual ossimPlanetTextureLayer* dup()const;
   virtual ossimPlanetTextureLayer* dupType()const;
   virtual ossimPlanetTextureLayerStateCode updateExtents();
   virtual void updateStats()const;
   virtual void resetStats()const;
   virtual ossimPlanetTextureLayerStateCode open(const std::string& location);
   virtual bool hasTexture(ossim_uint32 width,
                           ossim_uint32 height,
                           const ossimPlanetTerrainTileId& tileId,
                           const ossimPlanetGrid& theGrid);
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 width,
                                                     ossim_uint32 height,
                                                     const ossimPlanetTerrainTileId& tileId,
                                                     const ossimPlanetGrid& theGrid,
                                                     ossim_int32 padding=0);
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 level,
                                                     ossim_uint64 row,
                                                     ossim_uint64 col,
                                                     const ossimPlanetGridUtility& utility);
   
protected:
   class GeneralRasterInfo : public osg::Referenced
   {
   public:
      double                        theMinLat, theMinLon, theMaxLat, theMaxLon;
      std::string                   theFilename;
      ossimRefPtr<ossimGeneralRasterElevHandler> theGeneralRasterHandler;
      bool                          theOpenFlag;
      osg::Timer_t                  theTimeStamp;
   };
   typedef std::vector<osg::ref_ptr<ossimPlanetGeneralRasterElevationDatabase::GeneralRasterInfo> > GeneralRasterFilePointerList;

   osg::ref_ptr<ossimPlanetGeneralRasterElevationDatabase::GeneralRasterInfo> getHandlerInfo(const double& lat,
                                                                                             const double& lon);
   //void shrinkFilePointers();
/*    ossimFilename buildFilename(double lat, double lon)const; */
/*    osg::ref_ptr<ossimPlanetGeneralRasterElevationDatabase::SrtmInfo> findSrtmInfo(const std::string& srtmName); */
   
   std::string                            theLocation;
   mutable bool                           theOpenFlag;
   ossim_uint32                           theMaxOpenFiles;
   ossim_uint32                           theMinOpenFiles;
   GeneralRasterFilePointerList           theFilePointers;
   mutable ossim_int32                    theCurrentInfoIdx;
/*    ossimRefPtr<ossimMapProjection>        theProjection; */
/*    ossimRefPtr<ossimImageRenderer>        theRenderer; */
/*    ossimRefPtr<ossimOrthoImageMosaic>     theMosaic; */
/*    ossim_float32                          theNullHeightValue; */
    
};

#endif
