#ifndef ossimPlanetElevationGrid_HEADER
#define ossimPlanetElevationGrid_HEADER
#include <vector>
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <ossim/base/ossimConstants.h>
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetElevationGrid : public osg::Referenced
{
public:
   ossimPlanetElevationGrid(ossim_uint32 w=1,
                          ossim_uint32 h=1);

   float& operator[](ossim_uint32 idx)
   {
      return theGrid[idx];
   }
   const float& operator[](ossim_uint32 idx)const
   {
      return theGrid[idx];
   }
   float* row(ossim_uint32 rowIdx)
   {
      return &theGrid[rowIdx*theWidth];
   }
   ossim_uint32 getWidth()const
   {
      return theWidth;
   }
   ossim_uint32 getHeight()const
   {
      return theHeight;
   }
   
   void resize(ossim_uint32 w,
               ossim_uint32 h);

   const ossim_float32* data()const
   {
      return theGrid;
   }
   ossim_float32* data()
   {
      return theGrid;
   }
   osg::ref_ptr<ossimPlanetElevationGrid>  scaleBy2Nearest()const;
   osg::ref_ptr<ossimPlanetElevationGrid>  scaleBy2Bilinear()const;
   void copyGrid(ossim_uint32 ulx,
                 ossim_uint32 uly,
                 osg::ref_ptr<ossimPlanetElevationGrid> output)const;
   bool isEqualTo(osg::ref_ptr<ossimPlanetElevationGrid> grid)const;
protected:
   virtual ~ossimPlanetElevationGrid();
   ossim_uint32 theWidth;
   ossim_uint32 theHeight;
   ossim_float32* theGrid;
};

#endif
