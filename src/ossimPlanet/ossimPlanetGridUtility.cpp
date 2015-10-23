#include "ossimPlanet/ossimPlanetGridUtility.h"
#include <ossim/base/ossimCommon.h>
#include <iostream>
#include <ossim/base/ossimUnitConversionTool.h>
#include "ossimPlanet/mkUtils.h"



ossim_uint64 ossimPlanetGridUtility::getId(ossim_uint32 level,
                                           ossim_uint64 row,
                                           ossim_uint64 col)const
{
   ossim_uint64 w,h;
   getNumberOfTilesWideHigh(w, h, level);
   if(level == 0)
   {
      return ((ossim_uint64)row*w + (ossim_uint64)col);
   }
   ossim_uint64 shift = getTotalNumberOfTiles(level - 1);

   return (shift + ((ossim_uint64)w*row +
                    (ossim_uint64)col));
}

void ossimPlanetGridUtility::getPixelScaleAsDegrees(double& dx,
                                                    double& dy,
                                                    ossim_uint32 level,
                                                    ossim_uint64 row,
                                                    ossim_uint64 col)const
{
   ossimUnitType pixelScaleUnits;
   getPixelScale(dx, dy, pixelScaleUnits, level, row, col);

   if(pixelScaleUnits != OSSIM_DEGREES)
   {
      ossimUnitConversionTool conversion(dx, pixelScaleUnits);
      dx = conversion.getValue(OSSIM_DEGREES);
      conversion.setValue(dy, pixelScaleUnits);
      dy = conversion.getValue(OSSIM_DEGREES);
   }
}

void ossimPlanetGridUtility::getPixelScaleAsMeters(double& dx,
                                                   double& dy,
                                                   ossim_uint32 level,
                                                   ossim_uint64 row,
                                                   ossim_uint64 col)const
{
   ossimUnitType pixelScaleUnits;
   getPixelScale(dx, dy, pixelScaleUnits, level, row, col);

   if(pixelScaleUnits != OSSIM_METERS)
   {
      ossimUnitConversionTool conversion(dx, pixelScaleUnits);
      dx = conversion.getValue(OSSIM_METERS);
      conversion.setValue(dy, pixelScaleUnits);
      dy = conversion.getValue(OSSIM_METERS);
   }
}
      
void ossimPlanetGridUtility::mapToRowCol(ossim_uint64& targetRow,
                                         ossim_uint64& targetCol,
                                         ossim_uint32  targetLevel,
                                         ossim_uint32  srcLevel,
                                         ossim_uint64  srcRow,
                                         ossim_uint64  srcCol)const
{
   if(targetLevel > srcLevel)
   {
      ossim_uint32 diff = targetLevel-srcLevel;
      targetRow = srcRow<<diff;
      targetCol = srcCol<<diff;
   }
   else if(targetLevel < srcLevel)
   {
      ossim_uint32 diff = srcLevel-targetLevel;
      targetRow = srcRow>>diff;
      targetCol = srcCol>>diff;
   }
   else
   {
      targetRow = srcRow;
      targetCol = srcCol;
   }
}

void ossimPlanetGridUtility::getGeographicLonCrossings(std::vector<osg::Vec2d>& minMaxPairs,
                                                       ossim_uint32 level,
                                                       ossim_uint64 row,
                                                       ossim_uint64 col)const
{
   if(!crossesGeographicBounds(level, row, col))
   {
      double minLat, minLon, maxLat, maxLon;
      getLatLonBounds(minLat, minLon, maxLat, maxLon, level, row, col);
      minMaxPairs.push_back(osg::Vec2d(minLon, maxLon));
   }
   else
   {
      osg::Vec3d ul, ur, lr, ll;
      getLatLonCorners(ul, ur, lr, ll, level, row, col);
      osg::Vec2d pt;
      pt[0] = -180.0;
      pt[1] = -180.0;
      if(ul[ossimPlanetGridUtility::LON] < FLT_EPSILON)
      {
         if(ul[ossimPlanetGridUtility::LON] > pt[1])
         {
            pt[1] = ul[ossimPlanetGridUtility::LON];
         }
      }
      if(ur[ossimPlanetGridUtility::LON] < FLT_EPSILON)
      {
         if(ur[ossimPlanetGridUtility::LON] > pt[1])
         {
            pt[1] = ur[ossimPlanetGridUtility::LON];
         }
      }
      if(lr[ossimPlanetGridUtility::LON] < FLT_EPSILON)
      {
         if(lr[ossimPlanetGridUtility::LON] > pt[1])
         {
            pt[1] = lr[ossimPlanetGridUtility::LON];
         }
      }
      if(ll[ossimPlanetGridUtility::LON] < FLT_EPSILON)
      {
         if(ll[ossimPlanetGridUtility::LON] > pt[1])
         {
            pt[1] = ll[ossimPlanetGridUtility::LON];
         }
      }
      minMaxPairs.push_back(pt);
      
      pt[0] = 180.0;
      pt[1] = 180.0;
      if(ul[ossimPlanetGridUtility::LON] > -FLT_EPSILON)
      {
         if(ul[ossimPlanetGridUtility::LON] < pt[0])
         {
            pt[0] = ul[ossimPlanetGridUtility::LON];
         }
      }
      if(ur[ossimPlanetGridUtility::LON] > -FLT_EPSILON)
      {
         if(ur[ossimPlanetGridUtility::LON] < pt[0])
         {
            pt[0] = ur[ossimPlanetGridUtility::LON];
         }
      }
      if(lr[ossimPlanetGridUtility::LON] > -FLT_EPSILON)
      {
         if(lr[ossimPlanetGridUtility::LON] < pt[0])
         {
            pt[0] = lr[ossimPlanetGridUtility::LON];
         }
      }
      if(ll[ossimPlanetGridUtility::LON] > -FLT_EPSILON)
      {
         if(ll[ossimPlanetGridUtility::LON] < pt[0])
         {
            pt[0] = ll[ossimPlanetGridUtility::LON];
         }
      }
      minMaxPairs.push_back(pt);
   }
}

bool ossimPlanetGridUtility::crossesGeographicBounds(ossim_uint32 level,
                                                     ossim_uint64 row,
                                                     ossim_uint64 col)const
{
   osg::Vec3d ul;
   osg::Vec3d ur;
   osg::Vec3d lr;
   osg::Vec3d ll;

   getLatLonCorners(ul, ur, lr, ll, level, row, col);

   if((std::abs(ul[ossimPlanetGridUtility::LON]-ur[ossimPlanetGridUtility::LON])>180.0)||
      (std::abs(ur[ossimPlanetGridUtility::LON]-lr[ossimPlanetGridUtility::LON])>180.0)||
      (std::abs(lr[ossimPlanetGridUtility::LON]-ll[ossimPlanetGridUtility::LON])>180.0)||
      (std::abs(ll[ossimPlanetGridUtility::LON]-ul[ossimPlanetGridUtility::LON])>180.0))
   {
      return true;
   }
//    std::cout << "ul-ur = " << (ul[ossimPlanetGridUtility::LON]-ur[ossimPlanetGridUtility::LON]) << std::endl;
//    std::cout << "ur-lr = " << (ur[ossimPlanetGridUtility::LON]-lr[ossimPlanetGridUtility::LON]) << std::endl;
//    std::cout << "lr-ll = " << (lr[ossimPlanetGridUtility::LON]-ll[ossimPlanetGridUtility::LON]) << std::endl;
//    std::cout << "ll-ul = " << (ll[ossimPlanetGridUtility::LON]-ul[ossimPlanetGridUtility::LON]) << std::endl;
//    if((std::abs(ul[ossimPlanetGridUtility::LON] -
//                 ur[ossimPlanetGridUtility::LON]) > 200.0)||
//       (std::abs(ur[ossimPlanetGridUtility::LON] -
//                 lr[ossimPlanetGridUtility::LON]) > 200.0)||
//       (std::abs(lr[ossimPlanetGridUtility::LON] -
//                 ll[ossimPlanetGridUtility::LON]) > 200.0)||
//       (std::abs(ll[ossimPlanetGridUtility::LON] -
//                 ul[ossimPlanetGridUtility::LON]) > 200.0))
//    {
//       return true; 
//    }

   return false;
}

void ossimPlanetGridUtility::getWidthHeightInDegrees(double& deltaX,
                                                     double& deltaY,
                                                     ossim_uint32 level,
                                                     ossim_uint64 row,
                                                     ossim_uint64 col)const
{

   osg::Vec3d ul;
   osg::Vec3d ur;
   osg::Vec3d lr;
   osg::Vec3d ll;

   getLatLonCorners(ul, ur, lr, ll, level, row, col);
   ul[2] = 0;
   ur[2] = 0;
   lr[2] = 0;
   ll[2] = 0;
   deltaX = ossim::max((ul-ur).length(), (ll-lr).length());
   deltaY = ossim::max((ul-ll).length(), (ur-lr).length());
}

void ossimPlanetGridUtility::getLatLonCorners(osg::Vec3d& ul,
                                              osg::Vec3d& ur,
                                              osg::Vec3d& lr,
                                              osg::Vec3d& ll,
                                              ossim_uint32 level,
                                              ossim_uint32 row,
                                              ossim_uint32 col)const
{
   ossimPlanetGridUtility::GridPoint gPoint;
   getCenterGridPoint(gPoint,
                      level, row, col);
   const osg::Vec3d& gridPoint = gPoint.theGlobalGridPoint;
   ossimPlanetGridUtility::GridPoint p(gPoint);
   double xSpacing, ySpacing;
   getGridSpacing(xSpacing, ySpacing, level, row, col);

   double xHalf = xSpacing*.5;
   double yHalf = ySpacing*.5;
   p.theGlobalGridPoint = osg::Vec3d(gridPoint[ossimPlanetGridUtility::LAT] - yHalf,
                                     gridPoint[ossimPlanetGridUtility::LON] - xHalf,
                                     gridPoint[ossimPlanetGridUtility::HGT]);
   getLatLon(ul,
             p);
   
   p.theGlobalGridPoint = osg::Vec3d(gridPoint[ossimPlanetGridUtility::LAT] + yHalf,
                                     gridPoint[ossimPlanetGridUtility::LON] - xHalf,
                                     gridPoint[ossimPlanetGridUtility::HGT]);
   getLatLon(ur,
             p);
   
   p.theGlobalGridPoint = osg::Vec3d(gridPoint[ossimPlanetGridUtility::LAT] + yHalf,
                                     gridPoint[ossimPlanetGridUtility::LON] + xHalf,
                                     gridPoint[ossimPlanetGridUtility::HGT]);
   getLatLon(lr,
             p);
   
   p.theGlobalGridPoint = osg::Vec3d(gridPoint[ossimPlanetGridUtility::LAT] - yHalf,
                                     gridPoint[ossimPlanetGridUtility::LON] + xHalf,
                                     gridPoint[ossimPlanetGridUtility::HGT]);
   getLatLon(ll,
             p);
}

void ossimPlanetGridUtility::getCenterLatLon(osg::Vec3d& center,
                                             ossim_uint32 level,
                                             ossim_uint32 row,
                                             ossim_uint32 col)const
{
   ossimPlanetGridUtility::GridPoint centerPoint;
   getCenterGridPoint(centerPoint,
                      level,
                      row,
                      col);
   getLatLon(center,
             centerPoint);
}


void ossimPlanetGridUtility::getLatLonBounds(double& minLat,
                                             double& minLon,
                                             double& maxLat,
                                             double& maxLon,
                                             ossim_uint32 level,
                                             ossim_uint64 row,
                                             ossim_uint64 col)const
{
   std::vector<ossimPlanetGridUtility::GridPoint> points;
   createGridPoints(points,
                    level,
                    row,
                    col,
                    3,
                    3);
   minLat = 90.0;
   maxLat = -90.0;
   minLon = 180.0;
   maxLon = -180.0;

   ossim_uint32 idx = 0;
   ossim_uint32 size = points.size();
   osg::Vec3d latLon;
   for(idx = 0; idx < size; ++idx)
   {
      getLatLon(latLon, points[idx]);
      minLat = ossim::min(minLat, latLon[ossimPlanetGridUtility::LAT]);
      minLon = ossim::min(minLon, latLon[ossimPlanetGridUtility::LON]);
      maxLat = ossim::max(maxLat, latLon[ossimPlanetGridUtility::LAT]);
      maxLon = ossim::max(maxLon, latLon[ossimPlanetGridUtility::LON]);
   }
//    ossimPlanetGridUtility::GridPoint centerPoint;
//    osg::Vec3d ul;
//    osg::Vec3d ur;
//    osg::Vec3d lr;
//    osg::Vec3d ll;
//    osg::Vec3d center=ul;
//    ossimPlanetGridUtility::GridPoint point;
//    getCenterGridPoint(centerPoint, level, row, col);
//    getLatLon(center, centerPoint);
//    getLatLonCorners(ul, ur, lr, ll, level, row, col);
//    minLat = ossim::min(center[0], ossim::min(ll[0], ossim::min(lr[0], ossim::min(ul[0], ur[0]))));
//    maxLat = ossim::max(center[0], ossim::max(ll[0], ossim::max(lr[0], ossim::max(ul[0], ur[0]))));
//    minLon = ossim::min(center[1], ossim::min(ll[1], ossim::min(lr[1], ossim::min(ul[1], ur[1]))));
//    maxLon = ossim::max(center[1], ossim::max(ll[1], ossim::max(lr[1], ossim::max(ul[1], ur[1]))));
   
}

void ossimPlanetGridUtility::getCenterGridPoint(ossimPlanetGridUtility::GridPoint& point,
                                              ossim_uint32 level,
                                              ossim_uint32 row,
                                              ossim_uint32 col)const
{
   ossim_uint64 localRow; // local to the face
   ossim_uint64 localCol;
   ossim_uint64 wide;
   ossim_uint64 high;
   double xSpacing, ySpacing;
   getNumberOfTilesWideHighPerFace(wide, high, level);
   getGridSpacing(xSpacing, ySpacing, level, row, col);
   getLocalRowColumn(localRow, localCol,
                     level, row, col);
   
   point.theLocalGridPoint[ossimPlanetGridUtility::GRIDX] = .5;
   point.theLocalGridPoint[ossimPlanetGridUtility::GRIDY] = .5;
   point.theLocalGridPoint[ossimPlanetGridUtility::GRIDZ] = 0;
   point.theFace = getFace(level, row, col);

   point.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = ((double)localCol/(double)wide) + (xSpacing*.5);
   point.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = ((double)localRow/(double)high) + (ySpacing*.5);
   point.theGlobalGridPoint[ossimPlanetGridUtility::GRIDZ] = 0.0;
}

void ossimPlanetGridUtility::getCenterGridPoint(ossimPlanetGridUtility::GridPoint& gridPoint,
                                                ossim_uint32 level,
                                                const osg::Vec3d& latLon)const
{
   ossim_uint64 tilesWide, tilesHigh;
   getGridPoint(gridPoint, latLon);
   getNumberOfTilesWideHighPerFace(tilesWide, tilesHigh, level);
   double normWide = 1.0/tilesWide;
   double normHigh = 1.0/tilesHigh;

   ossim_uint32 col = (ossim_uint32)(gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX]/normWide);
   ossim_uint32 row = (ossim_uint32)(gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY]/normHigh);

   getCenterGridPoint(gridPoint,
                      level,
                      row,
                      col);
}

void ossimPlanetGridUtility::createGridPoints(std::vector<ossimPlanetGridUtility::GridPoint>& points,
                                              ossim_uint32 level,
                                              ossim_uint64 row,
                                              ossim_uint64 col,
                                              ossim_uint32 rows,
                                              ossim_uint32 cols)const
{
   ossim_uint64 wide;
   ossim_uint64 high;
   ossim_uint64 localRow;
   ossim_uint64 localCol;
   ossim_uint32 face = getFace(level, row, col);
   ossim_uint32 idx=0;
   ossim_uint32 rowIdx = 0;
   ossim_uint32 colIdx = 0;
   getLocalRowColumn(localRow, localCol,
                     level, row, col);
   getNumberOfTilesWideHighPerFace(wide, high, level);

   ossim_float64 globalColTOrigin = (double)localCol/(double)(wide);
   ossim_float64 globalRowTOrigin = (double)localRow/(double)(high);
   ossim_float64 spacingCol       = (1.0/(double)wide)/(cols-1);
   ossim_float64 spacingRow       = (1.0/(double)high)/(rows-1);
   ossim_uint32 size = (rows)*(cols);
   if(points.size() != size)
   {
      points.resize(size);
   }
  ossimPlanetGridUtility::GridPoint* pointList = &points.front();

   double rowT = globalRowTOrigin;
   for(rowIdx = 0; rowIdx < rows; ++rowIdx)
   {
      double localRowT  = ((double)rowIdx/((double)rows-1));
      double colT = globalColTOrigin;
      for(colIdx = 0; colIdx < cols; ++colIdx)
      {
         pointList[idx].theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = colT; 
         pointList[idx].theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = rowT;
         pointList[idx].theGlobalGridPoint[ossimPlanetGridUtility::GRIDZ] = 0.0;
         
         pointList[idx].theLocalGridPoint[ossimPlanetGridUtility::GRIDX] = (double)colIdx/((double)cols-1.0);
         pointList[idx].theLocalGridPoint[ossimPlanetGridUtility::GRIDY] = localRowT;
         pointList[idx].theLocalGridPoint[ossimPlanetGridUtility::GRIDZ] = 0.0;

         pointList[idx].theFace = face;
         ++idx;
         colT += spacingCol;
      }
      rowT += spacingRow;
   }
}

void ossimPlanetGridUtility::getGridSpacing(double& xSpacing,
                                            double& ySpacing,
                                            ossim_uint32 level,
                                            ossim_uint64 /*row*/,
                                            ossim_uint64 /*col*/)const
{
   ossim_uint64 wide;
   ossim_uint64 high;
   getNumberOfTilesWideHighPerFace(wide, high, level);
   xSpacing       = 1.0/(double)wide;
   ySpacing       = 1.0/(double)high;
   
}

void ossimPlanetGridUtility::getNumberOfTilesWideHigh(ossim_uint64 &wide,
                                                      ossim_uint64 &high,
                                                      ossim_uint32 level)const
{
   wide = (ossim_uint64)1<<(ossim_uint64)level;
   high = (ossim_uint64)1<<(ossim_uint64)level;

   wide*=getNumberOfFaces();
}

void ossimPlanetGridUtility::getLocalRowColumn(ossim_uint64& localRow,
                                               ossim_uint64& localCol,
                                               ossim_uint32 level,
                                               ossim_uint64 row,
                                               ossim_uint64 col)const
{
   ossim_uint64 wide;
   ossim_uint64 high;
   getNumberOfTilesWideHighPerFace(wide, high, level);
   ossim_uint32 face = getFace(level, row, col);

   localRow = row;
   localCol = col - face*wide;
}

ossim_uint32 ossimPlanetGridUtility::getFace(ossim_uint32 level,
                                             ossim_uint64 /*row*/,
                                             ossim_uint64 col)const
{
   return (col >> level);
   
}

void ossimPlanetGridUtility::getNumberOfTilesWideHighPerFace(ossim_uint64 &wide,
                                                             ossim_uint64 &high,
                                                             ossim_uint32 level)const
{
   wide = (ossim_uint64)1<<(ossim_uint64)level;
   high = (ossim_uint64)1<<(ossim_uint64)level;
}

ossim_uint64 ossimPlanetGridUtility::getNumberOfTiles(ossim_uint32 level)const
{
   ossim_uint64 wide;
   ossim_uint64 high;

   getNumberOfTilesWideHigh(wide, high, level);

   return ((ossim_uint64)wide * (ossim_uint64)high);
}


ossim_uint64 ossimPlanetGridUtility::getTotalNumberOfTiles(ossim_uint32 level)const
{
   ossim_uint64 result = 0;
   ossim_uint32 idx=0;

   for(idx = 0; idx <= level; ++idx)
   {
      result += getNumberOfTiles(idx);
   }

   return result;
}


ossim_uint32 ossimPlanetGridUtility::getTileWidth()const
{
   return theTileWidth;
}

ossim_uint32 ossimPlanetGridUtility::getTileHeight()const
{
   return theTileHeight;
}

void ossimPlanetGridUtility::setTileWidthHeight(ossim_uint32 tileWidth,
                                                ossim_uint32 tileHeight)
{
   theTileWidth  = tileWidth;
   theTileHeight = tileHeight;
}

