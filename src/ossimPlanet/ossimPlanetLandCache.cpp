#include <ossimPlanet/ossimPlanetLandCache.h>


void ossimPlanetLandCacheNode::access()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theTimeStamp = osg::Timer::instance()->tick();
}

void ossimPlanetLandCacheNode::protectedAccess()
{
   theTimeStamp = osg::Timer::instance()->tick();
   
}

void ossimPlanetLandCacheNode::setExtents(osg::ref_ptr<ossimPlanetExtents> extents)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theExtents = extents.get();
}

void ossimPlanetLandCacheNode::setTexture(ossim_uint32 idx,
                                          osg::ref_ptr<ossimPlanetImage> texture)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   
   if(idx >= theTextureList.size())
   {

      if(idx == theTextureList.size())
      {
         theTextureList.push_back(texture.get());
      }
      else
      {
         std::vector<osg::ref_ptr<ossimPlanetImage> > tempVec = theTextureList;

         theTextureList.resize(idx-(theTextureList.size()-1));
         theTextureList.insert(theTextureList.begin(),
                           tempVec.begin(),
                           tempVec.end());
         theTextureList[idx] = texture.get();
      }
   }
   else
   {
      theTextureList[idx] = texture.get();
   }

   adjustSize();
   protectedAccess();
}

void ossimPlanetLandCacheNode::clearTextures()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);

   theTextureList.clear();
   adjustSize();
   protectedAccess();   
}

void ossimPlanetLandCacheNode::setElevation(osg::ref_ptr<ossimPlanetImage> elevation)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theElevation = elevation.get();
   adjustSize();
}

void ossimPlanetLandCacheNode::setLandCache(ossimPlanetLandCache* landCache)
{
   theLandCache = landCache;
}

ossim_uint64 ossimPlanetLandCacheNode::getNodeSizeInBytes()const
{
   return theNodeSizeInBytes;
}

void ossimPlanetLandCacheNode::estimateSize()
{
   theNodeSizeInBytes = (sizeof(osg::Timer_t)+sizeof(OpenThreads::Mutex)+sizeof(theId)+sizeof(theNodeSizeInBytes)+
                         sizeof(theExtents)+sizeof(theTextureList)+sizeof(theElevation)+sizeof(theLandCache));

   if(theElevation.valid())
   {
      theNodeSizeInBytes += theElevation->getTotalSizeInBytes();
   }
   
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theTextureList.size();++idx)
   {
      if(theTextureList[idx].valid())
      {
         theNodeSizeInBytes += theTextureList[idx]->getTotalSizeInBytes();
      }
   }
}                         

void ossimPlanetLandCacheNode::adjustSize()
{
   if(theLandCache)
   {
      theLandCache->theCurrentCacheSize -= theNodeSizeInBytes;
      estimateSize();
      theLandCache->theCurrentCacheSize += theNodeSizeInBytes;
      theLandCache->shrinkCache();
   }
   else
   {
      estimateSize();
   }
}

ossimPlanetLandCache::ossimPlanetLandCache(ossim_uint64 maxCacheSize, ossim_uint64 minCacheSize)
{
   theMinCacheSize = minCacheSize;
   theMaxCacheSize = maxCacheSize;
}

bool ossimPlanetLandCache::addNode(ossimPlanetLandCacheNode* node)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   if(!node) return false;
   if(theMaxCacheSize < node->getNodeSizeInBytes()) return false;
      
   ossimPlanetLandCacheType::iterator iter = theCacheMap.find(node->getId());

   if(iter != theCacheMap.end())
   {
      iter->second = node;
      iter->second->access();
   }
   else
   {
      node->setLandCache(this);
      theCacheMap.insert(std::make_pair(node->getId(),
                                        node));
      theCurrentCacheSize += node->getNodeSizeInBytes();
   }

   shrinkCache();
   
   return true;
}

ossimPlanetLandCacheNode* ossimPlanetLandCache::getNode(ossim_uint64 id, bool allocateNewNodeIfNotPresent)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   if(theMaxCacheSize == 0) return 0;
   ossimPlanetLandCacheType::iterator iter = theCacheMap.find(id);
   if(iter != theCacheMap.end())
   {
      iter->second->access();
      
      return iter->second.get();
   }
   if(allocateNewNodeIfNotPresent)
   {
      ossimPlanetLandCacheNode* newNode = new ossimPlanetLandCacheNode(this, id);

      addNode(newNode);

      return newNode;
   }
   return 0;
}

osg::ref_ptr<ossimPlanetLandCacheNode> ossimPlanetLandCache::removeNode(ossim_uint64 id)
{
   osg::ref_ptr<ossimPlanetLandCacheNode> result;
   ossimPlanetLandCacheType::iterator iter = theCacheMap.find(id);

   if(iter != theCacheMap.end())
   {
      result = iter->second;
   }

   return result.get();
}

void ossimPlanetLandCache::setCacheSize(ossim_uint64 maxCacheSize, ossim_uint64 minCacheSize)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theMaxCacheSize = ossim::max(maxCacheSize, minCacheSize);
   theMinCacheSize = ossim::min(maxCacheSize, minCacheSize);
   protectedShrinkCache();
}

ossim_uint64 ossimPlanetLandCache::getCacheSize()const
{
   return theMaxCacheSize;
}

void ossimPlanetLandCache::shrinkCache()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   protectedShrinkCache();
}
void ossimPlanetLandCache::clearCache()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theCacheMap.clear();
   theCurrentCacheSize = 0;
}

void ossimPlanetLandCache::clearAllWithinExtents(osg::ref_ptr<ossimPlanetExtents> extents)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   ossimPlanetLandCacheType::iterator iter = theCacheMap.begin();

   while(iter != theCacheMap.end())
   {
      if(iter->second->getExtents())
      {
         if(iter->second->getExtents()->intersects(*extents))
         {
            ossimPlanetLandCacheType::iterator currentIter = iter;
            ++iter;
            theCurrentCacheSize -= currentIter->second->getNodeSizeInBytes();
            theCacheMap.erase(currentIter);
         }
         else
         {
            ++iter;
         }
      }
      else
      {
         ++iter;
      }
   }
}

void ossimPlanetLandCache::clearTexturesWithinExtents(osg::ref_ptr<ossimPlanetExtents> extents)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   ossimPlanetLandCacheType::iterator iter = theCacheMap.begin();

   while(iter != theCacheMap.end())
   {
      if(iter->second->getExtents())
      {
         if(iter->second->getExtents()->intersects(*extents))
         {
            theMutex.unlock();
            iter->second->clearTextures();
            theMutex.lock();
         }            
      }
      ++iter;
   }
}


void ossimPlanetLandCache::protectedShrinkCache()
{
   if((theCurrentCacheSize > theMaxCacheSize)&&theCacheMap.size() > 0)
   {
      osg::Timer_t timeStamp;
      while((theCurrentCacheSize > theMinCacheSize)&&(theCacheMap.size() > 0))
      {
         ossimPlanetLandCacheType::iterator iter = theCacheMap.begin();
         ossimPlanetLandCacheType::iterator smallestTimeStampIter = iter;
         timeStamp = iter->second->timeStamp();

         ++iter;
         while(iter != theCacheMap.end())
         {
            if(iter->second->timeStamp() < timeStamp)
            {
               smallestTimeStampIter = iter;
               timeStamp = iter->second->timeStamp();
            }
            
            ++iter;
         }

         if(smallestTimeStampIter!=theCacheMap.end())
         {
            theCurrentCacheSize -= smallestTimeStampIter->second->getNodeSizeInBytes();
            theCacheMap.erase(smallestTimeStampIter);
         }
      }
   }
   if(theCacheMap.size() == 0)
   {
      theCurrentCacheSize = 0;
   }   
}
