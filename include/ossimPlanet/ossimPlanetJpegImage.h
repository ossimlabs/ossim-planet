#ifndef ossimPlanetJpegImage_HEADER
#define ossimPlanetJpegImage_HEADER
#include <iostream>
#include "ossimPlanetExport.h"

class ossimPlanetImage;
class OSSIMPLANET_DLL ossimPlanetJpegImage
{
public:
   ossimPlanetJpegImage();
   virtual ~ossimPlanetJpegImage();
   bool loadFile(std::string& inputFile,
                 ossimPlanetImage& image);
   bool saveFile(std::string& outputFile,
                 ossimPlanetImage& image);
                 
   bool loadFile(std::istream& inputStream,
                 ossimPlanetImage& image);
   bool saveFile( std::ostream& stream,
                  ossimPlanetImage &image,
                  bool verbose=false);
};

#endif
