#ifndef ossimPlanetOssimImage_HEADER
#define ossimPlanetOssimImage_HEADER
#include <iostream>
#include "ossimPlanetExport.h"
#include <osg/Referenced>
#include <ossim/imaging/ossimImageHandler.h>

class ossimPlanetImage;
class OSSIMPLANET_DLL ossimPlanetOssimImage : public osg::Referenced
{
public:
   ossimPlanetOssimImage();
   virtual ~ossimPlanetOssimImage();
   bool loadFile(const std::string& inputFile,
                 ossimPlanetImage& image);

protected:
   ossimRefPtr<ossimImageHandler> theHandler;
};

#endif
