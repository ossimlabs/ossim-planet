#ifndef ossimPlanetTerrain_HEADER
#define ossimPlanetTerrain_HEADER
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetGrid.h>
#include <ossimPlanet/ossimPlanetOperation.h>
#include <ossimPlanet/ossimPlanetTerrainTile.h>
#include <ossimPlanet/ossimPlanetTerrainTechnique.h>
#include <ossimPlanet/ossimPlanetOperation.h>
#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <ossimPlanet/ossimPlanetElevationDatabaseGroup.h>
#include <osg/TexMat>
#include <osg/Timer>
#include <osg/GraphicsContext>
#include <osg/Geometry>
#include <osg/BufferObject>
#include <osgDB/DatabasePager>
#include <osgUtil/UpdateVisitor>
#include <osgUtil/CullVisitor>
#include <ossimPlanet/ossimPlanetElevationDatabaseGroup.h>
#include <ossimPlanet/ossimPlanetElevationDatabase.h>
#include <ossimPlanet/ossimPlanetTileRequest.h>
#include <ossimPlanet/ossimPlanetCache.h>
#include <set>
#include <map>
#include <queue>

/**
 *
 */
class OSSIMPLANET_DLL ossimPlanetTerrain : public ossimPlanetLayer
{
public:   
   class TextureCallback;
   class OSSIMPLANET_DLL UpdateTileCallback : public osg::NodeCallback
   {
   public:
      UpdateTileCallback(ossimPlanetTerrain* terrain)
      :theTerrain(terrain){}
      virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
      virtual void update(ossimPlanetTerrainTile* tile, osg::NodeVisitor* nv);
   protected:
      ossimPlanetTerrain* theTerrain;
   };   
   class OSSIMPLANET_DLL CullTileCallback : public osg::NodeCallback
   {
   public:
      CullTileCallback(ossimPlanetTerrain* terrain)
      :theTerrain(terrain){}
      virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
      virtual void cull(ossimPlanetTerrainTile* tile, osg::NodeVisitor* nv);
   protected:
      ossimPlanetTerrain* theTerrain;
   };

   enum CullAmountType
   {
      NO_CULL  = 0,
      LOW_CULL,
      MEDIUM_LOW_CULL,
      MEDIUM_CULL,
      MEDIUM_HIGH_CULL,
      HIGH_CULL
   };
   enum TextureDensityType
   {
      LOW_TEXTURE_DENSITY = 0, 
      MEDIUM_LOW_TEXTURE_DENSITY, 
      MEDIUM_TEXTURE_DENSITY, 
      MEDIUM_HIGH_TEXTURE_DENSITY, 
      HIGH_TEXTURE_DENSITY    
   };
   enum ElevationDensityType
   {
      LOW_ELEVATION_DENSITY = 0,
      MEDIUM_LOW_ELEVATION_DENSITY,
      MEDIUM_ELEVATION_DENSITY,
      MEDIUM_HIGH_ELEVATION_DENSITY,
      HIGH_ELEVATION_DENSITY
   };
   enum SplitMergeSpeedType
   {
      LOW_SPEED = 0,
      MEDIUM_LOW_SPEED,
      MEDIUM_SPEED,
      MEDIUM_HIGH_SPEED,
      HIGH_SPEED
   };
   enum SplitMergeMetricType
   {
       DISTANCE_METRIC = 0,
       PIXEL_METRIC
   };
   class FindCompileableGLObjectsVisitor;
   friend class UpdateTileCallback;
   friend class CullTileCallback;
   friend class TextureCallback;
   
   typedef std::list<osg::ref_ptr<ossimPlanetExtents> > RefreshImageExtentsList;
   typedef std::list<osg::ref_ptr<ossimPlanetExtents> > RefreshElevationExtentsList;
   typedef std::list<osg::ref_ptr<ossimPlanetTerrainTile> > RemoveChildrenList;
   typedef std::vector<osg::ref_ptr<ossimPlanetTextureLayer> > TextureLayers;
   typedef std::map<ossimPlanetTerrainTileId, ossimPlanetTerrainTile* > TileSetMap; 
   typedef std::set<ossimPlanetTerrainTile*> TileSet; 

   
   ossimPlanetTerrain();
   ossimPlanetTerrain(ossimPlanetGrid* grid);
   virtual ~ossimPlanetTerrain();
   void setDatabasePager(osgDB::DatabasePager* pager);
   osgDB::DatabasePager* getDatabasePager(){return thePager.get();}
   void setTerrainTechnique(ossimPlanetTerrainTechnique* technique);
   ossimPlanetTerrainTechnique* newTechnique();
   
   void setGrid(ossimPlanetGrid* grid);
   const ossimPlanetGrid* grid()const;
   ossimPlanetGrid* grid();
   void removeTerrainTileFromGraph(ossimPlanetTerrainTile* tile);
   void removeTerrainChildren(ossimPlanetTerrainTile* tile);
   
   void requestSplit(ossimPlanetTerrainTile* tile,
                     ossim_float64 priority,
                     const osg::FrameStamp* framestamp,
                     ossimPlanetOperation* request);
   void requestMerge(ossimPlanetTerrainTile* tile,
                     ossim_float64 priority,
                     const osg::FrameStamp* framestamp,
                     ossimPlanetOperation* request);
   void requestTexture(ossimPlanetTerrainTile* tile,
                       ossim_float64 priority,
                       const osg::FrameStamp* framestamp,
                       const std::vector<ossim_uint32>& indices,
                       ossimPlanetOperation* request);
   void requestElevation(ossimPlanetTerrainTile* tile,
                         ossim_float64 priority,
                         const osg::FrameStamp* framestamp,
                         ossimPlanetOperation* request);
   void compileGLObjects(osg::State& state, double compileTime);
  
   // Helper functions for determining if objects need to be
   // compiled.
   inline static bool isCompiled(const osg::Texture* texture,
                                 unsigned int contextID)
   {
      return( texture->getTextureObject(contextID) != NULL );
   }
   inline static bool isCompiled(const osg::StateSet* stateSet,
                                 unsigned int contextID)
   {
      for (unsigned i = 0;
           i < stateSet->getTextureAttributeList().size();
           ++i)
      {
         const osg::Texture* texture
         = dynamic_cast<const osg::Texture*>(stateSet->getTextureAttribute(i,osg::StateAttribute::TEXTURE));
         if (texture && !isCompiled(texture, contextID))
            return false;
      }
      return true;
   }
   inline static bool isCompiled(const osg::Drawable* drawable,
                                 unsigned int contextID)
   {
      // Worry about vbos later
      if (drawable->getUseDisplayList())
      {
         return drawable->getDisplayList(contextID) != 0;
      }
      else if(drawable->getUseVertexBufferObjects())
      {
         const osg::Geometry* geometry = drawable->asGeometry();
         if(geometry)
         { 
            osg::Geometry::ArrayList arrayList;
            geometry->getArrayList(arrayList);
            ossim_uint32 idx = 0;
            for(idx = 0; idx < arrayList.size();++idx)
            {
               if(arrayList[idx]->getVertexBufferObject())
               {
//                  if(arrayList[idx]->getVertexBufferObject()->isDirty(contextID))
                  {
                     return false;
                  }
               }
            }
         }
      }
      return true;
     }
   bool addElevation(const ossimFilename& file, bool sortFlag=false);
   bool addElevation(osg::ref_ptr<ossimPlanetElevationDatabase> database, bool sortFlag=false);
   
   /**
    * Will do a non destructive expand or shrink
    */
   void setNumberOfTextureLayers(ossim_uint32 size);
   
   ossim_uint32 numberOfTextureLayers()const;
   ossimPlanetTextureLayer* textureLayer(ossim_uint32 idx);
   bool setTextureLayer(ossim_uint32 idx, ossimPlanetTextureLayer* layer);
   
   ossimPlanetElevationDatabaseGroup* elevationLayer();
   const ossimPlanetElevationDatabaseGroup* elevationLayer()const;
   
   virtual void traverse(osg::NodeVisitor &nv);
   static ossimPlanetTerrain* findTerrain(osg::NodePath& currentNodePath);
   
   double maxTimeToSplit()const;
   double maxTimeToMerge()const;
   
   void registerTile(ossimPlanetTerrainTile* tile);
   void unregisterTile(ossimPlanetTerrainTile* tile);
   ossimPlanetTerrainTile* findTile(const ossimPlanetTerrainTileId& id);
   const ossimPlanetTerrainTile* findTile(const ossimPlanetTerrainTileId& id)const;
   
   void refreshImageLayers(ossimPlanetExtents* extents = 0);
   void refreshElevationLayers(ossimPlanetExtents* extents = 0);
   void refreshImageAndElevationLayers(ossimPlanetExtents* extents = 0);
   
   void resetImageLayers();
   void resetGraph();
   virtual void setModel(ossimPlanetGeoRefModel* model);
   void addRequestToReadyToApplyQueue(ossimPlanetTileRequest* request);
   void addRequestToNeedToCompileQueue(ossimPlanetTileRequest* request);
   
   void setMaxNumberOfOperationsToApplyToGraphPerFrame(ossim_uint32 value);
   
   /**
    * @brief set the cull amount used for the terrain.
    */
   virtual void setCullAmountType(CullAmountType cullAmount);
   virtual void setTextureTileSize(ossim_uint32 width,
                                   ossim_uint32 height);
   ossim_uint32 textureTileWidth()const;
   ossim_uint32 textureTileHeight()const;
   
   virtual void setElevationTileSize(ossim_uint32 width,
                                     ossim_uint32 height);
   ossim_uint32 elevationTileWidth()const;
   ossim_uint32 elevationTileHeight()const;
   
   virtual void setElevationDensityType(ElevationDensityType type);
   virtual void setTextureDensityType(TextureDensityType type);
   
   ElevationDensityType elevationDensityType()const;
   TextureDensityType textureDensityType()const;
   
   void setSplitMergeMetricType(SplitMergeMetricType type);
   SplitMergeMetricType splitMergeMetricType()const;
   /**
    * Split merge speed determines how fast or basically how soon the patch should split.
    * Currently our algorithm is based solely on distance which means if you set the split
    * merge speed type to a LOW_SPEED setting you have to move your eye closer to a patch 
    * before it is further refined.  If you set the speed to a higher speed that low it 
    * progressively refines the graph sooner.  So the eye can be further away and still refine without
    * having to be close so the higher the speed the more detail you see from further away.
    */
   virtual void setSplitMergeSpeedType(SplitMergeSpeedType type);

   /**
    *  The metric is based on a scale multiplier of the pixel dimensions of the texture.
    *  If the patch is smaller in projective scale than the mergeMetric*patchRadiusInPixels then it will
    *  merge the graph.  If its larger than the splitMetric*patchRadiusInpixels then split.
    */
   virtual void setSplitMergePixelMetricParameters(ossim_float64 mergeMetric,
                                                  ossim_float64 splitMetric);
   ossim_float64 splitPixelMetric()const;
   ossim_float64 mergePixelMetric()const;

   virtual void setSplitMergeLodScale(ossim_float64 ratio);
   ossim_float64 splitMergeLodScale()const; 
   virtual void setElevationExaggeration(ossim_float64 exaggeration);
   ossim_float64 elevationExaggeration()const;
   void setMinimumTimeToCompilePerFrameInSeconds(double timeInSeconds);
   ossim_float64 minimumTimeToCompilePerFrame()const;
   void setPrecompileEnabledFlag(bool flag);
   bool precompileEnabledFlag()const;
   void setElevationEnabledFlag(bool flag);
   bool elevationEnabledFlag()const;
   void initElevation();
   
   void setElevationMemoryCache(ossimPlanetMemoryImageCache* cache)
   {
      thePropertyMutex.lock();
      theElevationCache = cache;
      thePropertyMutex.unlock();
   }
   ossimPlanetImageCache* elevationCache()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theElevationCache.get();
   }
   const ossimPlanetImageCache* elevationCache()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theElevationCache.get();
   }
   
   bool priorityPointFlag()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return thePriorityPointFlag;
   }
   void setPriorityPointFlag(bool flag)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      thePriorityPointFlag = flag;
   }
   void setPriorityPointXyz(const osg::Vec3d& pt)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      thePriorityPoint = pt;
   }
   const osg::Vec3d& priorityPoint()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return thePriorityPoint;
   }

   
   bool falseEyeFlag()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theFalseEyeFlag;
   }
   void setFalseEyeFlag(bool flag)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theFalseEyeFlag = flag;
   }
   void setFalseEyeXyz(const osg::Vec3d& eye)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theFalseEye = eye;
   }
   const osg::Vec3d& falseEye()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theFalseEye;
   }
   
protected:
   void buildRoot();
   bool resetRootsFlag()const;
   void applyRequestsToGraph(double maxTime);
   void compileRequests(double maxTime);
   void pruneNeedToCompileAndAddToGraphThreadQueues();
   void removeTerrainChildren(double maxTime = 2.0);
   void refreshExtents();
   osg::ref_ptr<ossimPlanetTerrainTechnique> theTerrainTechnique;
   ossim_int64 theLastApplyToGraphFrameNumber;
   ossim_uint32 theMaxNumberOfOperationsToApplyToGraphPerFrame;
   mutable OpenThreads::Mutex          thePropertyMutex;
   mutable OpenThreads::ReentrantMutex theRootLayersMutex;
   mutable OpenThreads::ReentrantMutex theTextureQueueMutex;
   mutable OpenThreads::ReentrantMutex theElevationQueueMutex;
   mutable OpenThreads::ReentrantMutex theSplitQueueMutex;
   mutable OpenThreads::ReentrantMutex theMergeQueueMutex;
   
   mutable OpenThreads::ReentrantMutex theChildrenToRemoveMutex;
   RemoveChildrenList theChildrenToRemove;
   
   bool theResetRootsFlag;
   osg::ref_ptr<ossimPlanetGrid> theGrid;
   
   std::vector<osg::ref_ptr<ossimPlanetTerrainTile> > theRootLayers;
   
   TextureLayers      theTextureLayers;
   osg::ref_ptr<ossimPlanetElevationDatabaseGroup> theElevationLayer;
   
   
   osg::ref_ptr<ossimPlanetTileRequestThreadQueue> theElevationQueue;
   osg::ref_ptr<ossimPlanetTileRequestThreadQueue> theTextureQueue;
   
   
   /**
    *
    */
   osg::ref_ptr<ossimPlanetTileRequestThreadQueue> theSplitQueue;
   osg::ref_ptr<ossimPlanetTileRequestQueue> theMergeQueue;
   
   OpenThreads::ReentrantMutex    theReadyToApplyToGraphQueueMutex;
   ossimPlanetTileRequest::List   theReadyToApplyToGraphQueue;
   ossimPlanetTileRequest::List   theReadyToApplyToGraphNewNodesQueue;
   OpenThreads::ReentrantMutex    theNeedToCompileQueueMutex;
   ossimPlanetTileRequest::List   theNeedToCompileQueue;
      
   double theMaxTimeToSplit;
   double theMaxTimeToMerge;
   mutable OpenThreads::ReentrantMutex theTileSetMutex;
   mutable OpenThreads::ReentrantMutex theTileSetMapMutex;
   TileSet theTileSet;
   TileSetMap theTileSetMap;
   
   osg::Vec3d theLastEyePosition;
   
   osg::ref_ptr<TextureCallback> theTextureCallback;
   
   ossim_int64 theLastFrameNumber;
   
   osg::ref_ptr<ossimPlanetMemoryImageCache> theElevationCache;
   osg::ref_ptr<ossimPlanetImageCacheShrinkOperation> theElevationCacheShrinkOperation;
   osg::ref_ptr<ossimPlanetOperationThreadQueue> theCacheShrinkThreadQueue;
   
   mutable OpenThreads::ReentrantMutex theRefreshExtentsMutex;
   osg::ref_ptr<ossimPlanetExtents> theRefreshImageExtent;
   osg::ref_ptr<ossimPlanetExtents> theRefreshElevationExtent;
//   RefreshImageExtentsList     theRefreshImageExtentsList;
//   RefreshElevationExtentsList theRefreshElevationExtentsList;
   
   /************************** Scene Parameters ****************/
   osg::CullSettings::CullingModeValues theCullSettings;
   SplitMergeMetricType                 theSplitMergeMetricType;
   ossim_uint32                         theTextureTileWidth;
   ossim_uint32                         theTextureTileHeight;
   ossim_uint32                         theElevationTileWidth;
   ossim_uint32                         theElevationTileHeight;
   ossim_float64                        theSplitMergeLodScale;
   ossim_float64                        theSplitPixelMetric;
   ossim_float64                        theMergePixelMetric;
   ossim_float64                        theElevationExaggeration;
   double                               theMinimumTimeToCompilePerFrame;
   bool                                 thePrecompileEnabledFlag;
   bool                                 theElevationEnabledFlag;
   
   bool                                 theFalseEyeFlag;
   osg::Vec3d                           theFalseEye;
   bool                                 thePriorityPointFlag;
   osg::Vec3d                           thePriorityPoint;

   osg::ref_ptr<osgDB::DatabasePager>          thePager;
};


#endif
