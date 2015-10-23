#include <ossimPlanet/ossimPlanetPlaneGrid.h>
#include <ossim/base/ossimCommon.h>

ossim_uint32 ossimPlanetPlaneGrid::getNumberOfFaces()const
{
   return 2;// east and west hemispheres
}

void ossimPlanetPlaneGrid::getPixelScale(double& dx,
                                         double& dy,
                                         ossimUnitType& pixelScaleUnits,
                                         ossim_uint32 level,
                                         ossim_uint64 /*row*/,
                                         ossim_uint64 /*col*/)const
{
   dx = 180.0*(1.0/(1<<level));
   dy = dx;
   dx/=theTileWidth;
   dy/=theTileHeight;
   pixelScaleUnits = OSSIM_DEGREES;
}

void ossimPlanetPlaneGrid::getGridPoint(ossimPlanetGridUtility::GridPoint& gridPoint,
                                        const osg::Vec3d& latLon)const
{
   osg::Vec2d ll((ossim::clamp((double)latLon[ossimPlanetGridUtility::LAT],
                               (double)-90.0, (double)90.0)+90)/180,
                 (ossim::wrap((double)latLon[ossimPlanetGridUtility::LON],
                              (double)-180.0, (double)180.0)+180)/360);

   gridPoint.theFace = ll[ossimPlanetGridUtility::LON] < 0.0?0:1;
   
   gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = ll[ossimPlanetGridUtility::LAT];
   if(gridPoint.theFace == 0)
   {
      gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = ll[ossimPlanetGridUtility::LON]/.5;
   }
   else
   {
      gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = (ll[ossimPlanetGridUtility::LON]-.5)/.5;
   }

   gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX] = ossim::clamp(gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDX],
                                                                              0.0, 1.0);
   gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY] = ossim::clamp(gridPoint.theGlobalGridPoint[ossimPlanetGridUtility::GRIDY],
                                                                              0.0, 1.0);
}


void ossimPlanetPlaneGrid::getLatLon(osg::Vec3d& latLon,
                                     const ossimPlanetGridUtility::GridPoint& gridPoint)const
{
   latLon[2] = 0.0;
   switch(gridPoint.theFace)
   {
      case 0: // east hemisphere
      {
         latLon[0] = 90.0 - (180.0)*gridPoint.theGlobalGridPoint[1];
         latLon[1] = gridPoint.theGlobalGridPoint[0]*180.0 - 180.0;
         break;
      }
      case 1: // west hemisphere
      {
         latLon[0] = 90.0 - (180.0)*gridPoint.theGlobalGridPoint[1];
         latLon[1] = gridPoint.theGlobalGridPoint[0]*180.0;
         break;
      }
   }
}




