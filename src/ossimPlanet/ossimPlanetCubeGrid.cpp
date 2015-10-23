#include <ossimPlanet/ossimPlanetCubeGrid.h>
#include <ossimPlanet/mkUtils.h>
#include <iostream>

ossim_uint32 ossimPlanetCubeGrid::getNumberOfFaces()const
{
   return 6;
}
void ossimPlanetCubeGrid::getPixelScale(double& dx,
                                        double& dy,
                                        ossimUnitType& pixelScaleUnits,
                                        ossim_uint32 level,
                                        ossim_uint64 /*row*/,
                                        ossim_uint64 /*col*/)const
{
   dx = 90.0*(1.0/(1<<level));
   dy = dx;
   dx/=theTileWidth;
   dy/=theTileHeight;
   pixelScaleUnits = OSSIM_DEGREES;
}

void ossimPlanetCubeGrid::getWidthHeightInDegrees(double& deltaX,
                                                  double& deltaY,
                                                  ossim_uint32 level,
                                                  ossim_uint64 /*row*/,
                                                  ossim_uint64 /*col*/)const
{
   deltaX =  (90.0/((double)(1<<level)));
   deltaY = deltaX;
}

void ossimPlanetCubeGrid::getGridPoint(ossimPlanetGridUtility::GridPoint& gridPoint,
                                       const osg::Vec3d& latLon)const
{
   osg::Vec2d ll((ossim::clamp((double)latLon[ossimPlanetGridUtility::LAT],
                               (double)-90.0, (double)90.0)+90)/180,
                 (ossim::wrap((double)latLon[ossimPlanetGridUtility::LON],
                              (double)-180.0, (double)180.0)+180)/360);
   int face_x = (int)(4 * ll[ossimPlanetGridUtility::LON]);
   int face_y = (int)(2 * ll[ossimPlanetGridUtility::LAT] + 0.5);
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
   gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = 4 * ll[ossimPlanetGridUtility::LON] - face_x;
   gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = 2 * ll[ossimPlanetGridUtility::LAT] - 0.5;
   if(gridPoint.theFace < 4) // equatorial calculations done
   {
//       gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = 1-gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
      return;
   }

   double tmp=0.0;
   if(gridPoint.theFace == 4) // north polar face
   {
      gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = 1.5 - gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
      gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = (2 * (gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] - 0.5) *
                                                                     gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] + 0.5);
      switch(face_x)
      {
         case 0: // bottom
         {
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = 0.5 - gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
            break;
         }
         case 1: // right side, swap and reverse lat
         {
            tmp = gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX];
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = 0.5 + gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = tmp;
            break;
         }
         case 2: // top; reverse lat and lon
         {
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = 1 - gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX];
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = 0.5 + gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
            break;
         }
         case 3: // left side; swap and reverse lon
         {
            tmp = gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX];
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = 0.5 - gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = 1 - tmp;
            break;
         }
      }
   }
   else // south polar face
   {
      gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] += 0.5;
      gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = (2 * (gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] - 0.5) *
                                                                     gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] + 0.5);
      switch(face_x)
      {
         case 0: // left
         {
            tmp = gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX];
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = 0.5 - gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = tmp;
            break;
         }
         case 1: // top
         {
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = 0.5 + gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
            break;
         }
         case 2: // right
         {
            tmp = gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX];
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = 0.5 + gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = 1 - tmp;
            break;
         }
         case 3: // bottom
         {
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = 1 - gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX];
            gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = 0.5 - gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY];
            break;
         }
      }
   }
}


void ossimPlanetCubeGrid::getLatLon(osg::Vec3d& latLon,
                                    const ossimPlanetGridUtility::GridPoint& gridPoint)const
{
   double offset = 0.0;
   osg::Vec2d s(ossim::clamp(gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX], 0.0, 1.0),
                ossim::clamp(gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY], 0.0, 1.0));
   const osg::Vec3d& coord = gridPoint.theGlobalGridPoint; 
   latLon[ossimPlanetGridUtility::HGT] = 0.0;
   if(gridPoint.theFace < 4)
   {
      s[ossimPlanetGridUtility::GRIDX] = (coord[ossimPlanetGridUtility::GRIDX]+gridPoint.theFace)*.25;
      s[ossimPlanetGridUtility::GRIDY] = (coord[ossimPlanetGridUtility::GRIDY] + 0.5)*0.5;
//      latLon[ossimPlanetGridUtility::LON] = s[ossimPlanetGridUtility::GRIDX]*360.0 - 180.0;
//      latLon[ossimPlanetGridUtility::LAT] = 90 - s[ossimPlanetGridUtility::GRIDY]*180.0;
//      return;
   }
   else if(gridPoint.theFace == 4)
   {
      if(coord[ossimPlanetGridUtility::GRIDX] < coord[ossimPlanetGridUtility::GRIDY])
      {
         if(coord[ossimPlanetGridUtility::GRIDX] + coord[ossimPlanetGridUtility::GRIDY] < 1.0)
         {
            s[ossimPlanetGridUtility::GRIDX] = 1.0 - coord[ossimPlanetGridUtility::GRIDY];
            s[ossimPlanetGridUtility::GRIDY] = coord[ossimPlanetGridUtility::GRIDX];
            offset += 3;
         }
         else
         {
            s[ossimPlanetGridUtility::GRIDY] = 1.0 - coord[ossimPlanetGridUtility::GRIDY];
            s[ossimPlanetGridUtility::GRIDX] = 1.0 - coord[ossimPlanetGridUtility::GRIDX];
            offset += 2;
         }
      }
      else if(coord[ossimPlanetGridUtility::GRIDX] + coord[ossimPlanetGridUtility::GRIDY] >= 1.0)
      {
         s[ossimPlanetGridUtility::GRIDX] = coord[ossimPlanetGridUtility::GRIDY];
         s[ossimPlanetGridUtility::GRIDY] = 1.0 - coord[ossimPlanetGridUtility::GRIDX];
         offset += 1.0;
      }
      s[ossimPlanetGridUtility::GRIDX] -= s[ossimPlanetGridUtility::GRIDY];
      if(!ossim::almostEqual(s[ossimPlanetGridUtility::GRIDY], .5))
      {
         s[ossimPlanetGridUtility::GRIDX] *= .5/(.5 - s[ossimPlanetGridUtility::GRIDY]);
      }
      s[ossimPlanetGridUtility::GRIDX] = (s[ossimPlanetGridUtility::GRIDX] + offset)*0.25;
      s[ossimPlanetGridUtility::GRIDY] = (s[ossimPlanetGridUtility::GRIDY] + 1.5)*.5;
   }
   else if(gridPoint.theFace == 5)
   {
      offset = 1.0;
      if(coord[ossimPlanetGridUtility::GRIDX] > coord[ossimPlanetGridUtility::GRIDY])
      {
         if(coord[ossimPlanetGridUtility::GRIDX] + coord[ossimPlanetGridUtility::GRIDY] >= 1.0)
         {
            s[ossimPlanetGridUtility::GRIDX] = 1.0 - coord[ossimPlanetGridUtility::GRIDY];
            s[ossimPlanetGridUtility::GRIDY] = coord[ossimPlanetGridUtility::GRIDX] - .5;
            offset += 1.0;
         }
         else
         {
            s[ossimPlanetGridUtility::GRIDX] = 1.0 - coord[ossimPlanetGridUtility::GRIDX];
            s[ossimPlanetGridUtility::GRIDY] = 0.5 - coord[ossimPlanetGridUtility::GRIDY];
            offset+=2;
         }
      }
      else
      {
         if(coord[ossimPlanetGridUtility::GRIDX] + coord[ossimPlanetGridUtility::GRIDY] < 1.0)
         {
            s[ossimPlanetGridUtility::GRIDX] = coord[ossimPlanetGridUtility::GRIDY];
            s[ossimPlanetGridUtility::GRIDY] = 0.5 - coord[ossimPlanetGridUtility::GRIDX];
            offset -= 1.0;
         }
         else
         {
            s[ossimPlanetGridUtility::GRIDY] = coord[ossimPlanetGridUtility::GRIDY] - 0.5;
         }
      }
      if(!ossim::almostEqual((double)s[ossimPlanetGridUtility::GRIDY], (double)0.0))
      {
         s[ossimPlanetGridUtility::GRIDX] = (s[ossimPlanetGridUtility::GRIDX] - 0.5)*.5/s[ossimPlanetGridUtility::GRIDY] + .5;
      }
      s[ossimPlanetGridUtility::GRIDX] = (s[ossimPlanetGridUtility::GRIDX] + offset) *0.25;
      s[ossimPlanetGridUtility::GRIDY] *=.5;
   }
   latLon[ossimPlanetGridUtility::LON] = s[ossimPlanetGridUtility::GRIDX]*360.0 - 180.0;
   latLon[ossimPlanetGridUtility::LAT] = s[ossimPlanetGridUtility::GRIDY]*180.0 - 90.0;
}
