#ifndef ossimPlanetElevationDatabase_HEADER
#define ossimPlanetElevationDatabase_HEADER
#include <osg/Referenced>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/ossimPlanetGridUtility.h>
#include <ossimPlanet/ossimPlanetExtents.h>
#include <string>
#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>

class OSSIMPLANET_DLL ossimPlanetElevationDatabase : public ossimPlanetTextureLayer
{
public:
   ossimPlanetElevationDatabase()
      :ossimPlanetTextureLayer(),
   theFillNullWithGeoidOffsetFlag(false)
   {}
   ossimPlanetElevationDatabase(const ossimPlanetElevationDatabase& src)
      :   ossimPlanetTextureLayer(src)
   {
   }
   
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(const ossimPlanetTerrainTileId& tileId,
                                                     osg::ref_ptr<ossimPlanetGrid> theGrid);
      
   virtual ossimPlanetTextureLayerStateCode open(const std::string& location)=0;
   
   virtual const std::string& getLocation()const
   {
      return theLocation;
   }
   bool fillNullWithGeoidOffsetFlag()const
   {
      return theFillNullWithGeoidOffsetFlag;
   }
   virtual void setFillNullWithGeoidOffsetFlag(bool flag)
   {
      theFillNullWithGeoidOffsetFlag = flag;
   }
   virtual void setGeoRefModel(osg::ref_ptr<ossimPlanetGeoRefModel> model)
   {
      theGeoRefModel = model.get();
   }
   ossimPlanetGeoRefModel* geoRefModel()const
   {
      return theGeoRefModel.get();
   }
   virtual void sortByGsd()
   {
      
   }
protected:
   /**
    * 
    * destData is always assumed to be 1 band float and both buffers are of the same size.  It will make sure it's float before
    * merging the src into the destination.  This function basically copys good pixels into
    * non null regions of the destination.
    * 
    */ 
   void mergeDataObjects(ossimRefPtr<ossimImageData> destData,
                         ossimRefPtr<ossimImageData> srcData);
   
   std::string                      theLocation;
   bool                             theFillNullWithGeoidOffsetFlag;
   osg::ref_ptr<ossimPlanetGeoRefModel> theGeoRefModel;
};

#endif
