#ifndef ossimPlanetWmsImageLayer_HEADER
#define ossimPlanetWmsImageLayer_HEADER
#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossimPlanet/ossimPlanetWmsClient.h>
class OSSIMPLANET_DLL ossimPlanetWmsImageLayer : public ossimPlanetTextureLayer
{
public:
   ossimPlanetWmsImageLayer();
   ossimPlanetWmsImageLayer(const ossimPlanetWmsImageLayer& src);
   virtual ossimPlanetTextureLayer* dup()const;
   virtual ossimPlanetTextureLayer* dupType()const;
   virtual ossimString getClassName()const;
   virtual ossimPlanetTextureLayerStateCode updateExtents();
   virtual void updateStats()const;
   virtual void resetStats()const;

   virtual bool hasTexture(ossim_uint32 width,
                           ossim_uint32 height,
                           const ossimPlanetTerrainTileId& tileId,
                           const ossimPlanetGrid& grid);
   
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 width,
                                                     ossim_uint32 height,
                                                     const ossimPlanetTerrainTileId& tileId,
                                                     const ossimPlanetGrid& theGrid,
                                                     ossim_int32 padding=0);
  virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 level,
                                                     ossim_uint64 row,
                                                     ossim_uint64 col,
                                                     const ossimPlanetGridUtility& utility);
   
   void setServer(const std::string& serverString);
   const std::string& getServer()const;
   const std::string& getImageType()const;
   void setImageType(const std::string& imageType);

   void setCacheDirectory(const ossimFilename& cacheDir);
   const ossimFilename& getCacheDirectory()const;
   const ossimFilename& getCompleteCacheDirectory()const;

   void setRawCapabilities(const ossimString& rawCapabilities);
   const ossimString& getRawCapabilities()const;

   void setCapabilitiesUrl(const std::string& url);
   const std::string& getCapabilitiesUrl()const;

   void setBackgroundColor(const ossimString& color);
   const ossimString& getBackgroundColor()const;

   void setTransparentFlag(bool flag);
   bool getTransparentFlag()const;

   void setAdditionalParameters(const ossimString& additionalParameters);
   const ossimString& getAdditionalParameters()const;

   void setAutoCreateCacheFlag(bool value);
   bool getAutoCreateCacheFlag()const;

   void clearDiskCache();

   virtual ossimRefPtr<ossimXmlNode> saveXml(bool recurse=true)const;
   bool loadXml(ossimRefPtr<ossimXmlNode> node);

   void setProxyHost(const std::string& host)
   {
      theProxyHost = host;
      theWmsClient->setProxyHost(host);
   }
   void setProxyPort(const std::string& port)
   {
      theProxyPort = port;
      theWmsClient->setProxyPort(port);
   }
   void setProxyUser(const std::string& user)
   {
      theProxyUser = user;
      theWmsClient->setProxyUser(user);
   }
   void setProxyPassword(const std::string& password)
   {
      theProxyPassword = password;
      theWmsClient->setProxyPassword(password);
   }
   const std::string& proxyHost()const
   {
      return theProxyHost;
   }
   const std::string& proxyPort()const
   {
      return theProxyPort;
   }
   const std::string& proxyUser()const
   {
      return theProxyUser;
   }
   const std::string& proxyPassword()const
   {
      return theProxyPassword;
   }
   
protected:
   void adjustServerString();
   
   ossimFilename              theCacheDirectory;
   ossimFilename              theCompleteCacheDirectory;
   ossimString                theServer;
   ossimString                theAdjustedServer;
   ossimString                theImageType;
   ossimString                theRawCapabilities;
   ossimString                theCapabilitiesUrl;
   ossimString                theBackgroundColor;
   bool                       theTransparentFlag;
   ossimString                theAdditionalParameters;
   std::string theProxyHost;
   std::string theProxyPort;
   std::string theProxyUser;
   std::string theProxyPassword;
   mutable ossimPlanetReentrantMutex theWmsArchiveMutex;
   std::vector<ossimString>   theLayers;
   std::vector<ossimString>   theStyles;
   bool                       theAutoCreateCacheFlag;
   ossimRefPtr<ossimImageFileWriter> theWriter;
   osg::ref_ptr<ossimPlanetWmsClient> theWmsClient;
};

#endif
