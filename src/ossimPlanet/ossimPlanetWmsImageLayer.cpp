#include <ossimPlanet/ossimPlanetWmsImageLayer.h>
#include <sstream>
#include <ossimPlanet/ossimPlanetWmsClient.h>
#include <ossim/imaging/ossimMemoryImageSource.h>
#include <ossim/imaging/ossimJpegWriter.h>
#include <wms/wmsCapabilitiesParser.h>
#include <wms/wmsCapabilitiesRoot.h>
#include <wms/wmsCapabilitiesState.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>

ossimPlanetWmsImageLayer::ossimPlanetWmsImageLayer()
   :ossimPlanetTextureLayer(),
    theAutoCreateCacheFlag(true)

{
   theImageType = "image/jpeg";
   
   theTransparentColorFlag = true;
   theBackgroundColor = "0x000000";
   theTransparentFlag = false;
   theWmsClient = new ossimPlanetWmsClient;
}

ossimPlanetWmsImageLayer::ossimPlanetWmsImageLayer(const ossimPlanetWmsImageLayer& src)
   :ossimPlanetTextureLayer(src),
    theImageType(src.theImageType),
    theRawCapabilities(src.theRawCapabilities),
    theCapabilitiesUrl(src.theCapabilitiesUrl),
    theBackgroundColor(src.theBackgroundColor),
    theTransparentFlag(src.theTransparentFlag),
    theAdditionalParameters(src.theAdditionalParameters),
    theProxyHost(src.theProxyHost),
    theProxyPort(src.theProxyPort),
    theProxyUser(src.theProxyUser),
    theProxyPassword(src.theProxyPassword),
    theAutoCreateCacheFlag(src.theAutoCreateCacheFlag)
{
   if(src.theWmsClient.valid())
   {
      theWmsClient = new ossimPlanetWmsClient(*(src.theWmsClient.get()));
      theWmsClient->setProxyHost(theProxyHost);
      theWmsClient->setProxyPort(theProxyPort);
      theWmsClient->setProxyUser(theProxyUser);
      theWmsClient->setProxyPassword(theProxyPassword);
   }
   else
   {
      theWmsClient = new ossimPlanetWmsClient;
   }
   theWmsClient->setProxyHost(theProxyHost);
   theWmsClient->setProxyPort(theProxyPort);
   theWmsClient->setProxyUser(theProxyUser);
   theWmsClient->setProxyPassword(theProxyPassword);
}

ossimPlanetTextureLayer* ossimPlanetWmsImageLayer::dup()const
{
   return new ossimPlanetWmsImageLayer(*this);
}

ossimPlanetTextureLayer* ossimPlanetWmsImageLayer::dupType()const
{
   return new ossimPlanetWmsImageLayer;
}

ossimString ossimPlanetWmsImageLayer::getClassName()const
{
   return "ossimPlanetWmsImageLayer";
}

ossimPlanetTextureLayerStateCode ossimPlanetWmsImageLayer::updateExtents()
{
   theStateCode = ossimPlanetTextureLayer_VALID;
   if(!theDirtyExtentsFlag) return theStateCode;

   ossimString name = "LAYERS";
   if(!theServer.contains(name))
   {
      name = "layers";
      if(!theServer.contains(name))
      {
         return theStateCode;
      }
   }
#if 1  
   bool extentsSet = false;
   double minLat, maxLat, minLon, maxLon;
   ossimString after = theServer.after(name);
   after = after.after("=");
   after = after.trim();
   double maxScale = 0.0;
   double minScale = 0.0;
   if(after!= "")
   {
      wmsRefPtr<wmsCapabilitiesParser> parser = new wmsCapabilitiesParser;
      std::stringstream in(theRawCapabilities);
      wmsRefPtr<wmsCapabilitiesRoot> root = parser->parse(in);

      if(root.valid())
      {
         after = after.before("&");
         std::vector<ossimString> splitList;
         
         after.split(splitList, ",");
         ossim_uint32 idx = 0;
         for(idx = 0; idx < splitList.size();++idx)
         {
            wmsRefPtr<wmsCapabilitiesState> node = root->getNodeGivenName(splitList[idx]);
            if(node.valid())
            {
               node->getLatLonBoundingBox(minLat, minLon, maxLat, maxLon);
               ossimString tempMaxScale = node->maxScaleHint();
               ossimString tempMinScale = node->minScaleHint();
               double scale = tempMaxScale.toDouble();

               if(!tempMaxScale.empty())
               {
                  if(scale > maxScale)
                  {
                     maxScale = scale;
                  }
               }
               if(!tempMinScale.empty())
               {
                  minScale = tempMinScale.toDouble();
               }
               if(!extentsSet)
               {
                  theExtents->setMinMaxLatLon(minLat, minLon, maxLat, maxLon);
                  extentsSet = true;
               }
               else
               {
                  theExtents->combineMinMaxLatLon(minLat, minLon, maxLat, maxLon);
               }
            }
         }
         if(maxScale > 0.0)
         {
            theExtents->setMinMaxScale(minScale, maxScale);
         }
      }
   }
#endif
   theDirtyExtentsFlag = false;
   
   return theStateCode;
}


void ossimPlanetWmsImageLayer::updateStats()const
{
   theStats->setTotalTextureSize(0);
}

void ossimPlanetWmsImageLayer::resetStats()const
{
   updateStats();
   theStats->setBytesTransferred(0);
}

bool ossimPlanetWmsImageLayer::hasTexture(ossim_uint32 /* width */,
                                          ossim_uint32 height,
                                          const ossimPlanetTerrainTileId& tileId,
                                          const ossimPlanetGrid& grid)
{
   ossimPlanetGrid::GridBound bound;
   if(grid.findGridBound(tileId.face(),
                         ossimPlanetGrid::ModelPoint(theExtents->getMinLon(), theExtents->getMinLat()),
                         ossimPlanetGrid::ModelPoint(theExtents->getMaxLon(), theExtents->getMaxLat()),
                         bound))
   {
      ossimPlanetGrid::GridBound tileBound;
      grid.bounds(tileId,tileBound);
      if(!tileBound.toDrect().intersects(bound.toDrect()))
      {
         return false;
      }
   }
   osg::Vec2d deltaXY;
   grid.widthHeightInModelSpace(tileId, deltaXY);
   double deltaLat    = (deltaXY[1])/(double)(height);
   //double deltaLon    = (deltaXY[0])/(double)(width);
   
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;
   ossimPlanetGrid::ModelPoint minLatLon, maxLatLon;
   grid.modelBound(tileId, minLatLon, maxLatLon);
   if(gsd.y <= theExtents->getMaxScale())
   {
      return true;
   }
//   if((gsd.y >= theExtents->getMinScale()) &&
//      (gsd.y <= theExtents->getMaxScale()))
//   {
//      return true;
//   }
   
   return false;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetWmsImageLayer::getTexture(ossim_uint32 width,
                                                                    ossim_uint32 height,
                                                                    const ossimPlanetTerrainTileId& tileId,
                                                                    const ossimPlanetGrid& grid,
                                                                    ossim_int32 /*padding*/)
{
   if(!getEnableFlag())
   {
      return 0;
   }
   if(theDirtyExtentsFlag)
   {
      updateExtents();
   }
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theWmsArchiveMutex);
   if(theServer == "")
   {
      return 0;
   }
   ossimPlanetGrid::GridBound bound;
   ossimPlanetGrid::GridBound tileBound;
   if(grid.findGridBound(tileId.face(),
                         ossimPlanetGrid::ModelPoint(theExtents->getMinLon(), theExtents->getMinLat()),
                         ossimPlanetGrid::ModelPoint(theExtents->getMaxLon(), theExtents->getMaxLat()),
                         bound))
   {
      grid.bounds(tileId,tileBound);
      if(!tileBound.toDrect().intersects(bound.toDrect()))
      {
         return 0;
      }
   }
   osg::Vec2d deltaXY;
   grid.widthHeightInModelSpace(tileId, deltaXY);
   double deltaLat    = (deltaXY[1])/(double)(height);
   //double deltaLon    = (deltaXY[0])/(double)(width);
   //std::cout << "Delta lat = "<<deltaLat << std::endl;
   //std::cout << "Delta lon = "<<deltaLon << std::endl;
   
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;
   ossimPlanetGrid::ModelPoint minLatLon, maxLatLon;
   grid.modelBound(tileId, minLatLon, maxLatLon);
   
   if(gsd.y > theExtents->getMaxScale())//!//theExtents->intersectsScale(gsd.y-FLT_EPSILON,
      //                            gsd.y+FLT_EPSILON))//gsd.y <= theExtents.theMaxGsd)
   {
      return 0;
   }
   //std::cout << "Model bound = " << minLatLon << " , " << maxLatLon << std::endl;
   osg::ref_ptr<ossimPlanetImage> texture = 0;
   std::stringstream file;
   ossimFilename filename;
   //   ossimPlanetWmsClient client(theAdjustedServer);
   //   client.setImageType(theImageType);
   
   // bool hasCacheDir = false;
   if((theCompleteCacheDirectory!="")&&
      (!theCompleteCacheDirectory.exists())&&
      (theAutoCreateCacheFlag))
   {
      theCompleteCacheDirectory.createDirectory();
   }
   if((theCompleteCacheDirectory != "")&&
      theCompleteCacheDirectory.exists())
   {
      filename = theCompleteCacheDirectory;
      file << "F" << tileId.face() << "_L" <<tileId.level() <<"_X"<<tileId.x()<<"_Y"<<tileId.y();
      filename = filename.dirCat(file.str().c_str());
      if(filename.exists())
      {
         texture = theWmsClient->readLocalImage(filename);
      }
      // hasCacheDir = true;
   }
   if(!texture.valid())
   {
      if(!grid.isPolar(tileId))
      {
         ossimFilename tempFile;
         if(!filename.empty())
         {
            tempFile = filename+"_tmp";
         }
         texture = theWmsClient->createImage(width,
                                           height,
                                           minLatLon.y(),
                                           minLatLon.x(),
                                           maxLatLon.y(),
                                           maxLatLon.x(),
                                           tempFile);
         if(texture.valid())
         {
            if(tempFile.exists())
            {
               tempFile.rename(filename);
            }
         }
      }
   }
   if(texture.valid())
   {
      texture->flipVertical();
   }

   return texture.get();
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetWmsImageLayer::getTexture(ossim_uint32 level,
                                                                    ossim_uint64 row,
                                                                    ossim_uint64 col,
                                                                    const ossimPlanetGridUtility& utility)
{
   if(!getEnableFlag())
   {
      return 0;
   }
  if(theDirtyExtentsFlag)
   {
      updateExtents();
   }
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theWmsArchiveMutex);
   osg::ref_ptr<ossimPlanetImage> image = 0;

   if(theServer == "")
   {
      return NULL;
   }
   double minLat;
   double minLon;
   double maxLat;
   double maxLon;
   ossim_uint32 w = utility.getTileWidth();
   ossim_uint32 h = utility.getTileHeight();

   utility.getLatLonBounds(minLat,
                           minLon,
                           maxLat,
                           maxLon,
                           level,
                           row,
                           col);
  if(!theExtents->intersectsLatLon(minLat, minLon, maxLat, maxLon))
  {
     return 0;
  }
   double deltaX;
   double deltaY;
   utility.getWidthHeightInDegrees(deltaX, deltaY, level, row, col);
   
   double deltaLat    = deltaY/h;
//   double deltaLon    = deltaX/w;
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;

   osg::ref_ptr<ossimPlanetImage> texture = 0;
   
   if(gsd.y > theExtents->getMaxScale())//!//theExtents->intersectsScale(gsd.y-FLT_EPSILON,
       //                            gsd.y+FLT_EPSILON))//gsd.y <= theExtents.theMaxGsd)
   {
      return 0;
   }
   std::stringstream file;
   ossimFilename filename;
//   ossimPlanetWmsClient client(theAdjustedServer);
//   client.setImageType(theImageType);
   
   // bool hasCacheDir = false;
   if((theCompleteCacheDirectory!="")&&
      (!theCompleteCacheDirectory.exists())&&
      (theAutoCreateCacheFlag))
   {
      theCompleteCacheDirectory.createDirectory();
   }
   if((theCompleteCacheDirectory != "")&&
      theCompleteCacheDirectory.exists())
   {
      filename = theCompleteCacheDirectory;
      file << "L" <<level <<"_X"<<col<<"_Y"<<row;
      filename = filename.dirCat(file.str().c_str());
      // hasCacheDir = true;
   }
   if(!image.valid())
   {
      if(filename.exists())
      {
         filename.remove();
      }
      if(utility.getFace(level, row, col)<4)
      {
         ossimFilename tempFile;
         if(!filename.empty())
         {
            tempFile = filename+"_tmp";
         }
         image = theWmsClient->createImage(w,
                                    h,
                                    minLat,
                                    minLon,
                                    maxLat,
                                    maxLon,
                                    tempFile);
         if(image.valid())
         {
            if(tempFile.exists())
            {
               tempFile.rename(filename);
            }
         }
      }
      else
      {
         double deltaX;
         double deltaY;
         utility.getWidthHeightInDegrees(deltaX, deltaY, level, row, col);
         
         std::vector<ossimPlanetGridUtility::GridPoint> points;
         utility.createGridPoints(points,
                                  level,
                                  row,
                                  col,
                                  w,
                                  h);
         std::vector<osg::Vec2d> minMaxPairs;
         utility.getGeographicLonCrossings(minMaxPairs, level, row, col);
         
         ossim_uint32 size = minMaxPairs.size();
         ossim_uint32 idx = 0;
         
         for(idx = 0; idx < size; ++idx)
         {
            ossimFilename tempFile;
            if(filename != "")
            {
               tempFile = ossimFilename(filename + "_" + ossimString::toString(idx));
            }
            minLon = minMaxPairs[idx][0];
            maxLon = minMaxPairs[idx][1];
            
            double deltaLat    = deltaY/h;
            double deltaLon    = deltaX/w;
            //ossim_uint32 dx = (ossim_uint32)(((maxLon-minLon)/deltaLon));
            //ossim_uint32 dy = (ossim_uint32)(((maxLat-minLat)/deltaLat));
            ossimRefPtr<ossimImageData> data;
            osg::ref_ptr<ossimPlanetImage> dataPlanetImage;

            // need to fix this.  This is actually wrong for the polare faces to do it like this.  Note,
            // the only WMS output projection that can do the world is geographic (that I know of),  We really
            // need a polar projectino server that is comon amond all WMS's or can be queried for support.
            // For now I will leave it.  But later I need to ad looping over all geographc tiles as yoou approach the
            // pole.  This would force a slow hit.
            dataPlanetImage = theWmsClient->createImage(w,//dx,
                                                        h,//dy,
                                                        minLat,
                                                        minLon,
                                                        maxLat,
                                                        maxLon,
                                                        tempFile);
            if((tempFile!= "")&&tempFile.exists())
            {
               tempFile.remove();
            }
            if(dataPlanetImage.valid())
            {
               data = dataPlanetImage->toOssimImage();
            }
            else
            {
               data = 0;
            }
            if(data.valid()&&data->getBuf()&&(data->getDataObjectStatus()!=OSSIM_EMPTY))
            {
               if(!image.valid())
               {
                  image = new ossimPlanetImage(ossimPlanetTerrainTileId(0,
                                                                        level,
                                                                        row,
                                                                        col));
               }
               convertToOsg(data.get(),
                            image.get(),
                            osg::Vec2d(minLon,
                                       maxLat),
                            osg::Vec2d(maxLon+deltaLon,
                                       maxLat),
                            osg::Vec2d(maxLon+deltaLon,
                                       minLat-deltaLat),
                            osg::Vec2d(minLon,
                                       minLat-deltaLat),
                            points,
                            utility,
                            w,
                            h);
               
            }
         }
         if(image.valid())
         {
            if(filename != "")
            {
               ossimPushNotifyFlags();
               ossimSetNotifyFlag(ossimNotifyFlags_NONE);
               ossimRefPtr<ossimImageData> data = image->toOssimImage();
               ossimRefPtr<ossimMemoryImageSource> memSource = new ossimMemoryImageSource;
               
               memSource->setImage(data.get());
               theWriter->disconnect();
               theWriter->connectMyInputTo(0, memSource.get());
               theWriter->setOutputName(filename.c_str());
               theWriter->execute();
               ossimPopNotifyFlags();
            }
         }
      }
      if(image.valid())
      {
         image->setState(ossimPlanetImage::ossimPlanetImageStateType_LOADED);
      }
   }
   else
   {
      image->setState(ossimPlanetImage::ossimPlanetImageStateType_LOADED);
   }
   if(image.valid())
   {
      image->setId(ossimPlanetTerrainTileId(0, level, col, row));
      if(image->getNumberOfComponents() == 3)
      {
         insertAlpha(image.get());
      }
      if(theTransparentColorFlag)
      {
         addTransparent(image.get());
      }
      image->setPixelStatus();
      if(utility.getFace(level, row, col)<4)
      {
         image->flipVertical();
      }
   }
   
   return image;
}

void ossimPlanetWmsImageLayer::setServer(const std::string& serverString)
{
   theServer = serverString;

   adjustServerString();

   dirtyExtents();
}

const std::string& ossimPlanetWmsImageLayer::getServer()const
{
   return theServer;
}

void ossimPlanetWmsImageLayer::adjustServerString()
{
//    std::cout << "SERVER STRING START = " << theServer << std::endl;
   theAdjustedServer = theServer;
   ossimString name = "FORMAT";
   if(!theAdjustedServer.contains(name))
   {
      name = "format";
   }
   ossimString::size_type startPos = theAdjustedServer.find(name.string());
   if(startPos!=std::string::npos)//theServer.contains(name))
   {
      ossimString formatString = theAdjustedServer.after(name.string());
      formatString = formatString.after("=");
      formatString = formatString.trim();
      if(formatString!="")
      {
         theImageType = formatString.before("&");
      }
      ossimString::size_type endPos = theAdjustedServer.find("&", startPos);//+name.length());
      

      if(endPos != std::string::npos)
      {
         endPos += 1;
         theAdjustedServer.erase(theAdjustedServer.begin()+startPos, theAdjustedServer.begin()+endPos);
      }
      else
      {
         theAdjustedServer.erase(theAdjustedServer.begin()+startPos, theAdjustedServer.end());
     }
   }

//    std::cout << "ADJUSTED 1: " << theAdjustedServer << std::endl;
   // now just extract layers and styles so we can determine directory cache
   //
   name = "LAYERS";
   if(!theAdjustedServer.contains(name))
   {
      name = "layers";
   }
   startPos = theAdjustedServer.find(name.string());
   if(startPos!=std::string::npos)
   {
      ossimString value = theAdjustedServer.after(name.string());
      value = value.after("=");
      value = value.trim();
      if(value!="")
      {
         value = value.before("&");
      }
      ossimString::size_type endPos = theAdjustedServer.find("&", startPos);//+name.length());
      if(endPos != std::string::npos)
      {
         endPos += 1;
      }
      value.split(theLayers, ",");
   }
   // now just extract layers and styles so we can determine directory cache
   //
   name = "STYLES";
   if(!theAdjustedServer.contains(name))
   {
      name = "styles";
   }
   startPos = theAdjustedServer.find(name.string());
   if(startPos!=std::string::npos)
   {
      ossimString value = theAdjustedServer.after(name.string());
      value = value.after("=");
      value = value.trim();
      if(value!="")
      {
         value = value.before("&");
      }
      ossimString::size_type endPos = theAdjustedServer.find("&", startPos);//+name.length());
      if(endPos != std::string::npos)
      {
         endPos += 1;
      }
      value.split(theStyles, ",");
   }

   // make sure we reset the full path directory by forcing a call
   // to set cache directory
   setCacheDirectory(getCacheDirectory());

   theImageType = theImageType.downcase();
   theWriter = 0;
   // let's use our plugin writer if it exists.  Seems to core dump on windows when using the GDAL png.
//   if(theImageType.downcase() == "image/png")
//   {
//	   theWriter = ossimImageWriterFactoryRegistry::instance()->createWriter(ossimString("ossimPngWriter"));
//   }
   if(!theWriter.valid())
   {
	  theWriter = ossimImageWriterFactoryRegistry::instance()->createWriter(theImageType);
   }
   if(!theWriter.valid())
   {
      theWriter = new ossimJpegWriter;
   }

   if(!theAdjustedServer.empty())
   {
      if(!theAdjustedServer.contains("SRS=")&&
         !theAdjustedServer.contains("srs="))
      {
         if(*(theAdjustedServer.begin() + theAdjustedServer.size()-1) != '&')
         {
             theAdjustedServer += "&";
         }
         theAdjustedServer += "SRS=EPSG:4326";
      }
      if(!theAdjustedServer.contains("SERVICE=")&&
         !theAdjustedServer.contains("service="))
      {
         if(*(theAdjustedServer.begin() + theAdjustedServer.size()-1) != '&')
         {
             theAdjustedServer += "&";
         }
         theAdjustedServer += "SERVICE=WMS";
         
      }
   }
   theWmsClient->setServer(theAdjustedServer);
   theWmsClient->setImageType(theImageType);
}

const std::string& ossimPlanetWmsImageLayer::getImageType()const
{
   return theImageType;
}

void ossimPlanetWmsImageLayer::setImageType(const std::string& imageType)
{
   theImageType = imageType;
}

const ossimFilename& ossimPlanetWmsImageLayer::getCacheDirectory()const
{
   return theCacheDirectory;
}

const ossimFilename& ossimPlanetWmsImageLayer::getCompleteCacheDirectory()const
{
   return theCompleteCacheDirectory;
}

void ossimPlanetWmsImageLayer::setCacheDirectory(const ossimFilename& cacheDir)
{
   theCacheDirectory = cacheDir;
   theCompleteCacheDirectory=cacheDir;
   if(!theCacheDirectory.empty())
   {
      wmsUrl url(theAdjustedServer.string());
      ossimString server = ossimFilename(ossimString(url.server()).substitute(".", "_", true));
	  server = server.substitute("/","_",true);
	  server = server.substitute(":","_",true);

      if(!server.empty())
      {
         theCompleteCacheDirectory = theCacheDirectory.dirCat(server);
         if(theLayers.size() == 1)// if it's not a composite
         {
            theCompleteCacheDirectory = theCacheDirectory.dirCat(server);
            theCompleteCacheDirectory = theCompleteCacheDirectory.dirCat(theLayers[0]);
            if(theStyles.size()==1)
            {
               theCompleteCacheDirectory = theCompleteCacheDirectory.dirCat(theStyles[0]);
           }
         }
      }
	  if(!theCompleteCacheDirectory.exists())
	  {
		  theCompleteCacheDirectory.createDirectory(true);
	  }
   }
}

void ossimPlanetWmsImageLayer::setRawCapabilities(const ossimString& rawCapabilities)
{
   theRawCapabilities = rawCapabilities;
   dirtyExtents();
}

const ossimString& ossimPlanetWmsImageLayer::getRawCapabilities()const
{
   return theRawCapabilities;
}

void ossimPlanetWmsImageLayer::setCapabilitiesUrl(const std::string& url)
{
   theCapabilitiesUrl = url;
   dirtyExtents();

}

const std::string& ossimPlanetWmsImageLayer::getCapabilitiesUrl()const
{
   return theCapabilitiesUrl;
}

void ossimPlanetWmsImageLayer::setBackgroundColor(const ossimString& color)
{
   theBackgroundColor = color;
}

const ossimString& ossimPlanetWmsImageLayer::getBackgroundColor()const
{
   return theBackgroundColor;
}

void ossimPlanetWmsImageLayer::setTransparentFlag(bool flag)
{
   theTransparentFlag = flag;
}

bool ossimPlanetWmsImageLayer::getTransparentFlag()const
{
   return theTransparentFlag;
}

void ossimPlanetWmsImageLayer::setAdditionalParameters(const ossimString& additionalParameters)
{
   theAdditionalParameters = additionalParameters;
   dirtyExtents();
}

const ossimString& ossimPlanetWmsImageLayer::getAdditionalParameters()const
{
   return theAdditionalParameters;
}

void ossimPlanetWmsImageLayer::setAutoCreateCacheFlag(bool value)
{
   theAutoCreateCacheFlag = value;
}

bool ossimPlanetWmsImageLayer::getAutoCreateCacheFlag()const
{
   return theAutoCreateCacheFlag;
}
   
void ossimPlanetWmsImageLayer::clearDiskCache()
{
   if(theCompleteCacheDirectory.exists())
   {
      ossimFilename wildcardFiles = theCompleteCacheDirectory.dirCat(".*");
      ossimFilename::wildcardRemove(wildcardFiles);
   }
}

ossimRefPtr<ossimXmlNode> ossimPlanetWmsImageLayer::saveXml(bool /*recurse*/)const
{
   ossimRefPtr<ossimXmlNode> result = ossimPlanetTextureLayer::saveXml();

   result->addChildNode("cacheDirectory", theCacheDirectory);
   result->addChildNode("completeCacheDirectory", theCompleteCacheDirectory);
   result->addChildNode("server", theServer);
   result->addChildNode("imageType", theImageType);
   result->addChildNode("capabilitiesUrl", theCapabilitiesUrl);
   result->addChildNode("backgroundColor", theBackgroundColor);
   result->addChildNode("transparentColorFlag", ossimString::toString(theTransparentFlag));
   result->addChildNode("additionalParameters", theAdditionalParameters);
   
   result->addChildNode("proxyHost", theProxyHost);
   result->addChildNode("proxyPort", theProxyPort);
   result->addChildNode("proxyUser", theProxyUser);
   result->addChildNode("proxyPassword", theProxyPassword);

   return result.get();
}

bool ossimPlanetWmsImageLayer::loadXml(ossimRefPtr<ossimXmlNode> node)
{
   if(!ossimPlanetTextureLayer::loadXml(node)) return false;
   
   ossimRefPtr<ossimXmlNode> server               = node->findFirstNode("server");
   ossimRefPtr<ossimXmlNode> imageType            = node->findFirstNode("imageType");
   ossimRefPtr<ossimXmlNode> cacheDirectory       = node->findFirstNode("cacheDirectory");
   ossimRefPtr<ossimXmlNode> capabilitiesUrl      = node->findFirstNode("capabilitiesUrl");
   ossimRefPtr<ossimXmlNode> backgroundColor      = node->findFirstNode("backgroundColor");
   ossimRefPtr<ossimXmlNode> transparentFlag      = node->findFirstNode("transparentFlag");
   ossimRefPtr<ossimXmlNode> additionalParameters = node->findFirstNode("additionalParameters");
   ossimRefPtr<ossimXmlNode> extents              = node->findFirstNode("ossimPlanetExtents");
   ossimRefPtr<ossimXmlNode> lookAt               = node->findFirstNode("ossimPlanetLookAt");
   ossimRefPtr<ossimXmlNode> proxyHost            = node->findFirstNode("proxyHost");
   ossimRefPtr<ossimXmlNode> proxyPort            = node->findFirstNode("proxyPort");
   ossimRefPtr<ossimXmlNode> proxyUser            = node->findFirstNode("proxyUser");
   ossimRefPtr<ossimXmlNode> proxyPassword        = node->findFirstNode("proxyPassword");
  
   if(!transparentFlag.valid())
   {
      transparentFlag = node->findFirstNode("transparentColorFlag");
   }
   if(server.valid())
   {
      setServer(server->getText());
   }
   if(imageType.valid())
   {
      setImageType(imageType->getText());      
   }
   if(cacheDirectory.valid())
   {
      setCacheDirectory(cacheDirectory->getText());     
   }
   if(capabilitiesUrl.valid())
   {
      setCapabilitiesUrl(capabilitiesUrl->getText());      
   }
   if(backgroundColor.valid())
   {
      setBackgroundColor(backgroundColor->getText());      
   }
   if(transparentFlag.valid())
   {
      setTransparentFlag(transparentFlag->getText().toBool());
   }
   if(additionalParameters.valid())
   {
      setAdditionalParameters(additionalParameters->getText());      
   }
   if(extents.valid())
   {
      getExtents()->loadXml(extents.get());
   }
   if(proxyHost.valid())
   {
      setProxyHost(proxyHost->getText());
   }
   else
   {
      setProxyHost("");
   }
   if(proxyPort.valid())
   {
      setProxyPort(proxyPort->getText());
   }
   else
   {
      setProxyPort("");
   }
   if(proxyUser.valid())
   {
      setProxyUser(proxyUser->getText());
   }
   else
   {
      setProxyUser("");
   }
   if(proxyPassword.valid())
   {
      setProxyPassword(proxyPassword->getText());
   }
   else
   {
      setProxyPassword("");
   }
   return server.valid();
}
