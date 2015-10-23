#include <ossimPlanet/ossimPlanetGeneralRasterElevationDatabase.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimDirectory.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimGrect.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/support_data/ossimSrtmSupportData.h>
#include <ossim/projection/ossimImageViewProjectionTransform.h>

ossimPlanetGeneralRasterElevationDatabase::ossimPlanetGeneralRasterElevationDatabase()
   :ossimPlanetElevationDatabase()
{
   theOpenFlag = false;
   theLocation = "";
   theMaxOpenFiles = 25;
   theMinOpenFiles = 20;
   theCurrentInfoIdx = -1;
//    theRenderer = new ossimImageRenderer;
//    theRenderer->getResampler()->setFilterType("bilinear");
//    theProjection    = new ossimEquDistCylProjection;
//    theNullHeightValue = -32768.0;
}

ossimPlanetGeneralRasterElevationDatabase::ossimPlanetGeneralRasterElevationDatabase(const ossimPlanetGeneralRasterElevationDatabase& src)
   :ossimPlanetElevationDatabase(src),
    theLocation(src.theLocation),
    theOpenFlag(src.theOpenFlag),
    theMaxOpenFiles(src.theMaxOpenFiles),
    theMinOpenFiles(src.theMinOpenFiles)
{
   theCurrentInfoIdx = -1;
//    theMosaic   = new ossimOrthoImageMosaic;
//    theRenderer = new ossimImageRenderer;
//    theRenderer->getResampler()->setFilterType("bilinear");
//    theProjection    = new ossimEquDistCylProjection;
   open(theLocation);
}

ossimPlanetGeneralRasterElevationDatabase::~ossimPlanetGeneralRasterElevationDatabase()
{
   
}

ossimPlanetTextureLayer* ossimPlanetGeneralRasterElevationDatabase::dup()const
{
   return new ossimPlanetGeneralRasterElevationDatabase(*this);
}

ossimPlanetTextureLayer* ossimPlanetGeneralRasterElevationDatabase::dupType()const
{
   return new ossimPlanetGeneralRasterElevationDatabase;
}

ossimPlanetTextureLayerStateCode ossimPlanetGeneralRasterElevationDatabase::updateExtents()
{

   theDirtyExtentsFlag = false;
   return theStateCode;
}

void ossimPlanetGeneralRasterElevationDatabase::updateStats()const
{
   theStats->setTotalTextureSize(0);
   theDirtyStatsFlag = false;
   
}

void ossimPlanetGeneralRasterElevationDatabase::resetStats()const
{
   theStats->setBytesTransferred(0);
   theStats->setTotalTextureSize(0);
}


ossimPlanetTextureLayerStateCode ossimPlanetGeneralRasterElevationDatabase::open(const std::string& location)
{
   ossimFilename file(location);
   bool result = false;
   theLocation = "";
   theExtents = new ossimPlanetExtents;
   theCurrentInfoIdx = -1;

   if(file.exists())
   {
      if(file.isDir())
      {
         ossimDirectory dir;

         if(dir.open(file))
         {
            ossimFilename testFile;
            if(dir.getFirst(testFile))
            {
               do
               {
                  if(testFile.ext().downcase() == "ras")
                  {
                     ossimRefPtr<ossimGeneralRasterElevHandler> rasterHandler = new ossimGeneralRasterElevHandler();
                     if(rasterHandler->open(testFile))
                     {
                        osg::ref_ptr<ossimPlanetGeneralRasterElevationDatabase::GeneralRasterInfo> rasterInfo =  new ossimPlanetGeneralRasterElevationDatabase::GeneralRasterInfo;
                        ossimGrect grect = rasterHandler->getBoundingGndRect();
                        rasterInfo->theMinLat = ossim::min(grect.lr().latd(), grect.ul().latd());
                        rasterInfo->theMaxLat = ossim::max(grect.lr().latd(), grect.ul().latd());
                        rasterInfo->theMinLon = ossim::min(grect.lr().lond(), grect.ul().lond());
                        rasterInfo->theMaxLon = ossim::max(grect.lr().lond(), grect.ul().lond());
                        if(!result)
                        {
                           double gsd = rasterHandler->getMeanSpacingMeters();
                           theExtents->setMinMaxScale(gsd, gsd*std::pow(2.0, 12));
                           theExtents->setMinMaxLatLon(rasterInfo->theMinLat,
                                                       rasterInfo->theMinLon,
                                                       rasterInfo->theMaxLat,
                                                       rasterInfo->theMaxLon);

                        }
                        else
                        {
                           theExtents->combineMinMaxLatLon(rasterInfo->theMinLat,
                                                           rasterInfo->theMinLon,
                                                           rasterInfo->theMaxLat,
                                                           rasterInfo->theMaxLon);
                        }
                        result = true;
                        
                        rasterInfo->theFilename = testFile.string();
                        rasterInfo->theGeneralRasterHandler = rasterHandler;
                        
                       theFilePointers.push_back(rasterInfo.get());
                     }
                  }
               }while(dir.getNext(testFile)&&!result);
            }
         }
      }
   }
   
   theOpenFlag = result;
   if(theOpenFlag)
   {
      theStateCode = ossimPlanetTextureLayer_VALID;
      theLocation = location;
      theCurrentInfoIdx = 0;
   }
   else
   {
      theStateCode = ossimPlanetTextureLayer_NO_SOURCE_DATA;
   }
   
   return theStateCode;
}

bool ossimPlanetGeneralRasterElevationDatabase::hasTexture(ossim_uint32 width,
                                                           ossim_uint32 height,
                                                           const ossimPlanetTerrainTileId& tileId,
                                                           const ossimPlanetGrid& grid)
{
   if(!theOpenFlag)
   {
      return false;
   }
   
   if(!theEnableFlag)
   {
      return false;
   }
   osg::ref_ptr<ossimPlanetImage> texture;
   ossimPlanetGrid::GridBound bound;
   ossimPlanetGrid::GridBound tileBound;
   if(grid.findGridBound(tileId.face(),
                         ossimPlanetGrid::ModelPoint(theExtents->getMinLon(), theExtents->getMinLat()),
                         ossimPlanetGrid::ModelPoint(theExtents->getMaxLon(), theExtents->getMaxLat()),
                         bound))
   {
      grid.bounds(tileId,tileBound);
      if(!tileBound.toDrect().intersects(bound.toDrect()))
      {
         return false;
      }
   }
   osg::Vec2d deltaXY;
   grid.widthHeightInModelSpace(tileId, deltaXY);
   double deltaLat    = (deltaXY[1])/(double)(height);
   double deltaLon    = (deltaXY[0])/(double)(width);
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;
   if((gsd.x >= theExtents->getMinScale()) &&
      (gsd.y <= theExtents->getMaxScale()))
   {
      return true;
   }
   
   return false;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetGeneralRasterElevationDatabase::getTexture(ossim_uint32 width,
                                                                                     ossim_uint32 height,
                                                                                     const ossimPlanetTerrainTileId& tileId,
                                                                                     const ossimPlanetGrid& grid,
                                                                                     ossim_int32 padding)
{
   if(!theEnableFlag)
   {
      return 0;
   }
   osg::ref_ptr<ossimPlanetImage> texture;
   ossimPlanetGrid::GridBound bound;
   ossimPlanetGrid::GridBound tileBound;
   if(grid.findGridBound(tileId.face(),
                         ossimPlanetGrid::ModelPoint(theExtents->getMinLon(), theExtents->getMinLat()),
                         ossimPlanetGrid::ModelPoint(theExtents->getMaxLon(), theExtents->getMaxLat()),
                         bound))
   {
      grid.bounds(tileId,tileBound);
      if(!tileBound.toDrect().intersects(bound.toDrect()))
      {
         return 0;
      }
   }
   
   osg::Vec2d deltaXY;
   grid.widthHeightInModelSpace(tileId, deltaXY);
   double deltaLat    = (deltaXY[1])/(double)(height);
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;
   if(gsd.y < theExtents->getMaxScale())
   {
      ossimPlanetGrid::ModelPoints points;
      grid.createModelPoints(tileId,
                             width,
                             height,
                             points,
                             padding);
      ossim_uint32 idxPts = 0;
      ossim_uint32 nPoints = points.size();
      ossimPlanetGrid::ModelPoint* optimizedOutPtr = &points.front();
      texture = new ossimPlanetImage(tileId);
      ossimRefPtr<ossimImageData> compositeData = new ossimImageData(0,
                                                                     OSSIM_FLOAT32,
                                                                     1,
                                                                     width+2*padding,
                                                                     height+2*padding);
      compositeData->setNullPix(OSSIMPLANET_NULL_HEIGHT, 0);
      compositeData->initialize();
      texture->setPadding(padding);
      ossim_float32* bufPtr = (ossim_float32*)compositeData->getBuf();
      ossimElevManager* manager = ossimElevManager::instance();
      double minValue = 999999999.0;
      double maxValue = -999999999.0;
      for(idxPts = 0; idxPts < nPoints; ++idxPts,++optimizedOutPtr)
      {
         double h = manager->getHeightAboveEllipsoid(ossimGpt((*optimizedOutPtr).y(), (*optimizedOutPtr).x()));
         if(!ossim::isnan(h))
         {
            *bufPtr = h;
         }
         
         ++bufPtr;
      }
      compositeData->validate();
      if(compositeData->getDataObjectStatus() != OSSIM_EMPTY)
      {
         texture->fromOssimImage(compositeData, false);
         if(minValue < maxValue)
         {
            texture->setMinMax(minValue, maxValue);
         }
      }
      else
      {
         texture = 0;
      }
   }
#if 0
   if(!theOpenFlag)
   {
      return 0;
   }
   
   if(!theEnableFlag)
   {
      return 0;
   }
   osg::ref_ptr<ossimPlanetImage> texture;
   ossimPlanetGrid::GridBound bound;
   ossimPlanetGrid::GridBound tileBound;
   if(grid.findGridBound(tileId.face(),
                         ossimPlanetGrid::ModelPoint(theExtents->getMinLon(), theExtents->getMinLat()),
                         ossimPlanetGrid::ModelPoint(theExtents->getMaxLon(), theExtents->getMaxLat()),
                         bound))
   {
      grid.bounds(tileId,tileBound);
      if(!tileBound.toDrect().intersects(bound.toDrect()))
      {
         return 0;
      }
   }
   
   osg::Vec2d deltaXY;
   grid.widthHeightInModelSpace(tileId, deltaXY);
   double deltaLat    = (deltaXY[1])/(double)(height);
   double deltaLon    = (deltaXY[0])/(double)(width);
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;
   if(gsd.y < theExtents->getMaxScale())
   {
      ossimPlanetGrid::ModelPoints points;
      grid.createModelPoints(tileId,
                             width,
                             height,
                             points,
                             padding);
      ossim_float64 minModelX, minModelY, maxModelX, maxModelY;
      ossim_uint32 idxPts = 0;
      ossim_uint32 nPoints = points.size();
      minModelX = points[0].x();
      maxModelX = minModelX;
      minModelY = minModelX;
      maxModelY = minModelX;
      ossimPlanetGrid::ModelPoint* optimizedOutPtr = &points.front();
      ++optimizedOutPtr;
      for(idxPts = 1; idxPts < nPoints; ++idxPts,++optimizedOutPtr)
      {
         if(optimizedOutPtr->x() < minModelX) minModelX = optimizedOutPtr->x();
         if(optimizedOutPtr->x() > maxModelX) maxModelX = optimizedOutPtr->x();
         if(optimizedOutPtr->y() < minModelY) minModelY = optimizedOutPtr->y();
         if(optimizedOutPtr->y() > maxModelY) maxModelY = optimizedOutPtr->y();
      }
      
      //    std::vector<ossimDpt> latLonOrigins;
      osg::ref_ptr<ossimPlanetGeneralRasterElevationDatabase::GeneralRasterInfo> handlerInfo = 0;
      osg::Vec3d latLonPoint;
      double minValue = 1.0/DBL_EPSILON -1;
      double maxValue = -1.0/DBL_EPSILON +1;
      texture = new ossimPlanetImage(tileId);
      ossimRefPtr<ossimImageData> compositeData = new ossimImageData(0,
                                                                     OSSIM_FLOAT32,
                                                                     1,
                                                                     width+2*padding,
                                                                     height+2*padding);
      compositeData->setNullPix(OSSIMPLANET_NULL_HEIGHT, 0);
      compositeData->initialize();
      texture->setPadding(padding);
      ossim_float32* bufPtr = (ossim_float32*)compositeData->getBuf();
            optimizedOutPtr = &points.front();      
      optimizedOutPtr = &points.front();
      
      if(theFilePointers.size() == 1)
      {
         handlerInfo = getHandlerInfo(optimizedOutPtr->y(), optimizedOutPtr->x());
         if(!handlerInfo) return 0;
         ossim_float64 nullHeight = handlerInfo->theGeneralRasterHandler->getNullHeightValue();
        
         for(idxPts = 0; idxPts < nPoints; ++idxPts,++optimizedOutPtr)
         {
            if((*bufPtr == OSSIMPLANET_NULL_HEIGHT)&&handlerInfo.valid())
            {
               double h = handlerInfo->theGeneralRasterHandler->getHeightAboveMSL(ossimGpt(optimizedOutPtr->y(),
                                                                                           optimizedOutPtr->x()));
               if(!ossim::isnan(h)&&
                  (h!=nullHeight))
               {
                  if(theGeoRefModel.valid())
                  {
                     h+=theGeoRefModel->getGeoidOffset(optimizedOutPtr->y(), optimizedOutPtr->x());
                  }
                  *bufPtr = h;
                  if(h < minValue)
                  {
                     minValue = h;
                  }
                  if(h > maxValue)
                  {
                     maxValue = h;
                  }
               }   
               else
               {
                  *bufPtr = 0;
               }
            }
            ++bufPtr;
         }
      }
      else
      {
         for(idxPts = 0; idxPts < nPoints; ++idxPts,++optimizedOutPtr)
         {
            handlerInfo = getHandlerInfo(optimizedOutPtr->y(), optimizedOutPtr->x());
            if((*bufPtr == OSSIMPLANET_NULL_HEIGHT)&&handlerInfo.valid())
            {
               ossim_float64 nullHeight = handlerInfo->theGeneralRasterHandler->getNullHeightValue();
               
               double h = handlerInfo->theGeneralRasterHandler->getHeightAboveMSL(ossimGpt(optimizedOutPtr->y(),
                                                                                           optimizedOutPtr->x()));
               if(!ossim::isnan(h)&&
                  (h!=nullHeight))
               {
                  if(theGeoRefModel.valid())
                  {
                     h+=theGeoRefModel->getGeoidOffset(optimizedOutPtr->y(), optimizedOutPtr->x());
                  }
                  *bufPtr = h;
                  if(h < minValue)
                  {
                     minValue = h;
                  }
                  if(h > maxValue)
                  {
                     maxValue = h;
                  }
               }   
               else
               {
                  *bufPtr = 0;
               }
            }
            ++bufPtr;
         }
      }
      compositeData->validate();
      if(compositeData->getDataObjectStatus() != OSSIM_EMPTY)
      {
         texture->fromOssimImage(compositeData, false);
         if(minValue < maxValue)
         {
            texture->setMinMax(minValue, maxValue);
         }
      }
      else
      {
         texture = 0;
      }
   }
#endif
   return texture;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetGeneralRasterElevationDatabase::getTexture(ossim_uint32 level,
                                                                            ossim_uint64 row,
                                                                            ossim_uint64 col,
                                                                            const ossimPlanetGridUtility& utility)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);

   if(!theOpenFlag)
   {
      return 0;
   }

   if(!theEnableFlag)
   {
      return 0;
   }
   double minLat;
   double minLon;
   double maxLat;
   double maxLon;
   ossim_uint32 width = utility.getTileWidth();
   ossim_uint32 height = utility.getTileHeight();

   utility.getLatLonBounds(minLat,
                           minLon,
                           maxLat,
                           maxLon,
                           level,
                           row, 
                           col);

   if(!theExtents->intersectsLatLon(minLat, minLon, maxLat, maxLon))
   {
      return 0;
   }
   double deltaX;
   double deltaY;
   utility.getWidthHeightInDegrees(deltaX, deltaY, level, row, col);
   
   double deltaLat    = deltaY/height;
 //  double deltaLon    = deltaX/width;
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;

   osg::ref_ptr<ossimPlanetImage> texture = 0;
   
   if(!theExtents->intersectsScale(gsd.y-FLT_EPSILON,
                                  gsd.y+FLT_EPSILON))//gsd.y <= theExtents.theMaxGsd)
   {
      return 0;
   }

 //  double minSubRectLat = ossim::max(theExtents->getMinLat(),
 //                                    minLat);
   //double minSubRectLon = ossim::max(theExtents->getMinLon(),
   //                                  minLon);
 //  double maxSubRectLat = ossim::min(theExtents->getMaxLat(),
 //                                    maxLat);
   //double maxSubRectLon = ossim::min(theExtents->getMaxLon(),
   //                                  maxLon);
   //ossim_int32 wholeMinLat = (ossim_int32)std::floor(minSubRectLat);
   //ossim_int32 wholeMinLon = (ossim_int32)std::floor(minSubRectLon);
   //ossim_int32 wholeMaxLat = (ossim_int32)std::floor(maxSubRectLat);
   //ossim_int32 wholeMaxLon = (ossim_int32)std::floor(maxSubRectLon);
   

   //ossim_int32 lat = wholeMaxLat;
   //ossim_int32 lon = wholeMinLon;
   osg::ref_ptr<ossimPlanetGeneralRasterElevationDatabase::GeneralRasterInfo> handlerInfo = 0;//theFilePointers[theCurrentInfoIdx];
   texture = new ossimPlanetImage(ossimPlanetTerrainTileId(0,
                                                           level,
                                                           col,
                                                           row));
   ossimRefPtr<ossimImageData> compositeData = new ossimImageData(0,
                                                                  OSSIM_FLOAT32,
                                                                  1,
                                                                  width,
                                                                  height);
   compositeData->setNullPix(OSSIMPLANET_NULL_HEIGHT, 0);
   compositeData->initialize();
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
   double minValue = 1.0/DBL_EPSILON -1;
   double maxValue = -1.0/DBL_EPSILON +1;
   ossim_float32* bufPtr = (ossim_float32*)compositeData->getBuf();   
  for(idxPts = 0; idxPts < nPoints; ++idxPts)
   {      
      utility.getLatLon(latLonPoint, points[idxPts]);
      handlerInfo = getHandlerInfo(latLonPoint[0], latLonPoint[1]);
      if((*bufPtr == OSSIMPLANET_NULL_HEIGHT)&&handlerInfo.valid())
      {
         ossim_float64 nullHeight = handlerInfo->theGeneralRasterHandler->getNullHeightValue();
         utility.getLatLon(latLonPoint, points[idxPts]);
         double h = handlerInfo->theGeneralRasterHandler->getHeightAboveMSL(ossimGpt(latLonPoint[0],
                                                                                     latLonPoint[1]));
         if(!ossim::isnan(h)&&(h!=nullHeight))
         {            
            if(theGeoRefModel.valid())
            {
               h+=theGeoRefModel->getGeoidOffset(latLonPoint[0], latLonPoint[1]);
            }
            *bufPtr = h;
            if(*bufPtr < minValue)
            {
               minValue = *bufPtr;
            }
            if(*bufPtr > maxValue)
            {
               maxValue = *bufPtr;
            }
         }   
      }
      ++bufPtr;
   }
   
   compositeData->validate();
   
   if(compositeData->getDataObjectStatus() != OSSIM_EMPTY)
   {
      texture->fromOssimImage(compositeData, false);
      
      if(minValue < maxValue)
      {
         texture->setMinMax(minValue, maxValue);
      }
   }
   else
   {
      texture = 0;
   }

   return texture;
}
osg::ref_ptr<ossimPlanetGeneralRasterElevationDatabase::GeneralRasterInfo> ossimPlanetGeneralRasterElevationDatabase::getHandlerInfo(const double& lat,
                                                                                                                                     const double& lon)
{
   if(theFilePointers.size() < 1) return 0;

   osg::ref_ptr<ossimPlanetGeneralRasterElevationDatabase::GeneralRasterInfo> result = 0;

   if(theCurrentInfoIdx < 0) theCurrentInfoIdx = 0;
   result = theFilePointers[theCurrentInfoIdx];
   if((lat >= result->theMinLat)&&
      (lat <= result->theMaxLat)&&
      (lon >= result->theMinLon)&&
      (lon <= result->theMaxLon))
   {
      return result;
   }
   theCurrentInfoIdx = 0;
   while(theCurrentInfoIdx < (int)theFilePointers.size())
   {
      result = theFilePointers[theCurrentInfoIdx];
      if((lat >= result->theMinLat)&&
         (lat <= result->theMaxLat)&&
         (lon >= result->theMinLon)&&
         (lon <= result->theMaxLon))
      {
         return result;
      }
      ++theCurrentInfoIdx;
     
   }
   theCurrentInfoIdx = 0;
   
   return 0;
}

