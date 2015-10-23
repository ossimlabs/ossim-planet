#include <ossimPlanet/ossimPlanetOssimElevationDatabase.h>
#include <ossimPlanet/ossimPlanetGrid.h>

ossimPlanetOssimElevationDatabase::ossimPlanetOssimElevationDatabase()
:ossimPlanetElevationDatabase()
{
}

ossimPlanetOssimElevationDatabase::ossimPlanetOssimElevationDatabase(const ossimPlanetOssimElevationDatabase& src)
:ossimPlanetElevationDatabase(src),
m_cellDatabaseFlag(src.m_cellDatabaseFlag),
m_database(src.m_database)
{
}

ossimPlanetOssimElevationDatabase::~ossimPlanetOssimElevationDatabase()
{
}

ossimPlanetTextureLayer* ossimPlanetOssimElevationDatabase::dup()const
{
   return new ossimPlanetOssimElevationDatabase(*this);
}

ossimPlanetTextureLayer* ossimPlanetOssimElevationDatabase::dupType()const
{
   return new ossimPlanetOssimElevationDatabase();
}

ossimPlanetTextureLayerStateCode ossimPlanetOssimElevationDatabase::updateExtents()
{
   ossimPlanetTextureLayerStateCode result = ossimPlanetTextureLayer_VALID;
   if(m_database.valid())
   {
      theExtents = new ossimPlanetExtents;
      ossim_float64 meanGsd = m_database->getMeanSpacingMeters();
      
      if(ossim::isnan(meanGsd))
      {
         
         theExtents->setMinMaxScale(meanGsd, meanGsd*64);//pow(2.0, 5.0));
      }
   }
   else
   {
      result = ossimPlanetTextureLayer_NOT_OPENED;
   }
   theDirtyExtentsFlag = false;
   return result;
}

void ossimPlanetOssimElevationDatabase::updateStats()const
{
   theStats->setTotalTextureSize(0);
   theDirtyStatsFlag = false;
}

void ossimPlanetOssimElevationDatabase::resetStats()const
{
   theStats->setBytesTransferred(0);
   theStats->setTotalTextureSize(0);
}

ossimPlanetTextureLayerStateCode ossimPlanetOssimElevationDatabase::open(const std::string& location)
{
   return ossimPlanetTextureLayer_NOT_OPENED;
}

bool ossimPlanetOssimElevationDatabase::hasTexture(ossim_uint32 width,
                                                   ossim_uint32 height,
                                                   const ossimPlanetTerrainTileId& tileId,
                                                   const ossimPlanetGrid& grid)
{
   if(!theEnableFlag||!theExtents.valid())
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
   if(gsd.y < theExtents->getMaxScale())
   {
      return true;
   }
   
   return false;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetOssimElevationDatabase::getTexture(ossim_uint32 width,
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

   if(!theExtents.valid()) return 0;
   
   if(m_cellDatabaseFlag&&!grid.isPolar(tileId))
   {
     return getTextureCellDatabase(width, height, tileId, grid, padding);
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
      double minValue = 999999999.0;
      double maxValue = -999999999.0;
      for(idxPts = 0; idxPts < nPoints; ++idxPts,++optimizedOutPtr)
      {
         double h = m_database->getHeightAboveEllipsoid(ossimGpt((*optimizedOutPtr).y(), (*optimizedOutPtr).x()));
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

osg::ref_ptr<ossimPlanetImage> ossimPlanetOssimElevationDatabase::getTextureCellDatabase(ossim_uint32 width,
                                                                                         ossim_uint32 height,
                                                                                         const ossimPlanetTerrainTileId& tileId,
                                                                                         const ossimPlanetGrid& grid,
                                                                                         ossim_int32 padding)

{
   ossimElevationCellDatabase* cellElevationDatabase = (ossimElevationCellDatabase*)m_database.get();
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
//   std::cout << "EXTENTS CHECK = " << gsd.y << " < " << theExtents->getMaxScale() << std::endl;
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
      ossim_float64 minModelX, minModelY, maxModelX, maxModelY;
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
      
      std::vector<ossimGpt> latLonOrigins;
      
      for(;lat >= wholeMinY; --lat)
      {
         lon = wholeMinX;
         for(;lon <= wholeMaxX; ++lon)
         {
            ossimGpt gpt(lat, lon);
            if(m_database->pointHasCoverage(gpt))
            {
               latLonOrigins.push_back(gpt);
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
      ossim_uint32 numberNeeded = latLonOrigins.size();
      for(idx = 0; idx < numberNeeded;++idx)
      {
         ossimRefPtr<ossimElevCellHandler> cellHandler = cellElevationDatabase->getOrCreateCellHandler(latLonOrigins[idx]);
         if(cellHandler.valid())
         {
            ossimGrect grect = cellHandler->getBoundingGndRect();
            ossim_float64 minLat = grect.ll().latd();
            ossim_float64 minLon = grect.ll().lond();
            ossim_float64 maxLat = grect.ur().latd();
            ossim_float64 maxLon = grect.ur().lond();
            ossim_float64 nullHeight = cellHandler->getNullHeightValue();
            ossim_float32* bufPtr = (ossim_float32*)compositeData->getBuf();
            optimizedOutPtr = &points.front();
            for(idxPts = 0; idxPts < nPoints; ++idxPts,++optimizedOutPtr)
            {
               if((*bufPtr == OSSIMPLANET_NULL_HEIGHT)&&
                  (optimizedOutPtr->y() >= minLat)&&
                  (optimizedOutPtr->y() <= maxLat)&&
                  (optimizedOutPtr->x() >= minLon)&&
                  (optimizedOutPtr->x() <= maxLon))
               {
                  double h = cellHandler->getHeightAboveMSL(ossimGpt(optimizedOutPtr->y(),
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
   
   return texture;
}

