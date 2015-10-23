#include <ossimPlanet/ossimPlanetTileRequest.h>
#include <ossimPlanet/ossimPlanetTerrainTile.h>
#include <ossimPlanet/ossimPlanetTerrainLayer.h>
#include <ossimPlanet/ossimPlanetTerrain.h>
#include <ossimPlanet/ossimPlanetCubeGrid.h>
#include <osgUtil/GLObjectsVisitor>
#include <osg/Timer>

#if (((OPENSCENEGRAPH_MAJOR_VERSION<2) || (OPENSCENEGRAPH_MAJOR_VERSION==2 && (OPENSCENEGRAPH_MINOR_VERSION<8 || (OPENSCENEGRAPH_MINOR_VERSION==8 && OPENSCENEGRAPH_PATCH_VERSION<=2)))))
#   define USE_OLD_VBO_COMPILE 0
#else
#   define USE_OLD_VBO_COMPILE 1
#endif
void ossimPlanetTileRequest::FindCompileableGLObjectsVisitor::apply(osg::Node& node)
{
   apply(node.getStateSet());
   
   traverse(node);
}

void ossimPlanetTileRequest::FindCompileableGLObjectsVisitor::apply(osg::Geode& geode)
{
   apply(geode.getStateSet());
   
   for(unsigned int i=0;i<geode.getNumDrawables();++i)
   {
      apply(geode.getDrawable(i));
   }
   
   traverse(geode);
}

void ossimPlanetTileRequest::FindCompileableGLObjectsVisitor::apply(osg::StateSet* stateset)
{
   if (stateset)
   {
      bool compileStateSet = false;
      for(unsigned int i=0;i<stateset->getTextureAttributeList().size();++i)
      {
         osg::Texture* texture = dynamic_cast<osg::Texture*>(stateset->getTextureAttribute(i,osg::StateAttribute::TEXTURE));
         // Has this texture already been encountered?
         if (texture && !theTextureSet.count(texture))
         {
         //   if (texture->getTextureParameterDirty(theContextId)||
         //       (texture->getTextureObject(theContextId)==0))
            {
               theDataToCompile.textures.insert(texture);
            }
         }
      }
   }
}

void ossimPlanetTileRequest::FindCompileableGLObjectsVisitor::apply(osg::Drawable* drawable)
{
   if (theDrawableSet.count(drawable))
      return;
   
   apply(drawable->getStateSet());
 
   if(drawable->getUseVertexBufferObjects())
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
//               if(arrayList[idx]->getVertexBufferObject()->isDirty(theContextId))
               {
//	               theDataToCompile.vbos.insert(arrayList[idx]->getVertexBufferObject());
               }
            }
         }
      }
   }
}

ossimPlanetTileRequest::ossimPlanetTileRequest(ossimPlanetTerrainTile* tile)
{
   setThreadSafeRefUnref(true);
   setTile(tile);
}

void ossimPlanetTileRequest::setTile(ossimPlanetTerrainTile* tile)
{
   theTile = tile;
   if(theTile.valid())
   {
      theTileId  = theTile->tileId();
      theTerrain = theTile->terrain();
   }
}
ossimPlanetTerrainTile* ossimPlanetTileRequest::tile()
{
   return theTile.get();
}

const ossimPlanetTerrainTile* ossimPlanetTileRequest::tile()const
{
   return theTile.get();
}

bool ossimPlanetTileRequest::compileObjects(osg::RenderInfo& renderInfo, 
                                            double availableTimeInSeconds)
{
   return true; // we have finished
}

bool ossimPlanetTileRequest::populateCompileSet(osgUtil::IncrementalCompileOperation::ContextSet& contexts, 
                                osgUtil::IncrementalCompileOperation::CompileSet& compileSet)
{
   return true;
}

ossimPlanetTileRequestQueue::ossimPlanetTileRequestQueue(bool sortFlag)
:theCurrentFrameNumber(0),
theSortFlag(sortFlag)
{
   
}

void ossimPlanetTileRequestQueue::sort()
{
   if(theSortFlag)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
      theOperationQueue.sort(ossimPlanetTileRequest::SortFunctor());
   }
}

osg::ref_ptr<ossimPlanetOperation> ossimPlanetTileRequestQueue::nextOperation(bool blockIfEmptyFlag)
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
      ossimPlanetOperation::List::iterator iter = theOperationQueue.begin();
      ossimPlanetOperation::List::iterator endIter = theOperationQueue.end();
      
      while(iter != endIter)
      {
         // std::cout << "CHECKING STOPPED!!!!!!!!!!!!!!!!"<< std::endl;
         ossimPlanetTileRequest* tileRequest = dynamic_cast<ossimPlanetTileRequest*>((*iter).get());
         if(!tileRequest->isRequestCurrent(theCurrentFrameNumber)||tileRequest->isStopped())
         {
            iter = theOperationQueue.erase(iter);
         }
         else
         {
            ++iter;
         }
      }
   }
   if(theSortFlag)
   {
      sort();
   }
   osg::ref_ptr<ossimPlanetOperation> operation = ossimPlanetOperationQueue::nextOperation(blockIfEmptyFlag).get();
   while(operation.valid())
   {
      ossimPlanetTileRequest* tileRequest = dynamic_cast<ossimPlanetTileRequest*>(operation.get());
      if(!tileRequest)
      {
         return operation;
      }
      if(tileRequest->isRequestCurrent(theCurrentFrameNumber)&&!tileRequest->isStopped())
      {
         return operation;
      }
      operation = 0;
      operation = ossimPlanetOperationQueue::nextOperation(blockIfEmptyFlag).get();
   }
   
   return 0;
}

void ossimPlanetTileRequestQueue::add(ossimPlanetTileRequest* request)
{
   if(request)
   {
      if(request->referenceCount() == 1)
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
         request->setState(ossimPlanetOperation::READY_STATE);
         theOperationQueue.push_back(request);
      }
   }
}

void ossimPlanetTileRequestThreadQueue::add(ossimPlanetTileRequest* request)

{
   if(request)
   {
      if(request->referenceCount() == 1)
      {
         ossimPlanetOperationThreadQueue::add(request);
      }
   }
}
void ossimPlanetTileRequestThreadQueue::run()
{
   bool firstTime = true;
   
   do
   {
      // osg::notify(osg::NOTICE)<<"In thread loop "<<this<<std::endl;
      osg::ref_ptr<ossimPlanetOperation> operation;
      osg::ref_ptr<ossimPlanetOperationQueue> queue;
      
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
         queue = theOperationQueue;
      }
      operation = queue->nextOperation(true);
      
      if (theDoneFlag) break;
      
      if (operation.valid())
      {
         ossimPlanetTileRequest* request = dynamic_cast<ossimPlanetTileRequest*>(operation.get());
         if(request&&request->isRequestCurrent(theCurrentFrameNumber))
         {
            {
               OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
               theCurrentOperation = operation;
            }
            
            if(operation->state() == ossimPlanetOperation::READY_STATE)
            {
               operation->start();
               if(operation->state() != ossimPlanetOperation::CANCELED_STATE)
               {
                  ossimPlanetTileRequest* tileRequest = dynamic_cast<ossimPlanetTileRequest*>(operation.get());
                  if(tileRequest)
                  {
                     osg::ref_ptr<ossimPlanetTerrainTile> tile = tileRequest->tile();
                     if(tile.valid())
                     {
                        if(tileRequest->needsToCompile())
                        {

                           tile->terrain()->addRequestToNeedToCompileQueue(tileRequest);
                        }
                        else
                        {
                           tile->terrain()->addRequestToReadyToApplyQueue(tileRequest);
                        }
                     }
                  }
               }
            }
            {            
               OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
               theCurrentOperation = 0;
            }
         }
         operation = 0;
      }
      
      if (firstTime)
      {
         // do a yield to get round a peculiar thread hang when testCancel() is called 
         // in certain cirumstances - of which there is no particular pattern.
         YieldCurrentThread();
         firstTime = false;
      }
   } while (!testCancel() && !theDoneFlag);
}

#if 0
void ossimPlanetTerrain::TileRequestThreadQueue::applyToGraph(double availableTime)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theFinishedOperationsListMutex);
   osg::Timer_t startTick = osg::Timer::instance()->tick();
   bool doneFlag = theFinishedOperationsList.empty();
   ossim_uint32 operationCount = 0;
   while(!doneFlag)
   {
      theFinishedOperationsList.sort(SortRequestFunctor());
      (*theFinishedOperationsList.begin())->applyToGraph();
      theFinishedOperationsList.erase(theFinishedOperationsList.begin());
      ++operationCount;
      double delta = osg::Timer::instance()->delta_m(startTick, osg::Timer::instance()->tick());
      doneFlag = (theFinishedOperationsList.empty()||
                  (delta > availableTime)||
                  (operationCount >=theMaxNumberOfOperationsToApply));
   }
}
#endif

ossimPlanetSplitRequest::ossimPlanetSplitRequest(ossimPlanetTerrainTile* tile)
:ossimPlanetTileRequest(tile)
{
   if(tile)
   {
      tile->vacantChildIds(theNeededChildrenList);
   }
   
}


void ossimPlanetSplitRequest::run()
{
   theNewTiles.clear();
   if((theNeededChildrenList.size()>0)&&(state()!=CANCELED_STATE)&&(theTile.valid()))
   {
      ossim_uint32 idx = 0;
      osg::ref_ptr<ossimPlanetTerrainImageLayer> elevationLayer = theTile->elevationLayer();
      if(!elevationLayer)
      {
         return;
      }
      ossim_uint32 textureWidth    = theTile->terrain()->textureTileWidth();
      ossim_uint32 textureHeight   = theTile->terrain()->textureTileHeight();
      ossim_uint32 elevationWidth  = theTile->terrain()->elevationTileWidth();
      ossim_uint32 elevationHeight = theTile->terrain()->elevationTileHeight();
      osg::ref_ptr<ossimPlanetImage> inputElevationImage = elevationLayer->image();
      osg::ref_ptr<ossimPlanetImage> scaledImage;
      //               elevationImage = new ossimPlanetImage(inputElevationImage->tileId());
      //               elevationImage->allocateImage(inputElevationImage->widthWithoutPadding(), 
      //                                             inputElevationImage->heightWithoutPadding(), 
      //                                             1, 
      //                                             inputElevationImage->getPixelFormat(), 
      //                                             inputElevationImage->getDataType(), 
      //                                             inputElevationImage->getPacking());
      //               elevationImage->setPadding(0);
      
      if(inputElevationImage.valid())
      {
         if(inputElevationImage->padding() > 0)
         {
            // strip padding for scaling
            //
            osg::ref_ptr<ossimPlanetImage> tempImage = new ossimPlanetImage(*inputElevationImage);
            tempImage->stripPadding();
            inputElevationImage = tempImage;
         }
         
         scaledImage = inputElevationImage->scaleImagePowerOf2();
     }
      if(theNeededChildrenList.size() > 0)
      {
         ossimPlanetTerrainTile* newTile = new ossimPlanetTerrainTile(theNeededChildrenList[idx]);
         newTile->setTerrain(theTerrain.get());
         ossim_uint32 idx2 = 0;
         for(idx2 = 0; idx2 < newTile->numberOfImageLayers(); ++idx2)
         {
            if(theTile->imageLayer(idx2) && newTile->imageLayer(idx2))
            {
#if 1
               bool hasDataFlag = newTile->terrain()->textureLayer(idx2)->hasTexture(textureWidth, 
                                                                                     textureHeight,
                                                                                     newTile->tileId(), 
                                                                                     *theTile->grid());
               newTile->imageLayer(idx2)->setNoMoreDataFlag(!hasDataFlag);//theTile->imageLayer(idx2)->noMoreDataFlag());
#else
               newTile->imageLayer(idx2)->setNoMoreDataFlag(theTile->imageLayer(idx2)->noMoreDataFlag());
#endif
               
            }
         }
         osg::ref_ptr<ossimPlanetImage> elevationImage;
         osg::ref_ptr<ossimPlanetImageCache> cache = theTile->terrain()->elevationCache();
         if(cache.valid())
         {
            elevationImage = cache->get(newTile->tileId());
         }
         // interpolate elevation if it's present
         //
         if(!elevationImage.valid())
         {
            if(inputElevationImage.valid())
            {
               bool hasDataFlag = newTile->terrain()->elevationLayer()->hasTexture(elevationWidth, 
                                                                                   elevationHeight,
                                                                                   newTile->tileId(), 
                                                                                   *theTile->grid());
//               ossim_uint32 xLength = inputElevationImage->s()>>1;
//               ossim_uint32 yLength = inputElevationImage->t()>>1;
               ossim_uint32 x;
               ossim_uint32 y;
               theTile->terrainTechnique()->childTreePosition(theNeededChildrenList[idx], x, y);
               
#if 1
               x=x*(scaledImage->getWidth()*.5)  - x*scaledImage->padding();
               y=y*(scaledImage->getHeight()*.5) - y*scaledImage->padding();
//               elevationImage = new ossimPlanetImage(inputElevationImage->tileId());
//               elevationImage->allocateImage(inputElevationImage->widthWithoutPadding(), 
//                                             inputElevationImage->heightWithoutPadding(), 
//                                             1, 
//                                             inputElevationImage->getPixelFormat(), 
//                                             inputElevationImage->getDataType(), 
//                                             inputElevationImage->getPacking());
//               elevationImage->setPadding(0);
               elevationImage = new ossimPlanetImage(*inputElevationImage.get());
               scaledImage->copyTo(x, y, elevationImage.get());
#else
               elevationImage = new ossimPlanetImage(*inputElevationImage.get());
               x=x*(elevationImage->getWidth()*.5) - x*elevationImage->padding();
               y=y*(elevationImage->getHeight()*.5) - y*elevationImage->padding();
               elevationImage->copySubImageAndInsertPointsPowerOf2(x,
                                                                   y,
                                                                   elevationImage->getWidth(),
                                                                   elevationImage->getHeight(),
                                                                   inputElevationImage.get());
#endif
//               std::cout << "scaled w, h = " << scaledImage->getWidth() << ", " << scaledImage->getHeight() << std::endl;
//               std::cout << "w, h = " << inputElevationImage->getWidth() << ", " << inputElevationImage->getHeight() << std::endl;
//               std::cout << "x, y = " << x << ", " << y << std::endl;
               
               newTile->elevationLayer()->setImage(elevationImage.get());
            }
            //     newTile->elevationLayer()->setNoMoreDataFlag(!hasDataFlag);//theTile->elevationLayer()->noMoreDataFlag());
            newTile->elevationLayer()->setNoMoreDataFlag(theTile->elevationLayer()->noMoreDataFlag());
         }
         else
         {
            newTile->elevationLayer()->setImage(elevationImage.get());
            newTile->elevationLayer()->setNoMoreDataFlag(theTile->elevationLayer()->noMoreDataFlag());
         }
         newTile->init();
         theNewTiles.push_back(newTile);
      }
   }
}

bool ossimPlanetSplitRequest::populateCompileSet(osgUtil::IncrementalCompileOperation::ContextSet& contexts, 
                                                osgUtil::IncrementalCompileOperation::CompileSet& compileSet)
{
  if((state()!=CANCELED_STATE)&&theNewTiles.size()&&theTile.valid())
   {
      if(theDataToCompile.textures.empty()&&
         theDataToCompile.vbos.empty())
      {
         FindCompileableGLObjectsVisitor visitor(theDataToCompile,
                                                 theTerrain.get(),
                                                 0);
         ossim_uint32 idx = 0;
         for(idx = 0; idx < theNewTiles.size() ; ++idx)
         {
            theNewTiles[idx]->accept(visitor);
         }
      }
      if(!theDataToCompile.textures.empty())
      {
          for(osgUtil::IncrementalCompileOperation::ContextSet::iterator itr = contexts.begin();
              itr != contexts.end();
              ++itr)
          {
            ++compileSet._numberCompileListsToCompile;

            osgUtil::IncrementalCompileOperation::CompileList& cl = compileSet._compileMap[*itr];

            for(TextureSetList::iterator iter = theDataToCompile.textures.begin();
               iter!=theDataToCompile.textures.end();++iter)
            {
               cl.add((*iter).get());
            }
         }
      }
   }
   return true;
}
bool ossimPlanetSplitRequest::compileObjects(osg::RenderInfo& renderInfo, 
                                             double availableTimeInSeconds)
{
 //  osg::Timer_t tick = osg::Timer::instance()->tick();
   if((state()!=CANCELED_STATE)&&theNewTiles.size()&&theTile.valid()&&renderInfo.getState())
   {
      if(theDataToCompile.textures.empty()&&
         theDataToCompile.vbos.empty())
      {
         FindCompileableGLObjectsVisitor visitor(theDataToCompile,
                                                 theTerrain.get(),
                                                 renderInfo.getState()->getContextID());
         ossim_uint32 idx = 0;
         for(idx = 0; idx < theNewTiles.size() ; ++idx)
         {
            theNewTiles[idx]->accept(visitor);
         }
      }
      // do textures first
      while(!theDataToCompile.textures.empty())
      {
         (*theDataToCompile.textures.begin())->compileGLObjects(*renderInfo.getState());
         theDataToCompile.textures.erase(theDataToCompile.textures.begin());
      }
      while(!theDataToCompile.vbos.empty())
      {
#if 0
#if USE_OLD_VBO_COMPILE
       (*theDataToCompile.vbos.begin())->compileBuffer(*renderInfo.getState());
#else
         osg::GLBufferObject* glBufferObj = (*theDataToCompile.vbos.begin())->getOrCreateGLBufferObject(renderInfo.getState()->getContextID());

         if(glBufferObj&&glBufferObj->isDirty())
         {
            glBufferObj->compileBuffer();
         }
#endif
#endif
         theDataToCompile.vbos.erase(theDataToCompile.vbos.begin());

      }
   }
   if(state()==CANCELED_STATE)
   {
      theNewTiles.clear();
   }
   
   return true;
//   if(osg::Timer::instance()->delta_m(tick, osg::Timer::instance()->tick()) > 1.0)
//   {
//      std::cout << "ossimPlanetTextureRequest::compile: TOO LONG!!!!" << std::endl;
//   }
}


void ossimPlanetSplitRequest::applyToGraph()
{
   if((state()!=CANCELED_STATE)&&
       theTile.valid()&&theNewTiles.size())//&&(theTile->state() ==  ossimPlanetTerrainTile::NEED_SPLITTING))
   {
      ossim_uint32 idx = 0;
      for(idx = 0; idx < theNewTiles.size();++idx)
      {
         theTile->addChild(theNewTiles[idx].get());
      }
   }
   theNewTiles.clear();
}

void ossimPlanetSplitRequest::setTile(ossimPlanetTerrainTile* tile)
{
   ossimPlanetTileRequest::setTile(tile);
   theNeededChildrenList.clear();
   if(tile)
   {
      tile->vacantChildIds(theNeededChildrenList);
   }
}

ossimPlanetTextureRequest::ossimPlanetTextureRequest()
:ossimPlanetTileRequest(){
}

ossimPlanetTextureRequest::ossimPlanetTextureRequest(ossimPlanetTerrainTile* tile, 
                                                   ossim_uint32 imageLayerIdx)
:ossimPlanetTileRequest(tile)
{
   setImageLayerIdx(imageLayerIdx);
}

void ossimPlanetTextureRequest::setTextureLayerIndices(const std::vector<ossim_uint32>& values)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   ossim_uint32 idx;
   theResultList.clear();
   for(idx = 0; idx < values.size();++idx)
   {
      theResultList.insert(std::make_pair(values[idx], Result()));
   }
}

void ossimPlanetTextureRequest::run()
{
   if(theTile.valid()&&theTerrain.valid())
   {
      if(state() != CANCELED_STATE)
      {
         ossim_uint32 w,h;
         w = theTile->terrain()->textureTileWidth();
         h = theTile->terrain()->textureTileHeight();
         
         const ossimPlanetTerrainTileId& tileId = theTile->tileId();
         TextureResultMap::iterator iter = theResultList.begin();
         while(iter!=theResultList.end())
         {
            osg::ref_ptr<ossimPlanetTerrainImageLayer> imageLayer = theTile->imageLayer(iter->first);
            osg::ref_ptr<ossimPlanetTextureLayer> layer = theTerrain->textureLayer(iter->first);
            if(layer.valid())
            {
               iter->second.theImage = layer->getTexture(w,
                                                         h,
                                                         tileId,
                                                         *theTile->grid());
               if(iter->second.theImage.valid())
               {
                  iter->second.theImage->setId(tileId);
                  iter->second.theTexture = theTile->terrainTechnique()->newImageLayerTexture(iter->first);
                  iter->second.theTexture->setImage(iter->second.theImage.get());
               }
            }
            ++iter;
         }
      }
   }
}


bool ossimPlanetTextureRequest::compileObjects(osg::RenderInfo& renderInfo, 
                                               double availableTimeInSeconds)
{
   osg::Timer_t tick = osg::Timer::instance()->tick();
   TextureResultMap::iterator iter = theResultList.begin();
   while(iter!=theResultList.end())
   {
      if((state()!=CANCELED_STATE)&&
         iter->second.theTexture.get()&&
         theTile.valid()&&renderInfo.getState())
      {
         if ((iter->second.theTexture->getTextureParameterDirty(renderInfo.getState()->getContextID()))||
             (iter->second.theTexture->getTextureObject(renderInfo.getState()->getContextID())==0))
         {
            iter->second.theTexture->compileGLObjects(*renderInfo.getState());
         }
      }
      //double delta = osg::Timer::instance()->delta_s(tick, osg::Timer::instance()->tick());
      ++iter;
   }
      
   return true; // no more compilation required
}

bool ossimPlanetTextureRequest::populateCompileSet(osgUtil::IncrementalCompileOperation::ContextSet& contexts, 
                                                osgUtil::IncrementalCompileOperation::CompileSet& compileSet)
{
  if((state()!=CANCELED_STATE))
   {
      if(!theResultList.empty())
      {
          for(osgUtil::IncrementalCompileOperation::ContextSet::iterator itr = contexts.begin();
              itr != contexts.end();
              ++itr)
          {
            ++compileSet._numberCompileListsToCompile;

            osgUtil::IncrementalCompileOperation::CompileList& cl = compileSet._compileMap[*itr];
            TextureResultMap::iterator iter = theResultList.begin();
            while(iter!=theResultList.end())
            {
               cl.add(iter->second.theTexture.get());
               ++iter;
            }
         }

      }
   }
   return true;
}


void ossimPlanetTextureRequest::applyToGraph()
{
   //std::cout << " ossimPlanetTerrain::TextureRequest::applyToGraph() entered...." << std::endl;
   if(state()!=CANCELED_STATE)
   {
      TextureResultMap::iterator iter = theResultList.begin();
      while(iter!=theResultList.end()&&theTile.valid())
      {
         
         if(iter->second.theTexture.valid())
         {
            theTile->terrainTechnique()->setImageLayerTexture(iter->second.theTexture.get(), iter->first);
         }
         else
         {
            if(theTile->imageLayer(iter->first))
            {
               theTile->imageLayer(iter->first)->setNoMoreDataFlag(true);
            }
         }
         ++iter;
      }
   }
}

ossimPlanetElevationRequest::ossimPlanetElevationRequest()
:ossimPlanetTileRequest()
{
}

ossimPlanetElevationRequest::ossimPlanetElevationRequest(ossimPlanetTerrainTile* tile,
                                                         ossim_uint32 width,
                                                         ossim_uint32 height)
:ossimPlanetTileRequest(tile)
{
}

void ossimPlanetElevationRequest::run()
{
   theNewMesh = 0;
   theImage   = 0;
   theDataToCompile.textures.clear();
   theDataToCompile.vbos.clear();
   ossim_int32 padding = 1;
   if(theTile.valid()&&theTile->grid()&&theTile->terrain())
   {
      if(state() != CANCELED_STATE)
      {
         ossim_uint32 w, h;
         w = theTile->terrain()->elevationTileWidth();
         h = theTile->terrain()->elevationTileHeight();
         osg::ref_ptr<ossimPlanetTerrainImageLayer> imageLayer = theTile->elevationLayer();
         osg::ref_ptr<ossimPlanetTextureLayer> elevLayer = theTile->terrain()->elevationLayer();
         if(imageLayer.valid()&&elevLayer.valid())
         {
            osg::ref_ptr<ossimPlanetImageCache> cache = theTile->terrain()->elevationCache();
            if(cache.valid())
            {
               theImage = cache->get(theTile->tileId());
            }
//            if(elevLayer->hasTexture(w,
//                                     h,
//                                    theTile->tileId(),
//                                    *theTile->grid()) || (theTile->tileId().level()==0))
            {
               const ossimPlanetTerrainTileId& tileId = theTile->tileId();
               if(!theImage)
               {
                  theImage = elevLayer->getTexture(w,
                                                   h,
                                                   theTile->tileId(),
                                                   *theTile->grid(),
                                                   padding);
                  if(cache.valid())
                  {
                     cache->addOrUpdate(theImage.get());
                  }
               }
               if(theImage.valid())
               {
                  theImage->setId(tileId);
                  
                  theNewMesh = new ossimPlanetTerrainTile(theTile->tileId());
                  theNewMesh->copyCommonParameters(theTile.get());
                  //imageLayer->setImage(theImage.get());
                  //imageLayer->setRefreshFlag(false);
                  theNewMesh->elevationLayer()->setImage(theImage.get());
                  theNewMesh->init();
               }
               else
               {
                  //std::cout << "NO ELEVATION" << std::endl;
               }
            }
#if 0
            else
            {
               if(theTile->parentTile())
               {
                  osg::ref_ptr<ossimPlanetImage> img = theTile->parentTile()->elevationLayer()->image();
                  if(img.valid())
                  {
                     // we will do it the way the split does it
                     //
                     std::cout << "WE CAN SPLIT!!!!" << std::endl;
                  }
                  else
                  {
                     std::cout << "WE CAN'T SPLIT!!!!" << std::endl;
                  }
               }
               else
               {
                  std::cout << "NO PARENT WE CAN'T SPLIT!!!!" << std::endl;
               }
            }
#endif
         }
      }
   }
}

bool ossimPlanetElevationRequest::compileObjects(osg::RenderInfo& renderInfo, 
                                                 double availableTimeInSeconds)
{
   if(!renderInfo.getState()||!theNewMesh.valid()) return true; // no more to compile
   if(theDataToCompile.textures.empty()&&
      theDataToCompile.vbos.empty())
   {
      FindCompileableGLObjectsVisitor visitor(theDataToCompile,
                                              theTerrain.get(),
                                              renderInfo.getState()->getContextID());
      theNewMesh->accept(visitor);
   }
   // do textures first
   while(!theDataToCompile.textures.empty())
   {
      (*theDataToCompile.textures.begin())->compileGLObjects(*renderInfo.getState());
      theDataToCompile.textures.erase(theDataToCompile.textures.begin());
   }
   while(!theDataToCompile.vbos.empty())
   {
#if 0
#if USE_OLD_VBO_COMPILE
       (*theDataToCompile.vbos.begin())->compileBuffer(*renderInfo.getState());
#else
      osg::GLBufferObject* glBufferObj = (*theDataToCompile.vbos.begin())->getOrCreateGLBufferObject(renderInfo.getState()->getContextID());
      if(glBufferObj&&glBufferObj->isDirty())
      {
         glBufferObj->compileBuffer();
      }
#endif
#endif
      theDataToCompile.vbos.erase(theDataToCompile.vbos.begin());
   }
//   osg::Timer_t tick = osg::Timer::instance()->tick();
   if(state()==CANCELED_STATE)
   {
      theNewMesh = 0;
   }  
   return true; // no more to compile
}
bool ossimPlanetElevationRequest::populateCompileSet(osgUtil::IncrementalCompileOperation::ContextSet& contexts, 
                                                osgUtil::IncrementalCompileOperation::CompileSet& compileSet)
{
  if((state()!=CANCELED_STATE))
   {
      if(theDataToCompile.textures.empty()&&
         theDataToCompile.vbos.empty())
      {
         FindCompileableGLObjectsVisitor visitor(theDataToCompile,
                                                 theTerrain.get(),
                                                 0);
         theNewMesh->accept(visitor);
      }
      if(!theDataToCompile.textures.empty())
      {
         ossim_uint32 idx = 0;
          for(osgUtil::IncrementalCompileOperation::ContextSet::iterator itr = contexts.begin();
              itr != contexts.end();
              ++itr)
          {
            ++compileSet._numberCompileListsToCompile;

            osgUtil::IncrementalCompileOperation::CompileList& cl = compileSet._compileMap[*itr];

            // do textures first
            for(TextureSetList::iterator iter = theDataToCompile.textures.begin();
               iter!=theDataToCompile.textures.end();++iter)
            {
               cl.add((*iter).get());
            }
         }
      }
   }
   return true;
}

void ossimPlanetElevationRequest::applyToGraph()
{
   if((state()!=CANCELED_STATE)&&theTile.valid()&&theImage.valid()&&theNewMesh.valid())
   {
      osg::ref_ptr<ossimPlanetTerrainImageLayer> imageLayer = theTile->elevationLayer();
      theTile->terrainTechnique()->setElevationMeshFrom(theNewMesh.get());
   }
   theNewMesh = 0;
}

