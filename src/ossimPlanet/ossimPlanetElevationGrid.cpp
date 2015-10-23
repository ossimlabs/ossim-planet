#include <ossimPlanet/ossimPlanetElevationGrid.h>
#include <iostream>
#include <ossim/base/ossimCommon.h>
ossimPlanetElevationGrid::ossimPlanetElevationGrid(ossim_uint32 w,
                                               ossim_uint32 h)
{
   theGrid = 0;
   theWidth = 0;
   theHeight = 0;
   resize(w,h);
}

ossimPlanetElevationGrid::~ossimPlanetElevationGrid()
{
   if(theGrid)
   {
      delete [] theGrid;
      theGrid = 0;
   }
}

void ossimPlanetElevationGrid::resize(ossim_uint32 w,
                                    ossim_uint32 h)
{
   if(theGrid)
   {
      delete [] theGrid;
      theGrid = 0;
   }
   theGrid = new float[w*h];
   
   theWidth  = w;
   theHeight = h;
}

osg::ref_ptr<ossimPlanetElevationGrid>  ossimPlanetElevationGrid::scaleBy2Nearest()const
{
   // note our elevation has one extra row and 1 extra column so the
   // scaled should be 2*(height-1) + 1 and 2*(width-1) + 1 for
   // the new dimensions
   //
//    ossim_uint32 outputWidth  = ((theWidth-1) <<1) + 1;
//    ossim_uint32 outputHeight = ((theHeight-1)<<1) + 1;
   ossim_uint32 outputWidth  = theWidth<<1;
   ossim_uint32 outputHeight = theHeight<<1;
   osg::ref_ptr<ossimPlanetElevationGrid> result = new ossimPlanetElevationGrid(outputWidth,
                                                                            outputHeight);
   ossim_uint32 y = 0;
   ossim_uint32 x = 0;
   const float* inputData = theGrid;
   float* outputData = result->theGrid;
   ossim_uint32 inputOffsetY = 0;
   ossim_uint32 inputOffset = 0;
   for(y = 0; y < outputHeight; ++y)
   {
      inputOffsetY = (y>>1)*theWidth;
      for(x = 0; x < outputWidth; ++x)
      {
         inputOffset = inputOffsetY + (x>>1);
         *outputData= inputData[inputOffset];
         ++outputData;
      }
   }
   return result;
}

osg::ref_ptr<ossimPlanetElevationGrid>  ossimPlanetElevationGrid::scaleBy2Bilinear()const
{
//    ossim_uint32 outputWidth  = theWidth<<1;
//    ossim_uint32 outputHeight = theHeight<<1;
   ossim_uint32 outputWidth  = ( ((theWidth-1)<<1) + 1);
   ossim_uint32 outputHeight = ( ((theHeight-1)<<1) + 1);
   osg::ref_ptr<ossimPlanetElevationGrid> result = new ossimPlanetElevationGrid(outputWidth,
                                                                            outputHeight);
   ossim_uint32 y = 0;
   ossim_uint32 x = 0;
   const float* inputData = theGrid;
   float* outputData = result->theGrid;
   ossim_uint32 inputOffsetY = 0;
   ossim_uint32 inputOffset = 0;
   ossim_uint8 testLocation=0;
   for(y = 0; y < outputHeight; ++y)
   {
      inputOffsetY = (y>>1)*theWidth;
      for(x = 0; x < outputWidth; ++x)
      {
         inputOffset = inputOffsetY + (x>>1);
         testLocation = ((y&1) << 1) | (x&1);
         switch(testLocation)
         {
            case 0: // upper left 
            {
               // just copy the point
               *outputData = inputData[inputOffset];
               
               break;
            }
            case 1: // upper right
            {
               // do horizontal t for x edge
               *outputData = (inputData[inputOffset] +
                              inputData[inputOffset+1])*.5;
               break;
            }
            case 2: // lower left
            {
               // do vertical t for y edge
               *outputData = (inputData[inputOffset] +
                              inputData[inputOffset+theWidth])*.5;
               
               break;
            }
            case 3: // lower right
            {
               // do both horizontal and vertical using all 4 points
               *outputData = (inputData[inputOffset] +
                              inputData[inputOffset+theWidth]+
                              inputData[inputOffset+theWidth+1]+
                              inputData[inputOffset+1]
                              )*.25;
               break;
            }
         }
         ++outputData;
      }
   }
   return result;
}

void ossimPlanetElevationGrid::copyGrid(ossim_uint32 ulx,
                                      ossim_uint32 uly,
                                      osg::ref_ptr<ossimPlanetElevationGrid> output)const
{
   if(!output.valid()) return;
   ossim_uint32 outputWidth  = output->getWidth();
   ossim_uint32 outputHeight = output->getHeight();
   ossim_float32 *outputBuf  = output->theGrid;
   const ossim_float32 *inputBuf  = theGrid;
   ossim_uint32 leftOffset = 0;
   ossim_uint32 x = 0;
   ossim_uint32 y = 0;

   leftOffset = uly*theWidth + ulx;
   for(y=0;y < outputHeight;++y)
   {
      for(x=0;x < outputWidth; ++x)
      {
         *outputBuf = inputBuf[leftOffset+x];
         ++outputBuf;
      }
      leftOffset += theWidth;
   }
}

bool ossimPlanetElevationGrid::isEqualTo(osg::ref_ptr<ossimPlanetElevationGrid> grid)const
{
   if(!grid.valid()) return false;
   if(grid.get() == this) return true;
   if( (grid->theWidth!=theWidth)||
       (grid->theHeight != theHeight) ) return false;

   ossim_uint32 idx = 0;
   ossim_uint32 upperBound = theWidth*theHeight;

   for(idx = 0; idx < upperBound;++idx)
   {
      if(theGrid[idx] != grid->theGrid[idx])
      {
         return false;
      }
   }

   return true;
}
