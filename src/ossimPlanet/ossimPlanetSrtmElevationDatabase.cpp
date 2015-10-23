#include <ossimPlanet/ossimPlanetSrtmElevationDatabase.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimDirectory.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/support_data/ossimSrtmSupportData.h>
#include <ossim/projection/ossimImageViewProjectionTransform.h>

ossimPlanetSrtmElevationDatabase::ossimPlanetSrtmElevationDatabase()
   :ossimPlanetElevationDatabase()
{
   theOpenFlag = false;
   theLocation = "";
   theMaxOpenFiles = 25;
   theMinOpenFiles = 20;

//    theRenderer = new ossimImageRenderer;
//    theRenderer->getResampler()->setFilterType("bilinear");
//    theProjection    = new ossimEquDistCylProjection;
//    theNullHeightValue = -32768.0;
}

ossimPlanetSrtmElevationDatabase::ossimPlanetSrtmElevationDatabase(const ossimPlanetSrtmElevationDatabase& src)
   :ossimPlanetElevationDatabase(src),
    theLocation(src.theLocation),
    theOpenFlag(src.theOpenFlag),
    theMaxOpenFiles(src.theMaxOpenFiles),
    theMinOpenFiles(src.theMinOpenFiles)
{
//    theMosaic   = new ossimOrthoImageMosaic;
//    theRenderer = new ossimImageRenderer;
//    theRenderer->getResampler()->setFilterType("bilinear");
//    theProjection    = new ossimEquDistCylProjection;   
}

ossimPlanetSrtmElevationDatabase::~ossimPlanetSrtmElevationDatabase()
{
   
}

ossimPlanetTextureLayer* ossimPlanetSrtmElevationDatabase::dup()const
{
   return new ossimPlanetSrtmElevationDatabase(*this);
}

ossimPlanetTextureLayer* ossimPlanetSrtmElevationDatabase::dupType()const
{
   return new ossimPlanetSrtmElevationDatabase;
}

ossimPlanetTextureLayerStateCode ossimPlanetSrtmElevationDatabase::updateExtents()
{

   theDirtyExtentsFlag = false;
   return theStateCode;
}

void ossimPlanetSrtmElevationDatabase::updateStats()const
{
   theStats->setTotalTextureSize(0);
   theDirtyStatsFlag = false;
   
}

void ossimPlanetSrtmElevationDatabase::resetStats()const
{
   theStats->setBytesTransferred(0);
   theStats->setTotalTextureSize(0);
}


ossimPlanetTextureLayerStateCode ossimPlanetSrtmElevationDatabase::open(const std::string& location)
{
   ossimFilename file(location);
   bool result = false;
   theLocation = "";
   theExtents = new ossimPlanetExtents;

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
                  ossimSrtmSupportData supportData;
                  if(supportData.setFilename(testFile))
                  {
//                      double minLat = std::floor(supportData.getSouthwestLatitude());
//                      double minLon = std::floor(supportData.getSouthwestLongitude());
//                      double maxLat = minLat + 1.0;
//                      double maxLon = minLon + 1.0;
//                      if(!result)
//                      {
                     double metersPerPixel   = ossimGpt().metersPerDegree().y*supportData.getLatitudeSpacing();
//                         theExtents->setMinMaxLatLon(minLat, minLon,
//                                                     maxLat, maxLon);
                     result = true;
                     dirtyExtents(); // make sure parents are marked as dirty
                     if((supportData.getNumberOfLines() == 3601)&&
                        (supportData.getNumberOfSamples() == 3601))
                     {
                        theExtents->setMinMaxScale(metersPerPixel, metersPerPixel*std::pow(2.0, 10));
                        setName("SRTM1");
                        setDescription("SRTM 30 meter elevation database");
                     }
                     else if((supportData.getNumberOfLines() == 1201)&&
                             (supportData.getNumberOfSamples() == 1201))
                     {
                        theExtents->setMinMaxScale(metersPerPixel, metersPerPixel*std::pow(2.0, 8));
                        setName("SRTM3");
                        setDescription("SRTM 90 meter elevation database");
                     }
                     else if( (supportData.getNumberOfLines() > 3601)&&
                              (supportData.getNumberOfSamples() > 3601))
                     {
                        setName("SRTM");
                        setDescription("SRTM elevation database");
                        theExtents->setMinMaxScale(metersPerPixel, metersPerPixel*std::pow(2.0, 8)); 
                     }
                     else
                     {
                        setName("SRTM");
                        setDescription("SRTM elevation database");
                        theExtents->setMinMaxScale(metersPerPixel, metersPerPixel*std::pow(2.0, 8));
                     }
//                      }
//                      else
//                      {
//                         theExtents->combineMinMaxLatLon(minLat, minLon,
//                                                         maxLat, maxLon);
//                      }
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
   }
   else
   {
      theStateCode = ossimPlanetTextureLayer_NO_SOURCE_DATA;
   }
   
   return theStateCode;
}

bool ossimPlanetSrtmElevationDatabase::hasTexture(ossim_uint32 width,
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
   ossimPlanetGrid::GridBound bound;
   if(grid.findGridBound(tileId.face(),
                         ossimPlanetGrid::ModelPoint(theExtents->getMinLon(), theExtents->getMinLat()),
                         ossimPlanetGrid::ModelPoint(theExtents->getMaxLon(), theExtents->getMaxLat()),
                         bound))
   {
      ossimPlanetGrid::GridBound tileBound;
      grid.bounds(tileId,tileBound);
      if(!tileBound.toDrect().intersects(bound.toDrect()))
      {
         return false;
      }
   }
   osg::Vec2d deltaXY;
   grid.widthHeightInModelSpace(tileId, deltaXY);
   double deltaLat    = (deltaXY[1])/(double)(height);
   //double deltaLon    = (deltaXY[0])/(double)(width);
   
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;
   ossimPlanetGrid::ModelPoint minLatLon, maxLatLon;
   grid.modelBound(tileId, minLatLon, maxLatLon);
   if((gsd.y >= theExtents->getMinScale()) &&
      (gsd.y <= theExtents->getMaxScale()))
   {
      ossimPlanetGrid::ModelPoints points;
      grid.createModelPoints(tileId,
                             width,
                             height,
                             points);
      ossim_float64 minModelX, minModelY, maxModelX, maxModelY;
      ossim_uint32 idxPts = 0;
      ossim_uint32 nPoints = points.size();
      minModelX = points[0].x();
      maxModelX = minModelX;
      minModelY = points[0].y();
      maxModelY = minModelY;
      ossimPlanetGrid::ModelPoint* optimizedOutPtr = &points.front();
      ++optimizedOutPtr;
      for(idxPts = 1; idxPts < nPoints; ++idxPts,++optimizedOutPtr)
      {
         if(optimizedOutPtr->x() < minModelX) minModelX = optimizedOutPtr->x();
         if(optimizedOutPtr->x() > maxModelX) maxModelX = optimizedOutPtr->x();
         if(optimizedOutPtr->y() < minModelY) minModelY = optimizedOutPtr->y();
         if(optimizedOutPtr->y() > maxModelY) maxModelY = optimizedOutPtr->y();
      }
      ossim_int32 wholeMinY = (ossim_int32)std::floor(minModelY);
      ossim_int32 wholeMinX = (ossim_int32)std::floor(minModelX);
      ossim_int32 wholeMaxY = (ossim_int32)std::floor(maxModelY);
      ossim_int32 wholeMaxX = (ossim_int32)std::floor(maxModelX);
      
      
      ossim_int32 lat = wholeMaxY;
      ossim_int32 lon = wholeMinX;
      
      
      //    std::vector<ossimDpt> latLonOrigins;
      std::vector<std::string> latLonOrigins;
      
      for(;lat >= wholeMinY; --lat)
      {
         lon = wholeMinX;
         for(;lon <= wholeMaxX; ++lon)
         {
            ossimFilename filename = buildFilename(lat, lon);
            if(filename != "")
            {
               return true;
            }
         }
      }
      
   }
   
   return false;
}
osg::ref_ptr<ossimPlanetImage> ossimPlanetSrtmElevationDatabase::getTexture(ossim_uint32 width,
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
      minModelY = points[0].y();
      maxModelY = minModelY;
      ossimPlanetGrid::ModelPoint* optimizedOutPtr = &points.front();
      ++optimizedOutPtr;
      for(idxPts = 1; idxPts < nPoints; ++idxPts,++optimizedOutPtr)
      {
         if(optimizedOutPtr->x() < minModelX) minModelX = optimizedOutPtr->x();
         if(optimizedOutPtr->x() > maxModelX) maxModelX = optimizedOutPtr->x();
         if(optimizedOutPtr->y() < minModelY) minModelY = optimizedOutPtr->y();
         if(optimizedOutPtr->y() > maxModelY) maxModelY = optimizedOutPtr->y();
      }
      ossim_int32 wholeMinY = (ossim_int32)std::floor(minModelY);
      ossim_int32 wholeMinX = (ossim_int32)std::floor(minModelX);
      ossim_int32 wholeMaxY = (ossim_int32)std::floor(maxModelY);
      ossim_int32 wholeMaxX = (ossim_int32)std::floor(maxModelX);
      
      
      ossim_int32 lat = wholeMaxY;
      ossim_int32 lon = wholeMinX;
      
      
      //    std::vector<ossimDpt> latLonOrigins;
      std::vector<std::string> latLonOrigins;
      
      for(;lat >= wholeMinY; --lat)
      {
         lon = wholeMinX;
         for(;lon <= wholeMaxX; ++lon)
         {
            ossimFilename filename = buildFilename(lat, lon);
            if(filename != "")
            {
               latLonOrigins.push_back(filename);
            }
         }
      }
      
      osg::Vec3d latLonPoint;
      double minValue = 1.0/DBL_EPSILON -1;
      double maxValue = -1.0/DBL_EPSILON +1;
      if(latLonOrigins.size() == 0)
      {
         return 0;
      }
      ossim_uint32 idx  = 0;
      ossim_uint32 numberOfFilesNeeded = latLonOrigins.size();
      texture = new ossimPlanetImage(tileId);
      ossimRefPtr<ossimImageData> compositeData = new ossimImageData(0,
                                                                     OSSIM_FLOAT32,
                                                                     1,
                                                                     width+2*padding,
                                                                     height+2*padding);
      compositeData->setNullPix(OSSIMPLANET_NULL_HEIGHT, 0);
      compositeData->initialize();
      texture->setPadding(padding);
      for(idx = 0; idx < numberOfFilesNeeded;++idx)
      {
         osg::ref_ptr<ossimPlanetSrtmElevationDatabase::SrtmInfo> srtmFile = getInfo(latLonOrigins[idx]);
         
         if(srtmFile.valid())
         {
            ossim_float64 nullHeight = srtmFile->theSrtmHandler->getNullHeightValue();
            ossim_float32* bufPtr = (ossim_float32*)compositeData->getBuf();
            optimizedOutPtr = &points.front();      
            for(idxPts = 0; idxPts < nPoints; ++idxPts,++optimizedOutPtr)
            {
               if((*bufPtr == OSSIMPLANET_NULL_HEIGHT)&&
                  (optimizedOutPtr->y() >= srtmFile->theMinLat)&&
                  (optimizedOutPtr->y() <= srtmFile->theMaxLat)&&
                  (optimizedOutPtr->x() >= srtmFile->theMinLon)&&
                  (optimizedOutPtr->x() <= srtmFile->theMaxLon))
               {
                  double h = srtmFile->theSrtmHandler->getHeightAboveMSL(ossimGpt(optimizedOutPtr->y(),
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

osg::ref_ptr<ossimPlanetImage> ossimPlanetSrtmElevationDatabase::getTexture(ossim_uint32 level,
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

   double minSubRectLat = ossim::max(theExtents->getMinLat(),
                                     minLat);
   double minSubRectLon = ossim::max(theExtents->getMinLon(),
                                     minLon);
   double maxSubRectLat = ossim::min(theExtents->getMaxLat(),
                                     maxLat);
   double maxSubRectLon = ossim::min(theExtents->getMaxLon(),
                                     maxLon);
   ossim_int32 wholeMinLat = (ossim_int32)std::floor(minSubRectLat);
   ossim_int32 wholeMinLon = (ossim_int32)std::floor(minSubRectLon);
   ossim_int32 wholeMaxLat = (ossim_int32)std::floor(maxSubRectLat);
   ossim_int32 wholeMaxLon = (ossim_int32)std::floor(maxSubRectLon);
   

   ossim_int32 lat = wholeMaxLat;
   ossim_int32 lon = wholeMinLon;

//   ossim_uint32 addedFiles = 0;

//    std::vector<ossimDpt> latLonOrigins;
   std::vector<std::string> latLonOrigins;

   for(;lat >= wholeMinLat; --lat)
   {
      lon = wholeMinLon;
      for(;lon <= wholeMaxLon; ++lon)
      {
         ossimFilename filename = buildFilename(lat, lon);
         if(filename != "")
         {
            latLonOrigins.push_back(filename);
         }
      }
   }

   if(latLonOrigins.size() == 0)
   {
      return 0;
   }
   
   ossim_uint32 idx  = 0;
   ossim_uint32 numberOfFilesNeeded = latLonOrigins.size();
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

   for(idx = 0; idx < numberOfFilesNeeded;++idx)
   {

      osg::ref_ptr<ossimPlanetSrtmElevationDatabase::SrtmInfo> srtmFile = getInfo(latLonOrigins[idx]);

      if(srtmFile.valid())
      {
         ossim_float32* bufPtr = (ossim_float32*)compositeData->getBuf();
         for(idxPts = 0; idxPts < nPoints; ++idxPts)
         {
            utility.getLatLon(latLonPoint, points[idxPts]);
            if((*bufPtr == OSSIMPLANET_NULL_HEIGHT)&&
               (latLonPoint[0] >= srtmFile->theMinLat)&&
               (latLonPoint[0] <= srtmFile->theMaxLat)&&
               (latLonPoint[1] >= srtmFile->theMinLon)&&
               (latLonPoint[1] <= srtmFile->theMaxLon))
            {
               utility.getLatLon(latLonPoint, points[idxPts]);
               double h = srtmFile->theSrtmHandler->getHeightAboveMSL(ossimGpt(latLonPoint[0],
                                                                               latLonPoint[1]));
               if(!ossim::isnan(h))
               {
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
   
   return texture;
}

osg::ref_ptr<ossimPlanetSrtmElevationDatabase::SrtmInfo> ossimPlanetSrtmElevationDatabase::getInfo(const std::string& srtmName)
{
   SrtmFilePointerList::iterator iter = theFilePointers.find(srtmName);

   if(iter != theFilePointers.end())
   {
      iter->second->theTimeStamp = osg::Timer::instance()->tick();

      return iter->second;
   }
   osg::ref_ptr<SrtmInfo> info     = new SrtmInfo;
   
   ossimFilename srtmFile = ossimFilename(theLocation).dirCat(ossimFilename(srtmName));
   ossimSrtmSupportData supportData;
   if(supportData.setFilename(srtmFile))
   {
      info->theMinLat = std::floor(supportData.getSouthwestLatitude());
      info->theMinLon = std::floor(supportData.getSouthwestLongitude());
      info->theMaxLat = info->theMinLat + 1.0;
      info->theMaxLon = info->theMinLon + 1.0;
   }
   else
   {
      return 0;
   }
   
   info->theTimeStamp  = osg::Timer::instance()->tick();
   info->theFilename   = srtmFile.string();
   info->theSrtmHandler = new ossimSrtmHandler();
   info->theSrtmHandler->open(srtmFile);
   theFilePointers.insert(std::make_pair(srtmName, info));
   shrinkFilePointers();

   return info;
}

void ossimPlanetSrtmElevationDatabase::shrinkFilePointers()
{
   if(theFilePointers.size() <= theMaxOpenFiles) return;

   const osg::Timer* timer = osg::Timer::instance();
   
   osg::Timer_t currentTime = timer->tick();

   while((theFilePointers.size()>0) &&
         (theFilePointers.size() > theMinOpenFiles))
   {
      SrtmFilePointerList::iterator iter = theFilePointers.begin();
      SrtmFilePointerList::iterator currentLargestTimeDelta = iter;
      double delta = 0.0;
      while(iter!=theFilePointers.end())
      {
         double testDelta = timer->delta_m(currentLargestTimeDelta->second->theTimeStamp, currentTime);
         if(testDelta > delta)
         {
            currentLargestTimeDelta = iter;
            delta = testDelta;
         }
         ++iter;
      }

      if(currentLargestTimeDelta != theFilePointers.end())
      {
         theFilePointers.erase(currentLargestTimeDelta);
      }
   }
}

ossimFilename ossimPlanetSrtmElevationDatabase::buildFilename(double lat, double lon)const
{
   ossimFilename srtmFileBasename;

   int ilat =  static_cast<int>(floor(lat));
   if (ilat < 0)
   {
      srtmFileBasename = "S";
   }
   else
   {
      srtmFileBasename = "N";
   }

   ilat = abs(ilat);
   std::ostringstream  os1;
   
   os1 << std::setfill('0') << std::setw(2) <<ilat;
   
   srtmFileBasename += os1.str().c_str();

   int ilon = static_cast<int>(floor(lon));
   
   if (ilon < 0)
   {
      srtmFileBasename += "W";
   }
   else
   {
      srtmFileBasename += "E";
   }

   ilon = abs(ilon);
   std::ostringstream  os2;
   os2 << std::setfill('0') << std::setw(3) << ilon;
   
   srtmFileBasename += os2.str().c_str();
   srtmFileBasename.setExtension(".hgt");

   ossimFilename loc(theLocation);
   if(!loc.dirCat(srtmFileBasename).exists())
   {
      srtmFileBasename.setExtension(".HGT");
      if(!loc.dirCat(srtmFileBasename).exists())
      {
         srtmFileBasename = srtmFileBasename.downcase();
         
         if(!loc.dirCat(srtmFileBasename).exists())
         {
            srtmFileBasename = "";
         }
      }
   }

   return srtmFileBasename;
}

osg::ref_ptr<ossimPlanetSrtmElevationDatabase::SrtmInfo> ossimPlanetSrtmElevationDatabase::findSrtmInfo(const std::string& srtmName)
{
   SrtmFilePointerList::iterator iter = theFilePointers.find(srtmName);

   if(iter != theFilePointers.end())
   {
      iter->second->theTimeStamp = osg::Timer::instance()->tick();
      return iter->second;
   }

   return 0;
}
