#ifndef ossimPlanetElevationDatabaseGroup_HEADER
#define ossimPlanetElevationDatabaseGroup_HEADER
#include <ossimPlanet/ossimPlanetTextureLayerGroup.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetElevationDatabaseGroup : public ossimPlanetTextureLayerGroup
{
public:
   ossimPlanetElevationDatabaseGroup();

   ossimPlanetElevationDatabaseGroup(const ossimPlanetElevationDatabaseGroup& src);
   virtual ossimPlanetTextureLayer* dup()const;
   virtual ossimPlanetTextureLayer* dupType()const;
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 width,
                                                     ossim_uint32 height,
                                                     const ossimPlanetTerrainTileId& tileId,
                                                     const ossimPlanetGrid& theGrid,
                                                     ossim_int32 padding=0);
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 level,
                                                     ossim_uint64 row,
                                                     ossim_uint64 col,
                                                     const ossimPlanetGridUtility& utility);

   virtual bool replaceLayer(ossim_uint32 idx,
                             osg::ref_ptr<ossimPlanetTextureLayer> layer);
   
   virtual bool addTop(osg::ref_ptr<ossimPlanetTextureLayer> layer);
   virtual bool addBeforeIdx(ossim_uint32 idx, osg::ref_ptr<ossimPlanetTextureLayer> layer);
   virtual bool addBeforeLayer(const osg::ref_ptr<ossimPlanetTextureLayer> beforeLayer,
                       osg::ref_ptr<ossimPlanetTextureLayer> layerToAdd);
   virtual bool addAfterIdx(ossim_int32 idx, osg::ref_ptr<ossimPlanetTextureLayer> layer);
   virtual bool addAfterLayer(const osg::ref_ptr<ossimPlanetTextureLayer> afterLayer,
                      osg::ref_ptr<ossimPlanetTextureLayer> layerToAdd);
   virtual bool addBottom(osg::ref_ptr<ossimPlanetTextureLayer> layer);

   virtual void setGeoRefModel(osg::ref_ptr<ossimPlanetGeoRefModel> model);
   virtual void setFillNullWithGeoidOffsetFlag(bool flag);
   bool fillNullWithGeoidOffsetFlag()const
   {
      return theFillNullWithGeoidOffsetFlag;
   }

protected:
   /**
    *
    * Merging elevation cells are done differently.  We will override the base classes mergeImage.
    * 
    */ 
   virtual void mergeImage(ossimPlanetImage* result,
                           const ossimPlanetImage* source)const;   
   void setPixelStatus(ossimPlanetImage* image);
  
   osg::ref_ptr<ossimPlanetGeoRefModel> theGeoRefModel;
   bool                             theFillNullWithGeoidOffsetFlag;
};

#endif
