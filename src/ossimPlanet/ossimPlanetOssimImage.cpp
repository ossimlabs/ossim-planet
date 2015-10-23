#include <ossimPlanet/ossimPlanetOssimImage.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageHandler.h>

ossimPlanetOssimImage::ossimPlanetOssimImage()
{
}
ossimPlanetOssimImage::~ossimPlanetOssimImage()
{
}

bool ossimPlanetOssimImage::loadFile(const std::string& inputFile,
                                   ossimPlanetImage& image)
{
   if(theHandler.valid())
   {
      theHandler->close();
      if(!theHandler->open(ossimFilename(inputFile)))
      {
         theHandler = 0;
      }
   }
   ossimRefPtr<ossimImageData> data;
   if(!theHandler.valid())
   {
       theHandler = ossimImageHandlerRegistry::instance()->open(ossimFilename(inputFile.c_str()));
   }
   if(theHandler.valid())
   {
      data = theHandler->getTile(theHandler->getBoundingRect());
      if(data.valid())
      {
         image.fromOssimImage(data);
         return true;
      }
   }

   return false;
}
