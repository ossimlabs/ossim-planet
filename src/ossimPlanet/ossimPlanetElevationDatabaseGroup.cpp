#include <ossimPlanet/ossimPlanetElevationDatabaseGroup.h>
#include <ossimPlanet/ossimPlanetElevationDatabase.h>
#include <stack>
#include <ossim/base/ossimDrect.h>
#include <ossim/base/ossimGpt.h>

ossimPlanetElevationDatabaseGroup::ossimPlanetElevationDatabaseGroup()
   :ossimPlanetTextureLayerGroup(),
    theFillNullWithGeoidOffsetFlag(false)
{
}

ossimPlanetElevationDatabaseGroup::ossimPlanetElevationDatabaseGroup(const ossimPlanetElevationDatabaseGroup& src)
:ossimPlanetTextureLayerGroup(src),
theFillNullWithGeoidOffsetFlag(src.theFillNullWithGeoidOffsetFlag)
{
}

ossimPlanetTextureLayer* ossimPlanetElevationDatabaseGroup::dup()const
{
   return new ossimPlanetElevationDatabaseGroup(*this);
}

ossimPlanetTextureLayer* ossimPlanetElevationDatabaseGroup::dupType()const
{
   return new ossimPlanetElevationDatabaseGroup;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetElevationDatabaseGroup::getTexture(ossim_uint32 width,
                                                                             ossim_uint32 height,
                                                                             const ossimPlanetTerrainTileId& tileId,
                                                                             const ossimPlanetGrid& grid,
                                                                             ossim_int32 padding)
{
   ossimRefPtr<ossimImageData> compositeData = new ossimImageData(0,
                                                                  OSSIM_FLOAT32,
                                                                  1,
                                                                  width+2*padding,
                                                                  height+2*padding);
   compositeData->setNullPix(OSSIMPLANET_NULL_HEIGHT);
   compositeData->initialize();
   osg::ref_ptr<ossimPlanetImage> result = new ossimPlanetImage(tileId);
   result->fromOssimImage(compositeData, false);
   result->setPadding(padding);
   if(theDirtyExtentsFlag)
   {
      updateExtents();
   }
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   ossimPlanetGrid::GridBound bound;
   ossimPlanetGrid::GridBound tileBound;
   bool withinExtents = true;
   if(grid.findGridBound(tileId.face(),
                         ossimPlanetGrid::ModelPoint(theExtents->getMinLon(), theExtents->getMinLat()),
                         ossimPlanetGrid::ModelPoint(theExtents->getMaxLon(), theExtents->getMaxLat()),
                         bound))
   {
      grid.bounds(tileId,tileBound);
      if(!tileBound.toDrect().intersects(bound.toDrect()))
      {
         withinExtents = false;
      }
   }
   else
   {
      withinExtents = false;
   }
   ossim_uint32 idx = 0;
   for(idx = 0; (idx < theChildrenList.size())&&withinExtents&&getEnableFlag(); ++idx)
   {
      if(theChildrenList[idx].valid())
      {
         const osg::ref_ptr<ossimPlanetExtents> extents = theChildrenList[idx]->getExtents();
         
         if(extents.valid())
         {
            if(grid.findGridBound(tileId.face(),
                                  ossimPlanetGrid::ModelPoint(extents->getMinLon(), extents->getMinLat()),
                                  ossimPlanetGrid::ModelPoint(extents->getMaxLon(), extents->getMaxLat()),
                                  bound))
            {
//               std::cout << "TESTING INTERSECTS" << std::endl;
//               std::cout << "tileBound = " << tileBound.toDrect() << std::endl
//               << "fullBound = " << bound.toDrect() << std::endl;
               if(tileBound.toDrect().intersects(bound.toDrect()))
               {
                  osg::ref_ptr<ossimPlanetImage> image = theChildrenList[idx]->getTexture(width,
                                                                                          height,
                                                                                          tileId,
                                                                                          grid,
                                                                                          padding);
                  if(image.valid())
                  {
                     if(result.valid())
                     {
                        mergeImage(image.get(), result.get());
                        result = image;
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
      }
   }
   if((result->getPixelStatus() != ossimPlanetImage::ossimPlanetImagePixelStatus_FULL)&&
      (theFillNullWithGeoidOffsetFlag)&&
      (theGeoRefModel.valid()))
   {
      ossimPlanetGrid::ModelPoints points;
      grid.createModelPoints(tileId,
                             width,
                             height,
                             points,
                             result->padding());
      
      ossim_uint32 nPoints = points.size();
      osg::Vec3d latLonPoint;
      double minValue = 1.0/FLT_EPSILON -1;
      double maxValue = -1.0/FLT_EPSILON +1;
      ossim_float32* bufPtr = (ossim_float32*)result->data();   
      ossimPlanetGrid::ModelPoint* optimizedOutPtr = &points.front();
      for(idx = 0; idx < nPoints; ++idx,++optimizedOutPtr)
      { 
         if((*bufPtr == OSSIMPLANET_NULL_HEIGHT)||
            (ossim::isnan(*bufPtr)))
         {
            *bufPtr = theGeoRefModel->getGeoidOffset(ossim::clamp(optimizedOutPtr->y(),-90.0, 90.0), 
                                                     ossim::wrap(optimizedOutPtr->x(), -180.0, 180.0));
         }
         if(ossim::isnan(*bufPtr))
         {
            *bufPtr = 0;
         }
         else if(*bufPtr < minValue)
         {
            minValue = *bufPtr;
         }
         else if(*bufPtr > maxValue)
         {
            maxValue = *bufPtr;
         }
        ++bufPtr;
      }
      
      if(minValue < maxValue)
      {
         result->setMinMax(minValue, maxValue);
      }
      result->setPixelStatus(ossimPlanetImage::ossimPlanetImagePixelStatus_FULL);
   }
   return result;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetElevationDatabaseGroup::getTexture(ossim_uint32 level,
                                                                             ossim_uint64 row,
                                                                             ossim_uint64 col,
                                                                             const ossimPlanetGridUtility& utility)
{   
   ossim_uint32 width = utility.getTileWidth();
   ossim_uint32 height = utility.getTileHeight();

   ossimRefPtr<ossimImageData> compositeData = new ossimImageData(0,
                                                                  OSSIM_FLOAT32,
                                                                  1,
                                                                  width,
                                                                  height);
   compositeData->setNullPix(OSSIMPLANET_NULL_HEIGHT);
   compositeData->initialize();
   osg::ref_ptr<ossimPlanetImage> result = new ossimPlanetImage(ossimPlanetTerrainTileId(0, level, col, row));

   result->fromOssimImage(compositeData, false);
   if(theDirtyExtentsFlag)
   {
      updateExtents();
   }
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);

   ossim_uint32 idx = 0;
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
   double deltaX;
   double deltaY;
   utility.getWidthHeightInDegrees(deltaX, deltaY, level, row, col);
   
   double deltaLat    = deltaY/h;
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;
   if(theExtents->intersectsLatLon(minLat, minLon, maxLat, maxLon)&&
      theExtents->intersectsScale(gsd.y,
                                  gsd.y)&&
      getEnableFlag())
   {
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
   }
   if((result->getPixelStatus() != ossimPlanetImage::ossimPlanetImagePixelStatus_FULL)&&
      (theFillNullWithGeoidOffsetFlag)&&
      (theGeoRefModel.valid()))
   {
      std::vector<ossimPlanetGridUtility::GridPoint> points;
      utility.createGridPoints(points,
                               level,
                               row,
                               col,
                               height,
                               width);
      
      ossim_uint32 idxPts = 0;
      ossim_uint32 nPoints = points.size();
      osg::Vec3d latLonPoint;
      double minValue = 1.0/FLT_EPSILON -1;
      double maxValue = -1.0/FLT_EPSILON +1;
      ossim_float32* bufPtr = (ossim_float32*)result->data();   
      for(idxPts = 0; idxPts < nPoints; ++idxPts)
      { 
         if((*bufPtr == OSSIMPLANET_NULL_HEIGHT)||
            (ossim::isnan(*bufPtr)))
         {
            utility.getLatLon(latLonPoint, points[idxPts]);

            *bufPtr = theGeoRefModel->getGeoidOffset(latLonPoint[0], latLonPoint[1]);
         }
         if(*bufPtr < minValue)
         {
            minValue = *bufPtr;
         }
         if(*bufPtr > maxValue)
         {
            maxValue = *bufPtr;
         }
         ++bufPtr;
      }
      
      if(minValue < maxValue)
      {
         result->setMinMax(minValue, maxValue);
      }
      result->setPixelStatus(ossimPlanetImage::ossimPlanetImagePixelStatus_FULL);
   }
   return result;
   
}

bool ossimPlanetElevationDatabaseGroup::replaceLayer(ossim_uint32 idx,
                                                     osg::ref_ptr<ossimPlanetTextureLayer> layer)
{
   ossimPlanetElevationDatabase* databaseLayer = dynamic_cast<ossimPlanetElevationDatabase*>(layer.get());
   if(databaseLayer)
   {
      databaseLayer->setGeoRefModel(theGeoRefModel.get());
      return ossimPlanetTextureLayerGroup::replaceLayer(idx, layer.get());
   }

   return false;
}

bool ossimPlanetElevationDatabaseGroup::addTop(osg::ref_ptr<ossimPlanetTextureLayer> layer)
{
   ossimPlanetElevationDatabase* databaseLayer = dynamic_cast<ossimPlanetElevationDatabase*>(layer.get());
   if(databaseLayer)
   {
      databaseLayer->setGeoRefModel(theGeoRefModel.get());
      return ossimPlanetTextureLayerGroup::addTop(layer.get());
   }
   
   return false;
}

bool ossimPlanetElevationDatabaseGroup::addBeforeIdx(ossim_uint32 idx, osg::ref_ptr<ossimPlanetTextureLayer> layer)
{
   ossimPlanetElevationDatabase* databaseLayer = dynamic_cast<ossimPlanetElevationDatabase*>(layer.get());
   if(databaseLayer)
   {
      databaseLayer->setGeoRefModel(theGeoRefModel.get());
      return ossimPlanetTextureLayerGroup::addBeforeIdx(idx, layer.get());
   }

   return false;
}

bool ossimPlanetElevationDatabaseGroup::addBeforeLayer(const osg::ref_ptr<ossimPlanetTextureLayer> beforeLayer,
                                                       osg::ref_ptr<ossimPlanetTextureLayer> layerToAdd)
{
   ossimPlanetElevationDatabase* databaseLayer = dynamic_cast<ossimPlanetElevationDatabase*>(layerToAdd.get());
   if(databaseLayer)
   {
      databaseLayer->setGeoRefModel(theGeoRefModel.get());
      return ossimPlanetTextureLayerGroup::addBeforeLayer(beforeLayer.get(),
                                                          layerToAdd.get());
   }

   return false;
}

bool ossimPlanetElevationDatabaseGroup::addAfterIdx(ossim_int32 idx, osg::ref_ptr<ossimPlanetTextureLayer> layer)
{
   ossimPlanetElevationDatabase* databaseLayer = dynamic_cast<ossimPlanetElevationDatabase*>(layer.get());
   if(databaseLayer)
   {
      databaseLayer->setGeoRefModel(theGeoRefModel.get());
      return ossimPlanetTextureLayerGroup::addAfterIdx(idx, layer.get());
   }

   return false;
}

bool ossimPlanetElevationDatabaseGroup::addAfterLayer(const osg::ref_ptr<ossimPlanetTextureLayer> afterLayer,
                                                      osg::ref_ptr<ossimPlanetTextureLayer> layerToAdd)
{
   ossimPlanetElevationDatabase* databaseLayer = dynamic_cast<ossimPlanetElevationDatabase*>(layerToAdd.get());
   if(databaseLayer)
   {
      databaseLayer->setGeoRefModel(theGeoRefModel.get());
      return ossimPlanetTextureLayerGroup::addAfterLayer(afterLayer.get(),
                                                         layerToAdd.get());
      
   }

   return false;
}

bool ossimPlanetElevationDatabaseGroup::addBottom(osg::ref_ptr<ossimPlanetTextureLayer> layer)
{
   ossimPlanetElevationDatabase* databaseLayer = dynamic_cast<ossimPlanetElevationDatabase*>(layer.get());
   if(databaseLayer)
   {
      databaseLayer->setGeoRefModel(theGeoRefModel.get());
      return ossimPlanetTextureLayerGroup::addBottom(layer.get());
   }

   return false;
}

void ossimPlanetElevationDatabaseGroup::mergeImage(ossimPlanetImage* result,
                                                   const ossimPlanetImage* source)const
{
   if(result&&source)
   {
      
      if((source->getDataType() == GL_FLOAT)&&
         (source->getPixelFormat()== GL_LUMINANCE)&&
         (result->getDataType() == GL_FLOAT)&&
         (result->getPixelFormat()== GL_LUMINANCE)&&
         (result->s() == source->s())&&
         (result->t() == source->t()))
      {
         const ossim_float32* srcPtr = reinterpret_cast<const ossim_float32*>(source->data());
         ossim_float32* resultPtr = reinterpret_cast<ossim_float32*>(result->data());
         ossim_uint32 area = source->s()*source->t();
         
         if(area>0)
         {
            ossim_uint32 idx = 0;
            ossim_uint32 nullCount = 0;
            for(idx = 0; idx < area; ++idx)
            {
               if(*resultPtr ==  OSSIMPLANET_NULL_HEIGHT)
               {
                  if(*srcPtr !=  OSSIMPLANET_NULL_HEIGHT)
                  {
                     *resultPtr = *srcPtr;
                  }
                  else
                  {
                     ++nullCount;
                  }
               }
               ++resultPtr;
               ++srcPtr;
            }

            if(nullCount < 1)
            {
               result->setPixelStatus(ossimPlanetImage::ossimPlanetImagePixelStatus_FULL);
            }
            else if(nullCount == area)
            {
               result->setPixelStatus(ossimPlanetImage::ossimPlanetImagePixelStatus_EMPTY);
            }
            else if(nullCount > 0)
            {
               result->setPixelStatus(ossimPlanetImage::ossimPlanetImagePixelStatus_PARTIAL);
            }
         }
         else
         {
            result->setPixelStatus(ossimPlanetImage::ossimPlanetImagePixelStatus_EMPTY);
         }
         if(source->hasMinMax())
         {
            if(result->hasMinMax())
            {
               ossim_uint32 idx = 0;
               ossim_uint32 maxCount = ossim::max(source->minValue().size(), result->minValue().size());
               for(;idx < maxCount; ++idx)
               {
                  result->setMin(idx,
                                 ossim::min(source->minValue()[idx],
                                          result->minValue()[idx]));
                  result->setMax(idx,
                                 ossim::max(source->maxValue()[idx],
                                          result->maxValue()[idx]));
               }
            }
            else
            {
               result->setMinMax(source->minValue(),
                                 source->maxValue());
            }
         }
      }
   }
}

void ossimPlanetElevationDatabaseGroup::setPixelStatus(ossimPlanetImage* image)
{
   if((image->getDataType() == GL_FLOAT)&&
      (image->getPixelFormat()== GL_LUMINANCE))
   {
      ossim_uint32 area = image->s()*image->t();
      ossim_uint32 nullCount = 0;
      ossim_uint32 idx = 0;
      ossim_float32* imagePtr = reinterpret_cast<ossim_float32*>(image->data());
      image->setPixelStatus(ossimPlanetImage::ossimPlanetImagePixelStatus_EMPTY);
      
      for(idx = 0; idx < area; ++idx)
      {
         if(*imagePtr == OSSIMPLANET_NULL_HEIGHT)
         {
            ++nullCount;
         }
         ++imagePtr;
      }
      if(nullCount == area)
      {
         image->setPixelStatus(ossimPlanetImage::ossimPlanetImagePixelStatus_EMPTY);
      }
      else if(nullCount > 0)
      {
         image->setPixelStatus(ossimPlanetImage::ossimPlanetImagePixelStatus_PARTIAL);   
      }
   }
}

void ossimPlanetElevationDatabaseGroup::setGeoRefModel(osg::ref_ptr<ossimPlanetGeoRefModel> model)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildrenListMutex);
   theGeoRefModel = model.get();
   ossim_uint32 bound = theChildrenList.size();
   ossim_uint32 idx = 0;
   for(idx = 0; idx < bound; ++idx)
   {
      ossimPlanetElevationDatabase* database = dynamic_cast<ossimPlanetElevationDatabase*>(theChildrenList[idx].get());
      if(database)
      {
         database->setGeoRefModel(model.get());
      }
   }
}

void ossimPlanetElevationDatabaseGroup::setFillNullWithGeoidOffsetFlag(bool flag)
{
   theFillNullWithGeoidOffsetFlag = flag;
}
