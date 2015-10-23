#include <ossimPlanet/ossimPlanetTerrain.h>
#include <OpenThreads/ScopedLock>
#include <osg/NodeVisitor>
#include <ossimPlanet/ossimPlanetTerrainTechnique.h>
#include <ossim/elevation/ossimElevSourceFactory.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetTerrainGeometryTechnique.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetTerrainTechnique.h>
#include <ossimPlanet/ossimPlanetElevationRegistry.h>
#include <ossimPlanet/ossimPlanetViewer.h>
#include <ossimPlanet/ossimPlanetOssimElevationDatabase.h>
#include <osg/io_utils>
#include <osg/GraphicsContext>
#include <osgGA/EventVisitor>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/IntersectVisitor>
#include <osgUtil/IncrementalCompileOperation>
#include <stack>
#include <set>

struct ossimPlanetDatabasePagerCompileCompletedCallback : public osgUtil::IncrementalCompileOperation::CompileCompletedCallback
{
    ossimPlanetDatabasePagerCompileCompletedCallback(ossimPlanetTerrain* terrain, ossimPlanetTileRequest* request)
    :m_terrain(terrain), m_request(request){}

   virtual bool compileCompleted(osgUtil::IncrementalCompileOperation::CompileSet* /* compileSet */)
    {
      //std::cout << "COMPILE COMPLETED\n";
      m_terrain->addRequestToReadyToApplyQueue(m_request.get());
      return true;
    }

    osg::ref_ptr<ossimPlanetTerrain> m_terrain;
    osg::ref_ptr<ossimPlanetTileRequest> m_request;
};

class ossimPlanetTerrain::TextureCallback : public ossimPlanetTextureLayerCallback
{
public:
   TextureCallback(ossimPlanetTerrain* terrain)
   :theTerrain(terrain)
   {
   }
   
   void setTerrain(ossimPlanetTerrain* value)
   {
      theTerrain = value;
   }
   virtual void refreshExtent(osg::ref_ptr<ossimPlanetExtents> extent)
   {
      if(theTerrain)
      {
         theTerrain->refreshImageLayers(extent.get());
      }
   }
   virtual void layerAdded(osg::ref_ptr<ossimPlanetTextureLayer> layer)
   {
      if(theTerrain)
      {
         refreshExtent(layer->getExtents().get());
      }
   }
   virtual void layerRemoved(osg::ref_ptr<ossimPlanetTextureLayer> layer,
                             osg::ref_ptr<ossimPlanetTextureLayer> parent)
   {
      if(theTerrain)
      {
         // only refresh if it was originally enabled
         if(layer->getEnableFlag())
         {
            osg::ref_ptr<ossimPlanetExtents> extents = new ossimPlanetExtents(*layer->getExtents());
            if(parent.valid())
            {
               osg::ref_ptr<ossimPlanetExtents> parentExtents = parent->getExtents();
               if(parentExtents.valid())
               {
                  extents->combineScale(parentExtents->getMinScale(),
                                        parentExtents->getMaxScale());
               }
            }
            refreshExtent(extents.get());
         }
      }         
   }
   virtual void propertyChanged(const ossimString& name,
                                const ossimPlanetTextureLayer* object)
   {
      if(object&&name.contains("enable"))
      {
         if(object->getExtents().valid())
         {
            osg::ref_ptr<ossimPlanetExtents> extents = new ossimPlanetExtents(*object->getExtents());
            const ossimPlanetTextureLayerGroup* parent = object->getParent(0);
            if(parent)
            {
               osg::ref_ptr<ossimPlanetExtents> parentExtents = parent->getExtents();
               if(parentExtents.valid())
               {
                  extents->combineScale(parentExtents->getMinScale(),
                                        parentExtents->getMaxScale());
               }
            }
            
            refreshExtent(extents);
         }
      }
   }
   
protected:
   ossimPlanetTerrain* theTerrain;
};


class CountVisitor : public osg::NodeVisitor
{
public:
   CountVisitor()
   :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
   {
      theCullCount = 0;
      theNodeCount = 0;
   }
   virtual void apply(osg::Node& node)
   {
#if 0
      ossimPlanetTerrainTile* tile = dynamic_cast<ossimPlanetTerrainTile*>(&node);
      if(tile)
      {
         if(tile->culledFlag())
         {
            ++theCullCount;
         }
         ++theNodeCount;
      }
#else
      ossimPlanetTerrainTile* tile = dynamic_cast<ossimPlanetTerrainTile*>(&node);
      if(tile&&tile->culledFlag())
      {
         std::stack<ossimPlanetTerrainTile*> tileStack;
         tileStack.push(tile);
         ossimPlanetTerrainTile* current;
         ossim_uint32 idx = 0;
         while(!tileStack.empty()) 
         {
            current = tileStack.top();
            tileStack.pop();
            for(idx = 0; idx < current->getNumChildren();++idx)
            {
               ossimPlanetTerrainTile* t = (ossimPlanetTerrainTile*)current->getChild(idx);
               if(t)
               {
                  tileStack.push(t);
                  ++ theCullCount;
                  ++theNodeCount;
               }
            }
         }
         ++theNodeCount;
         ++theCullCount;
         return;
      }
      if(tile)
      {
         ++theNodeCount;
      }
#endif
      traverse(node);
   }
   ossim_uint32  theCullCount;
   ossim_uint32  theNodeCount;
};

class GatherChildrenVisitor : public osg::NodeVisitor
{
public:
   GatherChildrenVisitor()
   :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
   {
   }
   typedef  std::stack<osg::ref_ptr<ossimPlanetTerrainTile> > TileStack;
   virtual void apply(osg::Node& node)
   {
      ossimPlanetTerrainTile* tile = dynamic_cast<ossimPlanetTerrainTile*>(&node);
      if(tile)
      {
         theGatheredChildren.push(tile);
      }
      traverse(node);
   }
   TileStack& gatheredChildren()
   {
      return theGatheredChildren;
   }
protected:
   TileStack theGatheredChildren;
};

class RemoveChildrenFromGraphVisitor : public osg::NodeVisitor
{
public:
   RemoveChildrenFromGraphVisitor(ossimPlanetTerrain* terrain, ossim_int64 frameNumber, ossim_int64 delta=2)
   :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
   theTerrain(terrain),
   theFrameNumber(frameNumber),
   theCullDelta(delta)
   {
   }
   virtual void apply(osg::Node& node)
   {
      ossimPlanetTerrainTile* tile = dynamic_cast<ossimPlanetTerrainTile*>(&node);
      if(tile)
      {
         if(ossim::abs((tile->frameNumber()-theFrameNumber)) > theCullDelta)
         {
            theTerrain->removeTerrainTileFromGraph(tile);
            return;
         }
      }
      traverse(node);
   }
   void setFrameNumber(ossim_int64 num)
   {
      theFrameNumber = num;
   }
   
protected:
   ossimPlanetTerrain* theTerrain;
   ossim_int64 theFrameNumber;
   ossim_int64 theCullDelta;
};

void ossimPlanetTerrain::UpdateTileCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
   traverse(node,nv);
   update(dynamic_cast<ossimPlanetTerrainTile*>(node), nv);
}

void ossimPlanetTerrain::UpdateTileCallback::update(ossimPlanetTerrainTile* /* tile */,
                                                    osg::NodeVisitor* /* nv */)
{
}

void ossimPlanetTerrain::CullTileCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
   traverse(node,nv);
}

void ossimPlanetTerrain::CullTileCallback::cull(ossimPlanetTerrainTile* /* tile */,
                                                osg::NodeVisitor* /* nv */)
{
}

ossimPlanetTerrain::ossimPlanetTerrain()
:theResetRootsFlag(true),
theLastFrameNumber(-1),
theTextureTileWidth(256),
theTextureTileHeight(256),
theElevationTileWidth(9),//2^n + 1
theElevationTileHeight(9), //2^n + 1
theSplitMergeLodScale(3.0),
theElevationExaggeration(1.0),
theMinimumTimeToCompilePerFrame(.003),// 3 milliseconds
thePrecompileEnabledFlag(true),
theElevationEnabledFlag(true),
theFalseEyeFlag(false),
theFalseEye(0.0,0.0,0.0),
thePriorityPointFlag(false),
thePriorityPoint(0.0,0.0,0.0)
{
   theElevationCacheShrinkOperation = new ossimPlanetImageCacheShrinkOperation;
   theCacheShrinkThreadQueue = new ossimPlanetOperationThreadQueue;
   theElevationLayer = new ossimPlanetElevationDatabaseGroup;
   theElevationLayer->setFillNullWithGeoidOffsetFlag(true);
   theElevationLayer->setGeoRefModel(theModel.get());
   theLastApplyToGraphFrameNumber = -1;
   theMaxNumberOfOperationsToApplyToGraphPerFrame = 5;
   theTextureCallback = new ossimPlanetTerrain::TextureCallback(this);
   theTerrainTechnique = new ossimPlanetTerrainGeometryTechnique();
   theElevationQueue   = new ossimPlanetTileRequestThreadQueue;
   theTextureQueue     = new ossimPlanetTileRequestThreadQueue;
   theTextureLayers.push_back(new ossimPlanetTextureLayerGroup());
   theTextureLayers[0]->addCallback(theTextureCallback.get());
   theSplitQueue = new ossimPlanetTileRequestThreadQueue();
   theMergeQueue = new ossimPlanetTileRequestQueue();
   theMaxTimeToSplit = 0.0;
   theMaxTimeToMerge = 2.5;
   setGrid(new ossimPlanetAdjustableCubeGrid());
   theSplitMergeMetricType = DISTANCE_METRIC;
   theSplitPixelMetric = 8;
   theMergePixelMetric = 4;
   setCullAmountType(HIGH_CULL);
}

ossimPlanetTerrain::ossimPlanetTerrain(ossimPlanetGrid* grid)
:theResetRootsFlag(true),
theGrid(grid),
theLastFrameNumber(-1),
theTextureTileWidth(256),
theTextureTileHeight(256),
theElevationTileWidth(9),//2^n + 1
theElevationTileHeight(9), //2^n + 1
theSplitMergeLodScale(3.0),
theElevationExaggeration(1.0),
theMinimumTimeToCompilePerFrame(.003),// 3 milliseconds
thePrecompileEnabledFlag(true),
theElevationEnabledFlag(true),
theFalseEyeFlag(false),
theFalseEye(0.0,0.0,0.0),
thePriorityPointFlag(false),
thePriorityPoint(0.0,0.0,0.0)
{
   theElevationCacheShrinkOperation = new ossimPlanetImageCacheShrinkOperation;
   theCacheShrinkThreadQueue = new ossimPlanetOperationThreadQueue;
   theElevationLayer = new ossimPlanetElevationDatabaseGroup;
   theElevationLayer->setFillNullWithGeoidOffsetFlag(true);
   theElevationLayer->setGeoRefModel(theModel.get());
   theLastApplyToGraphFrameNumber = -1;
   theMaxNumberOfOperationsToApplyToGraphPerFrame= 5;
   theTextureCallback = new ossimPlanetTerrain::TextureCallback(this);
   theTerrainTechnique = new ossimPlanetTerrainGeometryTechnique();
   theTextureLayers.push_back(new ossimPlanetTextureLayerGroup());
   theTextureLayers[0]->addCallback(theTextureCallback.get());
   theElevationQueue  = new ossimPlanetTileRequestThreadQueue;
   theTextureQueue    = new ossimPlanetTileRequestThreadQueue;
   
   // make these support priority queues
   //
   theSplitQueue = new ossimPlanetTileRequestThreadQueue();
   theMergeQueue = new ossimPlanetTileRequestQueue();
   theMaxTimeToSplit = 0.0;
   theMaxTimeToMerge = 2.5;
   theSplitMergeMetricType = DISTANCE_METRIC;
   theSplitPixelMetric = 8;
   theMergePixelMetric = 4;
   setCullAmountType(HIGH_CULL);
}

ossimPlanetTerrain::~ossimPlanetTerrain()
{
   theTextureLayers[0]->removeCallback(theTextureCallback.get());
   
   if(theElevationQueue.valid())
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lockElevationQueue(theElevationQueueMutex);
      theElevationQueue->removeAllOperations();
      theElevationQueue->cancel();
      theElevationQueue = 0;
   }
   if(theTextureQueue.valid())
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lockTextureQueue(theTextureQueueMutex);
      theTextureQueue->removeAllOperations();
      theTextureQueue->cancel();
      theTextureQueue = 0;
   }
   if(theSplitQueue.valid())
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lockSplitQueue(theSplitQueueMutex);
      theSplitQueue->removeAllOperations();
      theSplitQueue->cancel();
      theSplitQueue = 0;
   }
   if(theMergeQueue.valid())
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lockMergeQueue(theMergeQueueMutex);
      theMergeQueue->removeAllOperations();
      theMergeQueue = 0;
   }

   theNeedToCompileQueue.clear();
   theReadyToApplyToGraphQueue.clear();
   theReadyToApplyToGraphNewNodesQueue.clear();
   
   {
      osg::Group::removeChildren(0, getNumChildren());
      //OpenThreads::ScopedLock<OpenThreads::Mutex> lockTileSet(theTileSetMutex);
      theTileSet.clear();
   }
   if(theElevationCache.valid())
   {
      theElevationCache->clean();
      theElevationCache = 0;
   }
}

void ossimPlanetTerrain::setTerrainTechnique(ossimPlanetTerrainTechnique* technique)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theResetRootsFlag = true;
   theTerrainTechnique = technique;
}

ossimPlanetTerrainTechnique* ossimPlanetTerrain::newTechnique()
{
   ossimPlanetTerrainTechnique* result = 0;
   
   if(theTerrainTechnique.valid())
   {
      theTerrainTechnique->setGrid(theGrid.get());
      if(thePlanet)
      {
         theTerrainTechnique->setModel(thePlanet->model().get());
      }
      result = (ossimPlanetTerrainTechnique*)theTerrainTechnique->clone(osg::CopyOp::SHALLOW_COPY);
   }
   
   return result;
}

void ossimPlanetTerrain::setElevationEnabledFlag(bool flag)
{
   bool refresh = elevationEnabledFlag()!=flag;
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theElevationEnabledFlag = flag;
   }
   if(refresh)
   {
      elevationLayer()->setEnableFlag(theElevationEnabledFlag);
      theElevationCache->setEnabledFlag(flag);
   }
}

bool ossimPlanetTerrain::elevationEnabledFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theElevationEnabledFlag;
}

void ossimPlanetTerrain::setDatabasePager(osgDB::DatabasePager* pager)
{
   thePager = pager;
}

void ossimPlanetTerrain::initElevation()
{
#if 1
   ossim_uint32 numberOfDatabases = ossimElevManager::instance()->getNumberOfElevationDatabases();
   ossim_uint32 idx = 0;
   for(idx = 0; idx < numberOfDatabases; ++idx)
   {
      ossimRefPtr<ossimElevationDatabase> database = ossimElevManager::instance()->getElevationDatabase(idx);
      if(database.valid())
      {
         ossimPlanetOssimElevationDatabase* planetDatabase = new ossimPlanetOssimElevationDatabase;
         planetDatabase->setDatabase(database.get());
         addElevation(planetDatabase);
      }
   }
#else
   ossim_uint32 idx = 0;
   ossim_uint32 numberOfDatabases = ossimElevManager::instance()->getNumberOfElevationDatabases();
   for(idx = 0; idx < numberOfDatabases; ++idx)
   {
      const ossimRefPtr<ossimElevationDatabase> database = ossimElevManager::instance()->getElevationDatabase(idx);
      
      if(database.valid())
      {
         ossimFilename directory = ossimFilename(database->getConnectionString());
         addElevation(directory, false);
      }
   }
   
   if(theElevationLayer.valid())
   {
      theElevationLayer->sortByGsd();
   }
#endif
}

bool ossimPlanetTerrain::addElevation(const ossimFilename& file, bool sortFlag)
{
   osg::ref_ptr<ossimPlanetElevationDatabase> database = ossimPlanetElevationRegistry::instance()->openDatabase(file);
   return addElevation(database.get(), sortFlag);
}

bool ossimPlanetTerrain::addElevation(osg::ref_ptr<ossimPlanetElevationDatabase> database, bool sortFlag)
{
   bool result = false;
   if(database.valid()&&theElevationLayer.valid())
   {
      database->setGeoRefModel(theModel.get());
      theElevationLayer->addBottom(database.get());
      result = true;
      if(sortFlag)
      {
         // make sure the highest accuracy is on top
         theElevationLayer->sortByGsd();
      }   
   }
   return result;
}

void ossimPlanetTerrain::setNumberOfTextureLayers(ossim_uint32 size)
{
   TextureLayers temp = theTextureLayers;
   theTextureLayers.resize(size);
   ossim_uint32 idx = 0;
   ossim_uint32 minBound = ossim::min(size, (ossim_uint32)temp.size());
   for(idx = 0; idx < minBound;++idx)
   {
      theTextureLayers[idx] = temp[idx];
   }
   resetImageLayers();
}

bool ossimPlanetTerrain::setTextureLayer(ossim_uint32 idx, 
                                         ossimPlanetTextureLayer* layer)
{
   bool result = true;
   if(idx < theTextureLayers.size())
   {
      if(theTextureLayers[idx].valid())
      {
         theTextureLayers[idx]->removeCallback(theTextureCallback.get());
      }
      theTextureLayers[idx] = layer;
      if(layer)
      {
         layer->addCallback(theTextureCallback.get());
      }
      refreshImageLayers();
   }
   else if(idx == theTextureLayers.size())
   {
      theTextureLayers.push_back(layer);
      if(layer)
      {
         layer->addCallback(theTextureCallback.get());
      }
      refreshImageLayers();
   }
   else 
   {
      result = false;
   }
   
   return result;
}

ossim_uint32 ossimPlanetTerrain::numberOfTextureLayers()const
{
   return theTextureLayers.size();
}

ossimPlanetTextureLayer* ossimPlanetTerrain::textureLayer(ossim_uint32 idx)
{
   if(idx < theTextureLayers.size())
   {
      return theTextureLayers[idx].get(); 
   }
   
   return 0;
}

ossimPlanetElevationDatabaseGroup* ossimPlanetTerrain::elevationLayer()
{
   return theElevationLayer.get();
}

const ossimPlanetElevationDatabaseGroup* ossimPlanetTerrain::elevationLayer()const
{
   return theElevationLayer.get();
}

void ossimPlanetTerrain::traverse(osg::NodeVisitor &nv)
{
   // first mark any tile for refresh
   
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      if(theElevationCache.valid()&&theElevationCache->exceedsMaxCacheSize())
      {
         if(theElevationCacheShrinkOperation->referenceCount() == 1)
         {
            theElevationCacheShrinkOperation->reset();
            theElevationCacheShrinkOperation->setCache(theElevationCache.get());
            theCacheShrinkThreadQueue->add(theElevationCacheShrinkOperation.get());
         }
      }
   }
//   osgUtil::IntersectionVisitor* iv = dynamic_cast<osgUtil::IntersectionVisitor*>(&nv);
//   if(iv)
//   {
//      iv->setUseKdTreeWhenAvailable(true);
//   }
//   osg::Timer_t tick = osg::Timer::instance()->tick();
   int savedCullSettings = 0;
   // int savedCullMask = 0;
   osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(&nv);
   if(cv)
   {
      // savedCullMask = cv->getCurrentCullingSet().getCullingMask();
      savedCullSettings = cv->getCullingMode();
      cv->getCurrentCullingSet().setCullingMask((osg::CullingSet::MaskValues)theCullSettings);
      cv->setCullingMode(theCullSettings);
   }
   // bool frameNumberChanged = false;
   
   if(nv.getFrameStamp())
   {
      // frameNumberChanged = nv.getFrameStamp()->getFrameNumber() != theLastFrameNumber;
      theLastFrameNumber = nv.getFrameStamp()->getFrameNumber();
   }
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::EVENT_VISITOR:
      {
         return;
      }
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         refreshExtents();
         theTextureQueue->setCurrentFrameNumber(theLastFrameNumber);
         theElevationQueue->setCurrentFrameNumber(theLastFrameNumber);
         theSplitQueue->setCurrentFrameNumber(theLastFrameNumber);
         theMergeQueue->setCurrentFrameNumber(theLastFrameNumber);
         theTextureQueue->operationQueue()->removeStoppedOperations();
         theElevationQueue->operationQueue()->removeStoppedOperations();
         
        // osg::Timer_t startTick = osg::Timer::instance()->tick();
        // double delta =0.0;
         if(resetRootsFlag())
         {
            buildRoot();
         }
         
         bool needToRedraw =  (!theTextureQueue->empty()||
                               !theElevationQueue->empty()||
                               !theSplitQueue->empty());
         {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReadyToApplyToGraphQueueMutex);
            if(!theReadyToApplyToGraphQueue.empty()||
               !theReadyToApplyToGraphNewNodesQueue.empty())
            {
               needToRedraw = true;
            }
         }
         {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theNeedToCompileQueueMutex);
            if(!theNeedToCompileQueue.empty())
            {
               needToRedraw = true;
            }
         }
         if(needToRedraw)
         {
            setRedrawFlag(true); // request a new frame if not empty
         }
         pruneNeedToCompileAndAddToGraphThreadQueues();
         removeTerrainChildren();
         if(nv.getFrameStamp()&&(nv.getFrameStamp()->getFrameNumber()!=theLastApplyToGraphFrameNumber))
         {
            applyRequestsToGraph(1.25);
            theLastApplyToGraphFrameNumber = nv.getFrameStamp()->getFrameNumber();
         }
         break;
      }
      default:
      {
         break;
      }
   }
#if 0
   ossim_uint32 idx = 0;
   CountVisitor count;
   for(idx = 0; idx < getNumChildren(); ++idx)
   {
      getChild(idx)->accept(count);
   }
   std::cout << "CULL COUNT ===== " << coun   std::cout << "NODE COUNT ===== " << count.theNodeCount << std::endl;
t.theCullCount << std::endl;
#endif
   ossimPlanetLayer::traverse(nv);
   if(cv)
   {
      cv->getCurrentCullingSet().setCullingMask((osg::CullingSet::MaskValues) savedCullSettings);
      cv->setCullingMode( (osg::CullSettings::CullingModeValues) savedCullSettings);
   }
   //std::cout << "f = " << theCurrentFrameNumber << std::endl;
   if((nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR))
   {
      
      RemoveChildrenFromGraphVisitor visitor(this, nv.getFrameStamp()->getFrameNumber());
      ossim_uint32 idx = 0;
      ossim_uint32 maxIdx = getNumChildren();
      for(idx = 0; idx < maxIdx; ++idx)
      {
         getChild(idx)->accept(visitor);
      }
   }
}

void ossimPlanetTerrain::setGrid(ossimPlanetGrid* grid)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   
   theResetRootsFlag = true;
   theGrid = grid;
}

const ossimPlanetGrid* ossimPlanetTerrain::grid()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theGrid.get();
}

ossimPlanetGrid* ossimPlanetTerrain::grid()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theGrid.get();
}

void ossimPlanetTerrain::removeTerrainTileFromGraph(ossimPlanetTerrainTile* tile)
{
   removeTerrainChildren(tile);
}

void ossimPlanetTerrain::removeTerrainChildren(ossimPlanetTerrainTile* tile)
{
   if(tile)
   {
      theChildrenToRemoveMutex.lock();
      osg::ref_ptr<ossimPlanetTerrainTile> currentTile = tile;
      // flatten graph to a single list of children
      //
      std::queue<osg::ref_ptr<ossimPlanetTerrainTile> > tiles;
      ossim_uint32 idx = 0;
      for(idx = 0; idx < currentTile->getNumChildren(); ++idx)
      {
         tiles.push((ossimPlanetTerrainTile*)currentTile->getChild(idx));
      }
      currentTile->removeChildren(0, currentTile->getNumChildren());
      // now let's flatten the graph out and add to the remove tile list
      //
      while(!tiles.empty())
      {
         currentTile = tiles.front().get();
         currentTile->cancelAllOperations();
         tiles.pop();
         //currentTile->setState(ossimPlanetTerrainTile::NEED_MERGING);
         theChildrenToRemove.push_back(currentTile.get());
         for(idx = 0; idx < currentTile->getNumChildren(); ++idx)
         {
            tiles.push((ossimPlanetTerrainTile*)currentTile->getChild(idx));
         }
         currentTile->removeChildren(0, currentTile->getNumChildren());
      }
      theChildrenToRemoveMutex.unlock();
   }
}

void ossimPlanetTerrain::requestSplit(ossimPlanetTerrainTile* tile,
                                      ossim_float64 priority,
                                      const osg::FrameStamp* framestamp,
                                      ossimPlanetOperation* request)
{
   if(theResetRootsFlag) return;
   ossimPlanetTileRequest* tileRequest = dynamic_cast<ossimPlanetTileRequest*>(request);
   if(tileRequest)
   {
      if(tileRequest->referenceCount() == 1)
      {
         tileRequest->setState(ossimPlanetOperation::READY_STATE);
         tileRequest->setTimestampFirstRequest(framestamp->getReferenceTime());
         tileRequest->setTile(tile);
         theSplitQueue->add(tileRequest);
      }
      tileRequest->setPriority(priority);
      tileRequest->setFrameNumberOfLastRequest(framestamp->getFrameNumber());
      tileRequest->setTimestampLastRequest(framestamp->getReferenceTime());
   }
}

void ossimPlanetTerrain::requestMerge(ossimPlanetTerrainTile* tile,
                                      ossim_float64 priority,
                                      const osg::FrameStamp* framestamp,
                                      ossimPlanetOperation* request)
{
   ossimPlanetTileRequest* tileRequest = dynamic_cast<ossimPlanetTileRequest*>(request);
   if(tileRequest)
   {
      tileRequest->reset();
      tileRequest->setTile(tile);
      tileRequest->setPriority(priority);
      tileRequest->setFrameNumberOfLastRequest(framestamp->getFrameNumber());
      theMergeQueue->add(tileRequest);
   }
   
}

void ossimPlanetTerrain::requestTexture(ossimPlanetTerrainTile* tile,
                                        ossim_float64 priority,
                                        const osg::FrameStamp* framestamp,
                                        const std::vector<ossim_uint32>& indices,
                                        ossimPlanetOperation* request)
{
   if(theResetRootsFlag) return;
   ossimPlanetTextureRequest* tileRequest = dynamic_cast<ossimPlanetTextureRequest*>(request);
   if(tileRequest)
   {
      if(tileRequest->referenceCount() == 1)
      {
         tileRequest->reset();
         tileRequest->setTimestampFirstRequest(framestamp->getReferenceTime());
         tileRequest->setTile(tile);
         tileRequest->setTextureLayerIndices(indices);
         theTextureQueue->add(tileRequest);
      }
      tileRequest->setPriority(priority);
      tileRequest->setFrameNumberOfLastRequest(framestamp->getFrameNumber());
      tileRequest->setTimestampLastRequest(framestamp->getReferenceTime());
   }
}

void ossimPlanetTerrain::requestElevation(ossimPlanetTerrainTile* tile,
                                          ossim_float64 priority,
                                          const osg::FrameStamp* framestamp,
                                          ossimPlanetOperation* request)
{
   if(theResetRootsFlag) return;
   ossimPlanetTileRequest* tileRequest = dynamic_cast<ossimPlanetTileRequest*>(request);
   if(tileRequest)
   {
      if(tileRequest->referenceCount() == 1)
      {
         tileRequest->reset();
         tileRequest->setTile(tile);
         tileRequest->setTimestampFirstRequest(framestamp->getReferenceTime());
         theElevationQueue->add(tileRequest);
      }
      tileRequest->setPriority(priority);
      tileRequest->setFrameNumberOfLastRequest(framestamp->getFrameNumber());
      tileRequest->setTimestampLastRequest(framestamp->getReferenceTime());
   }
}

void ossimPlanetTerrain::compileGLObjects(osg::State& state, double compileTime)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theNeedToCompileQueueMutex);
   osg::Timer_t startT = osg::Timer::instance()->tick();
   double delta = 0.0;
   double deltaNextTest = 0.0;
   double diff = 0.0;
   bool done = theNeedToCompileQueue.empty();
   if(!done)
   {
      setRedrawFlag(true);
   }
   osg::RenderInfo renderInfo;
   renderInfo.setState(&state);
   while(!done)
   {
      // returns false if no more objects in the request need compiling
      if(theNeedToCompileQueue.front()->compileObjects(renderInfo, compileTime))
      {
         addRequestToReadyToApplyQueue(theNeedToCompileQueue.front().get());
         theNeedToCompileQueue.pop_front();
      }
      osg::Timer_t testT = osg::Timer::instance()->tick();
      delta = osg::Timer::instance()->delta_s(startT, testT);
      diff = delta - deltaNextTest;
      if((delta > (compileTime-diff))||
         theNeedToCompileQueue.empty())
      {
         done = true;
      }
      deltaNextTest = delta;
   }
}

bool ossimPlanetTerrain::resetRootsFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theResetRootsFlag;
}

void ossimPlanetTerrain::buildRoot()
{
   if(!theGrid.valid()) return;
   
   theReadyToApplyToGraphQueue.clear();
   theReadyToApplyToGraphNewNodesQueue.clear();
   theNeedToCompileQueue.clear();
   theSplitQueue->removeAllOperations();
   theElevationQueue->removeAllOperations();
   theTextureQueue->removeAllOperations();
   bool hasActiveOperations = false;
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTileSetMutex);
      TileSet::iterator iter = theTileSet.begin();
      while(iter != theTileSet.end())
      {
         (*iter)->cancelAllOperations();
         if((*iter)->hasActiveOperations())
         {
            hasActiveOperations = true;
         }
         ++iter;
      }
      if(!hasActiveOperations)
      {
         theTileSet.clear();
         removeChildren(0, getNumChildren());
      }
   }
   if(!hasActiveOperations)
   {
      ossimPlanetGrid::TileIds rootIds;
      theGrid->getRootIds(rootIds);
      ossim_uint32 idx = 0;
      for(idx = 0; idx < rootIds.size(); ++idx)
      {
         ossimPlanetTerrainTile* tile = new ossimPlanetTerrainTile(rootIds[idx]);
         tile->setTerrain(this);
         tile->init();
         addChild(tile);
      }
      theResetRootsFlag = false;
      setRedrawFlag(true);
   }      
}

ossimPlanetTerrain* ossimPlanetTerrain::findTerrain(osg::NodePath& currentNodePath)
{
   if(currentNodePath.empty())
   {
      return 0;
   }
   for(osg::NodePath::reverse_iterator itr = currentNodePath.rbegin();
       itr != currentNodePath.rend();
       ++itr)
   {
      ossimPlanetTerrain* ts = dynamic_cast<ossimPlanetTerrain*>(*itr);
      if (ts) 
      {
         return ts;
      }
   }
   
   return 0;
}

double ossimPlanetTerrain::maxTimeToSplit()const
{
   return theMaxTimeToSplit;
}

double ossimPlanetTerrain::maxTimeToMerge()const
{
   return theMaxTimeToMerge;
}
void ossimPlanetTerrain::registerTile(ossimPlanetTerrainTile* tile)
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTileSetMutex);
      if(tile)
      {
         theTileSet.insert(tile);
      }
   }
}

void ossimPlanetTerrain::unregisterTile(ossimPlanetTerrainTile* tile)
{
   {   
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTileSetMutex);
      TileSet::iterator iter = theTileSet.find(tile);
      if(iter != theTileSet.end())
      {
         theTileSet.erase(iter);
      }
   }
}
ossimPlanetTerrainTile* ossimPlanetTerrain::findTile(const ossimPlanetTerrainTileId& id)
{
#if 0
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTileSetMapMutex);
   TileSetMap::iterator iter = theTileSetMap.find(id);
   if(iter != theTileSetMap.end())
   {
      return iter->second;
   }
#endif
   return 0;
}
const ossimPlanetTerrainTile* ossimPlanetTerrain::findTile(const ossimPlanetTerrainTileId& id)const
{
#if 0
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTileSetMapMutex);
   TileSetMap::const_iterator iter = theTileSetMap.find(id);
   if(iter != theTileSetMap.end())
   {
      return iter->second;
   }
#endif
   return 0;
}

void ossimPlanetTerrain::refreshImageAndElevationLayers(ossimPlanetExtents* extents)
{
   refreshImageLayers(extents);
   refreshElevationLayers(extents);
#if 0
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTileSetMutex);
   TileSet::iterator iter = theTileSet.begin();
   bool imageIntersects     = true;
   bool elevationIntersects = true;
   osg::ref_ptr<ossimPlanetExtents> imageExtent = new ossimPlanetExtents;
   osg::ref_ptr<ossimPlanetExtents> elevExtent = new ossimPlanetExtents;
   ossim_uint32 idx;
   while(iter != theTileSet.end())
   {
      ossim_uint32 bound = (*iter)->numberOfImageLayers();
      if(extents)
      {
         theGrid->convertToGeographicExtents((*iter)->tileId(),
                                             *imageExtent,
                                             textureTileWidth(),
                                             textureTileHeight());
         theGrid->convertToGeographicExtents((*iter)->tileId(),
                                             *elevExtent,
                                             elevationTileWidth(),
                                             elevationTileHeight());
         
         if(extents->intersectsLatLon(*imageExtent.get())&&
            extents->intersectsScale(*imageExtent.get()))
         {
            imageIntersects = true;
         }
         else
         {
            imageIntersects = false;
         }
         
         if(extents->intersectsLatLon(*elevExtent.get()))
         {
            elevationIntersects = true;
         }
         else
         {
            elevationIntersects = false;
         }
      }
      if(elevationIntersects)
      {
         (*iter)->elevationLayer()->setRefreshFlag(true);
         (*iter)->elevationLayer()->setNoMoreDataFlag(false);
      }
      
      if(imageIntersects)
      {
         for(idx = 0; idx < bound; ++idx)
         {
            (*iter)->imageLayer(idx)->setRefreshFlag(true);
            (*iter)->imageLayer(idx)->setNoMoreDataFlag(false);
            if(textureLayer(idx)&&((*iter)->tileId().level()>0))
            {
               bool hasDataFlag = textureLayer(idx)->hasTexture(textureTileWidth(), 
                                                                textureTileHeight(), 
                                                                (*iter)->tileId(), 
                                                                *grid());
               (*iter)->imageLayer(idx)->setNoMoreDataFlag(!hasDataFlag);
            }
         }
      }
      ++iter;
   }
   setRedrawFlag(true);
#endif
}

void ossimPlanetTerrain::refreshImageLayers(ossimPlanetExtents* extents)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theRefreshExtentsMutex);
   if(theRefreshImageExtent.valid())
   {
      if(extents)
      {
         theRefreshImageExtent->combine(extents);
      }
   }
   else if(extents)
   {
      theRefreshImageExtent = new ossimPlanetExtents(*extents); 
   }
   else
   {
      theRefreshImageExtent = new ossimPlanetExtents();
   }
   setRedrawFlag(true);
}

void ossimPlanetTerrain::refreshElevationLayers(ossimPlanetExtents* extents)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theRefreshExtentsMutex);
   if(theRefreshElevationExtent.valid())
   {
      if(extents)
      {
         theRefreshElevationExtent->combine(extents);
      }
   }
   else if(extents)
   {
      theRefreshElevationExtent = new ossimPlanetExtents(*extents); 
   }
   else
   {
      theRefreshElevationExtent = new ossimPlanetExtents();
   }
   setRedrawFlag(true);
}


void ossimPlanetTerrain::refreshExtents()
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theRefreshExtentsMutex);
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(theTileSetMutex);
      if(!theRefreshElevationExtent.valid()&&
         !theRefreshImageExtent.valid())
      {
         return;
      }
      ossim_uint32 idx = 0;
      TileSet::iterator iter = theTileSet.begin();
      osg::ref_ptr<ossimPlanetExtents> tileExtent = new ossimPlanetExtents;
      while(iter != theTileSet.end())
      {
         theGrid->convertToGeographicExtents((*iter)->tileId(),
                                             *tileExtent,
                                             textureTileWidth(),
                                             textureTileHeight());
         if(theRefreshImageExtent.valid())
         {
            if(theRefreshImageExtent->intersectsLatLon(*tileExtent.get()))//&&
//               theRefreshImageExtent->intersectsScale(*tileExtent.get()))
            {
               (*iter)->textureRequest()->cancel();
               ossim_uint32 bound = (*iter)->numberOfImageLayers();
               for(idx = 0; idx < bound; ++idx)
               {
                  (*iter)->imageLayer(idx)->setRefreshFlag(true);
                  (*iter)->imageLayer(idx)->setNoMoreDataFlag(false);
                  if(textureLayer(idx)&&((*iter)->tileId().level()>0))
                  {
                     bool hasDataFlag = textureLayer(idx)->hasTexture(textureTileWidth(), 
                                                                      textureTileHeight(), 
                                                                      (*iter)->tileId(), 
                                                                      *grid());
                     (*iter)->imageLayer(idx)->setNoMoreDataFlag(!hasDataFlag);
                  }
               }
            }
         }
         if(theRefreshElevationExtent.valid())
         {
            if(theRefreshElevationExtent->intersectsLatLon(*tileExtent.get()))
            {
               (*iter)->elevationRequest()->cancel();
               (*iter)->elevationLayer()->setRefreshFlag(true);
               (*iter)->elevationLayer()->setNoMoreDataFlag(false);
               if(elevationLayer()&&((*iter)->tileId().level()>0))
               {
#if 0
                  bool hasDataFlag = elevationLayer()->hasTexture(elevationTileWidth(), 
                                                                  elevationTileHeight(), 
                                                                  (*iter)->tileId(), 
                                                                  *grid());
                  (*iter)->elevationLayer()->setNoMoreDataFlag(!hasDataFlag);
#else
                  (*iter)->elevationLayer()->setNoMoreDataFlag(false);
#endif
               }
            }
         }
         
         ++iter;
      }
      
      theRefreshElevationExtent = 0;
      theRefreshImageExtent     = 0;
   }
   setRedrawFlag(true);
}

void ossimPlanetTerrain::resetImageLayers()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTileSetMutex);
   TileSet::iterator iter = theTileSet.begin();
   while(iter != theTileSet.end())
   {
      ossim_uint32 idx   = 0;
      ossim_uint32 bound = (*iter)->numberOfImageLayers();
      for(idx = 0; idx < bound; ++idx)
      {
         (*iter)->resetImageLayers();
      }
      ++iter;
   }
   setRedrawFlag(true);
}

void ossimPlanetTerrain::resetGraph()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theResetRootsFlag = true;
}

void ossimPlanetTerrain::setModel(ossimPlanetGeoRefModel* model)
{
   ossimPlanetLayer::setModel(model);
   if(theElevationLayer.valid())
   {
      theElevationLayer->setGeoRefModel(model);
   }
}

void ossimPlanetTerrain::addRequestToReadyToApplyQueue(ossimPlanetTileRequest* request)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReadyToApplyToGraphQueueMutex);
   if(dynamic_cast<ossimPlanetSplitRequest*>(request))
   {
      theReadyToApplyToGraphNewNodesQueue.push_back(request);
   }
   else
   {
      theReadyToApplyToGraphQueue.push_back(request);
   }
   setRedrawFlag(true);
}

void ossimPlanetTerrain::addRequestToNeedToCompileQueue(ossimPlanetTileRequest* request)
{
   if(thePager.valid())
   {

      osg::ref_ptr<osgUtil::IncrementalCompileOperation::CompileSet> compileSet = new osgUtil::IncrementalCompileOperation::CompileSet();
      osgUtil::IncrementalCompileOperation* compileOperation = thePager->getIncrementalCompileOperation();
      if(compileOperation&&!compileOperation->getContextSet().empty())
      {
            //osgUtil::StateToCompile stateToCompile(osgUtil::GLObjectsVisitor::COMPILE_DISPLAY_LISTS|osgUtil::GLObjectsVisitor::COMPILE_STATE_ATTRIBUTES);
            //compileSet->buildCompileMap(compileOperation->getContextSet());
         request->populateCompileSet(compileOperation->getContextSet(), *compileSet.get());
         compileSet->_compileCompletedCallback = new ossimPlanetDatabasePagerCompileCompletedCallback(this, request);
         compileOperation->add(compileSet.get(), false);
      }
      else
      {
         addRequestToReadyToApplyQueue(request);
      }
      //std::cout << compileOperation << std::endl;

   }
 }

void ossimPlanetTerrain::applyRequestsToGraph(double maxTime)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReadyToApplyToGraphQueueMutex);
   if(theReadyToApplyToGraphQueue.empty()&&theReadyToApplyToGraphNewNodesQueue.empty()) return;
   osg::ref_ptr<ossimPlanetTileRequest> request;
   bool doneFlag = false;
   ossim_uint32 operationCount = 0;
   osg::Timer_t startTick = osg::Timer::instance()->tick();
   while(!doneFlag)
   {
      if(!theReadyToApplyToGraphQueue.empty())
      {
      //  theReadyToApplyToGraphQueue.sort(ossimPlanetTileRequest::SortFunctor());
        if((*theReadyToApplyToGraphQueue.begin())->isRequestCurrent(theTextureQueue->currentFrameNumber()))
         {
            (*theReadyToApplyToGraphQueue.begin())->applyToGraph();
            ++operationCount;
            theReadyToApplyToGraphQueue.pop_front();
         }
         else
         {
            theReadyToApplyToGraphQueue.pop_front();
         }
      }
#if 1
      if(!theReadyToApplyToGraphNewNodesQueue.empty())
      {
        //theReadyToApplyToGraphNewNodesQueue.sort(ossimPlanetTileRequest::SortFunctor());
         if((*theReadyToApplyToGraphNewNodesQueue.begin())->isRequestCurrent(theTextureQueue->currentFrameNumber()))
         {
            (*theReadyToApplyToGraphNewNodesQueue.begin())->applyToGraph();
            ++operationCount;
            theReadyToApplyToGraphNewNodesQueue.pop_front();
         }
         else
         {
            theReadyToApplyToGraphNewNodesQueue.pop_front();
         }
      }
#endif
      double delta = osg::Timer::instance()->delta_m(startTick, osg::Timer::instance()->tick());
#if 1
      doneFlag = ((theReadyToApplyToGraphQueue.empty()&&theReadyToApplyToGraphNewNodesQueue.empty())||
                  (delta > maxTime)||
                  (operationCount>=theMaxNumberOfOperationsToApplyToGraphPerFrame));
#else
      doneFlag = ((theReadyToApplyToGraphQueue.empty())||
                  (delta > maxTime)||
                  (operationCount>=theMaxNumberOfOperationsToApplyToGraphPerFrame));
#endif
   }
   setRedrawFlag(true);
}


void ossimPlanetTerrain::pruneNeedToCompileAndAddToGraphThreadQueues()
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReadyToApplyToGraphQueueMutex);
      ossimPlanetTileRequest::List::iterator iter = theReadyToApplyToGraphQueue.begin();
      while(iter != theReadyToApplyToGraphQueue.end())
      {
         if(((*iter)->state() == ossimPlanetOperation::CANCELED_STATE)||
            (!(*iter)->isRequestCurrent(theTextureQueue->currentFrameNumber())))
         {
            iter = theReadyToApplyToGraphQueue.erase(iter);
         }
         else
         {
            ++iter;
         }
      }
   }
#if 1
   {
      
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReadyToApplyToGraphQueueMutex);
      ossimPlanetTileRequest::List::iterator iter = theReadyToApplyToGraphNewNodesQueue.begin();
      while(iter != theReadyToApplyToGraphNewNodesQueue.end())
      {
         if(((*iter)->state() == ossimPlanetOperation::CANCELED_STATE)||
            (!(*iter)->isRequestCurrent(theTextureQueue->currentFrameNumber())))
         {
            iter = theReadyToApplyToGraphNewNodesQueue.erase(iter);
         }
         else
         {
            ++iter;
         }
      }
   }
#endif
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theNeedToCompileQueueMutex);
      ossimPlanetTileRequest::List::iterator iter = theNeedToCompileQueue.begin();
      while(iter != theNeedToCompileQueue.end())
      {
         if(((*iter)->state() == ossimPlanetOperation::CANCELED_STATE)||
            (!(*iter)->isRequestCurrent(theTextureQueue->currentFrameNumber())))
         {
            iter = theNeedToCompileQueue.erase(iter);
         }
         else
         {
            ++iter;
         }
      }
   }
}

void ossimPlanetTerrain::removeTerrainChildren(double maxTime)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenToRemoveMutex);
   osg::Timer_t startTick = osg::Timer::instance()->tick();
   //   osgUtil::GLObjectsVisitor visitor(osgUtil::GLObjectsVisitor::RELEASE_DISPLAY_LISTS|
   //                                     osgUtil::GLObjectsVisitor::RELEASE_STATE_ATTRIBUTES);
   RemoveChildrenList::iterator iter = theChildrenToRemove.begin();
   while(iter!=theChildrenToRemove.end())
   {
      if((*iter)->hasActiveOperations())
      {
         (*iter)->cancelAllOperations();
         ++iter;
      }
      else
      {
         (*iter)->releaseGLObjects();
         iter = theChildrenToRemove.erase(iter);
      }
      double delta = osg::Timer::instance()->delta_m(startTick, osg::Timer::instance()->tick());
      if(delta > maxTime)
      {
         break;
      }
   }
   if(theChildrenToRemove.size()>0)
   {
      iter = theChildrenToRemove.begin();
      //std::cout << "_____________" << std::endl;
      while(iter!=theChildrenToRemove.end())
      {
         //std::cout << (*iter)->tileId() << std::endl;
         // std::cout << "elevationRequest = " << (*iter)->elevationRequest()->referenceCount() << std::endl;
         // std::cout << "textureRequest = " << (*iter)->textureRequest()->referenceCount() << std::endl;
         // std::cout << "splitRequest = " << (*iter)->splitRequest()->referenceCount() << std::endl;
         ++iter;
      }
      //std::cout << "Elevation queue size = " << theElevationQueue->operationQueue()->size() << std::endl;
      //std::cout << "List size = " << theChildrenToRemove.size() << std::endl;
   }
}

void ossimPlanetTerrain::setMaxNumberOfOperationsToApplyToGraphPerFrame(ossim_uint32 value)
{
   theMaxNumberOfOperationsToApplyToGraphPerFrame = value;
}

void ossimPlanetTerrain::setCullAmountType(CullAmountType cullAmount)
{
   switch(cullAmount)
   {
      case NO_CULL:
      {
         theCullSettings = osg::CullSettings::NO_CULLING;
         break;
      }
      case LOW_CULL:
      {
         theCullSettings = osg::CullSettings::SMALL_FEATURE_CULLING;
         break;
      }
      case MEDIUM_LOW_CULL:
      {
         theCullSettings = static_cast<osg::CullSettings::CullingModeValues>(osg::CullSettings::SMALL_FEATURE_CULLING|
                                                                             osg::CullSettings::CLUSTER_CULLING);
         break;
      }
      case MEDIUM_CULL:
      {
         theCullSettings = static_cast<osg::CullSettings::CullingModeValues>(osg::CullSettings::SMALL_FEATURE_CULLING|
                                                                             osg::CullSettings::CLUSTER_CULLING|
                                                                             osg::CullSettings::VIEW_FRUSTUM_SIDES_CULLING);
         break;
      }
      case MEDIUM_HIGH_CULL:
      {
         theCullSettings = static_cast<osg::CullSettings::CullingModeValues>(osg::CullSettings::SMALL_FEATURE_CULLING|
                                                                             osg::CullSettings::CLUSTER_CULLING|
                                                                             osg::CullSettings::VIEW_FRUSTUM_CULLING);
         break;
      }
      case HIGH_CULL:
      {
         theCullSettings = static_cast<osg::CullSettings::CullingModeValues>(osg::CullSettings::ENABLE_ALL_CULLING);
         break;
      }
   }
   setRedrawFlag(true);
}
void ossimPlanetTerrain::setTextureTileSize(ossim_uint32 width,
                                            ossim_uint32 height)
{
   theTextureTileWidth  = width;
   theTextureTileHeight = height;
}

ossim_uint32 ossimPlanetTerrain::textureTileWidth()const
{
   return theTextureTileWidth;
}

ossim_uint32 ossimPlanetTerrain::textureTileHeight()const
{
   return theTextureTileHeight;
}

void ossimPlanetTerrain::setElevationTileSize(ossim_uint32 width,
                                              ossim_uint32 height)
{
   theElevationTileWidth = width|1; // make sure that it's odd number
   theElevationTileHeight = height|1; // make sure that it's an odd number
}
ossim_uint32 ossimPlanetTerrain::elevationTileWidth()const
{
   return theElevationTileWidth;
}

ossim_uint32 ossimPlanetTerrain::elevationTileHeight()const
{
   return theElevationTileHeight;
}

void ossimPlanetTerrain::setElevationDensityType(ElevationDensityType type)
{
   switch(type)
   {
      case LOW_ELEVATION_DENSITY:
      {
         setElevationTileSize(9, 9);
         break;
      }
      case MEDIUM_LOW_ELEVATION_DENSITY:
      {
         setElevationTileSize(17, 17);
         break;
      }
      case MEDIUM_ELEVATION_DENSITY:
      {
         setElevationTileSize(33, 33);
         break;
      }
      case MEDIUM_HIGH_ELEVATION_DENSITY:
      {
         setElevationTileSize(65, 65);
         break;
      }
      case HIGH_ELEVATION_DENSITY:
      {
         setElevationTileSize(129, 129);
         break;
      }
   }
   if(theElevationCache.valid())
   {
      theElevationCache->clean();
   }
}

void ossimPlanetTerrain::setTextureDensityType(TextureDensityType type)
{
   switch(type)
   {
      case LOW_TEXTURE_DENSITY:
      {
         setTextureTileSize(64, 64);
         break;
      }
      case MEDIUM_LOW_TEXTURE_DENSITY:
      {
         setTextureTileSize(128, 128);
         break;
      }
      case MEDIUM_TEXTURE_DENSITY:
      {
         setTextureTileSize(256, 256);
         break;
      }
      case MEDIUM_HIGH_TEXTURE_DENSITY:
      {
         setTextureTileSize(512, 512);
         break;
      }
      case HIGH_TEXTURE_DENSITY:
      {
         setTextureTileSize(1024, 1024);
         break;
      }
   }
}

ossimPlanetTerrain::ElevationDensityType ossimPlanetTerrain::elevationDensityType()const
{
   ossim_int32 size = ossim::max(elevationTileHeight(), elevationTileWidth());
   
   if(size <=9)
   {
      return LOW_ELEVATION_DENSITY;
   }
   else if(size <=17)
   {
      return MEDIUM_LOW_ELEVATION_DENSITY;
   }
   else if(size <=33)
   {
      return MEDIUM_ELEVATION_DENSITY;
   }
   else if(size <=64)
   {
      return MEDIUM_HIGH_ELEVATION_DENSITY;
   }
   
   return HIGH_ELEVATION_DENSITY;
}

ossimPlanetTerrain::TextureDensityType ossimPlanetTerrain::textureDensityType()const
{
   ossim_int32 size = ossim::max(textureTileWidth(), textureTileHeight());
   
   if(size <=64)
   {
      return LOW_TEXTURE_DENSITY;
   }
   else if(size <=128)
   {
      return MEDIUM_LOW_TEXTURE_DENSITY;
   }
   else if(size <=256)
   {
      return MEDIUM_TEXTURE_DENSITY;
   }
   else if(size <=512)
   {
      return MEDIUM_HIGH_TEXTURE_DENSITY;
   }
   
   return HIGH_TEXTURE_DENSITY;
}

void ossimPlanetTerrain::setSplitMergePixelMetricParameters(ossim_float64 mergeMetric,
                                                            ossim_float64 splitMetric)
{
  theSplitPixelMetric = splitMetric;
  theMergePixelMetric = mergeMetric;
}

void ossimPlanetTerrain::setSplitMergeMetricType(SplitMergeMetricType type)
{
  theSplitMergeMetricType = type;
}

ossimPlanetTerrain::SplitMergeMetricType ossimPlanetTerrain::splitMergeMetricType()const
{
  return theSplitMergeMetricType;
}

ossim_float64 ossimPlanetTerrain::splitPixelMetric()const
{
  return theSplitPixelMetric;
}

ossim_float64 ossimPlanetTerrain::mergePixelMetric()const
{
  return theMergePixelMetric;

}

void ossimPlanetTerrain::setSplitMergeSpeedType(SplitMergeSpeedType type)
{
   switch(type)
   {
      case LOW_SPEED:
      {
         setSplitMergeLodScale(1.5);
         theSplitPixelMetric = 16;
         theMergePixelMetric = 8;
         break;
      }
      case MEDIUM_LOW_SPEED:
      {
         setSplitMergeLodScale(3.0);
         theSplitPixelMetric = 10;
         theMergePixelMetric = 5;
         break;
      }
      case MEDIUM_SPEED:
      {
         setSplitMergeLodScale(5.0);
         theSplitPixelMetric = 8;
         theMergePixelMetric = 4;
         break;
      }
      case MEDIUM_HIGH_SPEED:
      {
         setSplitMergeLodScale(6.5);
         theSplitPixelMetric = 6;
         theMergePixelMetric = 3;
         break;
      }
      case HIGH_SPEED:
      {
         setSplitMergeLodScale(8.0);
         theSplitPixelMetric = 4;
         theMergePixelMetric = 2;
         break;
      }
   }
}


void ossimPlanetTerrain::setSplitMergeLodScale(ossim_float64 ratio)
{
   theSplitMergeLodScale = ratio;
}

ossim_float64 ossimPlanetTerrain::splitMergeLodScale()const
{
   return theSplitMergeLodScale;
}

void ossimPlanetTerrain::setElevationExaggeration(ossim_float64 exaggeration)
{
   theElevationExaggeration = exaggeration;
}

ossim_float64 ossimPlanetTerrain::elevationExaggeration()const
{
   return theElevationExaggeration;
}

void ossimPlanetTerrain::setMinimumTimeToCompilePerFrameInSeconds(double timeInSeconds)
{
   theMinimumTimeToCompilePerFrame = timeInSeconds;
}

ossim_float64 ossimPlanetTerrain::minimumTimeToCompilePerFrame()const
{
   return theMinimumTimeToCompilePerFrame;
}

void ossimPlanetTerrain::setPrecompileEnabledFlag(bool flag)
{
   thePrecompileEnabledFlag = flag;
}

bool ossimPlanetTerrain::precompileEnabledFlag()const
{
   return thePrecompileEnabledFlag;
}
