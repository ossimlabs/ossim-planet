#include <ossimPlanet/ossimPlanetCacheTextureLayer.h>

ossimPlanetCacheTextureLayer::ossimPlanetCacheTextureLayer()
{

}

ossimPlanetCacheTextureLayer::ossimPlanetCacheTextureLayer(const ossimPlanetCacheTextureLayer& src)
:m_textureLayer(src.m_textureLayer.get()),
m_cache(src.m_cache.get())
{

}

ossimPlanetTextureLayer* ossimPlanetCacheTextureLayer::dup()const
{
  return new ossimPlanetCacheTextureLayer(*this);
}

ossimPlanetTextureLayer* ossimPlanetCacheTextureLayer::dupType()const
{
  return new ossimPlanetCacheTextureLayer();
}

ossimString ossimPlanetCacheTextureLayer::getClassName()const
{
  return "ossimPlanetCacheTextureLayer";
}

ossimPlanetTextureLayerStateCode ossimPlanetCacheTextureLayer::updateExtents()
{
  ossimPlanetTextureLayerStateCode result = ossimPlanetTextureLayer_NO_SOURCE_DATA;
  if(m_textureLayer.valid())
  {
    result = m_textureLayer->updateExtents();
    theExtents = m_textureLayer->getExtents();
  }

  return result;
}

void ossimPlanetCacheTextureLayer::updateStats()const
{

}

void ossimPlanetCacheTextureLayer::setTextureLayer(ossimPlanetTextureLayer* layer)
{
  m_textureLayer = layer;
}

void ossimPlanetCacheTextureLayer::setCache(ossimPlanetImageCache* cache)
{
  m_cache = cache;
}


bool ossimPlanetCacheTextureLayer::hasTexture(ossim_uint32 width,
                                              ossim_uint32 height,
                                              const ossimPlanetTerrainTileId& tileId,
                                              const ossimPlanetGrid& grid)
{
  bool result = false;
  if(m_cache.valid())
  {
    result = m_cache->hasImage(tileId);
  }
  if(!result)
  {
    if(m_textureLayer.valid())
    {
      result = m_textureLayer->hasTexture(width, height, tileId, grid);
    }
  }

  return result;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetCacheTextureLayer::getTexture(ossim_uint32 width,
                                                                        ossim_uint32 height,
                                                                        const ossimPlanetTerrainTileId& tileId,
                                                                        const ossimPlanetGrid& grid,
                                                                        ossim_int32 padding)
{
  if(!enableFlag())
  {
    return 0;
  }
  osg::ref_ptr<ossimPlanetImage> result;
  if(m_cache.valid())
  {
    result = m_cache->get(tileId);
  }

  if(!result.valid()&&m_textureLayer.valid())
  {
    result = m_textureLayer->getTexture(width, height, tileId, grid, padding);

    if(result.valid()&&m_cache.valid())
    {
      m_cache->addOrUpdate(result.get());
    }
  }
  result = applyBrightnessContrast(result.get(), true);

  return result;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetCacheTextureLayer::getTexture(ossim_uint32 /* level */,
                                                                        ossim_uint64 /* row */,
                                                                        ossim_uint64 /* col */,
                                                                        const ossimPlanetGridUtility& /* utility */)
{
  return 0;
}

const osg::ref_ptr<ossimPlanetLookAt> ossimPlanetCacheTextureLayer::getLookAt()const
{
  return theLookAt.get();
}

void ossimPlanetCacheTextureLayer::setLookAt(osg::ref_ptr<ossimPlanetLookAt> lookAt)
{
  theLookAt = lookAt.get();
  if(m_textureLayer.valid())
  {
    m_textureLayer->setLookAt(lookAt);
  }
}


void ossimPlanetCacheTextureLayer::getDateRange(ossimDate& minDate,
                               ossimDate& maxDate)const
{
  ossimPlanetTextureLayer::getDateRange(minDate, maxDate);
}

double ossimPlanetCacheTextureLayer::getApproximateHypotneusLength()const
{
  if(m_textureLayer.valid())
  {
    return m_textureLayer->getApproximateHypotneusLength();
  }

  return ossimPlanetTextureLayer::getApproximateHypotneusLength();
}

void ossimPlanetCacheTextureLayer::getCenterLatLonLength(double& centerLat,
                                                         double& centerLon,
                                                         double& length)const
{
  if(m_textureLayer.valid())
  {
    m_textureLayer->getCenterLatLonLength(centerLat, centerLon, length);
  }

  ossimPlanetTextureLayer::getCenterLatLonLength(centerLat, centerLon, length);

}

void ossimPlanetCacheTextureLayer::setEnableFlag(bool flag)
{
  ossimPlanetTextureLayer::setEnableFlag(flag);
}

void ossimPlanetCacheTextureLayer::setFilterType(const ossimString& filterType)
{
  ossimPlanetTextureLayer::setFilterType(filterType);
  if(m_textureLayer.valid())
  {
    m_textureLayer->setFilterType(filterType);
  }
}

void ossimPlanetCacheTextureLayer::getMetadata(ossimRefPtr<ossimXmlNode> metadata)const
{
  if(m_textureLayer.valid())
  {
    m_textureLayer->getMetadata(metadata.get());
  }
}

ossimRefPtr<ossimXmlNode> ossimPlanetCacheTextureLayer::saveXml(bool recurseFlag)const
{
  if(m_textureLayer.valid())
   {
     return m_textureLayer->saveXml(recurseFlag);
   }

  return 0;
}

bool ossimPlanetCacheTextureLayer::loadXml(ossimRefPtr<ossimXmlNode> node)
{
  if(m_textureLayer.valid())
    {
      return m_textureLayer->loadXml(node.get());
    }

   return false;

}

void ossimPlanetCacheTextureLayer::resetLookAt()
{
  if(m_textureLayer.valid())
  {
      m_textureLayer->resetLookAt();
      theLookAt = m_textureLayer->getLookAt();
  }

}

ossimPlanetTextureLayer* ossimPlanetCacheTextureLayer::findLayerByName(const ossimString& layerName,
                                                       bool recurseFlag)
{
  if(m_textureLayer.valid())
  {
    return m_textureLayer->findLayerByName(layerName, recurseFlag);
  }

  return 0;
}

const ossimPlanetTextureLayer* ossimPlanetCacheTextureLayer::findLayerByName(const ossimString& layerName,
                                                             bool recurseFlag)const
{
  if(m_textureLayer.valid())
  {
    return m_textureLayer->findLayerByName(layerName, recurseFlag);
  }

  return 0;

}

ossimPlanetTextureLayer* ossimPlanetCacheTextureLayer::findLayerByNameAndId(const ossimString& layerName,
                                                            const ossimString& layerId)
{
  if(m_textureLayer.valid())
  {
    return m_textureLayer->findLayerByNameAndId(layerName, layerId);
  }

  return 0;

}

const ossimPlanetTextureLayer* ossimPlanetCacheTextureLayer::findLayerByNameAndId(const ossimString& layerName,
                                                                  const ossimString& layerId)const
{
  if(m_textureLayer.valid())
  {
    return m_textureLayer->findLayerByNameAndId(layerName, layerId);
  }

  return 0;

}

ossimPlanetTextureLayer* ossimPlanetCacheTextureLayer::findLayerById(const ossimString& layerId,
                                                                     bool recurseFlag)
{
  if(m_textureLayer.valid())
  {
    return m_textureLayer->findLayerById(layerId, recurseFlag);
  }

  return 0;

}

const ossimPlanetTextureLayer* ossimPlanetCacheTextureLayer::findLayerById(const ossimString& layerId,
                                                                           bool recurseFlag)const
{
  if(m_textureLayer.valid())
  {
    return m_textureLayer->findLayerById(layerId, recurseFlag);
  }

  return 0;
}

