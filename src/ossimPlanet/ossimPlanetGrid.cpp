#include <ossimPlanet/ossimPlanetGrid.h>
#include <ossimPlanet/ossimPlanetPlaneGrid.h>
#include <ossimPlanet/ossimPlanetCubeGrid.h>
#include <osg/Vec2d>
#include <osg/io_utils>
#include <ossim/base/ossimDrect.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimUnitConversionTool.h>
#include <iomanip>

void ossimPlanetGrid::numberOfTilesPerFace(ossim_uint32 lod, ossim_uint64& tilesWide, ossim_uint64& tilesHigh) const
{
   ossim_uint64 value = 1<<lod;
   
   tilesWide = value;
   tilesHigh = value;
}

void ossimPlanetGrid::bounds(const ossimPlanetTerrainTileId& tileId, GridBound& bound)const
{
   ossim_float64 w,h;
   GridPoint gridPoint;
   origin(tileId, gridPoint);
   widthHeight(tileId, w, h);
   
   bound.theMinx = gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
   bound.theMiny = gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
   bound.theWidth  = w;
   bound.theHeight = h;
   bound.theFace   = tileId.face();
}

void ossimPlanetGrid::boundsToModel(const ossimPlanetTerrainTileId& tileId, 
                                    ModelPoint& p0, 
                                    ModelPoint& p1, 
                                    ModelPoint& p2,
                                    ModelPoint& p3)const
{
   GridBound bound;
   bounds(tileId, bound);
   globalGridToModel(GridPoint(tileId.face(),bound.minx(), bound.miny()),
                     p0);
   globalGridToModel(GridPoint(tileId.face(),bound.maxx(), bound.miny()),
                     p1);
   globalGridToModel(GridPoint(tileId.face(),bound.maxx(), bound.maxy()),
                     p2);
   globalGridToModel(GridPoint(tileId.face(),bound.minx(), bound.maxy()),
                     p3);
}

void ossimPlanetGrid::centerGrid(const ossimPlanetTerrainTileId& tileId, GridPoint& gridPoint)
{
   ossim_float64 w,h;
   origin(tileId, gridPoint);
   widthHeight(tileId, w, h);
   gridPoint.theXYZ[ossimPlanetGrid::X_IDX] += (w*.5);
   gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] += (h*.5);
}

void ossimPlanetGrid::centerModel(const ossimPlanetTerrainTileId& tileId, ModelPoint& modelPoint)
{
   GridPoint gridPoint;
   centerGrid(tileId, gridPoint);
   globalGridToModel(gridPoint, modelPoint);
}

void ossimPlanetGrid::widthHeight(const ossimPlanetTerrainTileId& tileId, ossim_float64& width, ossim_float64& height)const
{
   // assume square in NDC 0..1 and divide by the 2^level
   //
   ossim_float64 value = 1.0/(ossim_float64)(1<<tileId.level());
   
   width  = value;
   height = value;
}

void ossimPlanetGrid::origin(const ossimPlanetTerrainTileId& tileId, GridPoint& gridPoint)const
{
   ossim_float64 w,h;
   
   // get the width hieght in global 0..1 grid space.
   widthHeight(tileId, w, h);
   gridPoint.theFace = tileId.face();
   gridPoint.theXYZ[ossimPlanetGrid::X_IDX] = tileId.x()*w;
   gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] = tileId.y()*h;
}

void ossimPlanetGrid::getUnitsPerPixel(osg::Vec2d& unitsPerPixel,
                                       const ossimPlanetTerrainTileId& tileId, 
                                       ossim_uint32 w, 
                                       ossim_uint32 h,
                                       const ossimUnitType unitType)const
{
   unitsPerPixel[0] = unitsPerPixel[1] = 0.0;
   if(theModelType == GEODETIC_MODEL)
   {
      osg::Vec2d modelWH;

      widthHeightInModelSpace(tileId, modelWH);
      ossimUnitConversionTool toolX(modelWH[0], OSSIM_DEGREES);
      ossimUnitConversionTool toolY(modelWH[1], OSSIM_DEGREES);
      unitsPerPixel[0]  =  toolX.getValue(unitType);
      unitsPerPixel[1]  =  toolY.getValue(unitType);
      unitsPerPixel[0] /= w;
      unitsPerPixel[1] /= h;
   }
}

bool ossimPlanetGrid::convertToGeographicExtents(const ossimPlanetTerrainTileId& tileId,
                                                 ossimPlanetExtents& extents, 
                                                 ossim_uint32 w, ossim_uint32 h)const
{
   static const ossimGpt wgs84Point;
   bool result = false;
   if(theModelType == GEODETIC_MODEL)
   {
      ModelPoint minPoint, maxPoint;
      osg::Vec2d modelWH;
      modelBound(tileId, minPoint, maxPoint);
      extents.setMinMaxLatLon(minPoint.y(), minPoint.x(),
                              maxPoint.y(), maxPoint.x());
      widthHeightInModelSpace(tileId, modelWH);
      ossim_float64 average = (modelWH[0]+modelWH[1])*.5;
      ossim_float64 mpd = wgs84Point.metersPerDegree().y;
      ossim_float64 mpp = (mpd*average)/((w+h)*.5);
      // we will set the gsd as the center for this tile and then make a range that covers to approximately
      // the next level down and the next level up.
      //
      extents.setMinMaxScale(mpp*.5, mpp*2.0);
      
      result = true;
   }
   return result;
}

void ossimPlanetGrid::localNdcToGlobalGrid(const ossimPlanetTerrainTileId& tileId, const LocalNdcPoint& localNdc, GridPoint& globalGrid)const
{
   GridBound b;
   bounds(tileId, b);
   
   globalGrid.setZ(localNdc.z());
   globalGrid.setX(b.minx() + b.width()*localNdc.x());
   globalGrid.setY(b.miny() + b.height()*localNdc.y());
   globalGrid.setFace(tileId.face());
}

void ossimPlanetGrid::localNdcToModel(const ossimPlanetTerrainTileId& tileId, const LocalNdcPoint& localNdc, ModelPoint& model)const
{
   GridPoint gridPoint;
   localNdcToGlobalGrid(tileId, localNdc, gridPoint);
   globalGridToModel(gridPoint, model);
}

void ossimPlanetCubeGrid2::getRootIds(TileIds &ids) const
{
   ids.push_back(ossimPlanetTerrainTileId(0,0,0,0));
   ids.push_back(ossimPlanetTerrainTileId(1,0,0,0));
   ids.push_back(ossimPlanetTerrainTileId(2,0,0,0));
   ids.push_back(ossimPlanetTerrainTileId(3,0,0,0));
   ids.push_back(ossimPlanetTerrainTileId(4,0,0,0));
   ids.push_back(ossimPlanetTerrainTileId(5,0,0,0));
}


void ossimPlanetGrid::getInternationalDateLineCrossings(const ossimPlanetTerrainTileId& tileId,
                                                        std::vector<osg::Vec2d>& minMaxPairs)const
{
   ModelPoint ll, lr, ur, ul;
   
   GridBound bound;
   bounds(tileId, bound);
   
   globalGridToModel(GridPoint(tileId.face(), bound.minx(), bound.miny()), ll);
   globalGridToModel(GridPoint(tileId.face(), bound.maxx(), bound.miny()), lr);
   globalGridToModel(GridPoint(tileId.face(), bound.maxx(), bound.maxy()), ur);
   globalGridToModel(GridPoint(tileId.face(), bound.minx(), bound.maxy()), ul);
   if(!crossesInternationalDateLine(tileId)||(theModelType!=GEODETIC_MODEL))
   {
      double minLon = ossim::min(ll.x(), ossim::min(lr.x(), ossim::min(ur.x(), ul.x())));
      double maxLon = ossim::max(ll.x(), ossim::max(lr.x(), ossim::max(ur.x(), ul.x())));
      minMaxPairs.push_back(osg::Vec2d(minLon, maxLon));
   }
   else if(theModelType == GEODETIC_MODEL) 
   {
      
      osg::Vec2d pt;
      pt[0] = -180.0;
      pt[1] = -180.0;
      if(ul.x() < FLT_EPSILON)
      {
         if(ul.x() > pt[1])
         {
            pt[1] = ul.x();
         }
      }
      if(ur.x() < FLT_EPSILON)
      {
         if(ur.x() > pt[1])
         {
            pt[1] = ur.x();
         }
      }
      if(lr.x() < FLT_EPSILON)
      {
         if(lr.x() > pt[1])
         {
            pt[1] = lr.x();
         }
      }
      if(ll.x() < FLT_EPSILON)
      {
         if(ll.x() > pt[1])
         {
            pt[1] = ll.x();
         }
      }
      minMaxPairs.push_back(pt);
      
      pt[0] = 180.0;
      pt[1] = 180.0;
      if(ul.x() > -FLT_EPSILON)
      {
         if(ul.x() < pt[0])
         {
            pt[0] = ul.x();
         }
      }
      if(ur.x() > -FLT_EPSILON)
      {
         if(ur.x() < pt[0])
         {
            pt[0] = ur.x();
         }
      }
      if(lr.x() > -FLT_EPSILON)
      {
         if(lr.x() < pt[0])
         {
            pt[0] = lr.x();
         }
      }
      if(ll.x() > -FLT_EPSILON)
      {
         if(ll.x() < pt[0])
         {
            pt[0] = ll.x();
         }
      }
      minMaxPairs.push_back(pt);
   }
}

void ossimPlanetGrid::createModelPoints(const ossimPlanetTerrainTileId& tileId,
                                        ossim_uint32 w,
                                        ossim_uint32 h,
                                        ossimPlanetGrid::ModelPoints& modelPoints,
                                        ossim_uint32 padding)const
{
   ossim_int32 signedPadding = padding;
   ossim_int32 rowIdx = 0;
   ossim_int32 colIdx = 0;
   ossim_float64 incH = 1.0/(h-1);
   ossim_float64 incW = 1.0/(w-1);
   ossim_int32 maxH = h + padding;
   ossim_int32 maxW = w + padding;
   ossimPlanetGrid::LocalNdcPoint localPoint;
   ossimPlanetGrid::ModelPoint modelPoint;
   modelPoints.clear();
   for(rowIdx = -signedPadding; rowIdx < maxH;++rowIdx)
   {
      localPoint.setY(rowIdx*incH);
      for(colIdx = -signedPadding; colIdx < maxW;++colIdx)
      {
         localPoint.setX(colIdx*incW);
         localNdcToModel(tileId, localPoint, modelPoint);
         modelPoints.push_back(modelPoint);
      }
   }
}

bool ossimPlanetGrid::crossesInternationalDateLine(const ossimPlanetTerrainTileId& tileId)const
{
   if(theModelType != GEODETIC_MODEL) return false;
   ModelPoint minPoint, maxPoint;
   modelBound(tileId, minPoint, maxPoint);
   return (fabs(minPoint.x()-maxPoint.x()) > 180.0);
}

void ossimPlanetGrid::modelBound(const ossimPlanetTerrainTileId& tileId, 
                                 ModelPoint& minPoint, ModelPoint& maxPoint)const
{
   ModelPoints points;
   createModelPoints(tileId,
                     3,
                     3,
                     points);
   minPoint.setX(1.0/FLT_EPSILON);
   minPoint.setY(1.0/FLT_EPSILON);
   minPoint.setZ(1.0/FLT_EPSILON);
   maxPoint.setX(-1.0/FLT_EPSILON);
   maxPoint.setY(-1.0/FLT_EPSILON);
   maxPoint.setZ(-1.0/FLT_EPSILON);
   ossim_uint32 idx = 0;
   ossim_uint32 size = points.size();
   osg::Vec3d latLon;
   for(idx = 0; idx < size; ++idx)
   {
      minPoint.setX(ossim::min(minPoint.x(), points[idx].x()));
      minPoint.setY(ossim::min(minPoint.y(), points[idx].y()));
      minPoint.setZ(ossim::min(minPoint.z(), points[idx].z()));
      maxPoint.setX(ossim::max(maxPoint.x(), points[idx].x()));
      maxPoint.setY(ossim::max(maxPoint.y(), points[idx].y()));
      maxPoint.setZ(ossim::max(maxPoint.z(), points[idx].z()));
   }
}

void ossimPlanetCubeGrid2::modelToGlobalGrid(const ModelPoint& modelPoint, 
                                             GridPoint& gridPoint)const
{
   osg::Vec2d ll((ossim::clamp((double)modelPoint.y(),
                               (double)-90.0, (double)90.0)+90)/180,
                 (ossim::wrap((double)modelPoint.x(),
                              (double)-180.0, (double)180.0)+180)/360);
   int face_x = (int)(4 * ll[ossimPlanetGrid::LON_IDX]);
   int face_y = (int)(2 * ll[ossimPlanetGrid::LAT_IDX] + 0.5);
   if(face_x == 4)
   {
      face_x = 3;
   }
   if(face_y == 1)
   {
      gridPoint.theFace = face_x;
   }
   else
   {
      gridPoint.theFace = face_y < 1 ? 5 : 4;
   }
   gridPoint.theXYZ[ossimPlanetGrid::X_IDX] = 4 * ll[ossimPlanetGrid::LON_IDX] - face_x;
   gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] = 2 * ll[ossimPlanetGrid::LAT_IDX] - 0.5;
   if(gridPoint.theFace < 4) // equatorial calculations done
   {
      //       gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = 1-gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
      return;
   }
   
   double tmp=0.0;
   if(gridPoint.theFace == 4) // north polar face
   {
      gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] = 1.5 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
      gridPoint.theXYZ[ossimPlanetGrid::X_IDX] = (2 * (gridPoint.theXYZ[ossimPlanetGrid::X_IDX] - 0.5) * 
                                                  gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] + 0.5);
      switch(face_x)
      {
         case 0: // bottom
         {
            gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] = 0.5 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            break;
         }
         case 1: // right side, swap and reverse lat
         {
            tmp = gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            gridPoint.theXYZ[ossimPlanetGrid::X_IDX] = 0.5 + gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] = tmp;
            break;
         }
         case 2: // top; reverse lat and lon
         {
            gridPoint.theXYZ[ossimPlanetGrid::X_IDX] = 1 - gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] = 0.5 + gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            break;
         }
         case 3: // left side; swap and reverse lon
         {
            tmp = gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            gridPoint.theXYZ[ossimPlanetGrid::X_IDX] = 0.5 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            gridPoint.theXYZ[ossimPlanetGrid::Y_IDX]= 1 - tmp;
            break;
         }
      }
   }
   else // south polar face
   {
      gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] += 0.5;
      gridPoint.theXYZ[ossimPlanetGrid::X_IDX] = (2 * (gridPoint.theXYZ[ossimPlanetGrid::X_IDX] - 0.5) * gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] + 0.5);
      switch(face_x)
      {
         case 0: // left
         {
            tmp = gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            gridPoint.theXYZ[ossimPlanetGrid::X_IDX] = 0.5 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] = tmp;
            break;
         }
         case 1: // top
         {
            gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] = 0.5 + gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            break;
         }
         case 2: // right
         {
            tmp = gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            gridPoint.theXYZ[ossimPlanetGrid::X_IDX] = 0.5 + gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] = 1 - tmp;
            break;
         }
         case 3: // bottom
         {
            gridPoint.theXYZ[ossimPlanetGrid::X_IDX] = 1 - gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] = 0.5 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            break;
         }
      }
   }
}

void ossimPlanetCubeGrid2::globalGridToModel(const GridPoint& gridPoint, 
                                             ModelPoint& modelPoint)const
{
   globalGridToModelLat45(gridPoint, modelPoint);
}

void ossimPlanetCubeGrid2::globalGridToModelLat45(const GridPoint& gridPoint, 
                                                  ModelPoint& modelPoint)const
{
   
   double offset = 0.0;
   osg::Vec2d s(ossim::clamp(gridPoint.theXYZ[ossimPlanetGrid::X_IDX], 0.0, 1.0),
                ossim::clamp(gridPoint.theXYZ[ossimPlanetGrid::Y_IDX], 0.0, 1.0));
   modelPoint.theXYZ[ossimPlanetGrid::Z_IDX] = gridPoint.z();
   if(gridPoint.theFace < 4)
   {
      s[ossimPlanetGrid::X_IDX] = (gridPoint.theXYZ[ossimPlanetGrid::X_IDX]+gridPoint.theFace)*.25;
      s[ossimPlanetGrid::Y_IDX] = (gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] + 0.5)*0.5;
      //      latLon.x() = s[ossimPlanetGridUtility::GRIDX]*360.0 - 180.0;
      //      latLon[ossimPlanetGridUtility::LAT] = 90 - s[ossimPlanetGridUtility::GRIDY]*180.0;
      //      return;
   }
   else if(gridPoint.theFace == 4)
   {
      if(gridPoint.theXYZ[ossimPlanetGrid::X_IDX] < gridPoint.theXYZ[ossimPlanetGrid::Y_IDX])
      {
         if(gridPoint.theXYZ[ossimPlanetGrid::X_IDX] + gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] < 1.0)
         {
            s[ossimPlanetGrid::X_IDX] = 1.0 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            s[ossimPlanetGrid::Y_IDX] = gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            offset += 3;
         }
         else
         {
            s[ossimPlanetGrid::Y_IDX] = 1.0 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            s[ossimPlanetGrid::X_IDX] = 1.0 - gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            offset += 2;
         }
      }
      else if((gridPoint.theXYZ[ossimPlanetGrid::X_IDX] + gridPoint.theXYZ[ossimPlanetGrid::Y_IDX]) >= 1.0)
      {
         s[ossimPlanetGrid::X_IDX] = gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
         s[ossimPlanetGrid::Y_IDX] = 1.0 - gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
         offset += 1.0;
      }
      s[ossimPlanetGrid::X_IDX] -= s[ossimPlanetGrid::Y_IDX];
      if(!ossim::almostEqual(s[ossimPlanetGrid::Y_IDX], .5))
      {
         s[ossimPlanetGrid::X_IDX] *= .5/(.5 - s[ossimPlanetGrid::Y_IDX]);
      }
      s[ossimPlanetGrid::X_IDX] = (s[ossimPlanetGrid::X_IDX] + offset)*0.25;
      s[ossimPlanetGrid::Y_IDX] = (s[ossimPlanetGrid::Y_IDX] + 1.5)*.5;
   }
   else if(gridPoint.theFace == 5)
   {
      offset = 1.0;
      if(gridPoint.theXYZ[ossimPlanetGrid::X_IDX] > gridPoint.theXYZ[ossimPlanetGrid::Y_IDX])
      {
         if(gridPoint.theXYZ[ossimPlanetGrid::X_IDX] + gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] >= 1.0)
         {
            s[ossimPlanetGrid::X_IDX] = 1.0 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            s[ossimPlanetGrid::Y_IDX] = gridPoint.theXYZ[ossimPlanetGrid::X_IDX] - .5;
            offset += 1.0;
         }
         else
         {
            s[ossimPlanetGrid::X_IDX] = 1.0 - gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            s[ossimPlanetGrid::Y_IDX] = 0.5 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            offset+=2;
         }
      }
      else
      {
         if(gridPoint.theXYZ[ossimPlanetGrid::X_IDX] +  gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] < 1.0)
         {
            s[ossimPlanetGrid::X_IDX] = gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            s[ossimPlanetGrid::Y_IDX] = 0.5 - gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            offset -= 1.0;
         }
         else
         {
            s[ossimPlanetGrid::Y_IDX] = gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] - 0.5;
         }
      }
      if(!ossim::almostEqual((double)s[ossimPlanetGrid::Y_IDX], (double)0.0))
      {
         //         s[ossimPlanetGrid::X_IDX] = ((s[ossimPlanetGrid::X_IDX] - 0.5)*.5)/s[ossimPlanetGrid::Y_IDX] + .5;
         s[ossimPlanetGrid::X_IDX] = ((s[ossimPlanetGrid::X_IDX] - 0.5)*.5)/s[ossimPlanetGrid::Y_IDX] + .5;
      }
      s[ossimPlanetGrid::X_IDX] = (s[ossimPlanetGrid::X_IDX] + offset) *0.25;
      s[ossimPlanetGrid::Y_IDX] *=.5;
   }
   modelPoint.theXYZ[ossimPlanetGrid::X_IDX] = s[ossimPlanetGrid::X_IDX]*360.0 - 180.0;
   modelPoint.theXYZ[ossimPlanetGrid::Y_IDX] = s[ossimPlanetGrid::Y_IDX]*180.0 - 90;
   modelPoint.theXYZ[ossimPlanetGrid::Z_IDX] = gridPoint.z();
}

void ossimPlanetCubeGrid2::globalGridToModelLat67_5(const GridPoint& gridPoint, ModelPoint& modelPoint)const
{
   
   // double offset = 0.0;
   osg::Vec2d s(ossim::clamp(gridPoint.theXYZ[ossimPlanetGrid::X_IDX], 0.0, 1.0),
                ossim::clamp(gridPoint.theXYZ[ossimPlanetGrid::Y_IDX], 0.0, 1.0));
   modelPoint.theXYZ[ossimPlanetGrid::Z_IDX] = gridPoint.z();
   if(gridPoint.theFace < 24)
   {
      modelPoint.setX(gridPoint.x()*360.0 - 180.0);
      modelPoint.setY(-67.5 + gridPoint.y()*135);
      return;
   }
   else if(gridPoint.theFace == 24)// north cap
   {
   }
   else if(gridPoint.theFace == 25)
   {
   }
}

void ossimPlanetCubeGrid2::widthHeightInModelSpace(const ossimPlanetTerrainTileId& tileId, osg::Vec2d& deltaXY)const
{
   GridBound b;
   
   bounds(tileId, b);
   ossim_float64 multiplier = 90;
   deltaXY[0] = multiplier*b.width();
   deltaXY[1] = multiplier*b.height();
}


bool ossimPlanetCubeGrid2::findGridBound(ossim_uint32 face,
                                         const ModelPoint& minPoint,
                                         const ModelPoint& maxPoint,
                                         GridBound& bound,
                                         ossim_uint32 numberOfPoints)const
{
   ossimDrect modelClipBound;
   ossimDrect modelBound(minPoint.x(), maxPoint.y(), maxPoint.x(), minPoint.y(), OSSIM_RIGHT_HANDED);
   ossimDrect modelFaceBound;
   modelFaceBound.makeNan();
   double epsilon = 1e-10;
   if(face < 4)
   {
      // ossim_uint32 dx=0;
      ossim_float64 originLat=-45, originLon=-180;
      originLon = -180.0+(face*90);
      modelFaceBound = ossimDrect(originLon, 
                                  originLat+90-epsilon, originLon + 90.0-epsilon, originLat, OSSIM_RIGHT_HANDED);
   }
   else if(face == 4)
   {
      modelFaceBound = ossimDrect(-180.0, 90.0, 180.0, 45.0, OSSIM_RIGHT_HANDED);
      
   }
   else if(face == 5)
   {
      modelFaceBound = ossimDrect(-180.0, -45.0-epsilon, 180.0-epsilon, -90.0, OSSIM_RIGHT_HANDED);
   }
   bool result = modelFaceBound.intersects(modelBound);
   if(result)
   {
      modelClipBound = modelBound.clipToRect(modelFaceBound);
      
      std::vector<ossimDpt> gridPoints;
      ossim_uint32 idxY = 0;
      ossim_uint32 idxX = 0;
      double deltaModelX = modelClipBound.ur().x-modelClipBound.ll().x;
      double deltaModelY = modelClipBound.ur().y-modelClipBound.ll().y;
      ossimDpt ll = modelClipBound.ll();
      
      ossimPlanetGrid::GridPoint gridPoint;
      for(idxY = 0; idxY <numberOfPoints; ++idxY)
      {
         for(idxX = 0; idxX < numberOfPoints; ++idxX)
         {
            double tx = (double)idxX/(numberOfPoints-1);
            double ty = (double)idxY/(numberOfPoints-1);
            
            ossimPlanetGrid::ModelPoint modelPoint(ll.x + deltaModelX*tx,
                                                   ll.y + deltaModelY*ty);
            modelToGlobalGrid(modelPoint, gridPoint);
           gridPoints.push_back(ossimDpt(gridPoint.x(), gridPoint.y()));
         }
      }
      ossimDrect rect(gridPoints, OSSIM_RIGHT_HANDED);
      bound.theMinx = rect.ll().x;
      bound.theMiny = rect.ll().y;
      bound.theWidth = (rect.ur().x-rect.ll().x);
      bound.theHeight= (rect.ur().y-rect.ll().y);
      bound.theFace = face;
   }
   return result;
}

ossim_uint32 ossimPlanetCubeGrid2::numberOfFaces()const
{
   return 6;
}

ossimPlanetGridUtility* ossimPlanetCubeGrid2::newBackwardCompatableGrid(ossim_uint32 width,
                                                                        ossim_uint32 height)const
{
   return new ossimPlanetCubeGrid(width, height);
}

bool ossimPlanetCubeGrid2::isPolar(const ossimPlanetTerrainTileId& id)const
{
   return (id.face() >3);
}

void ossimPlanetPlaneGrid2::getRootIds(TileIds &ids) const
{
   ids.push_back(ossimPlanetTerrainTileId(0,0,0,0));
   ids.push_back(ossimPlanetTerrainTileId(1,0,0,0));
}

void ossimPlanetPlaneGrid2::globalGridToModel(const GridPoint& gridPoint, ModelPoint& modelPoint)const
{
   modelPoint.setY(180.0*gridPoint.y() - 90.0);
   if(gridPoint.face() == 0)
   {
      modelPoint.setX(180.0*gridPoint.x() - 180.0);
   }
   else
   {
      modelPoint.setX(180.0*gridPoint.x());
   }
   
   modelPoint.setZ(gridPoint.z());
}

void ossimPlanetPlaneGrid2::modelToGlobalGrid(const ModelPoint& modelPoint, 
                                             GridPoint& gridPoint)const
{
   gridPoint.setY((modelPoint.y()+90.0)/180.0);      
   if(modelPoint.x() < 0.0)
   {
      gridPoint.setFace(0);
      gridPoint.setX(modelPoint.x()/180.0 + 1.0);
   }
   else
   {
      gridPoint.setFace(1);
      gridPoint.setX(modelPoint.x()/180.0);
   }
   
   gridPoint.setZ(modelPoint.z());
}

void ossimPlanetPlaneGrid2::widthHeightInModelSpace(const ossimPlanetTerrainTileId& tileId, 
                                                    osg::Vec2d& deltaXY)const
{
   GridBound b;
   
   bounds(tileId, b);
   
   deltaXY[0] = 180.0*b.width();
   deltaXY[1] = 180.0*b.height();
}

bool ossimPlanetPlaneGrid2::findGridBound(ossim_uint32 face,
                                          const ModelPoint& minPoint,
                                          const ModelPoint& maxPoint,
                                          GridBound& bound,
                                          ossim_uint32 /* numberOfPoints */)const
{
   ossimDrect modelClipBound;
   ossimDrect modelBound(minPoint.x(), maxPoint.y(), maxPoint.x(), minPoint.y(), OSSIM_RIGHT_HANDED);
   ossimDrect modelFaceBound;
   if(face == 0)
   {
      modelFaceBound = ossimDrect(-180.0, 90.0, 0.0, -90.0, OSSIM_RIGHT_HANDED);
   }
   else
   {
      modelFaceBound = ossimDrect(0.0, 90, 180.0, -90.0, OSSIM_RIGHT_HANDED);
   }
   bool result = modelFaceBound.intersects(modelBound);
   if(result)
   {
      modelClipBound = modelBound.clipToRect(modelFaceBound);
//      std::cout << "modelClipBound " << modelClipBound << std::endl;
  }
      
   bound.theMiny = (90.0 + modelClipBound.ll().y)/180.0;
   bound.theMinx = modelClipBound.ll().x/180.0;
   if(face == 0)
   {
       bound.theMinx+= 1.0;
   }
   
   bound.setFace(face);
   bound.theWidth  = (modelClipBound.ur().x-modelClipBound.ll().x)/180.0;
   bound.theHeight = (modelClipBound.ur().y-modelClipBound.ll().y)/180.0;
   
//   std::cout << "bound.theMinX = " << bound.theMinx << "\n"
//   << "bound.theMinY = " << bound.theMiny << "\n";
   return result;
}

ossim_uint32 ossimPlanetPlaneGrid2::numberOfFaces()const
{
   return 12;
}

ossimPlanetGridUtility* ossimPlanetPlaneGrid2::newBackwardCompatableGrid(ossim_uint32 width,
                                                                         ossim_uint32 height)const
{
   return new ossimPlanetPlaneGrid(width, height);
}

ossimPlanetAdjustableCubeGrid::ossimPlanetAdjustableCubeGrid(CapLocation location)
:ossimPlanetCubeGrid2()
{
   setCapLocation(location);
}

void ossimPlanetAdjustableCubeGrid::setCapLocation(CapLocation location)
{
   theCapLocation = location;
   switch(location)
   {
      case LOW_CAP:
      {
         thePolarLat = 45.0;
         break;
      }
      case MEDIUM_LOW_CAP:
      {
         thePolarLat = 67.5;
         break;
      }
      case MEDIUM_CAP:
      {
         thePolarLat = 78.75;
         break;
      }
      case MEDIUM_HIGH_CAP:
      {
         thePolarLat = 84.375;
         break;
      }
      case HIGH_CAP:
      {
         thePolarLat = 87.1875;
         break;
      }
      default:
      {
         thePolarLat = 78.75;
         break;
      }
   }
   theUpperEquatorialBandLatDelta = thePolarLat-45.0;
   thePolarWidth                  = 2*(90-thePolarLat);// used in calculating crude ground distance.
}

void ossimPlanetAdjustableCubeGrid::getRootIds(TileIds &ids) const
{
   if(theCapLocation == LOW_CAP)
   {
      ossimPlanetCubeGrid2::getRootIds(ids);
   }
   else
   {
      ossim_uint32 idx = 0;
      for(idx = 0; idx < numberOfFaces(); ++idx)
      {
         ids.push_back(ossimPlanetTerrainTileId(idx,0,0,0));
      }
   }
}

void ossimPlanetAdjustableCubeGrid::globalGridToModel(const GridPoint& gridPoint, ModelPoint& modelPoint)const
{
   if(theCapLocation == LOW_CAP)
   {
      ossimPlanetCubeGrid2::globalGridToModel(gridPoint, modelPoint);
      return;
   }
   double xt = ossim::clamp(gridPoint.x(), 0.0, 1.0);
   double yt = ossim::clamp(gridPoint.y(), 0.0, 1.0);
   // equatorial 
   if(gridPoint.face() < 4)
   {
      modelPoint.setY(90.0*yt - 45.0);
      modelPoint.setX(-180 + 90*(gridPoint.face() + xt));
   }
   // northern band
   else if(gridPoint.face() < 8)
   {
      modelPoint.setY(45+theUpperEquatorialBandLatDelta*yt);
      modelPoint.setX(-180 + 90.0*(gridPoint.face()-4 + xt));
   }
   // southern band
   else if(gridPoint.face() < 12)
   {
      modelPoint.setY(-thePolarLat + theUpperEquatorialBandLatDelta*yt);
      modelPoint.setX(-180 + 90.0*(gridPoint.face()-8 +xt));
   }
   // north polar cap
   else if(gridPoint.face() == 12)
   {
      GridPoint gPt(gridPoint);
      
      gPt.setFace(4);
      ossimPlanetCubeGrid2::globalGridToModel(gPt, modelPoint);
      // since we are using the formula for a 45 degree lat cube we will remap
      // to our polar hack location.
      //
      double latT = (modelPoint.theXYZ[ossimPlanetGrid::Y_IDX] - 45.0)/45;
      modelPoint.theXYZ[ossimPlanetGrid::Y_IDX] = thePolarLat + latT*(90.0-thePolarLat);
   }
   // south polar cap
   else if(gridPoint.face() == 13)
   {
      double offset = 1.0;
      if(gridPoint.theXYZ[ossimPlanetGrid::X_IDX] > gridPoint.theXYZ[ossimPlanetGrid::Y_IDX])
      {
         if(gridPoint.theXYZ[ossimPlanetGrid::X_IDX] + gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] >= 1.0)
         {
            xt = 1.0 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            yt = gridPoint.theXYZ[ossimPlanetGrid::X_IDX] - .5;
            offset += 1.0;
         }
         else
         {
            xt = 1.0 - gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            yt = 0.5 - gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            offset+=2;
         }
      }
      else
      {
         if(gridPoint.theXYZ[ossimPlanetGrid::X_IDX] +  gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] < 1.0)
         {
            xt = gridPoint.theXYZ[ossimPlanetGrid::Y_IDX];
            yt = 0.5 - gridPoint.theXYZ[ossimPlanetGrid::X_IDX];
            offset -= 1.0;
         }
         else
         {
            yt = gridPoint.theXYZ[ossimPlanetGrid::Y_IDX] - 0.5;
         }
      }
      if(!ossim::almostEqual((double)yt, (double)0.0))
      {
         xt = ((xt - 0.5)*.5)/yt + .5;
      }
      xt = (xt + offset) *0.25;
      yt *=.5;
      modelPoint.theXYZ[ossimPlanetGrid::X_IDX] = xt*360.0 - 180.0;
      modelPoint.theXYZ[ossimPlanetGrid::Y_IDX] = yt*180.0 - 90;
      double latT = (modelPoint.theXYZ[ossimPlanetGrid::Y_IDX] + 45.0)/45;
      modelPoint.theXYZ[ossimPlanetGrid::Y_IDX] = -thePolarLat - latT*(-90+thePolarLat);
   }
   modelPoint.setZ(gridPoint.z());
}

void ossimPlanetAdjustableCubeGrid::modelToGlobalGrid(const ModelPoint& modelPoint, 
                                              GridPoint& gridPoint)const
{
   if(theCapLocation == LOW_CAP)
   {
      ossimPlanetCubeGrid2::modelToGlobalGrid(modelPoint, gridPoint);
      return;
   }
   double shiftedX = ossim::clamp(modelPoint.x() + 180.0,
                                  0.0, 360.0);
   // south polar region
   if(modelPoint.y() >= thePolarLat)
   {
      gridPoint.setFace(12);
      // map into 45 to 90
      //
      double adjustedModelY = 45 + 45*((modelPoint.y()-thePolarLat)/(.5*thePolarWidth)); 
      ModelPoint adjustedModel(modelPoint.x(),
                               adjustedModelY,
                               modelPoint.z());
      ossimPlanetCubeGrid2::modelToGlobalGrid(adjustedModel, gridPoint);
      gridPoint.setFace(12);
   }
   // south polar region
   else if(modelPoint.y() < -thePolarLat)
   {
      double adjustedModelY = -45.0+((modelPoint.y() + thePolarLat)/(thePolarWidth*.5))*45;
      ModelPoint adjustedModel(modelPoint.x(),
                               adjustedModelY,
                               modelPoint.z());
      ossimPlanetCubeGrid2::modelToGlobalGrid(adjustedModel, gridPoint);
      gridPoint.setFace(13);
   }
   // north band
   else if(modelPoint.y() > 45.0)
   {
      ossim_uint32 face = (ossim_uint32)ossim::clamp((shiftedX / 90),
                                                     0.0, 4.0);
      gridPoint.setX((shiftedX-face*90.0)/90.0);
      gridPoint.setY((modelPoint.y()-45.0)/theUpperEquatorialBandLatDelta);
      gridPoint.setFace(face+4);
   }
   // equatorial band
   else if(modelPoint.y() >= -45.0)
   {
      ossim_uint32 face = ossim::clamp(((modelPoint.x()+180.0) / 90.0),
                                       0.0, 3.0);
      gridPoint.setX((shiftedX-face*90.0)/90.0);
      gridPoint.setY((45.0+modelPoint.y())/90.0);
      gridPoint.setFace(face);
   }
   // south lat band
   else if(modelPoint.y() < -45.0)
   {
      ossim_uint32 face = ossim::clamp((shiftedX / 90),
                                       0.0, 7.0);
      gridPoint.setX((shiftedX-face*90.0)/90.0);
      gridPoint.setY((45.0+modelPoint.y())/theUpperEquatorialBandLatDelta);
      gridPoint.setFace(face+8);
   }
}

void ossimPlanetAdjustableCubeGrid::widthHeightInModelSpace(const ossimPlanetTerrainTileId& tileId, 
                                                    osg::Vec2d& deltaXY)const
{
   if(theCapLocation == LOW_CAP)
   {
      ossimPlanetCubeGrid2::widthHeightInModelSpace(tileId, deltaXY);
      return;
   }
   GridBound b;
   
   bounds(tileId, b);
   
   ossim_float64 multiplierx = 90;
   ossim_float64 multipliery = 90;
   
   if(tileId.face() < 4)
   {
      // purposely left blank;
   }
   else if(tileId.face() < 12)
   {
      
      multiplierx = 90;
      multipliery = theUpperEquatorialBandLatDelta;
   }
   else if((tileId.face() == 12)||
           (tileId.face() == 13))
   {
      multiplierx = (90-thePolarLat)*2.0;
      multipliery = multiplierx;
   }
   deltaXY[0] = multiplierx*b.width();
   deltaXY[1] = multipliery*b.height();
}

bool ossimPlanetAdjustableCubeGrid::findGridBound(ossim_uint32 face,
                                              const ModelPoint& minPoint,
                                              const ModelPoint& maxPoint,
                                              GridBound& bound,
                                              ossim_uint32 numberOfPoints)const
{
   if(theCapLocation == LOW_CAP)
   {
      return ossimPlanetCubeGrid2::findGridBound(face, minPoint, maxPoint, bound, numberOfPoints);
   }
   ossimDrect modelClipBound;
   ossimDrect modelBound(minPoint.x(), maxPoint.y(), maxPoint.x(), minPoint.y(), OSSIM_RIGHT_HANDED);
   ossimDrect modelFaceBound;
   double epsilon = 1e-12;

   if(face < 4)
   {
      double originX = -180.0 + face*90.0;
      modelFaceBound = ossimDrect(originX, 45-epsilon, originX + 90.0-epsilon, -45.0, OSSIM_RIGHT_HANDED);
   }
   else if(face < 8)
   {
      double originX = -180.0 + (face-4)*90.0;
      modelFaceBound = ossimDrect(originX, thePolarLat-epsilon, originX + 90.0, 45, OSSIM_RIGHT_HANDED);
   }
   else if(face < 12)
   {
      double originX = -180.0 + (face-8)*90.0;
      modelFaceBound = ossimDrect(originX, -45.0+epsilon, originX + 90.0, -thePolarLat, OSSIM_RIGHT_HANDED);
   }
   else if(face == 12)
   {
      modelFaceBound = ossimDrect(-180.0, 90.0, 180.0, thePolarLat, OSSIM_RIGHT_HANDED);
   }
   else if(face == 13)
   {
      modelFaceBound = ossimDrect(-180.0, -thePolarLat+epsilon, 180.0, -90.0, OSSIM_RIGHT_HANDED);
   }
   else
   {
      return false;
   }
   bool result = modelFaceBound.intersects(modelBound);
   if(result)
   {
      modelClipBound = modelBound.clipToRect(modelFaceBound);
      //      std::cout << "modelClipBound " << modelClipBound << std::endl;
      // 4 equatorial faces
      if(face < 4)
      {
         bound.theMiny = (45.0 + modelClipBound.ll().y)/90.0;
         bound.theMinx = ((180.0 +modelClipBound.ll().x)/90.0) - face;
         bound.theWidth  = (modelClipBound.ur().x-modelClipBound.ll().x)/90.0;
         bound.theHeight = (modelClipBound.ur().y-modelClipBound.ll().y)/90.0;
      }
      // 4 upper band faces
      else if(face < 8)
      {
         bound.theMiny = (modelClipBound.ll().y - 45)/theUpperEquatorialBandLatDelta;
         bound.theMinx = (180.0 + modelClipBound.ll().x)/90 - (face-4);
         bound.theWidth  = (modelClipBound.ur().x-modelClipBound.ll().x)/90.0;
         bound.theHeight = (modelClipBound.ur().y-modelClipBound.ll().y)/theUpperEquatorialBandLatDelta;
      }
      // 4 lower band faces
      else if(face < 12)
      {
         bound.theMiny = (modelClipBound.ll().y + thePolarLat)/theUpperEquatorialBandLatDelta;
         bound.theMinx = (180.0 + modelClipBound.ll().x)/90.0 - (face-8);
         bound.theWidth  = (modelClipBound.ur().x-modelClipBound.ll().x)/90.0;
         bound.theHeight = (modelClipBound.ur().y-modelClipBound.ll().y)/theUpperEquatorialBandLatDelta;
      }
      // north and south polar faces
      else if((face == 12)||
              (face == 13))
      {
         std::vector<ossimDpt> gridPoints;
         ossim_uint32 idxY = 0;
         ossim_uint32 idxX = 0;
         double deltaModelX = modelClipBound.ur().x-modelClipBound.ll().x;
         double deltaModelY = modelClipBound.ur().y-modelClipBound.ll().y;
         ossimDpt ll = modelClipBound.ll();
         
         ossimPlanetGrid::GridPoint gridPoint;
         for(idxY = 0; idxY <numberOfPoints; ++idxY)
         {
            for(idxX = 0; idxX < numberOfPoints; ++idxX)
            {
               double tx = (double)idxX/(numberOfPoints-1);
               double ty = (double)idxY/(numberOfPoints-1);
               
               ossimPlanetGrid::ModelPoint modelPoint(ll.x + deltaModelX*tx,
                                                      ll.y + deltaModelY*ty);
               modelToGlobalGrid(modelPoint, gridPoint);
               gridPoints.push_back(ossimDpt(gridPoint.x(), gridPoint.y()));
            }
         }
         ossimDrect rect(gridPoints, OSSIM_RIGHT_HANDED);
         bound.theMinx = rect.ll().x;
         bound.theMiny = rect.ll().y;
         bound.theWidth = (rect.ur().x-rect.ll().x);
         bound.theHeight= (rect.ur().y-rect.ll().y);
         bound.theFace = face;
      }
      bound.setFace(face);
   }
  
   //   std::cout << "bound.theMinX = " << bound.theMinx << "\n"
   //   << "bound.theMinY = " << bound.theMiny << "\n";
   return result;
}

ossim_uint32 ossimPlanetAdjustableCubeGrid::numberOfFaces()const
{
   if(theCapLocation == LOW_CAP)
   {
      return ossimPlanetCubeGrid2::numberOfFaces();
   }
   return 14;
}

bool ossimPlanetAdjustableCubeGrid::isPolar(const ossimPlanetTerrainTileId& id)const
{
   if(theCapLocation == LOW_CAP)
   {
      return ossimPlanetCubeGrid2::isPolar(id);
   }
   return ((id.face() == 12)||
           (id.face() == 13));
}

