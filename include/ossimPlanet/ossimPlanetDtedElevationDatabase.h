#ifndef ossimPlanetDtedElevationDatabase_HEADER
#define ossimPlanetDtedElevationDatabase_HEADER
#include <ossimPlanet/ossimPlanetElevationDatabase.h>
#include <ossim/elevation/ossimDtedHandler.h>
#include <osg/Timer>
#include <ossimPlanet/ossimPlanetExport.h>
#include <OpenThreads/ReentrantMutex>

class OSSIMPLANET_DLL ossimPlanetDtedElevationDatabase : public ossimPlanetElevationDatabase
{
public:
   ossimPlanetDtedElevationDatabase();
   ossimPlanetDtedElevationDatabase(const ossimPlanetDtedElevationDatabase& src);
   virtual ~ossimPlanetDtedElevationDatabase();
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
                                                     const ossimPlanetGrid& grid,
                                                     ossim_int32 padding=0);
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 level,
                                                     ossim_uint64 row,
                                                     ossim_uint64 col,
                                                     const ossimPlanetGridUtility& utility);
protected:
    class DtedInfo : public osg::Referenced
   {
   public:
      double                        theMinLat, theMinLon, theMaxLat, theMaxLon;
      double                        theNumLatPoints;
      double                        theNumLonLines;
      double                        theLatSpacing;
      double                        theLonSpacing;
      ossimFilename                 theFilename;
      ossimRefPtr<ossimDtedHandler> theHandler;
      osg::Timer_t                  theTimeStamp;
   };
   
   typedef std::map<std::string, osg::ref_ptr<DtedInfo> > DtedFilePointerList;


   osg::ref_ptr<ossimPlanetDtedElevationDatabase::DtedInfo> getInfo(const std::string& name);
   void shrinkFilePointers();
   ossimFilename buildFilename(double lat, double lon)const;
   osg::ref_ptr<ossimPlanetDtedElevationDatabase::DtedInfo> findDtedInfo(const std::string& name);
   double getHeightAboveMSL(osg::ref_ptr<DtedInfo> info, double lat, double lon)const;
   ossim_sint16 convertSignedMagnitude(ossim_uint16& s) const;
  
   std::string                            theLocation;
   mutable bool                           theOpenFlag;
   ossim_uint32                           theMaxOpenFiles;
   ossim_uint32                           theMinOpenFiles;
   DtedFilePointerList                    theFilePointers;
   ossim_float32                          theNullHeightValue;
   bool                                   theSwapBytesFlag;
   mutable OpenThreads::ReentrantMutex    theDtedInfoMutex;

};
inline ossim_sint16 ossimPlanetDtedElevationDatabase::convertSignedMagnitude(ossim_uint16& s) const
{
   // DATA_VALUE_MASK 0x7fff = 0111 1111 1111 1111
   // DATA_SIGN_MASK  0x8000 = 1000 0000 0000 0000
   
   // First check to see if the bytes need swapped.
   s = (theSwapBytesFlag ? ( ((s & 0x00ff) << 8) | ((s & 0xff00) >> 8) ) : s);
   
   // If the sign bit is set, mask it out then multiply by negative one.
   if (s & 0x8000)
   {
      return (static_cast<ossim_sint16>(s & 0x7fff) * -1);
   }
   
   return static_cast<ossim_sint16>(s);
}
#endif
