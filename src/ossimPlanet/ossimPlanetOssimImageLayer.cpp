#include <ossimPlanet/ossimPlanetOssimImageLayer.h>
#include <ossimPlanet/ossimPlanetTextureLayerGroup.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimBandSelector.h>
#include <ossim/imaging/ossimScalarRemapper.h>
#include <ossim/imaging/ossimNitfTileSource.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/support_data/ossimNitfImageHeader.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/projection/ossimImageViewTransform.h>
#include <ossim/projection/ossimImageViewProjectionTransform.h>
#include <ossim/imaging/ossimFilterResampler.h>
#include <ossim/imaging/ossimCacheTileSource.h>
#include <ossim/imaging/ossimMeanMedianFilter.h>
#include <ossimPlanet/mkUtils.h>
#include <osg/io_utils>
ossimPlanetOssimImageLayer::ossimPlanetOssimImageLayer()
:theCut(0),
ossimPlanetTextureLayer(),
theHistogramRemapper(0),
theHistogramStretchMode(ossimHistogramRemapper::STRETCH_UNKNOWN),
theHistogramStretchEnableFlag(false),
theCenterLat(0.0),
theCenterLon(0.0),
theLength(0.0)
{
   theViewInterface = 0;
   theStateCode = ossimPlanetTextureLayer_NOT_OPENED;
}

ossimPlanetOssimImageLayer::ossimPlanetOssimImageLayer(const ossimPlanetOssimImageLayer& src)
:ossimPlanetTextureLayer(src),
theCut(0),
theFilename(src.theFilename),
theCenterLat(src.theCenterLat),
theCenterLon(src.theCenterLon)
{
   theHistogramRemapper = 0;
   if(src.theSource.valid())
   {
      theSource = (ossimImageHandler*)src.theSource->dup();
   }
   theViewInterface = 0;
   theRenderer = 0;
   theChain.clear();
   theImageSpaceChain.clear();
   theProjection = 0;
   buildChain();
}

ossimPlanetOssimImageLayer::~ossimPlanetOssimImageLayer()
{
   if(theSource.valid())
   {
      theSource->disconnect();
      theSource = 0;
   }
   theRenderer = 0;
   clearChains();
}
void ossimPlanetOssimImageLayer::clearChains()
{
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theChain.size(); ++idx)
   {
      theChain[idx]->disconnect();
      theChain[idx] = 0;
   }
   theChain.clear();
   for(idx = 0; idx < theImageSpaceChain.size(); ++idx)
   {
      theImageSpaceChain[idx]->disconnect();
      theImageSpaceChain[idx] = 0;
   }
   theImageSpaceChain.clear();
   
}

ossimPlanetTextureLayerStateCode ossimPlanetOssimImageLayer::openImage(const ossimFilename& filename,
                                                                       ossim_int32 entryIdx)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
   theViewInterface = 0;
   theFilename = "";
   theSource = 0;
   theRenderer = 0;
   theCenterLat = 0.0;
   theCenterLon = 0.0;
   theLength    = 0.0;
   theHistogramRemapper = 0;
   theStateCode = ossimPlanetTextureLayer_VALID;
   
   ossimRefPtr<ossimImageHandler> source = (ossimImageHandlerRegistry::instance()->open(filename));
   if(theOverviewFile.exists()&&source.valid())
   {
      source->openOverview(theOverviewFile);
   }
   if(source.valid()&&(entryIdx > -1))
   {
      if(!source->setCurrentEntry(entryIdx))
      {
         setState(ossimPlanetTextureLayer_NO_SOURCE_DATA);
         return theStateCode;
      }
   }
   else if(!source.valid())
   {
      setState(ossimPlanetTextureLayer_NO_SOURCE_DATA);
      return theStateCode;
   }
   setState (setHandler(source) );
   return theStateCode;
}

ossimPlanetTextureLayerStateCode ossimPlanetOssimImageLayer::setCurrentEntry(ossim_int32 idx)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
   theViewInterface = 0;
   theRenderer = 0;
   theCenterLat = 0.0;
   theCenterLon = 0.0;
   theLength    = 0.0;
   theHistogramRemapper = 0;
   theStateCode = ossimPlanetTextureLayer_VALID;
   
   if(theSource.valid())
   {
      if(!theSource->setCurrentEntry(idx))
      {
         setState(ossimPlanetTextureLayer_NO_SOURCE_DATA);
         return theStateCode;
      }
   }
   else
   {
      setState(ossimPlanetTextureLayer_NO_SOURCE_DATA);
      return theStateCode;
   }
   if(theOverviewFile.exists()&&theSource.valid())
   {
      theSource->openOverview(theOverviewFile);
   }
   setState(setHandler(theSource.get()));
   return theStateCode;
}

ossim_uint32 ossimPlanetOssimImageLayer::getNumberOfEntries()const
{
   if(theSource.valid())
   {
      return theSource->getNumberOfEntries();
   }
   
   return 0;   
}

void ossimPlanetOssimImageLayer::setOverviewFile(const ossimFilename& filename)
{
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
      theOverviewFile = filename;
      if(theSource.valid())
      {
         if(theOverviewFile.exists())
         {
            if(theSource->openOverview(theOverviewFile))
            {
               clearState(ossimPlanetTextureLayer_NO_OVERVIEWS);
            }
         }
         else
         {
            theSource->closeOverview();
         }
         dirtyExtents();
      }
      notifyPropertyChanged("overviewFilename", this);
	}
	if(filename.exists())
	{
		notifyRefreshExtent(theExtents.get());
	}
}

void ossimPlanetOssimImageLayer::setHistogramFile(const ossimFilename& file)
{
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
      theHistogramFile = file;
      if(theHistogramRemapper)
      {
         if(theHistogramRemapper->openHistogram(file))
         {
            theHistogramRemapper->setStretchMode(theHistogramStretchMode);
            clearState(ossimPlanetTextureLayer_NO_HISTOGRAMS);
         }
         dirtyExtents();
      }
      notifyPropertyChanged("histogramFilename", this);
	}
	if(file.exists())
	{
		notifyRefreshExtent(theExtents.get());
	}
}

void ossimPlanetOssimImageLayer::setHistogramStretchMode(const ossimString& mode)
{
   bool changed = false;
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
      
      if(mode == "None")
      {
         theHistogramStretchMode = ossimHistogramRemapper::STRETCH_UNKNOWN;
      }
      else if(mode == "Linear Auto Min Max")
      {
         theHistogramStretchMode = ossimHistogramRemapper::LINEAR_AUTO_MIN_MAX;
      }
      else if(mode == "1 Standard Deviation")
      {
         theHistogramStretchMode = ossimHistogramRemapper::LINEAR_1STD_FROM_MEAN;
      }
      else if(mode == "2 Standard Deviation")
      {
         theHistogramStretchMode = ossimHistogramRemapper::LINEAR_2STD_FROM_MEAN;
      }
      else if(mode == "3 Standard Deviation")
      {
         theHistogramStretchMode = ossimHistogramRemapper::LINEAR_3STD_FROM_MEAN;
      }
      else
      {
         theHistogramStretchMode = ossimHistogramRemapper::STRETCH_UNKNOWN;
      }
      if(theHistogramRemapper)
      {
         changed = theHistogramRemapper->getStretchMode() != theHistogramStretchMode;
         theHistogramRemapper->setStretchMode(theHistogramStretchMode);
      }
   }
   if(changed)
   {
      notifyRefreshExtent(theExtents.get());
   }
}

void ossimPlanetOssimImageLayer::setHistogramStretchEnableFlag(bool flag)
{
   bool changed = false;
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
      if(theHistogramRemapper)
      {
         theHistogramStretchEnableFlag = flag;
         if(flag != theHistogramRemapper->getEnableFlag())
         {
            theHistogramRemapper->setEnableFlag(theHistogramStretchEnableFlag);
            changed = true;
         }
      }
   }
   if(changed)
   {
      notifyRefreshExtent(theExtents.get());
   }
}

ossimString ossimPlanetOssimImageLayer::histogramStretchMode()const
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
   switch(theHistogramStretchMode)
   {
      case ossimHistogramRemapper::STRETCH_UNKNOWN:
      {
         return "None";
      }
      case ossimHistogramRemapper::LINEAR_AUTO_MIN_MAX:
      {
         return "Linear Auto Min Max";
      }
      case ossimHistogramRemapper::LINEAR_1STD_FROM_MEAN:
      {
         return "1 Standard Deviation";
      }
      case ossimHistogramRemapper::LINEAR_2STD_FROM_MEAN:
      {
         return "2 Standard Deviation";
      }
      case ossimHistogramRemapper::LINEAR_3STD_FROM_MEAN:
      {
         return "3 Standard Deviation";
      }
		default:
		{
			break;
		}
   }
   return "None";
}

void ossimPlanetOssimImageLayer::getHistogramStretchModes(std::vector<ossimString>& modes)
{
   modes.push_back("None");
   modes.push_back("Linear Auto Min Max");
   modes.push_back("1 Standard Deviation");
   modes.push_back("2 Standard Deviation");
   modes.push_back("3 Standard Deviation");
}

ossimPlanetTextureLayerStateCode ossimPlanetOssimImageLayer::setHandler(ossimRefPtr<ossimImageHandler> handler)
{
   theFilename = "";
   theSource = handler;
   theRenderer = 0;
   theCenterLat = 0.0;
   theCenterLon = 0.0;
   theLength    = 0.0;
   
   if(handler.valid())
   {
      theFilename = handler->getFilename();
      if(name().empty())
      {
         ossimString name = theFilename.file();
         
         if(handler->getNumberOfEntries() > 1)
         {
            name +=  ": entry " + ossimString::toString(handler->getCurrentEntry());
         }
         setName(name);
      }
   }
	
   return buildChain();
}

ossimRefPtr<ossimImageHandler> ossimPlanetOssimImageLayer::getHandler()
{
   return theSource;
}

const ossimRefPtr<ossimImageHandler> ossimPlanetOssimImageLayer::getHandler()const
{
   return theSource;
}

double ossimPlanetOssimImageLayer::getApproximateHypotneusLength()const
{
   return theLength;
}

ossimPlanetTextureLayer* ossimPlanetOssimImageLayer::dup()const
{
   return new ossimPlanetOssimImageLayer(*this);
}

ossimPlanetTextureLayer* ossimPlanetOssimImageLayer::dupType()const
{
   return new ossimPlanetOssimImageLayer;
}

ossimString ossimPlanetOssimImageLayer::getClassName()const
{
   return "ossimPlanetOssimImageLayer";
}

ossimPlanetTextureLayerStateCode ossimPlanetOssimImageLayer::updateExtents()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
   return updateExtentsNoMutex();
}
ossimPlanetTextureLayerStateCode ossimPlanetOssimImageLayer::updateExtentsNoMutex()
{
   if(!theDirtyExtentsFlag) return theStateCode;

   clearState(ossimPlanetTextureLayerStateCode(ossimPlanetTextureLayer_NO_OVERVIEWS |
                                               ossimPlanetTextureLayer_NO_GEOM |
                                               ossimPlanetTextureLayer_NO_SOURCE_DATA));
   theCenterLat = 0.0;
   theCenterLon = 0.0;
   theLength    = 0.0;
   theInputProjection = 0;
   if(theSource.valid())
   {
      double levels = theSource->getNumberOfDecimationLevels();
      ossimRefPtr<ossimImageGeometry> geom = theSource->getImageGeometry();
      theInputProjection = geom.valid()?geom->getProjection():0;
      if(levels == 1)
      {
         setState(ossimPlanetTextureLayer_NO_OVERVIEWS);
      }
      
      if(theInputProjection.valid())
      {
         
         ossimGpt ulGpt;
         ossimGpt urGpt;
         ossimGpt lrGpt;
         ossimGpt llGpt;
         ossimGpt centerGpt;
         
         ossimIrect rect = theSource->getBoundingRect();
         geom->localToWorld(rect.midPoint(),
                                               centerGpt);
         theCenterLat = centerGpt.latd();
         theCenterLon = centerGpt.lond();
         
         geom->localToWorld(rect.ul(),
                            ulGpt);
         geom->localToWorld(rect.ur(),
                            urGpt);
         geom->localToWorld(rect.lr(),
                            lrGpt);
         geom->localToWorld(rect.ll(),
                            llGpt);
         ossimDpt metersPerPixel = geom->getMetersPerPixel();
         theLength = ((rect.ul()-rect.lr()).length()*
                      (metersPerPixel.y));
         
         ossim_uint32 levels = theSource->getNumberOfDecimationLevels();
         ossim_uint32 stopLevels = levels;
         ossim_uint32 idx = 0;
#if 1
         for(idx = 0; idx < levels; ++idx)
         {
            ossimIrect rect = theSource->getBoundingRect(idx);
            if((rect.width()>16)||(rect.height()>16))
            {
               ++stopLevels;
            }
            else
            {
               break;
            }
         }
#endif
         theExtents->setMinMaxScale(metersPerPixel.y,
                                    metersPerPixel.y*std::pow(2.0, (double)(stopLevels)));// clamp the zoom out though
         theExtents->setMinMaxLatLon(mkUtils::clamp(ossim::min(ulGpt.latd(),ossim::min(urGpt.latd(),ossim::min(lrGpt.latd(),llGpt.latd()))), -90.0, 90.0),
                                     mkUtils::clamp(ossim::min(ulGpt.lond(),ossim::min(urGpt.lond(),ossim::min(lrGpt.lond(),llGpt.lond()))), -180.0, 180.0),
                                     mkUtils::clamp(ossim::max(ulGpt.latd(),ossim::max(urGpt.latd(),ossim::max(lrGpt.latd(),llGpt.latd()))), -90.0, 90.0),
                                     mkUtils::clamp(ossim::max(ulGpt.lond(),ossim::max(urGpt.lond(),ossim::max(lrGpt.lond(),llGpt.lond()))), -180.0, 180.0));        
         
//         std::cout << theExtents->getMinLon() << ", " << theExtents->getMinLat() << ", " << theExtents->getMaxLon() << ", " << theExtents->getMaxLat() << std::endl;
      }
      else
      {
         setState(ossimPlanetTextureLayer_NO_GEOM);
      }
   }
   else
   {
      setState(ossimPlanetTextureLayer_NO_SOURCE_DATA);
   }
   theDirtyExtentsFlag = false;
   updateStats();
   
   
   return theStateCode;
}

void ossimPlanetOssimImageLayer::updateStats()const
{
   if(theSource.valid())
   {
      ossimIrect rect = theSource->getBoundingRect();
      ossim_uint64 bands = theSource->getNumberOfInputBands();
      ossimScalarType scalarType = theSource->getOutputScalarType();
      ossim_uint64 w = rect.width();
      ossim_uint64 h = rect.height();
      ossim_uint64 scalarSizeInBytes = ossim::scalarSizeInBytes(scalarType); 
      theStats->setTotalTextureSize(scalarSizeInBytes*w*h*bands);
      
      if(theSource->getNumberOfDecimationLevels()>1)
      {
         ossim_uint32 levels = theSource->getNumberOfDecimationLevels();
         ossim_uint32 idx = 0;
         for(idx=1; idx < levels; ++idx)
         {
            rect = theSource->getBoundingRect(idx);
            w = rect.width();
            h = rect.height();
            theStats->setTotalTextureSize(theStats->totalTextureSize() +
                                          scalarSizeInBytes*w*h*bands);
            
         }
      }
   }
   theDirtyStatsFlag = false;
}

void ossimPlanetOssimImageLayer::resetStats()const
{
   updateStats();
   theStats->setBytesTransferred(0);
}

ossimScalarType ossimPlanetOssimImageLayer::scalarType()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
   if(theSource.valid())
   {
      return theSource->getOutputScalarType();
   }
   
   return OSSIM_SCALAR_UNKNOWN;
}

bool ossimPlanetOssimImageLayer::hasTexture(ossim_uint32 width,
                                            ossim_uint32 height,
                                            const ossimPlanetTerrainTileId& tileId,
                                            const ossimPlanetGrid& grid)
{
   if(!getEnableFlag())
   {
      return false;
   }
   if(theDirtyExtentsFlag)
   {
      updateExtents();
   }
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
   if(!theInputProjection.valid() ||
      !theSource.valid())
   {
      return false;
   }
   osg::Vec2d metersPerPixel;
   grid.getUnitsPerPixel(metersPerPixel, tileId, width, height, OSSIM_METERS);
   ossimDpt metersGsd(metersPerPixel[0], metersPerPixel[1]);
   if(theExtents.valid())
   {
      osg::ref_ptr<ossimPlanetExtents> extents = new ossimPlanetExtents;
      if(grid.convertToGeographicExtents(tileId, *extents, width, height))
      {
         if(theExtents->intersectsLatLon(*extents)&&
            theExtents->intersectsScale(*extents))
         {
            return true;
         }
      }
   }

   return false;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetOssimImageLayer::getTexture(ossim_uint32 width,
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
   
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
   if(!theInputProjection.valid() ||
      !theSource.valid())
   {
      return 0;
   }
   
   osg::ref_ptr<ossimPlanetExtents> tileExtents = new ossimPlanetExtents;
   if(grid.convertToGeographicExtents(tileId, *tileExtents, width, height))
   {
      if(theExtents.valid())
      {
         if(!theExtents->intersectsLatLon(*tileExtents))
         {
            return 0;
         }
      }
   }
   osg::Vec2d unitsPerPixel;
   osg::Vec2d metersPerPixel;
   grid.getUnitsPerPixel(unitsPerPixel, tileId, width, height, OSSIM_DEGREES);
   grid.getUnitsPerPixel(metersPerPixel, tileId, width, height, OSSIM_METERS);
   ossimDpt gsd(unitsPerPixel[0], unitsPerPixel[1]);
   ossimDpt metersGsd(metersPerPixel[0], metersPerPixel[1]);
   osg::Vec2d deltaXY;
   grid.widthHeightInModelSpace(tileId, deltaXY);

   osg::ref_ptr<ossimPlanetImage> texture = 0;
   
   if(metersGsd.y < theExtents->getMaxScale())
   {
//      std::cout << "LEVEL === " << tileId.level() << std::endl;
      if(!grid.isPolar(tileId))
      {
         ossimIrect requestRect(0,
                                0,
                                width-1,
                                height-1);
//         theProjection->setDecimalDegreesPerPixel(ossimDpt(deltaLon,
//                                                           deltaLat));
         ossimGpt testUl(tileExtents->getMaxLat()-(gsd.y*.5), 
                         tileExtents->getMinLon()+(gsd.x*.5));
         theProjection->setDecimalDegreesPerPixel(ossimDpt(gsd.x,
                                                           gsd.y));
         theProjection->setUlTiePoints(testUl);
         theProjection->update();
         theViewInterface->setView(theImageGeometry.get());
         theChain[0]->initialize();
         
         ossimRefPtr<ossimImageData> data;
         data = theChain[0]->getTile(requestRect);
         
         if(data.valid()&&data->getBuf()&&(data->getDataObjectStatus()!=OSSIM_EMPTY))
         {
            addBytesTransferredStat(data->getSizeInBytes());
            texture = new ossimPlanetImage(tileId);
            convertToOsg(data.get(), texture.get());
            texture->flipVertical();
         }  
      }
      else
      {
      //   std::cout << "________________________" << std::endl;
         ossimPlanetGrid::ModelPoints modelPoints;
         grid.createModelPoints(tileId,
                                width,
                                height,
                                modelPoints);
         
         //std::cout << "NEW WAY" << std::endl;
         std::vector<osg::Vec2d> minMaxPairs;
         grid.getInternationalDateLineCrossings(tileId, minMaxPairs);
         ossim_uint32 size = minMaxPairs.size();
         ossim_uint32 idx = 0;
         double maxLon=0.0, minLon=0.0, maxLat=tileExtents->getMaxLat(), minLat=tileExtents->getMinLat();
         for(idx = 0; idx < size; ++idx)
         {
            minLon = minMaxPairs[idx][0];
            maxLon = minMaxPairs[idx][1];
            
            ossim_uint32 dx = (ossim_uint32)(((maxLon-minLon)/gsd.x));//+.5);
            ossim_uint32 dy = (ossim_uint32)(((maxLat-minLat)/gsd.y));//+.5);
            ossimIrect requestRect(0,
                                   0,
                                   dx-1,
                                   dy-1);
            ossimRefPtr<ossimImageData> data;
            
            ossim_float32 tiles = (double)requestRect.width()/(double)width;
            ossim_uint32 nTiles = floor((double)requestRect.width()/(double)width);
            ossim_float32 residualTile = tiles - nTiles;
            ossim_uint32 tileX = 0;
            theProjection->setDecimalDegreesPerPixel(gsd);
#if 0
            theProjection->setUlTiePoints(ossimGpt(maxLat-(gsd.y*.5), 
                                                   minLon+(gsd.x*.5)));
            theViewInterface->setView(theImageGeometry.get());
            data = theChain[0]->getTile(requestRect);
            theChain->initialize();
            if(data.valid()&&data->getBuf()&&(data->getDataObjectStatus()!=OSSIM_EMPTY))
            {
               addBytesTransferredStat(data->getSizeInBytes());
               if(!texture.valid())
               {
                  texture = new ossimPlanetImage(tileId);
               }
               
               convertToOsg(data.get(),
                            texture.get(),
                            osg::Vec2d(minLon,
                                       maxLat),
                            osg::Vec2d(maxLon,//deltaLon,
                                       maxLat),
                            osg::Vec2d(maxLon,//deltaLon,
                                       minLat),//deltaLat),
                            osg::Vec2d(minLon,
                                       minLat),//deltaLat),
                            modelPoints,
                            width,
                            height);
               
            }
#else
            for(tileX = 0; tileX < nTiles; ++tileX)
            {
               ossimIpt origin(tileX*width, 0);
               ossimIrect tempRect(0, 0, width - 1, dy-1);
               
               double tempMinLon = minLon     + gsd.x*(origin.x);
               double tempMaxLon = tempMinLon + gsd.x*width;
               theProjection->setUlTiePoints(ossimGpt(maxLat-(gsd.y*.5), 
                                                      tempMinLon+(gsd.x*.5)));
               theViewInterface->setView(theImageGeometry.get());
               theChain[0]->initialize();
               data = theChain[0]->getTile(tempRect);
               if(data.valid()&&data->getBuf()&&(data->getDataObjectStatus()!=OSSIM_EMPTY))
               {
                  if(!texture.valid())
                  {
                     texture = new ossimPlanetImage(tileId);
                  }
                  
                  convertToOsg(data.get(),
                               texture.get(),
                               osg::Vec2d(tempMinLon,
                                          maxLat),
                               osg::Vec2d(tempMaxLon,//deltaLon,
                                          maxLat),
                               osg::Vec2d(tempMaxLon,//deltaLon,
                                          minLat),//deltaLat),
                               osg::Vec2d(tempMinLon,
                                          minLat),//deltaLat),
                               modelPoints,
                               width,
                               height);
                  
               }
               data = 0;
            }
            if(residualTile > FLT_EPSILON)
            {
               ossim_uint32 w = (residualTile*width);
               ossimIpt origin(nTiles*width, 0);
               ossimIrect tempRect(0, 0, w - 1, dy-1);
               double tempMinLon = minLon + gsd.x*(origin.x);
               double tempMaxLon = maxLon;
               theProjection->setUlTiePoints(ossimGpt(maxLat,//-(deltaLat*.5), 
                                                      tempMinLon));//+(deltaLon*.5)));
               theViewInterface->setView(theImageGeometry.get());
               theChain[0]->initialize();
               data = theChain[0]->getTile(tempRect);
               if(data.valid()&&data->getBuf()&&(data->getDataObjectStatus()!=OSSIM_EMPTY))
               {
                  if(!texture.valid())
                  {
                     texture = new ossimPlanetImage(tileId);
                     if(texture->data())
                     {
                        memset(texture->data(), '\0', texture->getWidth()*texture->getHeight()*4);
                     }
                  }
                  
                  convertToOsg(data.get(),
                               texture.get(),
                               osg::Vec2d(tempMinLon,
                                          maxLat),
                               osg::Vec2d(tempMaxLon,//deltaLon,
                                          maxLat),
                               osg::Vec2d(tempMaxLon,//deltaLon,
                                          minLat),//deltaLat),
                               osg::Vec2d(tempMinLon,
                                          minLat),//deltaLat),
                               modelPoints,
                               width,
                               height);
                  
               }
               data = 0;
            }
#endif
         }
      }
   }
   return texture;
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetOssimImageLayer::getTexture(ossim_uint32 level,
                                                                      ossim_uint64 row,
                                                                      ossim_uint64 col,
                                                                      const ossimPlanetGridUtility& utility)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
   if(!getEnableFlag())
   {
      return 0;
   }
   if(theDirtyExtentsFlag)
   {
      updateExtents();
   }
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(theOssimImageLayerMutex);
   
   if(!theProjection.valid() ||
      !theSource.valid())
   {
      return 0;
   }
   
   double minLat;
   double minLon;
   double maxLat;
   double maxLon;
   unsigned int w = utility.getTileWidth();
   unsigned int h = utility.getTileHeight();
   
   utility.getLatLonBounds(minLat,
                           minLon,
                           maxLat,
                           maxLon,
                           level,
                           row, // row 
                           col);
   if(!theExtents->intersectsLatLon(minLat, minLon, maxLat, maxLon))
   {
      return 0;
   }
   
   double deltaX;
   double deltaY;
   utility.getWidthHeightInDegrees(deltaX, deltaY, level, row, col);
   
   double deltaLat    = deltaY/h;
   double deltaLon    = deltaX/w;
   ossimDpt gsd = ossimGpt().metersPerDegree();
   gsd.y *= deltaLat;
   
   osg::ref_ptr<ossimPlanetImage> texture = 0;
   
   if(gsd.y < theExtents->getMaxScale())
   {
      if(utility.getFace(level, row, col)<4)
      {
         ossimIrect requestRect(0,
                                0,
                                w-1,
                                h-1);
         theProjection->setDecimalDegreesPerPixel(ossimDpt(deltaLon,
                                                           deltaLat));
         
         theProjection->setUlGpt(ossimGpt(maxLat-deltaLat*.5, minLon+deltaLon*.5));
         //theProjection->setUlGpt(ossimGpt(maxLat, minLon));
         theViewInterface->setView(theImageGeometry.get());
         ossimRefPtr<ossimImageData> data;
          data = theChain[0]->getTile(requestRect);
         
         if(data.valid()&&data->getBuf()&&(data->getDataObjectStatus()!=OSSIM_EMPTY))
         {
            addBytesTransferredStat(data->getSizeInBytes());
            texture = new ossimPlanetImage(ossimPlanetTerrainTileId(0,
                                           level,
                                           col,
                                           row));
            convertToOsg(data.get(), texture.get());
            texture->flipVertical();
         }  
      }
      else
      {
        // std::cout << "________________________________" << std::endl;
         std::vector<ossimPlanetGridUtility::GridPoint> points;
         utility.createGridPoints(points,
                                  level,
                                  row,
                                  col,
                                  w,
                                  h);
         osg::Vec3d llTemp;
         utility.getLatLon(llTemp, points[0]);

         std::vector<osg::Vec2d> minMaxPairs;
         utility.getGeographicLonCrossings(minMaxPairs, level, row, col);
         
         ossim_uint32 size = minMaxPairs.size();
         ossim_uint32 idx = 0;
         //std::cout << "OLD WAY!" << std::endl;
         for(idx = 0; idx < size; ++idx)
         {
            minLon = minMaxPairs[idx][0];
            maxLon = minMaxPairs[idx][1];
            //std::cout << "minLon = " << minLon << std::endl;
            //std::cout << "maxLon = " << maxLon << std::endl;
            
            ossim_uint32 dx = (ossim_uint32)(((maxLon-minLon)/deltaLon));//+.5);
            ossim_uint32 dy = (ossim_uint32)(((maxLat-minLat)/deltaLat));//+.5);
            ossimDpt degreesPerPixel(deltaLon,
                                     deltaLat);
            //std::cout << "degrees per pix = " << degreesPerPixel << std::endl;
            theProjection->setDecimalDegreesPerPixel(degreesPerPixel);
            theProjection->setUlGpt(ossimGpt(maxLat-deltaLat*.5, minLon+deltaLon*.5));
           // theProjection->setUlGpt(ossimGpt(maxLat, minLon));
            //std::cout << "Ul tie = " << ossimGpt(maxLat, minLon) << std::endl;
            theViewInterface->setView(theImageGeometry.get());
            ossimIrect requestRect(0,
                                   0,
                                   dx-1,
                                   dy-1);
            ossimRefPtr<ossimImageData> data;
            data = theChain[0]->getTile(requestRect);
            if(data.valid()&&data->getBuf()&&(data->getDataObjectStatus()!=OSSIM_EMPTY))
            {
               addBytesTransferredStat(data->getSizeInBytes());
               if(!texture.valid())
               {
                  texture = new ossimPlanetImage(ossimPlanetTerrainTileId(0,
                                                                          level,
                                                                          col,
                                                                          row));
               }
               
               convertToOsg(data.get(),
                            texture.get(),
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
      }
   }
   return texture;
}

void ossimPlanetOssimImageLayer::getCenterLatLonLength(double& centerLat,
                                                       double& centerLon,
                                                       double& length)const
{
   centerLat = theCenterLat;
   centerLon = theCenterLon;
   length    = theLength;
}

void ossimPlanetOssimImageLayer::setFilterType(const ossimString& filterType)
{
   ossimPlanetTextureLayer::setFilterType(filterType);
   if(theRenderer.valid())
   {
      theRenderer->getResampler()->setFilterType(theFilterType);
   }
}

bool ossimPlanetOssimImageLayer::isMultiEntry()const
{
   if(theSource.valid())
   {
      return (theSource->getNumberOfEntries() > 1);
   }
   
   return false;
}

bool ossimPlanetOssimImageLayer::buildOverview()
{
   if(theSource.valid())
   {
      return theSource->buildOverview();
   }
   
   return false;
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetOssimImageLayer::groupAllEntries()
{
   osg::ref_ptr<ossimPlanetTextureLayerGroup> groupResult = new ossimPlanetTextureLayerGroup;
   if(theSource.valid())
   {
      ossim_uint32 thisEntry = theSource->getCurrentEntry();
      std::vector<ossim_uint32> entryList;
      theSource->getEntryList(entryList);
      
      for(ossim_uint32 idx = 0; idx < entryList.size(); ++idx)
      {
         if(entryList[idx] != thisEntry)
         {
            osg::ref_ptr<ossimPlanetOssimImageLayer> layer = new ossimPlanetOssimImageLayer;         
            ossimRefPtr<ossimImageHandler> handler = (ossimImageHandler*)theSource->dup();
            handler->setCurrentEntry(entryList[idx]);
            layer->setHandler(handler.get());
				layer->dirtyExtents();
				layer->updateExtents();
            groupResult->addBottom(layer.get());
         }
      }
      
      groupResult->addTop(this);
   }
   return groupResult.get();
}

ossimPlanetTextureLayerStateCode ossimPlanetOssimImageLayer::buildChain()
{
   clearChains();
   theHistogramRemapper = 0;
   if(theSource.valid())
   {
      theChain.push_back(theSource.get());
      //ossimMeanMedianFilter* expand = new ossimMeanMedianFilter;
      //expand->setEnableFlag(true);
      //expand->setWindowSize(3);
      //expand->setFilterType(ossimMeanMedianFilter::OSSIM_MEAN_NULL_CENTER_ONLY);
      //expand->setAutoGrowRectFlag(true);
      //expand->connectMyInputTo(0, theChain[0].get());
      //theChain.insert(theChain.begin(), expand);
      if(theSource->getNumberOfOutputBands() > 3)
      {
         std::vector<ossim_uint32> outputBandList;
         outputBandList.push_back(0);
         outputBandList.push_back(1);
         outputBandList.push_back(2);
         ossimBandSelector* bandSelector = new ossimBandSelector;
         bandSelector->setOutputBandList(outputBandList);
         bandSelector->connectMyInputTo(0, theChain[0].get());
         theChain.insert(theChain.begin(), bandSelector);
         theImageSpaceChain.insert(theImageSpaceChain.begin(), bandSelector);
      }
      else if(theSource->getNumberOfOutputBands() <3)
      {
         std::vector<ossim_uint32> outputBandList;
         outputBandList.push_back(0);
         outputBandList.push_back(0);
         outputBandList.push_back(0);
         ossimBandSelector* bandSelector = new ossimBandSelector;
         bandSelector->setOutputBandList(outputBandList);
         bandSelector->connectMyInputTo(0, theChain[0].get());
         
         theChain.insert(theChain.begin(), bandSelector);
         theImageSpaceChain.insert(theImageSpaceChain.begin(), bandSelector);
     }
      theHistogramRemapper = new ossimHistogramRemapper;
      theHistogramRemapper->connectMyInputTo(0, theChain[0].get());
      if(theHistogramFile.empty())
      {
         theHistogramFile = theSource->createDefaultHistogramFilename();
      }
      if(theHistogramFile.exists())
      {
         theHistogramRemapper->openHistogram(theHistogramFile);
      }
		else
		{
			setState(ossimPlanetTextureLayer_NO_HISTOGRAMS);
		}
      theHistogramRemapper->setStretchMode(theHistogramStretchMode);
      theHistogramRemapper->setEnableFlag(theHistogramStretchEnableFlag);
      theChain.insert(theChain.begin(), theHistogramRemapper);
      theImageSpaceChain.insert(theImageSpaceChain.begin(), theHistogramRemapper);
      theViewInterface = PTR_CAST(ossimViewInterface, theSource.get());
      theImageGeometry = new ossimImageGeometry();
  
      ossimRefPtr<ossimRectangleCutFilter> theCut = new ossimRectangleCutFilter();
      theCut->setRectangle(theSource->getBoundingRect());
      theCut->connectMyInputTo(0, theChain[0].get());
      theChain.insert(theChain.begin(), theCut.get());
      

      theProjection = new ossimLlxyProjection;
     // theProjection = new ossimEquDistCylProjection;
      
      theImageGeometry->setProjection(theProjection.get());
      if(!theViewInterface)
      {
         theRenderer = new ossimImageRenderer;
         theViewInterface = theRenderer.get();
         theRenderer->setView(theImageGeometry.get());
         theRenderer->getResampler()->setFilterType(theFilterType);
         
         theRenderer->connectMyInputTo(0, theChain[0].get());
         theChain.insert(theChain.begin(), theRenderer.get());
         ossimImageViewTransform* ivt = theRenderer->getImageViewTransform();
         if(ivt)
         {
            ossimImageViewProjectionTransform* projectionTransform = PTR_CAST(ossimImageViewProjectionTransform,
                                                                              ivt);
            if(projectionTransform)
            {
               ossimRefPtr<ossimImageGeometry> geom = projectionTransform->getImageGeometry();
               if(geom.valid())
               {
                  ossimMapProjection* mapProj = PTR_CAST(ossimMapProjection,
                                                         geom->getProjection());
                  if(mapProj)
                  {
                     mapProj->setElevationLookupFlag(false);
                     theProjection->setElevationLookupFlag(false);
                  }
               }
            }
         }
      }
      else
      {
         theViewInterface->setView(theImageGeometry.get());
      }
      if(theSource->getOutputScalarType() != OSSIM_UINT8)
      {
         ossimScalarRemapper* scalarRemapper = new ossimScalarRemapper;
         scalarRemapper->connectMyInputTo(0, theChain[0].get());
         theChain.insert(theChain.begin(), scalarRemapper);
         ossimScalarRemapper* scalarRemapper2 = new ossimScalarRemapper;
         scalarRemapper2->connectMyInputTo(0, theImageSpaceChain[0].get());
         theImageSpaceChain.insert(theImageSpaceChain.begin(), scalarRemapper2);
      }
//      theImageSpaceChain.insert(theImageSpaceChain.begin(), expand);
      dirtyExtents();
      updateExtentsNoMutex();
   }
   return theStateCode;
}

void ossimPlanetOssimImageLayer::getMetadata(ossimRefPtr<ossimXmlNode> metadata)const
{
   if(theSource.valid())
   {
      ossimScalarType scalarType = theSource->getOutputScalarType();
      metadata->setTag("Layer");
      metadata->addChildNode("entry",
                             ossimString::toString(theSource->getCurrentEntry()));
      metadata->addChildNode("url",
                             ossimString("file://") + ossimString(theSource->getFilename()));
      metadata->addChildNode("bits", ossimString::toString(ossim::scalarSizeInBytes(scalarType)*8));
      ossimString scalarTypeString = ossimScalarTypeLut::instance()->getEntryString((ossim_int32)scalarType);
      scalarTypeString = scalarTypeString.substitute("ossim_", "");
      metadata->addChildNode("scalar", scalarTypeString);
      metadata->addChildNode("PyramidLevels",
                             ossimString::toString(theSource->getNumberOfDecimationLevels()));
      ossim_uint32 idx    = 0;
      ossim_uint32 idxMax = theSource->getNumberOfInputBands();
      ossimRefPtr<ossimXmlNode> bandListNode = new  ossimXmlNode();
      
      bandListNode->setTag("Bands");
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode();
      node->setTag("Count");
      node->setText(ossimString::toString(idxMax));
      
      bandListNode->addChildNode(node.get());
      metadata->addChildNode(bandListNode.get());
      for(idx = 0; idx < idxMax; ++idx)
      {
         ossimRefPtr<ossimXmlNode> bandNode = new  ossimXmlNode();
         bandNode->setTag("Band");
         bandNode->setText(ossimString::toString(idx));
         bandNode->addChildNode("min", ossimString::toString(theSource->getMinPixelValue(idx)));
         bandNode->addChildNode("max", ossimString::toString(theSource->getMaxPixelValue(idx)));
         bandNode->addChildNode("null", ossimString::toString(theSource->getNullPixelValue(idx)));
         
         bandListNode->addChildNode(bandNode.get());
      }
      
      std::vector<ossimRefPtr<ossimProperty> > properties;
      theSource->getPropertyList(properties);
      
      ossimRefPtr<ossimXmlNode> nativeProperties = new ossimXmlNode;
      
      nativeProperties->setTag("NativeProperties");
      
      for(idx = 0; idx < properties.size(); ++idx)
      {
         ossimRefPtr<ossimXmlNode> xmlNode = properties[idx]->toXml();
         if(xmlNode.valid())
         {
            nativeProperties->addChildNode(xmlNode.get());
         }
      }
      
      metadata->addChildNode(nativeProperties.get());
      
      ossimRefPtr<ossimXmlNode> modelNode = new  ossimXmlNode();
      
      modelNode->setTag("Model");
      ossimKeywordlist kwl;
      ossimRefPtr<ossimImageGeometry> geom = theSource->getImageGeometry();
      if(geom.valid())
      {
         if(geom->getProjection())
         {
            geom->getProjection()->saveState(kwl);
         }
         ossimKeywordlist::KeywordMap::const_iterator iter = kwl.getMap().begin();
         while(iter != kwl.getMap().end())
         {
            ossimRefPtr<ossimXmlNode> modelChildNode = new ossimXmlNode();
            modelChildNode->setTag(iter->first);
            modelChildNode->setText(iter->second);
            modelNode->addChildNode(modelChildNode.get());
            ++iter;
         }
         ossimString type = kwl.find(ossimKeywordNames::TYPE_KW);
         
         type = type.substitute("ossim", "");
         
         modelNode->setText(type);
      }
      metadata->addChildNode(modelNode.get());
   }
   
}

ossimRefPtr<ossimXmlNode> ossimPlanetOssimImageLayer::saveXml(bool /*recurseFlag*/)const
{
   ossimRefPtr<ossimXmlNode> result = ossimPlanetTextureLayer::saveXml();
   
   result->addChildNode("filename", theFilename);
   if(theSource.valid())
   {
      result->addChildNode("entry", ossimString::toString(theSource->getCurrentEntry()));
   }
   if(!theOverviewFile.empty())
   {
      result->addChildNode("overviewFilename", theOverviewFile);
   }
   result->addChildNode("histogramStretchMode", histogramStretchMode());
   result->addChildNode("histogramStretchEnabled", ossimString::toString(theHistogramStretchEnableFlag));
   
   return result;
}

bool ossimPlanetOssimImageLayer::loadXml(ossimRefPtr<ossimXmlNode> node)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOssimImageLayerMutex);
   blockCallbacks(true);
   bool result = false;
   ossimRefPtr<ossimXmlNode> filenameNode         = node->findFirstNode("filename");
   ossimRefPtr<ossimXmlNode> overviewFilenameNode = node->findFirstNode("overviewFilename");
   ossimRefPtr<ossimXmlNode> entryNode            = node->findFirstNode("entry");
   ossimRefPtr<ossimXmlNode> histogramStretchMode = node->findFirstNode("histogramStretchMode");
   ossimRefPtr<ossimXmlNode> histogramStretchEnabled = node->findFirstNode("histogramStretchEnabled");
   if(overviewFilenameNode.valid())
   {
      setOverviewFile(ossimFilename(overviewFilenameNode->getText()));
   }
   else
   {
      setOverviewFile(ossimFilename(""));
   }
   result = ossimPlanetTextureLayer::loadXml(node);
   if(histogramStretchEnabled.valid())
   {
      theHistogramStretchEnableFlag = histogramStretchEnabled->getText().toBool();
   }
   if(filenameNode.valid()&&result)
   {
      ossimFilename filename(filenameNode->getText());
      ossim_int32 entryIdx = -1;
      
      if(entryNode.valid())
      {
         entryIdx = entryNode->getText().toInt32();
      }
      
      openImage(filename, entryIdx);
      result = !isStateSet(ossimPlanetTextureLayer_NO_SOURCE_DATA);
   }
   else
   {
      setState(ossimPlanetTextureLayer_NO_SOURCE_DATA);
      result = false;
   }
   if(histogramStretchMode.valid())
   {
      setHistogramStretchMode(histogramStretchMode->getText());
   }
   blockCallbacks(false);
   notifyPropertyChanged("stateCode", this);
   return result;
}

void ossimPlanetOssimImageLayer::initializeResamplePoints(const ossimPlanetTerrainTileId& tileId,
                                                          const ossimPlanetGrid& grid,
                                                          ossim_uint32 tileWidth,
                                                          ossim_uint32 tileHeight,
                                                          ResampleCorners& corners)
{
   ossimPlanetGrid::GridBound tileBound;
   grid.bounds(tileId, tileBound);
   ossimDrect tileBoundDrect = tileBound.toDrect();
   
   // need to add elevation lookups if the projection is affected by elevation
   // We will do that in a bit after we get it working
   //
   // counter clockwise right handed lower left origin.  This will be openGL style texture
   // ordering where 0, 0 is lower left
   //
   ossimPlanetGrid::ModelPoint p1, p2, p3, p4;
   grid.globalGridToModel(ossimPlanetGrid::GridPoint(tileId.face(),
                                                     tileBoundDrect.ll().x,
                                                     tileBoundDrect.ll().y),
                          p1);
   grid.globalGridToModel(ossimPlanetGrid::GridPoint(tileId.face(),
                                                     tileBoundDrect.lr().x,
                                                     tileBoundDrect.lr().y),
                          p2);
   grid.globalGridToModel(ossimPlanetGrid::GridPoint(tileId.face(),
                                                     tileBoundDrect.ur().x,
                                                     tileBoundDrect.ur().y),
                          p3);
   grid.globalGridToModel(ossimPlanetGrid::GridPoint(tileId.face(),
                                                     tileBoundDrect.ul().x,
                                                     tileBoundDrect.ul().y),
                          p4);
   
   ossimDpt imagePoint1, imagePoint2, imagePoint3, imagePoint4;
   theInputProjection->worldToLineSample(ossimGpt(p1.y(), p1.x()),
                                         imagePoint1);
   theInputProjection->worldToLineSample(ossimGpt(p2.y(), p2.x()),
                                         imagePoint2);
   theInputProjection->worldToLineSample(ossimGpt(p3.y(), p3.x()),
                                         imagePoint3);
   theInputProjection->worldToLineSample(ossimGpt(p4.y(), p4.x()),
                                         imagePoint4);
   
   osg::Vec2d gsd;
   grid.widthHeightInModelSpace(tileId, gsd);
   gsd[0]/=tileWidth;
   gsd[1]/=tileHeight;
   ossimDpt mpd = ossimGpt().metersPerDegree();
   gsd[0] *= mpd.x;
   gsd[1] *= mpd.y;
   
   ossimDpt metersPerPixel = theInputProjection->getMetersPerPixel();
   ossimDpt decimationFactor;
   decimationFactor.x = (metersPerPixel.y/gsd[0] + metersPerPixel.y/gsd[1])*.5;
   decimationFactor.y = decimationFactor.x;
   //double levels = theSource->getNumberOfDecimationLevels();
   
   double factor = 1.0;
   ossim_uint32 resLevel = 0;
   if(decimationFactor.x < 1.0)
   {
      // always use the higher level for now
      while(factor*.5 > decimationFactor.x)
      {
         factor *= .5;
         ++resLevel;
      }
   }
   ResamplePoint resample1(ossimPlanetGrid::GridPoint(tileId.face(),
                                                      tileBoundDrect.ll().x,
                                                      tileBoundDrect.ll().y),
                           p1,
                           imagePoint1,
                           ossimDpt(0.0,0.0),
                           decimationFactor,
                           resLevel);
   
   ResamplePoint resample2(ossimPlanetGrid::GridPoint(tileId.face(),
                                                      tileBoundDrect.lr().x,
                                                      tileBoundDrect.lr().y),
                           p2,
                           imagePoint2,
                           ossimDpt(tileWidth-1,0),
                           decimationFactor,
                           resLevel);
   ResamplePoint resample3(ossimPlanetGrid::GridPoint(tileId.face(),
                                                      tileBoundDrect.ur().x,
                                                      tileBoundDrect.ur().y),
                           p3,
                           imagePoint3,
                           ossimDpt(tileWidth-1,tileHeight-1),
                           decimationFactor,
                           resLevel);
   ResamplePoint resample4(ossimPlanetGrid::GridPoint(tileId.face(),
                                                      tileBoundDrect.ul().x,
                                                      tileBoundDrect.ul().y),
                           p4,
                           imagePoint4,
                           ossimDpt(0,tileHeight-1),
                           decimationFactor,
                           resLevel);
   
                        
   corners.push(ResampleCorner(resample1, resample2, resample3, resample4));
   
}
