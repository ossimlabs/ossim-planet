#include <ossimPlanet/ossimPlanetCache.h>
#include <ossimPlanet/ossimPlanetJpegImage.h>
#include <ossim/imaging/ossimJpegWriter.h>
#include <ossim/imaging/ossimMemoryImageSource.h>
#include <ossim/imaging/ossimJpegTileSource.h>
void ossimPlanetMemoryImageCache::shrink()
{
   if(!exceedsMinCacheSize())
   {
      return;
   }
   osg::Timer* timer = osg::Timer::instance();
   ossim_float64 t;
   while(exceedsMinCacheSize())
   {
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
         t = timer->tick();
         TileMap::iterator iter = theTileCache.begin();
         TileMap::iterator currentToErase = iter;
         ossim_float64 delta = timer->delta_m(iter->second.theTimeStamp,
                                              t);
         ossim_float64 testDelta = delta;
         ++iter;
         while(iter!=theTileCache.end())
         {
            testDelta = timer->delta_m(iter->second.theTimeStamp,
                                       t);
            if(testDelta > delta)
            {
               currentToErase = iter;
               delta = testDelta;
            }
            ++iter;
         }
         if(currentToErase != theTileCache.end())
         {
            theCurrentCacheSize -= currentToErase->second.theImage->sizeInBytes();
            if(theCurrentCacheSize < 0) theCurrentCacheSize = 0;
            theTileCache.erase(currentToErase);
         }
      }
   }
}

void ossimPlanetMemoryImageCache::addOrUpdate(ossimPlanetImage* image)
{
   if(!image||(maxCacheSizeInBytes()<=0)||!enabledFlag())
   {
      return;
   }
   ossimPlanetTerrainTileId tileId = image->tileId();
   if(!get(tileId))
   {   
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
         TileInfo info;
         info.theImage = image;
         info.theTimeStamp = osg::Timer::instance()->tick();
         theTileCache.insert(std::make_pair(tileId, info));
         theCurrentCacheSize += image->sizeInBytes();
      }
   }
}

ossimPlanetImage* ossimPlanetMemoryImageCache::get(const ossimPlanetTerrainTileId& id)
{
   if(!enabledFlag()) return 0;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   TileMap::iterator tile = theTileCache.find(id);
   if(tile!=theTileCache.end())
   {
      tile->second.theTimeStamp = osg::Timer::instance()->tick();
      return tile->second.theImage.get();
   }
   
   return 0;
}

bool ossimPlanetMemoryImageCache::hasImage(const ossimPlanetTerrainTileId& id)const
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
  return (theTileCache.find(id) != theTileCache.end());
}

ossimPlanetDiskImageCache::ossimPlanetDiskImageCache()
{

}

bool ossimPlanetDiskImageCache::openDirectory(const ossimFilename& file, bool createIfNotExistsFlag)
{
  bool result = false;
  m_indexFileKwl.clear();
  if(createIfNotExistsFlag&&!file.exists())
  {
    file.createDirectory(true);
  }
  if(file.exists())
  {
    m_directory = file;
    m_indexFile = file.dirCat("cache.idx");
    if(m_indexFile.exists())
    {
      m_indexFileKwl.addFile(m_indexFile.c_str());
    }

    result = true;
  }

  return result;
}

void ossimPlanetDiskImageCache::clean()
{
}

void ossimPlanetDiskImageCache::shrink()
{
}

void ossimPlanetDiskImageCache::addOrUpdate(ossimPlanetImage* image)
{
  ossimFilename tileFile = m_directory.dirCat(buildTileFile(image->tileId()));

  ossimFilename directory = tileFile.path();
  if(!directory.exists())
  {
    directory.createDirectory(true);
  }

  if(directory.exists())
  {
    ossimRefPtr<ossimJpegWriter> jpegWriter = new ossimJpegWriter;
    ossimRefPtr<ossimMemoryImageSource> memSource = new ossimMemoryImageSource;
    ossimRefPtr<ossimImageData> data = new ossimImageData(0, OSSIM_UINT8, image->getNumberOfComponents(), image->getWidth(), image->getHeight());
    data->loadTile(image->data(), data->getImageRectangle(),OSSIM_BIP);

    memSource->setImage(data.get());
    jpegWriter->connectMyInputTo(memSource.get());
    jpegWriter->setFilename(tileFile);
    jpegWriter->execute();
    jpegWriter->disconnect();
    memSource->disconnect();
  }
}

bool ossimPlanetDiskImageCache::hasImage(const ossimPlanetTerrainTileId& id)const
{
  ossimFilename tileFile = m_directory.dirCat(buildTileFile(id));

  return tileFile.exists();
}

ossimPlanetImage* ossimPlanetDiskImageCache::get(const ossimPlanetTerrainTileId& id)
{
  ossimFilename tileFile = m_directory.dirCat(buildTileFile(id));
  osg::ref_ptr<ossimPlanetImage> result;

  if(tileFile.exists())
  {
    ossimRefPtr<ossimJpegTileSource> jpegReader = new ossimJpegTileSource;
    if(jpegReader->open(tileFile))
    {
      ossimRefPtr<ossimImageData> data = jpegReader->getTile(jpegReader->getBoundingRect());

      if(data.valid())
      {
        result = new ossimPlanetImage(id);
        result->fromOssimImage(data.get());
      }
    }
//    ossimPlanetJpegImage jpegImageTile;

//    result = new ossimPlanetImage(id);

//    if(!jpegImageTile.loadFile(tileFile, *result))
//    {
//      result = 0;
//    }
  }

  return result.release();
}

ossimFilename ossimPlanetDiskImageCache::buildTileFile(const ossimPlanetTerrainTileId& id)const
{
  ossimFilename tileFile = ossimString::toString(id.face());
  tileFile = tileFile.dirCat(ossimString::toString(id.level()));
  tileFile = tileFile.dirCat(ossimString::toString(id.x()));
  tileFile = tileFile.dirCat(ossimString::toString(id.y()));

  return tileFile;
}

