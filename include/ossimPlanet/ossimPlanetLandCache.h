#ifndef ossimPlanetLandCache_HEADER
#define ossimPlanetLandCache_HEADER

#include <osg/Timer>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/ossimPlanetExtents.h>
#include <osg/Referenced>
#include <osg/Geometry>
#include <ossimPlanet/ossimPlanetExport.h>

class ossimPlanetLandCache;
class OSSIMPLANET_DLL ossimPlanetLandCacheNode : public osg::Referenced
{
public:
   friend class ossimPlanetLandCache;
   ossimPlanetLandCacheNode(ossimPlanetLandCache* landCache=0,
                            ossim_uint64 id=0)
      :theTimeStamp(osg::Timer::instance()->tick()),
      theLandCache(landCache),
      theId(id)
   {
      estimateSize();
   }
   ossim_uint64 getId()const
   {
      return theId;
   }
   void access();
   
   void setExtents(osg::ref_ptr<ossimPlanetExtents> extents);   
   void setTexture(ossim_uint32 idx, osg::ref_ptr<ossimPlanetImage> texture);
   void setElevation(osg::ref_ptr<ossimPlanetImage> elevation);

   void setLandCache(ossimPlanetLandCache* landCache);
   const ossimPlanetExtents* getExtents()const
   {
      return theExtents.get();
   }
   
   void clearTextures();
   ossimPlanetImage* getTexture(ossim_uint32 idx)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      if(idx < theTextureList.size())
      {
         return theTextureList[idx].get();
      }

      return 0;
   }
   ossimPlanetImage* getElevation()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
      return theElevation.get();
   }
   ossim_uint32 getTextureListSize()const
   {
      return theTextureList.size();
   }
   ossim_uint64 getNodeSizeInBytes()const;
   osg::Timer_t timeStamp()const
   {
      return theTimeStamp;
   }
protected:
   void protectedAccess();
   void estimateSize();
   /**
    * Handles auto communicating the changed size to the cache that owns this node.
    */ 
   void adjustSize();
   mutable osg::Timer_t                          theTimeStamp;
   ossimPlanetReentrantMutex                            theMutex;
   ossimPlanetLandCache*                         theLandCache;
   ossim_uint64                                  theId;
   ossim_uint32                                  theNodeSizeInBytes;
   osg::ref_ptr<ossimPlanetExtents>              theExtents;
   std::vector<osg::ref_ptr<ossimPlanetImage> >  theTextureList;
   osg::ref_ptr<ossimPlanetImage>                theElevation;
};

class ossimPlanetLandCache : public osg::Referenced
{
public:
   friend class ossimPlanetLandCacheNode;
   typedef std::map<ossim_uint64, osg::ref_ptr<ossimPlanetLandCacheNode> > ossimPlanetLandCacheType;
   ossimPlanetLandCache(ossim_uint64 maxCacheSize=0, ossim_uint64 minCacheSize=0);

   bool addNode(ossimPlanetLandCacheNode* node);
   ossimPlanetLandCacheNode* getNode(ossim_uint64 id, bool allocateNewNodeIfNotPresent=true);
   osg::ref_ptr<ossimPlanetLandCacheNode> removeNode(ossim_uint64 id);
   void setCacheSize(ossim_uint64 maxCacheSize, ossim_uint64 minCacheSize);
   ossim_uint64 getCacheSize()const;
   
   void shrinkCache();
   void clearCache();

   void clearTexturesWithinExtents(osg::ref_ptr<ossimPlanetExtents> extents);
   void clearAllWithinExtents(osg::ref_ptr<ossimPlanetExtents> extents);
   
protected:
   /**
    * Will check to see if the cache exceeds max and if so shrinks until doesn't exceed min.
    */ 
   void protectedShrinkCache();
   
   ossimPlanetReentrantMutex       theMutex; 
   ossim_uint64             theMaxCacheSize;
   ossim_uint64             theMinCacheSize; // used in shrinking the cache if exceeds the max
   ossim_uint64             theCurrentCacheSize;
   ossimPlanetLandCacheType theCacheMap;
};

#endif
