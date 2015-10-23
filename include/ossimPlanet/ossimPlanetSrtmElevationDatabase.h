#ifndef ossimPlanetSrtmElevationDatabase_HEADER
#define ossimPlanetSrtmElevationDatabase_HEADER
#include <ossimPlanet/ossimPlanetElevationDatabase.h>
#include <osg/Timer>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/imaging/ossimOrthoImageMosaic.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/elevation/ossimSrtmHandler.h>
class OSSIMPLANET_DLL ossimPlanetSrtmElevationDatabase : public ossimPlanetElevationDatabase
{
public:
   ossimPlanetSrtmElevationDatabase();
   ossimPlanetSrtmElevationDatabase(const ossimPlanetSrtmElevationDatabase& src);
   virtual ~ossimPlanetSrtmElevationDatabase();
   virtual ossimPlanetTextureLayer* dup()const;
   virtual ossimPlanetTextureLayer* dupType()const;
   virtual ossimPlanetTextureLayerStateCode updateExtents();
   virtual void updateStats()const;
   virtual void resetStats()const;
   virtual ossimPlanetTextureLayerStateCode open(const std::string& location);
   virtual bool hasTexture(ossim_uint32 width,
                           ossim_uint32 height,
                           const ossimPlanetTerrainTileId& tileId,
                           const ossimPlanetGrid& grid);
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
   class SrtmInfo : public osg::Referenced
   {
   public:
      double                        theMinLat, theMinLon, theMaxLat, theMaxLon;
      std::string                   theFilename;
      ossimRefPtr<ossimSrtmHandler> theSrtmHandler;
      osg::Timer_t                  theTimeStamp;
   };
   typedef std::map<std::string, osg::ref_ptr<SrtmInfo> > SrtmFilePointerList;


   osg::ref_ptr<ossimPlanetSrtmElevationDatabase::SrtmInfo> getInfo(const std::string& srtmName);
   void shrinkFilePointers();
   ossimFilename buildFilename(double lat, double lon)const;
   osg::ref_ptr<ossimPlanetSrtmElevationDatabase::SrtmInfo> findSrtmInfo(const std::string& srtmName);
   
   std::string                            theLocation;
   mutable bool                           theOpenFlag;
   ossim_uint32                           theMaxOpenFiles;
   ossim_uint32                           theMinOpenFiles;
   SrtmFilePointerList                    theFilePointers;
/*    ossimRefPtr<ossimMapProjection>        theProjection; */
/*    ossimRefPtr<ossimImageRenderer>        theRenderer; */
/*    ossimRefPtr<ossimOrthoImageMosaic>     theMosaic; */
/*    ossim_float32                          theNullHeightValue; */
   
};

#endif
