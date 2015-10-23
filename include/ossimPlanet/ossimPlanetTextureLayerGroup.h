#ifndef ossimPlanetTextureLayerGroup_HEADER
#define ossimPlanetTextureLayerGroup_HEADER
#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <vector>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/ReentrantMutex>

class OSSIMPLANET_DLL ossimPlanetTextureLayerGroup : public ossimPlanetTextureLayer
{
public:
   ossimPlanetTextureLayerGroup();
   ossimPlanetTextureLayerGroup(const ossimPlanetTextureLayerGroup& src);
   ~ossimPlanetTextureLayerGroup();
   virtual ossimPlanetTextureLayer* dup()const;
   virtual ossimPlanetTextureLayer* dupType()const;
   virtual ossimString getClassName()const;
   virtual ossimPlanetTextureLayerStateCode updateExtents();
   virtual void resetStats()const;
   virtual void updateStats()const;
   
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
   
   virtual ossimPlanetTextureLayerGroup* asGroup();
   virtual const ossimPlanetTextureLayerGroup* asGroup()const;
   virtual bool swapLayers(ossim_uint32 idx1, ossim_uint32 idx2, bool notifyFlag=true);
   virtual bool swapLayers(osg::ref_ptr<ossimPlanetTextureLayer> layer1, 
                           osg::ref_ptr<ossimPlanetTextureLayer> layer2,
                           bool notifyFlag=true);
   virtual bool replaceLayer(ossim_uint32 idx,
                             osg::ref_ptr<ossimPlanetTextureLayer> layer,
                             bool notifyFlag = true);
   
   virtual bool addTop(osg::ref_ptr<ossimPlanetTextureLayer> layer, bool notifyFlag=true);
   virtual bool addBeforeIdx(ossim_uint32 idx, osg::ref_ptr<ossimPlanetTextureLayer> layer, bool notifyFlag=true);
   virtual bool addBeforeLayer(const osg::ref_ptr<ossimPlanetTextureLayer> beforeLayer,
                               osg::ref_ptr<ossimPlanetTextureLayer> layerToAdd, bool notifyFlag=true);
   virtual bool addAfterIdx(ossim_int32 idx, osg::ref_ptr<ossimPlanetTextureLayer> layer, bool notifyFlag=true);
   virtual bool addAfterLayer(const osg::ref_ptr<ossimPlanetTextureLayer> afterLayer,
                              osg::ref_ptr<ossimPlanetTextureLayer> layerToAdd, bool notifyFlag=true);
   virtual bool addBottom(osg::ref_ptr<ossimPlanetTextureLayer> layer, bool notifyFlag=true);
   osg::ref_ptr<ossimPlanetTextureLayer> removeLayer(ossim_uint32 idx, bool notifyFlag=true);
   void removeLayers(ossim_uint32 idx, ossim_uint32 length=1, bool notifyFlag=true);
   bool removeLayer(osg::ref_ptr<ossimPlanetTextureLayer> layer, bool notifyFlag=true);
   
   ossim_int32 findLayerIndex(osg::ref_ptr<ossimPlanetTextureLayer> layer)const;
   bool containsLayer(osg::ref_ptr<ossimPlanetTextureLayer> layer)const;
   
   ossim_uint32 numberOfLayers()const;
   const osg::ref_ptr<ossimPlanetTextureLayer> layer(ossim_uint32 idx)const;
   osg::ref_ptr<ossimPlanetTextureLayer> layer(ossim_uint32 idx);
   
   virtual ossimPlanetTextureLayer* findLayerByName(const ossimString& layerName,
                                                    bool recurseFlag=false);
   virtual const ossimPlanetTextureLayer* findLayerByName(const ossimString& layerName,
                                                          bool recurseFlag=false)const;
   
   virtual ossimPlanetTextureLayer* findLayerByNameAndId(const ossimString& layerName,
                                                         const ossimString& layerId);
   virtual const ossimPlanetTextureLayer* findLayerByNameAndId(const ossimString& layerName,
                                                               const ossimString& layerId)const;
   virtual ossimPlanetTextureLayer* findLayerById(const ossimString& layerId,
                                                  bool recurseFlag=false);
   virtual const ossimPlanetTextureLayer* findLayerById(const ossimString& layerId,
                                                        bool recurseFlag=false)const;
   
   /**
    * The empty tile color is used to return a tile that has color if there is 
    * no texture in the group.  The color is a normalized float vector and the normal
    * use is to pass in (osg::Vec4f(1.0,1.0,1.0,1.0),0).  This will enable the color if
    * the level passed in is >= 0.
    */
   void setFillNullOrEmptyTileMaxLevel(ossim_int32 maxLevel=0);
   
   /**
    * Set background fill color for partial tiles.  If a tile is partial it 
    * will fill any invalid color with this value.
    */
   void setBackgroundColor(const osg::Vec4f& color);
   void setFillTranslucentPixelsWithBackground(bool on=true);
   
   virtual void sortByGsd();
   virtual ossimRefPtr<ossimXmlNode> saveXml(bool recurseFlag=true)const;
   virtual bool loadXml(ossimRefPtr<ossimXmlNode> node);
   
protected:
   bool containsLayerNoMutex(osg::ref_ptr<ossimPlanetTextureLayer> layer)const;
   bool removeLayerNoMutex(osg::ref_ptr<ossimPlanetTextureLayer> layer, bool notifyFlag);
   osg::ref_ptr<ossimPlanetTextureLayer> removeLayerNoMutex(ossim_uint32 idx, bool notifyFlag);
   std::vector<osg::ref_ptr<ossimPlanetTextureLayer> > removeLayersNoMutex(ossim_uint32 idx, ossim_uint32 length=1, bool notifyFlag=true);
   ossim_int32 findLayerIndexNoMutex(osg::ref_ptr<ossimPlanetTextureLayer> layer)const;
   
   mutable OpenThreads::Mutex theChildrenListMutex;
   std::vector<osg::ref_ptr<ossimPlanetTextureLayer> > theChildrenList;
   osg::ref_ptr<ossimPlanetTextureLayerCallback>    theChildListener;
   
   osg::Vec4f  theBackgroundColor;
   ossim_int32 theFillEmptyNullTileMaxLevel;
   bool        theFillTranslucentPixelsWithBackgroundEnabled;
   
   
};

#endif
