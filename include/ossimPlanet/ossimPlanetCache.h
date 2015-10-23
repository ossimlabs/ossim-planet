#ifndef ossimPlanetCache_HEADER
#define ossimPlanetCache_HEADER
#include <osg/Referenced>
#include <osg/Timer>
#include <OpenThreads/ReentrantMutex>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/ossimPlanetOperation.h>
#include <ossimPlanet/ossimPlanetTerrainTileId.h>
#include <ossim/base/ossimKeywordlist.h>

#include <map>

class OSSIMPLANET_DLL ossimPlanetImageCache : public osg::Referenced
{
public:
   struct TileInfo
   {
      osg::Timer_t                   theTimeStamp;
      osg::ref_ptr<ossimPlanetImage> theImage;
   };
   ossimPlanetImageCache()
   :theCurrentCacheSize(0),
   theMinCacheSize(0),
   theMaxCacheSize(0),
   theEnabledFlag(true)
   {
      
   }
   virtual ~ossimPlanetImageCache()
   {
   }
   void setEnabledFlag(bool flag)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      theEnabledFlag = flag;
   }
   bool enabledFlag()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      return theEnabledFlag;
   }
   virtual void clean() = 0;
   virtual void shrink()=0;
   virtual void addOrUpdate(ossimPlanetImage* image)=0;
   virtual ossimPlanetImage* get(const ossimPlanetTerrainTileId& id)=0;
   virtual bool hasImage(const ossimPlanetTerrainTileId& id)const=0;

  void setMinMaxCacheSizeInBytes(ossim_int64 minSize, ossim_int64 maxSize)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      theMinCacheSize = minSize<maxSize?minSize:maxSize;
      theMaxCacheSize = minSize>maxSize?minSize:maxSize;
   }
   void setMinMaxCacheSizeInMegaBytes(ossim_int64 minSize, ossim_int64 maxSize)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      theMinCacheSize = minSize<maxSize?minSize:maxSize;
      theMaxCacheSize = minSize>maxSize?minSize:maxSize;
      theMinCacheSize *= static_cast<ossim_int64>(1024*1024);
      theMaxCacheSize *= static_cast<ossim_int64>(1024*1024);
   }
   void setMinMaxCacheSizeInGigaBytes(ossim_int64 minSize, ossim_int64 maxSize)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      theMinCacheSize = minSize<maxSize?minSize:maxSize;
      theMaxCacheSize = minSize>maxSize?minSize:maxSize;
      theMinCacheSize *= static_cast<ossim_int64>(1024*1024*1024);
      theMaxCacheSize *= static_cast<ossim_int64>(1024*1024*1024);
   }
   ossim_int64 cacheSizeInBytes()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      return theCurrentCacheSize;
   }
   ossim_int64 maxCacheSizeInBytes()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      return theMaxCacheSize;
   }
   ossim_int64 minCacheSizeInBytes()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      return theMinCacheSize;
   }
   bool exceedsMaxCacheSize()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      return (theCurrentCacheSize>theMaxCacheSize);
   }
   bool exceedsMinCacheSize()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      return (theCurrentCacheSize>theMinCacheSize);
   }

protected:
   mutable OpenThreads::Mutex theMutex;
   ossim_int64 theCurrentCacheSize;
   ossim_int64 theMinCacheSize;
   ossim_int64 theMaxCacheSize;
   bool theEnabledFlag;
};

class OSSIMPLANET_DLL ossimPlanetMemoryImageCache : public ossimPlanetImageCache
{
public:
   typedef std::map<ossimPlanetTerrainTileId, TileInfo> TileMap;
   ossimPlanetMemoryImageCache()
   :ossimPlanetImageCache()
   {
   }
   virtual void clean()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      theCurrentCacheSize = 0;
      theTileCache.clear();
   }
   virtual void shrink();
   virtual void addOrUpdate(ossimPlanetImage* image);
   virtual ossimPlanetImage* get(const ossimPlanetTerrainTileId& id);
   bool hasImage(const ossimPlanetTerrainTileId& id)const;

protected:
   TileMap theTileCache;

};

class OSSIMPLANET_DLL ossimPlanetDiskImageCache : public ossimPlanetImageCache
{
public:
    enum OutputType
    {
      DISK_CACHE_RAW = 0,
      DISK_CACHE_JPEG
    };
    ossimPlanetDiskImageCache();
    bool openDirectory(const ossimFilename& file, bool createIfNotExistsFlag=true);
    virtual void clean();
    virtual void shrink();
    virtual void addOrUpdate(ossimPlanetImage* image);
    virtual ossimPlanetImage* get(const ossimPlanetTerrainTileId& id);
    bool hasImage(const ossimPlanetTerrainTileId& id)const;
protected:
    ossimFilename buildTileFile(const ossimPlanetTerrainTileId& id)const;
    ossimFilename m_directory;
    ossimFilename m_indexFile;
    ossimKeywordlist m_indexFileKwl;
    OutputType       m_outputType;
};

class OSSIMPLANET_DLL ossimPlanetImageCacheShrinkOperation : public ossimPlanetOperation
{
public:
   ossimPlanetImageCacheShrinkOperation()
   :theCache(0)
   {
   }
   virtual ~ossimPlanetImageCacheShrinkOperation()
   {
      theCache = 0;
   }
   void setCache(ossimPlanetImageCache* value)
   {
      theCache = value;
   }
   virtual void run()
   {
      if(theCache.valid())
      {
         theCache->shrink();
      }
   }
protected:
   osg::ref_ptr<ossimPlanetImageCache> theCache;
};

#endif
