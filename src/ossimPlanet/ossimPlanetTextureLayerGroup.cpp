#include <ossimPlanet/ossimPlanetTextureLayerGroup.h>
#include <algorithm>
#include <stack>
#include <queue>
#include <ossim/base/ossimDrect.h>
#include <ossim/base/ossimGpt.h>


class ossimPlanetTextureLayerListener : public ossimPlanetTextureLayerCallback
{
public:
   ossimPlanetTextureLayerListener(ossimPlanetTextureLayer* layer)
      :theLayer(layer)
      {
      }

   void setLayer(ossimPlanetTextureLayer* layer)
      {
         theLayer = layer;
      }
   virtual void layerAdded(osg::ref_ptr<ossimPlanetTextureLayer> layer)
      {
         if(theLayer)
         {
            theLayer->notifyLayerAdded(layer);
         }
      }
   virtual void refreshExtent(osg::ref_ptr<ossimPlanetExtents> extent)
      {
         if(theLayer)
         {
            theLayer->notifyRefreshExtent(extent);            
         }
      }
   virtual void layerRemoved(osg::ref_ptr<ossimPlanetTextureLayer> layer,
                             osg::ref_ptr<ossimPlanetTextureLayer> parent)
      {
         if(theLayer)
         {
            theLayer->notifyLayerRemoved(layer, parent);
         }
      }
   virtual void propertyChanged(const ossimString& name,
                                const ossimPlanetTextureLayer* object)
   {
      if(theLayer)
      {
         theLayer->notifyPropertyChanged(name, object);
      }
   }
protected:
   ossimPlanetTextureLayer* theLayer;
};

ossimPlanetTextureLayerGroup::ossimPlanetTextureLayerGroup()
:theBackgroundColor(1.0,1.0,1.0,1.0),
theFillEmptyNullTileMaxLevel(-1),
theFillTranslucentPixelsWithBackgroundEnabled(false)
{
   theName        = "ossimPlanetTextureLayerGroup";
   theDescription = "ossimPlanetTextureLayerGroup";
   theChildListener = new ossimPlanetTextureLayerListener(this);
}

ossimPlanetTextureLayerGroup::ossimPlanetTextureLayerGroup(const ossimPlanetTextureLayerGroup& src)
   :ossimPlanetTextureLayer(src)
{
   
   theChildListener = new ossimPlanetTextureLayerListener(this);
   if(&src != this)
   {
      ossim_uint32 idx = 0;
      
      for(idx = 0; idx < src.theChildrenList.size(); ++idx)
      {
         if(src.theChildrenList[idx].valid())
         {
            osg::ref_ptr<ossimPlanetTextureLayer> layer = src.theChildrenList[idx]->dup();
            addBottom(layer.get());
         }
         else
         {
            // empty for now.  Not sure if we want to push a null layer
         }
      }
   }
   
   theBackgroundColor = src.theBackgroundColor;
   theFillEmptyNullTileMaxLevel = src.theFillEmptyNullTileMaxLevel;
   
   theFillTranslucentPixelsWithBackgroundEnabled = src.theFillTranslucentPixelsWithBackgroundEnabled;
}

ossimPlanetTextureLayerGroup::~ossimPlanetTextureLayerGroup()
{
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theChildrenList.size();++idx)
   {
      theChildrenList[idx]->removeCallback(theChildListener);
      theChildrenList[idx]->removeParent(this);
   }
}

ossimPlanetTextureLayer* ossimPlanetTextureLayerGroup::dup()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);

   return new ossimPlanetTextureLayerGroup(*this);
}

ossimPlanetTextureLayer* ossimPlanetTextureLayerGroup::dupType()const
{
   return new ossimPlanetTextureLayerGroup();
}

ossimString ossimPlanetTextureLayerGroup::getClassName()const
{
   return "ossimPlanetTextureLayerGroup";
}

ossimPlanetTextureLayerStateCode ossimPlanetTextureLayerGroup::updateExtents()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);

   theStateCode = ossimPlanetTextureLayer_VALID;
   ossim_uint32 idx;
   for(idx = 0; idx < theChildrenList.size(); ++idx)
   {
     theStateCode = (ossimPlanetTextureLayerStateCode)(theStateCode|theChildrenList[idx]->updateExtents());

      const osg::ref_ptr<ossimPlanetExtents> extents = theChildrenList[idx]->getExtents();

      if(idx != 0)
      {
         theExtents->combine(extents.get());
      }
      else
      {
         theExtents = extents->clone();
      }

   }
   theDirtyExtentsFlag = false;
   
   return theStateCode;
}

void ossimPlanetTextureLayerGroup::updateStats()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(theChildrenListMutex);
   theStats->setTotalTextureSize(0);
   theStats->setBytesTransferred(0);
   ossim_uint32 idx;
   for(idx = 0; idx < theChildrenList.size(); ++idx)
   {
      theChildrenList[idx]->updateStats();
      osg::ref_ptr<ossimPlanetTextureLayer::Stats> stats = theChildrenList[idx]->getStats();
      theStats->setBytesTransferred(theStats->bytesTransferred() + stats->bytesTransferred());
      theStats->setTotalTextureSize(theStats->totalTextureSize() + stats->totalTextureSize());
   }
   theDirtyStatsFlag = false;
}

void ossimPlanetTextureLayerGroup::resetStats()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(theChildrenListMutex);
   ossim_uint32 idx;
   theStats->setBytesTransferred(0);
   theStats->setTotalTextureSize(0);
   for(idx = 0; idx < theChildrenList.size(); ++idx)
   {
      osg::ref_ptr<ossimPlanetTextureLayer::Stats> stats = theChildrenList[idx]->getStats();
      theStats->setTotalTextureSize(theStats->totalTextureSize() + stats->totalTextureSize());
   }
}

bool ossimPlanetTextureLayerGroup::hasTexture(ossim_uint32 width,
                                              ossim_uint32 height,
                                              const ossimPlanetTerrainTileId& tileId,
                                              const ossimPlanetGrid& grid)
{
   if(!theEnableFlag)
   {
      return false;
   }
   if(tileId.level() == 0) return true;

   if(theDirtyExtentsFlag)
   {
      updateExtents();
   }
   
   if(theExtents.valid())
   {
      osg::ref_ptr<ossimPlanetExtents> extents = new ossimPlanetExtents;
      if(grid.convertToGeographicExtents(tileId, *extents, width, height))
      {
         if(!theExtents->intersectsLatLon(*extents)&&
            !theExtents->intersectsScale(*extents))
         {
            return false;
         }
      }
   }
   else
   {
      return false;
   }
   
   bool result = false;
   ossim_uint32 idx = 0;
   for(idx = 0; ((idx < theChildrenList.size())&&!result); ++idx)
   {
      if(theChildrenList[idx]->hasTexture(width, height, tileId, grid))
      {
         result = true;
      }
   }
   return result;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetTextureLayerGroup::getTexture(ossim_uint32 width,
                                                                        ossim_uint32 height,
                                                                        const ossimPlanetTerrainTileId& tileId,
                                                                        const ossimPlanetGrid& grid,
                                                                        ossim_int32 padding)
{
   if(!theEnableFlag)
   {
      return 0;
   }
   if(theDirtyExtentsFlag)
   {
      updateExtents();
   }
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   osg::ref_ptr<ossimPlanetExtents> tileExtents = new ossimPlanetExtents;
   if(grid.convertToGeographicExtents(tileId, *tileExtents, width, height))
   {
      if(theExtents.valid())
      {
         if(!theExtents->intersectsLatLon(*tileExtents)&&
            !theExtents->intersectsScale(*tileExtents))
         {
            return 0;
         }
      }
   }
   ossim_uint32 idx = 0;
   osg::ref_ptr<ossimPlanetImage> result;
   ossim_float32 opacity = 1.0;
   if(theChildrenList.size() == 1)
   {
     result = theChildrenList[idx]->getTexture(width, height, tileId, grid);
   }
   else
   {
     for(idx = 0; idx < theChildrenList.size(); ++idx)
     {
        if(theChildrenList[idx].valid()&&(theChildrenList[idx]->opacity() > 0.0))
        {
           osg::ref_ptr<ossimPlanetImage> image = theChildrenList[idx]->getTexture(width, height, tileId, grid);

           if(image.valid())
           {

             ossimPlanetImage::ossimPlanetImagePixelStatus pixelStatus = image->getPixelStatus();
              if(result.valid())
              {
                // use previous opacity for merging
                 //mergeImage(image.get(), result.get(), 1.0 - opacity);
                 mergeImage(image.get(), result.get(), opacity);
                 // update opacity
                 opacity = theChildrenList[idx]->opacity();
                 result = image;
                 result->getPixelStatus();
                 pixelStatus = result->getPixelStatus();
                 if((pixelStatus == ossimPlanetImage::ossimPlanetImagePixelStatus_FULL)&&
                     (opacity == 1.0))
                 {
                   // we are finished
                    break;
                 }
              }
              else
              {
                opacity = theChildrenList[idx]->opacity();
                if((opacity == 1.0) &&
                    (pixelStatus == ossimPlanetImage::ossimPlanetImagePixelStatus_FULL))
                {
                  result = image;
                  break;
                }
                else
                {
                  result = (ossimPlanetImage*)image->clone(osg::CopyOp::DEEP_COPY_ALL);
                }
              }// end if result.valid()
           }// end if image.valid()
        }
     }
   }
   if(result.get())
   {
      result = applyBrightnessContrast(result.get(), true);
   }
   if((!result.valid()&&((ossim_int32)tileId.level()<=theFillEmptyNullTileMaxLevel))||
      (result.valid()&&result->getPixelStatus()==ossimPlanetImage::ossimPlanetImagePixelStatus_EMPTY))
   {
      ossim_uint32 area = width*height;
      unsigned char* newData = new unsigned char[area*4];
      ossim_uint8 r = static_cast<ossim_uint8>(theBackgroundColor[0]*255);
      ossim_uint8 g = static_cast<ossim_uint8>(theBackgroundColor[1]*255);
      ossim_uint8 b = static_cast<ossim_uint8>(theBackgroundColor[2]*255);
      ossim_uint8 a = static_cast<ossim_uint8>(theBackgroundColor[3]*255);
      if(r==g&&r==b&&r==a)
      {
         memset(newData, r, area*4);
      }
      else
      {
         ossim_uint32 idx = 0;
         unsigned char* newDataPtr = newData;
         for(idx = 0; idx < area; ++idx,newDataPtr+=4)
         {
            newDataPtr[0] = r;
            newDataPtr[1] = g;
            newDataPtr[2] = b;
            newDataPtr[3] = a;
         }
      }
      result = new ossimPlanetImage();
      result->setImage(width, height, 1,
                        GL_RGBA,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        newData,
                       osg::Image::USE_NEW_DELETE);
   }
   else if(result.valid()&&theFillTranslucentPixelsWithBackgroundEnabled)
   {
      ossimPlanetImage::ossimPlanetImagePixelStatus pixelStatus = result->getPixelStatus();
      if(pixelStatus != ossimPlanetImage::ossimPlanetImagePixelStatus_FULL)
      {
         unsigned char* dataPtr = result->data();
         ossim_uint32 area = result->width()*result->height();
         ossim_uint32 idx = 0;
         ossim_uint8 r = static_cast<ossim_uint8>(theBackgroundColor[0]*255);
         ossim_uint8 g = static_cast<ossim_uint8>(theBackgroundColor[1]*255);
         ossim_uint8 b = static_cast<ossim_uint8>(theBackgroundColor[2]*255);
         ossim_uint8 a = static_cast<ossim_uint8>(theBackgroundColor[3]*255);
         for(idx = 0; idx < area;++idx,dataPtr+=4)
         {
            if(dataPtr[3] < 1)
            {
               dataPtr[0] = r;
               dataPtr[1] = g;
               dataPtr[2] = b;
               dataPtr[3] = a;
            }
         }
         result->setPixelStatus();
      }

   }
   return result;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetTextureLayerGroup::getTexture(ossim_uint32 level,
                                                                        ossim_uint64 row,
                                                                        ossim_uint64 col,
                                                                        const ossimPlanetGridUtility& utility)
{
   if(!theEnableFlag)
   {
      return 0;
   }
   if(theDirtyExtentsFlag)
   {
      updateExtents();
   }
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);

   ossim_uint32 idx = 0;
   osg::ref_ptr<ossimPlanetImage> result;
   double minLat;
   double minLon;
   double maxLat;
   double maxLon;
//   unsigned int w = utility.getTileWidth();
   unsigned int h = utility.getTileHeight();

   utility.getLatLonBounds(minLat,
                           minLon,
                           maxLat,
                           maxLon,
                           level,
                           row, 
                           col );
   if(!theExtents->intersectsLatLon(minLat, minLon, maxLat, maxLon))
   {
      return 0;
   }
   double deltaX;
   double deltaY;
   utility.getWidthHeightInDegrees(deltaX, deltaY, level, row, col);
   
   double deltaLat    = deltaY/h;
//   double deltaLon    = deltaX/w;
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;
   if(gsd.y>theExtents->getMaxScale())//!theExtents->intersectsScale(gsd.y,
       //                            gsd.y))
   {
      return 0;
   }
      
   for(idx = 0; idx < theChildrenList.size(); ++idx)
   {
      if(theChildrenList[idx].valid())
      {
         const osg::ref_ptr<ossimPlanetExtents> extents = theChildrenList[idx]->getExtents();
         
         if(extents->intersectsLatLon(minLat, minLon, maxLat, maxLon))
         {
            osg::ref_ptr<ossimPlanetImage> image = theChildrenList[idx]->getTexture(level, row, col, utility);
            if(image.valid())
            {
               if(result.valid())
               {
                  mergeImage(image.get(), result.get());
                  result = image;
                  result->setPixelStatus();
                  if(result->getPixelStatus() == ossimPlanetImage::ossimPlanetImagePixelStatus_FULL)
                  {
                     return result;
                  }
               }
               else
               {
                  result = (ossimPlanetImage*)image->clone(osg::CopyOp::DEEP_COPY_ALL);
                  if(result->getPixelStatus() == ossimPlanetImage::ossimPlanetImagePixelStatus_FULL)
                  {
                     return result;
                  }
               }
            }
         }
      }
   }
   
   return result;
}

ossimPlanetTextureLayerGroup* ossimPlanetTextureLayerGroup::asGroup()
{
   return this;
}

const ossimPlanetTextureLayerGroup* ossimPlanetTextureLayerGroup::asGroup()const
{
   return this;
}

bool ossimPlanetTextureLayerGroup::swapLayers(osg::ref_ptr<ossimPlanetTextureLayer> layer1, 
                                              osg::ref_ptr<ossimPlanetTextureLayer> layer2,
                                              bool notifyFlag)
{
   ossim_int32 idx1 = findLayerIndex(layer1.get());
   ossim_int32 idx2 = findLayerIndex(layer2.get());
   
   if(idx1 < 0 || idx2 < 0) return false;
   
   return swapLayers(idx1, idx2, notifyFlag);
}

bool ossimPlanetTextureLayerGroup::swapLayers(ossim_uint32 idx1, ossim_uint32 idx2, bool notifyFlag)
{
   if((idx1 < theChildrenList.size())&&
      (idx2 < theChildrenList.size()))
   {
      std::swap(theChildrenList[idx1], theChildrenList[idx2]);
      if(notifyFlag)
      {
         if(theChildrenList[idx1]->getExtents().valid()&&
            theChildrenList[idx2]->getExtents().valid())
         {
            osg::ref_ptr<ossimPlanetExtents> extent = new ossimPlanetExtents(*theChildrenList[idx1]->getExtents());
            extent->combine(theChildrenList[idx2]->getExtents().get());
            notifyRefreshExtent(extent);
         }
      }
      
      return true;
   }
   
   return false;
}

bool ossimPlanetTextureLayerGroup::replaceLayer(ossim_uint32 idx,
                                                osg::ref_ptr<ossimPlanetTextureLayer> layer, 
                                                bool notifyFlag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
    bool result = false;
    if(layer.valid()&&idx < theChildrenList.size())
    {
       if(!containsLayerNoMutex(layer))
       {
          if(theChildrenList[idx].valid())
          {
             theChildrenList[idx]->removeCallback(theChildListener);
             theChildrenList[idx]->removeParent(this);
          }
          layer->addParent(this);
          layer->addCallback(theChildListener);
          theChildrenList[idx] = layer.get();
          dirtyExtents();
          dirtyStats();
          result = true;
          if(notifyFlag)
          {
             notifyLayerAdded(layer);
          }
       }
    }
    
    return result;
}

bool ossimPlanetTextureLayerGroup::addTop(osg::ref_ptr<ossimPlanetTextureLayer> layer, 
                                          bool notifyFlag)
{
   if(layer.get() == this) return false;
//   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   bool result = false;
   
   if(layer.valid())
   {
      if(!containsLayer(layer))
      {
         layer->addParent(this);
         layer->addCallback(theChildListener);
         theChildrenListMutex.lock();
         theChildrenList.insert(theChildrenList.begin(), layer);
         theChildrenListMutex.unlock();
         dirtyExtents(); // notify parent layers
         dirtyStats();
         result = true;
         if(notifyFlag)
         {
            notifyLayerAdded(layer);
         }
      }
   }

   return result;
}

bool ossimPlanetTextureLayerGroup::addBeforeIdx(ossim_uint32 idx,
                                                osg::ref_ptr<ossimPlanetTextureLayer> layer, 
                                                bool notifyFlag)
{
   bool result = false;

   theChildrenListMutex.lock();
   if((idx < theChildrenList.size())&&(!containsLayerNoMutex(layer)))
   {
      layer->addParent(this);
      layer->addCallback(theChildListener);
      theChildrenList.insert(theChildrenList.begin()+idx, layer);
      theChildrenListMutex.unlock();
     dirtyExtents();
      dirtyStats();
      result = true;
      if(notifyFlag)
      {
         notifyLayerAdded(layer);   
      }
   }
   else
    {
      theChildrenListMutex.unlock();
    }

   return result;
}

bool ossimPlanetTextureLayerGroup::addBeforeLayer(const osg::ref_ptr<ossimPlanetTextureLayer> beforeLayer,
                                                  osg::ref_ptr<ossimPlanetTextureLayer> layerToAdd, 
                                                  bool notifyFlag)
{
   return addBeforeIdx(findLayerIndex(beforeLayer),
                       layerToAdd,
                       notifyFlag);
}

bool ossimPlanetTextureLayerGroup::addAfterIdx(ossim_int32 idx,
                                               osg::ref_ptr<ossimPlanetTextureLayer> layer, 
                                               bool notifyFlag)
{
   bool result = false;
   
   theChildrenListMutex.lock();
   if(containsLayerNoMutex(layer))
   {
      return result;
   }
   if(idx == -1)
   {
      layer->addParent(this);
      layer->addCallback(theChildListener);
      theChildrenList.insert(theChildrenList.begin(), layer);
      theChildrenListMutex.unlock();
      dirtyExtents();
      dirtyStats();
      result = true;
      if(notifyFlag)
      {
         notifyLayerAdded(layer);   
      }
   }
   else if(idx < theChildrenList.size())
   {
      layer->addParent(this);
      layer->addCallback(theChildListener);
      theChildrenList.insert(theChildrenList.begin()+idx+1, layer);
      theChildrenListMutex.unlock();
      dirtyExtents();
      dirtyStats();
      result = true;
      if(notifyFlag)
      {
         notifyLayerAdded(layer);   
      }
   }
   else if(idx == theChildrenList.size())
   {
      layer->addParent(this);
      layer->addCallback(theChildListener);
      theChildrenList.push_back(layer.get());
      theChildrenListMutex.unlock();
      dirtyExtents();
      dirtyStats();
      result = true;
      if(notifyFlag)
      {
         notifyLayerAdded(layer);   
      }
   }
   else
   {
     theChildrenListMutex.unlock();
   }
   return result;
}

bool ossimPlanetTextureLayerGroup::addAfterLayer(const osg::ref_ptr<ossimPlanetTextureLayer> afterLayer,
                                                 osg::ref_ptr<ossimPlanetTextureLayer> layerToAdd, 
                                                 bool notifyFlag)
{
   return addAfterIdx(findLayerIndex(afterLayer),
                      layerToAdd,
                      notifyFlag);
}

bool ossimPlanetTextureLayerGroup::addBottom(osg::ref_ptr<ossimPlanetTextureLayer> layer, 
                                             bool notifyFlag)
{
   bool result = false;
   if(layer.valid())
   {
      if(!containsLayerNoMutex(layer))
      {
         layer->addParent(this);
         layer->addCallback(theChildListener);
         theChildrenListMutex.lock();
         theChildrenList.push_back(layer);
         theChildrenListMutex.unlock();
         dirtyExtents();
         dirtyStats();
         result = true;
         if(notifyFlag)
         {
            notifyLayerAdded(layer);   
         }
      }
   }

   return result;
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetTextureLayerGroup::removeLayer(ossim_uint32 idx, 
                                                                                bool notifyFlag)
{
   theChildrenListMutex.lock();
   osg::ref_ptr<ossimPlanetTextureLayer> layer = removeLayerNoMutex(idx, false);
   theChildrenListMutex.unlock();   
   if(notifyFlag)
   {
      notifyLayerRemoved(layer, this);
   }
   return layer.get();
}

void ossimPlanetTextureLayerGroup::removeLayers(ossim_uint32 idx, ossim_uint32 length, 
                                                bool notifyFlag)
{
  std::vector<osg::ref_ptr<ossimPlanetTextureLayer> > layerList;
  {
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);

   layerList = removeLayersNoMutex(idx, length, false);
  }
  if(notifyFlag)
  {
    ossim_uint32 tempIdx = layerList.size();
     for(tempIdx = 0; tempIdx < layerList.size();++tempIdx)
     {
        notifyLayerRemoved(layerList[tempIdx].get(), this);
     }
  }

}

bool ossimPlanetTextureLayerGroup::removeLayer(osg::ref_ptr<ossimPlanetTextureLayer> layer, 
                                               bool notifyFlag)
{
   theChildrenListMutex.lock();
   bool result = removeLayerNoMutex(layer, false);
   theChildrenListMutex.unlock();   
   if(notifyFlag)
   {
      notifyLayerRemoved(layer, this);
   }
   return result;
}


ossimPlanetTextureLayer* ossimPlanetTextureLayerGroup::findLayerByName(const ossimString& layerName,
                                                                       bool recurseFlag)
{
   std::queue<ossimPlanetTextureLayer*> tempQueue;
   
   if(name() == layerName) return this;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   ossim_uint32 idx = 0;
   for(idx = 0; (idx < theChildrenList.size());++idx)
   {
      if(theChildrenList[idx]->name() == layerName) return theChildrenList[idx].get();
      if(theChildrenList[idx]->asGroup()&&recurseFlag)
      {
         tempQueue.push(theChildrenList[idx].get());
      }
   }
   ossimPlanetTextureLayer* result = 0;

   while(!tempQueue.empty()&&!result)
   {
      result = tempQueue.front()->findLayerByName(layerName, recurseFlag);
      tempQueue.pop();
   }
   
   return result;
}

const ossimPlanetTextureLayer* ossimPlanetTextureLayerGroup::findLayerByName(const ossimString& layerName,
                                                                             bool recurseFlag)const
{
   std::queue<const ossimPlanetTextureLayer*> tempQueue;
   
   if(theName == layerName) return this;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   ossim_uint32 idx = 0;
   for(idx = 0; (idx < theChildrenList.size());++idx)
   {
      if(theChildrenList[idx]->name() == layerName) return theChildrenList[idx].get();
      if(theChildrenList[idx]->asGroup()&&recurseFlag)
      {
         tempQueue.push(theChildrenList[idx].get());
      }
   }
   const ossimPlanetTextureLayer* result = 0;

   while(!tempQueue.empty()&&!result)
   {
      result = tempQueue.front()->findLayerByName(layerName, recurseFlag);
      tempQueue.pop();
   }
   
   return result;
}
ossimPlanetTextureLayer* ossimPlanetTextureLayerGroup::findLayerByNameAndId(const ossimString& layerName,
                                                                            const ossimString& id)
{
   ossimPlanetTextureLayer* layer = this;
   if(layer->name() != layerName)
   {
      layer = findLayerByName(layerName, true);
   }
   if(layer)
   {
      return layer->findLayerById(id, true);
   }
   
   return 0;
}

const ossimPlanetTextureLayer* ossimPlanetTextureLayerGroup::findLayerByNameAndId(const ossimString& layerName,
                                                                                  const ossimString& id)const
{
   const ossimPlanetTextureLayer* layer = this;
   if(layer->name() != layerName)
   {
      layer = findLayerByName(layerName, true);
   }
   if(layer)
   {
      return layer->findLayerById(id, true);
   }
   return 0;
}
ossimPlanetTextureLayer* ossimPlanetTextureLayerGroup::findLayerById(const ossimString& layerId,
                                                                     bool recurseFlag)
{
   std::queue<ossimPlanetTextureLayer*> tempQueue;
   
   if(theId == layerId) return this;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   ossim_uint32 idx = 0;
   for(idx = 0; (idx < theChildrenList.size());++idx)
   {
      if(theChildrenList[idx]->id() == layerId) return theChildrenList[idx].get();
      if(theChildrenList[idx]->asGroup()&&recurseFlag)
      {
         tempQueue.push(theChildrenList[idx].get());
      }
   }
   ossimPlanetTextureLayer* result = 0;

   while(!tempQueue.empty()&&!result)
   {
      result = tempQueue.front()->findLayerById(layerId, recurseFlag);
      tempQueue.pop();
   }
   
   return result;
}

const ossimPlanetTextureLayer* ossimPlanetTextureLayerGroup::findLayerById(const ossimString& layerId,
                                                                           bool recurseFlag)const
{
   std::queue<const ossimPlanetTextureLayer*> tempQueue;
   
   if(theId == layerId) return this;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   ossim_uint32 idx = 0;
   for(idx = 0; (idx < theChildrenList.size());++idx)
   {
      if(theChildrenList[idx]->id() == layerId) return theChildrenList[idx].get();
      if(theChildrenList[idx]->asGroup()&&recurseFlag)
      {
         tempQueue.push(theChildrenList[idx].get());
      }
   }
   const ossimPlanetTextureLayer* result = 0;

   while(!tempQueue.empty()&&!result)
   {
      result = tempQueue.front()->findLayerById(layerId, recurseFlag);
      tempQueue.pop();
   }
   
   return result;
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetTextureLayerGroup::removeLayerNoMutex(ossim_uint32 idx, bool notifyFlag)
{
   osg::ref_ptr<ossimPlanetTextureLayer> result;
   if(idx < theChildrenList.size())
   {
      result = theChildrenList[idx];
      result->removeParent(this);
      result->removeCallback(theChildListener);
      theChildrenList.erase(theChildrenList.begin()+idx);
      dirtyExtents();
      dirtyStats();
   }
   if(notifyFlag)
   {
      notifyLayerRemoved(result, this);
   }
   return result;
}

std::vector<osg::ref_ptr<ossimPlanetTextureLayer> > ossimPlanetTextureLayerGroup::removeLayersNoMutex(ossim_uint32 idx, ossim_uint32 length, bool notifyFlag)
{
  std::vector<osg::ref_ptr<ossimPlanetTextureLayer> > layerList;
  if(idx >= theChildrenList.size()) return layerList;
   ossim_uint32 endIdx = idx;
   ossim_uint32 maxCount = ossim::min(idx+length, (ossim_uint32)theChildrenList.size());
   for(endIdx = idx; endIdx <maxCount ; ++endIdx)
   {
      theChildrenList[endIdx]->removeParent(this);
      theChildrenList[endIdx]->removeCallback(theChildListener);
      layerList.push_back(theChildrenList[endIdx]);
   }
   theChildrenList.erase(theChildrenList.begin() + idx,
                         theChildrenList.begin() + maxCount);
   dirtyExtents();
   dirtyStats();
	ossim_uint32 tempIdx = 0;

	return layerList;
}

bool ossimPlanetTextureLayerGroup::removeLayerNoMutex(osg::ref_ptr<ossimPlanetTextureLayer> layer, bool notifyFlag)
{

   ossim_int32 idx = findLayerIndexNoMutex(layer);
   if(idx > -1)
   {
      return removeLayerNoMutex((ossim_uint32)idx, notifyFlag).valid();
   }

   return false;
}

ossim_int32 ossimPlanetTextureLayerGroup::findLayerIndexNoMutex(osg::ref_ptr<ossimPlanetTextureLayer> layer)const
{
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theChildrenList.size(); ++idx)
   {
      if(theChildrenList[idx].get() == layer.get())
      {
         return (ossim_int32)idx;
      }
   }
   
   return -1;
   
}

ossim_int32 ossimPlanetTextureLayerGroup::findLayerIndex(osg::ref_ptr<ossimPlanetTextureLayer> layer)const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);

   return findLayerIndexNoMutex(layer);
}

bool ossimPlanetTextureLayerGroup::containsLayer(osg::ref_ptr<ossimPlanetTextureLayer> layer)const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   return containsLayerNoMutex(layer);
}

ossim_uint32 ossimPlanetTextureLayerGroup::numberOfLayers()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   return theChildrenList.size();
}

const osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetTextureLayerGroup::layer(ossim_uint32 idx)const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   if(idx < theChildrenList.size())
   {
      return theChildrenList[idx];
   }

   return 0;
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetTextureLayerGroup::layer(ossim_uint32 idx)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   if(idx < theChildrenList.size())
   {
      return theChildrenList[idx];
   }

   return 0;
}

static bool gsdCmp( osg::ref_ptr<ossimPlanetTextureLayer> a, osg::ref_ptr<ossimPlanetTextureLayer> b )
{
   osg::ref_ptr<ossimPlanetExtents> aExtents = a->getExtents();
   osg::ref_ptr<ossimPlanetExtents> bExtents = b->getExtents();

   double aScale = aExtents->getMinScale();
   double bScale = bExtents->getMinScale();
   
   if(aScale > 0.0)
   {
      if(bScale > 0.0)
      {
         return aScale < bScale;
      }
   }
   else if(bScale > 0.0)
   {
      return false;
   }

   return true;
}


void ossimPlanetTextureLayerGroup::setFillNullOrEmptyTileMaxLevel(ossim_int32 maxLevel)
{
   theFillEmptyNullTileMaxLevel = maxLevel;
}

void ossimPlanetTextureLayerGroup::setBackgroundColor(const osg::Vec4f& color)
{
   theBackgroundColor = color;
}

void ossimPlanetTextureLayerGroup::setFillTranslucentPixelsWithBackground(bool on)
{
   theFillTranslucentPixelsWithBackgroundEnabled = on;
}

void ossimPlanetTextureLayerGroup::sortByGsd()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);

   std::sort(theChildrenList.begin(),
             theChildrenList.end(),
             gsdCmp);
}

bool ossimPlanetTextureLayerGroup::containsLayerNoMutex(osg::ref_ptr<ossimPlanetTextureLayer> layer)const
{
   std::stack<const ossimPlanetTextureLayerGroup* >tempStack;
//   ossim_uint32 idx = 0;
   tempStack.push(this);

   while(!tempStack.empty())
   {
      const ossimPlanetTextureLayerGroup* current = tempStack.top();
      tempStack.pop();
      ossim_uint32 idx = 0;
      for(idx = 0; idx < current->theChildrenList.size(); ++idx)
      {
         if(current->theChildrenList[idx].get() == layer.get())
         {
           return true;
         }
         if(current->theChildrenList[idx].valid())
         {
            if(current->theChildrenList[idx]->asGroup())
            {
               tempStack.push(current->theChildrenList[idx]->asGroup());
            }
         }
      }
   }   

   return false;   
}

ossimRefPtr<ossimXmlNode> ossimPlanetTextureLayerGroup::saveXml(bool recurseFlag)const
{
   ossimRefPtr<ossimXmlNode> result = ossimPlanetTextureLayer::saveXml(recurseFlag);
   
   if(recurseFlag)
   {
      ossim_uint32 idx = 0;
      for(idx = 0; idx < theChildrenList.size(); ++idx)
      {
         ossimRefPtr<ossimXmlNode> node = theChildrenList[idx]->saveXml().get();
         result->addChildNode(node.get());
      }
   }

   return result;
}

bool ossimPlanetTextureLayerGroup::loadXml(ossimRefPtr<ossimXmlNode> node)
{
   std::cout << "ossimPlanetTextureLayerGroup::loadXml: NOT IMPLEMENTED YET!!!" << std::endl;
   return ossimPlanetTextureLayer::loadXml(node);
}


