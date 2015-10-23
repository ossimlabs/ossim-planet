#ifndef ossimPlanetCacheTextureLayer_HEADER
#define ossimPlanetCacheTextureLayer_HEADER
#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <ossimPlanet/ossimPlanetCache.h>

class OSSIMPLANET_DLL ossimPlanetCacheTextureLayer : public ossimPlanetTextureLayer
{
public:
    ossimPlanetCacheTextureLayer();
    ossimPlanetCacheTextureLayer(const ossimPlanetCacheTextureLayer& src);
    virtual ossimPlanetTextureLayer* dup()const;
    virtual ossimPlanetTextureLayer* dupType()const;
    virtual ossimString getClassName()const;
    virtual ossimPlanetTextureLayerStateCode updateExtents();
    virtual void updateStats()const;
    void setTextureLayer(ossimPlanetTextureLayer* layer);
    void setCache(ossimPlanetImageCache* cache);


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

     virtual const osg::ref_ptr<ossimPlanetLookAt> getLookAt()const;
     virtual void setLookAt(osg::ref_ptr<ossimPlanetLookAt> lookAt);

     virtual void getDateRange(ossimDate& minDate,
                               ossimDate& maxDate)const;
     /**
      * Approximate length in meters of the hypotneus.  Just uses the getExtents
      * and ten approximates a meter gsd from the degree bounds.
      */
     virtual double getApproximateHypotneusLength()const;

     /**
      * Will return the center lat lon and the approximate hypotneus length in meters.
      */
     virtual void getCenterLatLonLength(double& centerLat,
                                        double& centerLon,
                                        double& length)const;
     virtual void setEnableFlag(bool flag);
     virtual void setFilterType(const ossimString& filterType);
     virtual void getMetadata(ossimRefPtr<ossimXmlNode> metadata)const;
     virtual ossimRefPtr<ossimXmlNode> saveXml(bool recurseFlag=true)const;
     virtual bool loadXml(ossimRefPtr<ossimXmlNode> node);
     virtual void resetLookAt();

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
protected:
    osg::ref_ptr<ossimPlanetTextureLayer> m_textureLayer;
    osg::ref_ptr<ossimPlanetImageCache>   m_cache;

};

#endif
