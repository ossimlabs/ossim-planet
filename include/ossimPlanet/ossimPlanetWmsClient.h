#ifndef OSSIMPLANET_WITHOUT_WMS
#ifndef ossimPlanetWmsClient_HEADER
#define ossimPlanetWmsClient_HEADER
#include <vector>
#include <string>
#include "ossimPlanetExport.h"
#include <ossim/base/ossimFilename.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/base/ossimConstants.h>
#include <wms/wmsClient.h>
#include <wms/wmsRefPtr.h>
#include <osg/ref_ptr>
#include <osg/Referenced>
#include <ossimPlanet/ossimPlanetOssimImage.h>

class wmsClient;
class ossimPlanetImage;
namespace osg
{
   class Image;
}

class OSSIMPLANET_DLL ossimPlanetWmsClient : public osg::Referenced
{
 public:
   ossimPlanetWmsClient(const std::string& server=std::string(""),
                      const std::string& path=std::string(""));
   ossimPlanetWmsClient(const ossimPlanetWmsClient& src);

   virtual ~ossimPlanetWmsClient();

   void setServer(const std::string& server);
   void setPath(const std::string& path);

   virtual osg::ref_ptr<ossimPlanetImage> createImage(ossim_uint32 width,
                                                    ossim_uint32 height,
                                                    const double& minLat,
                                                    const double& minLon,
                                                    const double& maxLat,
                                                    const double& maxLon,
                                                    const std::string& filename = std::string(""));

   virtual osg::ref_ptr<ossimPlanetImage> readLocalImage(const std::string& filename)const;
   
   void setImageType(const std::string& imageType);
   std::string getImageType()const;
   std::string getServer()const;
   std::string getPath()const;
   void setVersion(const ossimString& version);
   void setAdditionalParameters(const ossimString& additionalParameters);
   void setTransparentFlag(bool flag);
   void setBackgroundColor(const std::string& color);
   void setProxyHost(const std::string& host)
   {
      theWmsClient->setProxyHost(host);
   }
   void setProxyPort(const std::string& port)
   {
      theWmsClient->setProxyPort(port);
   }
   void setProxyUser(const std::string& userName)
   {
      theWmsClient->setProxyUser(userName);
   }
   void setProxyPassword(const std::string& password)
   {
      theWmsClient->setProxyPassword(password);
   }
   const std::string& proxyHost()const
   {
      return theWmsClient->proxyHost();
   }
   const std::string& proxyPort()const
   {
      return theWmsClient->proxyPort();
   }
   const std::string& proxyUser()const
   {
      return theWmsClient->proxyUser();
   }
   const std::string& proxyPassword()const
   {
      return theWmsClient->proxyPassword();
   }
protected:
   wmsRefPtr<wmsClient> theWmsClient; 
   std::string theServer;
   std::string thePath;
   std::string theImageFormats;
   std::string theImageType;
   std::string theVersion;
   std::string theAdditionalParameters;
   std::string theBackgroundColor;
   bool theTransparentFlag;
   osg::ref_ptr<ossimPlanetOssimImage> theImageReader;
};

#endif
#endif
