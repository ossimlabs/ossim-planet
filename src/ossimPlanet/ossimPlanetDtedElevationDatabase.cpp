#include <ossimPlanet/ossimPlanetDtedElevationDatabase.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimDirectory.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimRegExp.h>
#include <ossim/support_data/ossimDtedVol.h>
#include <ossim/support_data/ossimDtedHdr.h>
#include <ossim/support_data/ossimDtedUhl.h>
#include <ossim/support_data/ossimDtedDsi.h>
#include <ossim/support_data/ossimDtedAcc.h>
#include <ossim/support_data/ossimDtedRecord.h>
#include <sstream>

ossimPlanetDtedElevationDatabase::ossimPlanetDtedElevationDatabase()
   :ossimPlanetElevationDatabase()
{
   theOpenFlag = false;
   theLocation = "";
   theMaxOpenFiles = 25;
   theMinOpenFiles = 20;
   
   theSwapBytesFlag = ossim::byteOrder() == OSSIM_LITTLE_ENDIAN;
}

ossimPlanetDtedElevationDatabase::ossimPlanetDtedElevationDatabase(const ossimPlanetDtedElevationDatabase& src)
   :ossimPlanetElevationDatabase(src),
    theLocation(src.theLocation),
    theOpenFlag(src.theOpenFlag),
    theMaxOpenFiles(src.theMaxOpenFiles),
    theMinOpenFiles(src.theMinOpenFiles)
{
   theSwapBytesFlag = ossim::byteOrder() == OSSIM_LITTLE_ENDIAN;
}

ossimPlanetDtedElevationDatabase::~ossimPlanetDtedElevationDatabase()
{
}

ossimPlanetTextureLayer* ossimPlanetDtedElevationDatabase::dup()const
{
   return new ossimPlanetDtedElevationDatabase(*this);
}

ossimPlanetTextureLayer* ossimPlanetDtedElevationDatabase::dupType()const
{
   return new ossimPlanetDtedElevationDatabase;
}

ossimPlanetTextureLayerStateCode ossimPlanetDtedElevationDatabase::updateExtents()
{

   theDirtyExtentsFlag = false;
   return theStateCode;
}

void ossimPlanetDtedElevationDatabase::updateStats()const
{
   theStats->setTotalTextureSize(0);
   theDirtyStatsFlag = false;
   
}

void ossimPlanetDtedElevationDatabase::resetStats()const
{
   theStats->setBytesTransferred(0);
   theStats->setTotalTextureSize(0);
}

ossimPlanetTextureLayerStateCode ossimPlanetDtedElevationDatabase::open(const std::string& location)
{
   ossimElevManager::ConnectionStringVisitor visitor(location);
   
   ossimElevManager::instance()->accept(visitor);
   //std::cout << "DATABASE ==== " << (visitor.getElevationDatabase()?visitor.getElevationDatabase()->getConnectionString():"NOT FOUND") << std::endl;
   
   ossimFilename file(location);
   bool result = false;
   theLocation = "";
   theExtents = new ossimPlanetExtents;
   ossim_uint32 count = 0;
   ossim_uint32 maxCount = 25;
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
               ++count;
               do
               {
                  ossimRegExp eastern;
                  ossimRegExp western;
                  ossimRegExp easternUp;
                  ossimRegExp westernUp;
                  eastern.compile("e[0-9][0-9][0-9]");
                  western.compile("w[0-9][0-9][0-9]");
                  easternUp.compile("E[0-9][0-9][0-9]");
                  westernUp.compile("W[0-9][0-9][0-9]");
                  if(testFile.isDir())
                  {
                     if(eastern.find(testFile.c_str())||
                        western.find(testFile.c_str())||
                        easternUp.find(testFile.c_str())||
                        westernUp.find(testFile.c_str()))
                     {
                        result = true;
                     }
                     if(result)
                     {
                        result = false; // now find a North South file
                        ossimDirectory dirNS;
                        dirNS.open(testFile);
                        if(dirNS.getFirst(testFile))
                        {
                           do
                           {
                              std::ifstream in;
                              
                              in.open(testFile.c_str(), std::ios::binary|std::ios::in);
                              
                              if(!in.fail())
                              {
                                 
                                 ossimDtedVol vol(in);
                                 ossimDtedHdr hdr(in);
                                 ossimDtedUhl uhl(in);
                                 ossimDtedDsi dsi(in);
                                 ossimDtedAcc acc(in);
                                 
                                 if(uhl.getErrorStatus() != ossimErrorCodes::OSSIM_ERROR)
                                 {
                                    double metersPerPixel   = ossimGpt().metersPerDegree().y*uhl.latInterval();
                                    dirtyExtents(); // make sure parents are marked as dirty

                                    if(fabs(metersPerPixel - (ossimGpt().metersPerDegree().y *(30.0/3600))) < .25) // 30 arc
                                    {
                                       setName("DTED0");
                                       setDescription("DTED 1 kilometer elevation database");
                                       theExtents->setMinMaxScale(metersPerPixel, metersPerPixel*std::pow(2.0, 6));
                                    }
                                    else if(fabs(metersPerPixel - (ossimGpt().metersPerDegree().y *(3.0/3600))) < .25) // 3 arc
                                    {
                                       setName("DTED1");
                                       setDescription("DTED 90 meter elevation database");
                                       theExtents->setMinMaxScale(metersPerPixel, metersPerPixel*std::pow(2.0, 8));
                                    }
                                    else if(fabs(metersPerPixel - (ossimGpt().metersPerDegree().y *(1.0/3600))) < .25) // 1 arc
                                    {
                                       setName("DTED2");
                                       setDescription("DTED 30 meter elevation database");
                                       theExtents->setMinMaxScale(metersPerPixel, metersPerPixel*std::pow(2.0, 10));
                                    }
                                    else
                                    {
                                       setName("DTED");
                                       setDescription("DTED elevation database");
                                       theExtents->setMinMaxScale(metersPerPixel, metersPerPixel*std::pow(2.0, 12));
                                    }
                                    result = true;
                                 }
                              }
                           }while(dirNS.getNext(testFile)&&(!result));
                        }
                     }
                  }

               }while(dir.getNext(testFile)&&(!result)&&(count < maxCount));
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
bool ossimPlanetDtedElevationDatabase::hasTexture(ossim_uint32 width,
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

osg::ref_ptr<ossimPlanetImage> ossimPlanetDtedElevationDatabase::getTexture(ossim_uint32 width,
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
            

   return texture;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetDtedElevationDatabase::getTexture(ossim_uint32 level,
                                                                            ossim_uint64 row,
                                                                            ossim_uint64 col,
                                                                            const ossimPlanetGridUtility& utility)
{
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
   //double offset = 0.0;

   for(idx = 0; idx < numberOfFilesNeeded;++idx)
   {

      osg::ref_ptr<ossimPlanetDtedElevationDatabase::DtedInfo> dtedFile = getInfo(latLonOrigins[idx]);

      if(dtedFile.valid())
      {
         ossim_float32* bufPtr = (ossim_float32*)compositeData->getBuf();
         for(idxPts = 0; idxPts < nPoints; ++idxPts)
         {
            utility.getLatLon(latLonPoint, points[idxPts]);
            if((*bufPtr == OSSIMPLANET_NULL_HEIGHT)&&
               (latLonPoint[0] >= dtedFile->theMinLat)&&
               (latLonPoint[0] <= dtedFile->theMaxLat)&&
               (latLonPoint[1] >= dtedFile->theMinLon)&&
               (latLonPoint[1] <= dtedFile->theMaxLon))
            {
               utility.getLatLon(latLonPoint, points[idxPts]);
               ossim_float64 nullHeight = dtedFile->theHandler->getNullHeightValue();
              double h = dtedFile->theHandler->getHeightAboveMSL(ossimGpt(latLonPoint[0],
                                                                           latLonPoint[1]));
               if(!ossim::isnan(h)&&
                  (h!=nullHeight))
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

   return texture;

}

osg::ref_ptr<ossimPlanetDtedElevationDatabase::DtedInfo> ossimPlanetDtedElevationDatabase::getInfo(const std::string& name)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theDtedInfoMutex);
   DtedFilePointerList::iterator iter = theFilePointers.find(name);

   if(iter != theFilePointers.end())
   {
      iter->second->theTimeStamp = osg::Timer::instance()->tick();

      return iter->second;
   }
   osg::ref_ptr<DtedInfo> info     = new DtedInfo;
   
   ossimFilename dtedFile = ossimFilename(theLocation).dirCat(ossimFilename(name));

   ifstream in;

   in.open(dtedFile.c_str(), std::ios::binary|std::ios::in);

   if(in.fail()) return 0;

   ossimDtedVol vol(in);
   ossimDtedHdr hdr(in);
   ossimDtedUhl uhl(in);

   in.close();
   
   if((uhl.getErrorStatus() == ossimErrorCodes::OSSIM_ERROR))
   {
      return 0;
   }
   info->theNumLonLines  = uhl.numLonLines();
   info->theNumLatPoints = uhl.numLatPoints();
   info->theLatSpacing   = uhl.latInterval();
   info->theLonSpacing   = uhl.lonInterval();
   info->theMinLat = uhl.latOrigin();
   info->theMinLon = uhl.lonOrigin();
   info->theMaxLat = info->theMinLat + info->theLatSpacing*(info->theNumLatPoints-1);
   info->theMaxLon = info->theMinLon + info->theLonSpacing*(info->theNumLonLines-1);
   info->theTimeStamp  = osg::Timer::instance()->tick();
   info->theFilename   = dtedFile;
   info->theHandler = new ossimDtedHandler(dtedFile, false);
   //info->theHandler->setMemoryMapFlag(false);
   theFilePointers.insert(std::make_pair(name, info));
   shrinkFilePointers();
   
   return info;
   
}

void ossimPlanetDtedElevationDatabase::shrinkFilePointers()
{
   if(theFilePointers.size() <= theMaxOpenFiles) return;

   const osg::Timer* timer = osg::Timer::instance();
   
   osg::Timer_t currentTime = timer->tick();

   while((theFilePointers.size()>0) &&
         (theFilePointers.size() > theMinOpenFiles))
   {
      DtedFilePointerList::iterator iter = theFilePointers.begin();
      DtedFilePointerList::iterator currentLargestTimeDelta = iter;
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

ossimFilename ossimPlanetDtedElevationDatabase::buildFilename(double lat, double lon)const
{
   ossimFilename dtedFileBase;
   
   int ilon = static_cast<int>(lon);
   
   if (ilon < 0)
   {
      dtedFileBase = "w";
   }
   else
   {
      dtedFileBase = "e";
   }
   
   ilon = abs(ilon);
   std::ostringstream  s1;
   s1 << std::setfill('0') << std::setw(3)<< ilon;
   
   dtedFileBase += s1.str().c_str();//ossimString::toString(ilon);
   dtedFileBase += "/";
   
   int ilat =  static_cast<int>(floor(lat));
   if (ilat < 0)
   {
      dtedFileBase += "s";
   }
   else
   {
      dtedFileBase += "n";
   }

   ilat = abs(ilat);
   std::ostringstream  s2;

   s2<< std::setfill('0') << std::setw(2)<< ilat;
   
   dtedFileBase += s2.str().c_str();

   ossimFilename tempDir(theLocation);
   // Look for a dted file with a level 3 extension first.
   ossimFilename dtedName = tempDir.dirCat(dtedFileBase);
   dtedName += ".dt3";

   if (dtedName.exists())
   {
      return dtedFileBase + ".dt3";
   }

   // Look for a dted file with a level 2 extension next.
   dtedName = tempDir.dirCat(dtedFileBase);
   dtedName += ".dt2";

   if(dtedName.exists())
   {
      return dtedFileBase + ".dt2";
   }

   // Look for a dted file with a level 1 extension next.
   dtedName = tempDir.dirCat(dtedFileBase);
   dtedName += ".dt1";
  if(dtedName.exists())
   {
      return dtedFileBase + ".dt1";
   }

   // Look for a dted file with a level 0 extension next.
   dtedName = tempDir.dirCat(dtedFileBase);
   dtedName += ".dt0";
   if(dtedName.exists())
   {
      return dtedFileBase + ".dt0";
   }
   
   return "";
}

osg::ref_ptr<ossimPlanetDtedElevationDatabase::DtedInfo> ossimPlanetDtedElevationDatabase::findDtedInfo(const std::string& name)
{
   DtedFilePointerList::iterator iter = theFilePointers.find(name);
   
   if(iter != theFilePointers.end())
   {
      iter->second->theTimeStamp = osg::Timer::instance()->tick();
      return iter->second;
   }

   return 0;
 
}

