#ifndef OSGPLANET_WITHOUT_WMS
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <ossimPlanet/ossimPlanetWmsClient.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/ossimPlanetJpegImage.h>
#include <ossimPlanet/ossimPlanetOssimImage.h>
#include <wms/wmsCurlMemoryStream.h>
#include <wms/wmsMemoryStream.h>
#include <wms/wmsClient.h>
#include <ossim/imaging/ossimJpegTileSource.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/base/ossimIrect.h>
#include <osgDB/ReadFile>
#include <osgDB/Input>
#include <osgDB/Registry>
#include <ossimPlanet/ossimPlanetJpegImage.h>

ossimPlanetWmsClient::ossimPlanetWmsClient(const std::string& server,
                                       const std::string& path)
      :osg::Referenced(),
       theServer(server),
       thePath(path),
       theImageType("image/jpeg"),
       theVersion("1.1.1")
{
   theWmsClient = new wmsClient;
   theImageReader = new ossimPlanetOssimImage;
}
ossimPlanetWmsClient::ossimPlanetWmsClient(const ossimPlanetWmsClient& src)
	:osg::Referenced(),
        theServer(src.theServer),
	thePath(src.thePath),
	theImageFormats(src.theImageFormats),
	theImageType(src.theImageType),
	theVersion(src.theVersion),
        theAdditionalParameters(src.theAdditionalParameters),
	theBackgroundColor(src.theBackgroundColor),
	theTransparentFlag(src.theTransparentFlag)
{

}

ossimPlanetWmsClient::~ossimPlanetWmsClient()
{
}

void ossimPlanetWmsClient::setServer(const std::string& server)
{
   theServer = server;
}

void ossimPlanetWmsClient::setPath(const std::string& path)
{
   thePath = path;
}

void ossimPlanetWmsClient::setAdditionalParameters(const ossimString& additionalParameters)
{
   theAdditionalParameters = additionalParameters.string();
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetWmsClient::createImage(ossim_uint32 width,
                                                             ossim_uint32 height,
                                                             const double& minLat,
                                                             const double& minLon,
                                                             const double& maxLat,
                                                             const double& maxLon,
                                                             const std::string& filename)
{
   
   
   osg::ref_ptr<ossimPlanetImage> planetImage = NULL;
   ossimFilename file = filename.c_str();
   wmsRefPtr<wmsMemoryStream> s;
   bool needToRetrieve = true;

   if(file!= "")
   {
      planetImage = readLocalImage(file);
      if(planetImage.valid())
      {
         return planetImage;
      }
   }
   if(needToRetrieve)
   {
      wmsUrl tempUrl = theWmsClient->getMapUrl(theServer,
                                               width,
                                               height,
                                               minLat,
                                               minLon,
                                               maxLat,
                                               maxLon,
                                               theImageType,
                                               theVersion);
//       std::cout << "ossimPlanetWmsClient::createImage: Server = " << tempUrl << std::endl;

      std::string tempStr = tempUrl.url();
      if(theBackgroundColor != "")
      {
         tempStr += ("&BGCOLOR="+theBackgroundColor);
         if(theTransparentFlag)
         {
            tempStr += ("&TRANSPARENT=TRUE");
         }
         else
         {
            tempStr += ("&TRANSPARENT=FALSE");
         }
         if(theAdditionalParameters != "")
         {
            tempStr += "&"+theAdditionalParameters;
         }
      }
//       std::cout << "ossimPlanetWmsClient::createImage: GETTING URL = " << tempStr << std::endl;
      tempUrl = tempStr;
      if(theWmsClient->get(tempUrl))
      {
         wmsRefPtr<wmsMemoryStream> memS = theWmsClient->getStream();
         if(memS->getBufferSize()>0)
         {
            if(file!= "")
            {
               ofstream out(file.c_str(),
                            ios::out|ios::binary);
               if(out.good())
               {
                  out.write((char*)(memS->getBuffer()),
                            memS->getBufferSize());
                  out.close();
                  planetImage = new ossimPlanetImage;
                  ossimPlanetOssimImage inputImage;
                  if(!inputImage.loadFile(file, *planetImage))
                  {
                     planetImage = 0;
                  }
               }
            }
            else
            {
               planetImage = new ossimPlanetImage;
               if(planetImage.valid())
               {
                  ossimPlanetJpegImage jpegImage;
                  if(!jpegImage.loadFile(*memS, *planetImage))
                  {
                     planetImage = 0;
                  }
               }
               else
               {
                  std::cerr << "ERROR: ossimPlanetWmsClient::createImage allocating planetImage" << std::endl;
                  planetImage = 0;
               }
            }
         }
         else
         {
            std::cerr << "ERROR: ossimPlanetWmsClient::createImage allocating osgMemoryStream" << std::endl;
         }
      }
   }
   
   return planetImage;
}


osg::ref_ptr<ossimPlanetImage> ossimPlanetWmsClient::readLocalImage(const std::string& filename)const
{
   ossimFilename file = filename.c_str();
   osg::ref_ptr<ossimPlanetImage> result;
   if(file.exists())
   {
      result = new ossimPlanetImage;
      bool readImage = false;
//       ossimPlanetJpegImage jpegImage;
//       if(jpegImage.loadFile(file, *result))
//       {
//          readImage = true;
//       }
      if(!readImage)
      {
//         ossimPlanetOssimImage inputImage;
         if(!theImageReader->loadFile(file, *result))
         {
            result = 0;
         }
      }
   }

   return result;
}

void ossimPlanetWmsClient::setImageType(const std::string& imageType)
{
   if(imageType == "")
   {
      theImageType = "image/jpeg";
   }
   else
   {
      theImageType = imageType;
   }
}

// void ossimPlanetWmsClient::setCacheDir(const std::string& cacheDir)
// {
//    theCacheDir = cacheDir;
// }

std::string ossimPlanetWmsClient::getImageType()const
{
   return theImageType;
}
   
std::string ossimPlanetWmsClient::getServer()const
{
	return theServer;
}

std::string ossimPlanetWmsClient::getPath()const
{
	return thePath;
}

void ossimPlanetWmsClient::setTransparentFlag(bool flag)
{
   theTransparentFlag = flag;
}

void ossimPlanetWmsClient::setBackgroundColor(const std::string& color)
{
   theBackgroundColor = color;
}


// std::string ossimPlanetWmsClient::getCacheDir()const
// {
//    return theCacheDir;
// }

#endif
