#include <ossimPlanet/ossimPlanetLand.h>
#include <osgUtil/CullVisitor>
#include <osgDB/DatabasePager>
#include <osgDB/Registry>
#include <osgGA/EventVisitor>
#include <ossimPlanet/ossimPlanetGridUtility.h>
#include <ossimPlanet/ossimPlanetPagedLandLod.h>
#include <ossimPlanet/ossimPlanetDatabasePager.h>
#include <ossimPlanet/ossimPlanetTextureLayerGroup.h>
#include <ossimPlanet/ossimPlanetLandReaderWriter.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossim/base/ossimFilename.h>
#include <iostream>
#include <osg/GL2Extensions>
#include <ossimPlanet/ossimPlanetSrtmElevationDatabase.h>
#include <ossim/elevation/ossimElevSourceFactory.h>
#include <ossimPlanet/ossimPlanetElevationRegistry.h>
#include <ossimPlanet/ossimPlanetSrtmElevationDatabase.h>
#include <ossimPlanet/ossimPlanetDtedElevationDatabase.h>
#include <ossimPlanet/ossimPlanetGeneralRasterElevationDatabase.h>
#include <ossimPlanet/ossimPlanetActionRouter.h>
#include <ossimPlanet/ossimPlanetTextureLayerRegistry.h>

class ossimPlanetLandUpdateCallback : public osg::NodeCallback
{
public:
   virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
      {
         ossimPlanetLand* n = dynamic_cast<ossimPlanetLand*>(node);
         if(n)
         {
            n->traverse(*nv);

            return;
         }
      }
};

#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
static ossim_uint32 allocated = 0;
#endif

class ossimPlanetLandRefreshVisitor : public osg::NodeVisitor
{
public:
   ossimPlanetLandRefreshVisitor(ossimPlanetLand* land)
      :osg::NodeVisitor(NODE_VISITOR,
                        TRAVERSE_ALL_CHILDREN),
       theLand(land)
      {
         theGrid = theLand->theReaderWriter->gridUtility();
         theW = theGrid->getTileWidth();
         theH = theGrid->getTileHeight();
      }
   virtual void apply(osg::Group& node)
      {
         ossimPlanetPagedLandLod* lod = dynamic_cast<ossimPlanetPagedLandLod*>(&node);
         if(lod)
         {
            double deltaX;
            double deltaY;
            theGrid->getWidthHeightInDegrees(deltaX,
                                             deltaY,
                                             lod->getLevel(),
                                             lod->getRow(),
                                             lod->getCol());
            double minLat, minLon, maxLat, maxLon;
            double deltaLat    = deltaY/theH;
//            double deltaLon    = deltaX/theW;
            ossimDpt gsd = ossimGpt().metersPerDegree();
            gsd.y *= deltaLat;
            theGrid->getLatLonBounds(minLat,
                                     minLon,
                                     maxLat,
                                     maxLon,
                                     lod->getLevel(),
                                     lod->getRow(),
                                     lod->getCol());
            osg::ref_ptr<ossimPlanetLand::refreshInfo> info = intersects(minLat, minLon, maxLat, maxLon, gsd.y);
            if(info.valid())
            {
               lod->setRefreshType((ossimPlanetLandRefreshType)(lod->refreshType()|info->theRefreshType));
            }
         }
         osg::NodeVisitor::apply(node);
      }
   osg::ref_ptr<ossimPlanetLand::refreshInfo> intersects(double minLat, double minLon, double maxLat, double maxLon,
                                        double gsd)const
      {
         ossim_uint32 idx = 0;
         ossim_uint32 size = theLand->theExtentRefreshList.size();
         
         for(idx = 0; idx < size; ++idx)
         {
            if(theLand->theExtentRefreshList[idx]->theExtents->intersectsLatLon(minLat, minLon, maxLat, maxLon)&&
               theLand->theExtentRefreshList[idx]->theExtents->intersectsScale(gsd, gsd))
            {
               return theLand->theExtentRefreshList[idx].get();
            }
         }

         return 0;
      }
protected:
   ossimPlanetLand* theLand;
   const ossimPlanetGridUtility* theGrid;
   ossim_uint32 theW;
   ossim_uint32 theH;
};

class ossimPlanetLandTextureCallback : public ossimPlanetTextureLayerCallback
{
public:
   ossimPlanetLandTextureCallback(ossimPlanetLand* land, ossimPlanetLandRefreshType refreshType)
      :theLand(land),
       theRefreshType(refreshType)
      {
      }
   
   void setLand(ossimPlanetLand* land)
      {
         theLand = land;
      }
   virtual void refreshExtent(osg::ref_ptr<ossimPlanetExtents> extent)
      {
         if(theLand)
         {
            theLand->resetGraph(extent, theRefreshType);
         }
      }
   virtual void layerAdded(osg::ref_ptr<ossimPlanetTextureLayer> layer)
      {
         if(theLand)
         {
            theLand->resetGraph(layer->getExtents(), theRefreshType);
         }
      }
   virtual void layerRemoved(osg::ref_ptr<ossimPlanetTextureLayer> layer,
                             osg::ref_ptr<ossimPlanetTextureLayer> /*parent*/)
      {
         if(theLand)
         {
            // only refresh if it was originally enabled
            if(layer->getEnableFlag())
            {
               refreshExtent(layer->getExtents());
            }
         }         
      }
//    virtual bool addRequest(osg::ref_ptr<ossimPlanetTextureLayer::TextureRequest> request)
//       {
//          return false;
//       }
  
protected:
   ossimPlanetLand* theLand;
   ossimPlanetLandRefreshType theRefreshType;
};

ossimPlanetLand::ossimPlanetLand()
{
   theShadersInitializedFlag = false;
//   setPathnameAndRegister(":land");
//    ossim_uint64 megaByte     = 1024*1024; 
//    ossim_uint64 gigaByte     = 1024*megaByte; 
//    ossim_uint64 maxCacheSize = megaByte*2;
   theTextureLayerCallback = new ossimPlanetLandTextureCallback(this, ossimPlanetLandRefreshType_TEXTURE);
   theElevationLayerCallback = new ossimPlanetLandTextureCallback(this, ossimPlanetLandRefreshType_PRUNE);
   //theLandCache = new ossimPlanetLandCache(maxCacheSize, (ossim_uint64)(maxCacheSize*.85));
   theCurrentShaderProgram = new ossimPlanetShaderProgramSetup();
   theLandCache = new ossimPlanetLandCache(0,0);
   theReferenceLayer = new ossimPlanetTextureLayerGroup;
   theOverlayLayers  = new ossimPlanetTextureLayerGroup;
   theReferenceLayer->setName("Reference");
   theOverlayLayers->setName("Overlay");
   theReaderWriter = new ossimPlanetLandReaderWriter();
   theReaderWriter->setMultiTextureEnableFlag(false);
   theReaderWriter->setReferenceLayer(theReferenceLayer.get());
   theReaderWriter->setOverlayLayers(theOverlayLayers);
   theReaderWriter->setLandCache(theLandCache.get());
   initElevation();
   theReaderWriter->setElevationDatabase(theElevationDatabase.get());
//    theReaderWriter->setShaderProgram(theCurrentShaderProgram.get());
   osgDB::Registry::instance()->addReaderWriter(theReaderWriter.get());
//    theGraphResetFlag = true;
   setUpdateCallback(new ossimPlanetLandUpdateCallback);
   theCullCallback = new ossimPlanetLandCullCallback();
   theReaderWriter->setLandNodeCullCallback(theCullCallback.get());
   theReferenceLayer->addCallback(theTextureLayerCallback);
   theOverlayLayers->addCallback(theTextureLayerCallback);
   theElevationDatabase->addCallback( theElevationLayerCallback );
   
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   ++allocated;
   std::cout << "ossimPlanetLand count++: " << allocated << std::endl;
#endif
}

ossimPlanetLand::~ossimPlanetLand()
{
   ((ossimPlanetLandTextureCallback*) theTextureLayerCallback.get())->setLand(0);
   if(theReferenceLayer.valid())
   {
      theReferenceLayer->removeCallback(theTextureLayerCallback);
   }
   if(theOverlayLayers.valid())
   {
      theOverlayLayers->removeCallback(theTextureLayerCallback);
   }
   if( theElevationDatabase.valid())
   {
      theElevationDatabase->removeCallback( theTextureLayerCallback );
   }
   theStateSet = 0;
  // OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   osgDB::Registry::instance()->removeReaderWriter(theReaderWriter.get());
//    theNeighborhoodGraph = 0;
   theReaderWriter      = 0;
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   --allocated;
   std::cout << "ossimPlanetLand count--: " << allocated << std::endl;
#endif
}

void ossimPlanetLand::traverse(osg::NodeVisitor& nv)
{
//    static osg::Timer_t lastTick = osg::Timer::instance()->tick();
   //OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if((theCurrentShaderProgram.valid())&&(!theGL2Extensions.valid()))//&&
            //(theCurrentShaderProgram->fragmentType() != ossimPlanetShaderProgramSetup::NO_SHADER))
         {
            if(!theStateSet.valid())
            {
               theStateSet = getOrCreateStateSet();
               setStateSet(theStateSet.get());
               initShaders();
            }
         }
         if(!getNumChildren())
         {
            resetGraphLocal();
         }
         {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theRefreshMutex);
            if(theExtentRefreshList.size())
            {
#if 0
               osgDB::DatabasePager* pager = dynamic_cast<osgDB::DatabasePager*>(nv.getDatabaseRequestHandler());
               
               
               if(pager)
               {
                   pager->setAcceptNewDatabaseRequests(false);
                   pager->clear();
                   pager->setDatabasePagerThreadPause(true);
                  
                  ossimPlanetLandRefreshVisitor refreshVisitor(this);
                  ossim_uint32 idx = 0;
                  for(idx = 0; idx < getNumChildren(); ++idx)
                  {
                     getChild(idx)->accept(refreshVisitor);
                  }
                  pager->setAcceptNewDatabaseRequests(true);
                  pager->setDatabasePagerThreadPause(false);

                  for(idx = 0; idx < theExtentRefreshList.size(); ++idx)
                  {
                     theLandCache->clearTexturesWithinExtents(theExtentRefreshList[idx]->theExtents.get());
                  }
               }
#else
               osgDB::DatabasePager* pager = dynamic_cast<osgDB::DatabasePager*>(nv.getDatabaseRequestHandler());
               if(pager)
               {
                  pager->setAcceptNewDatabaseRequests(false);
                  pager->clear();
                  pager->setDatabasePagerThreadPause(true);
               }
               ossimPlanetLandRefreshVisitor refreshVisitor(this);
               ossim_uint32 idx = 0;
               for(idx = 0; idx < getNumChildren(); ++idx)
               {
                  getChild(idx)->accept(refreshVisitor);
               }
               for(idx = 0; idx < theExtentRefreshList.size(); ++idx)
               {
                  theLandCache->clearTexturesWithinExtents(theExtentRefreshList[idx]->theExtents.get());
               }
               if(pager)
               {
                  pager->setAcceptNewDatabaseRequests(true);
                  pager->setDatabasePagerThreadPause(false);
               }
#endif
               theExtentRefreshList.clear();
               setRedrawFlag(true);
            }
         }
         ossimPlanetLayer::traverse(nv);
         
	break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
         osgUtil::CullVisitor* cullVisitor = dynamic_cast<osgUtil::CullVisitor*>(&nv);
         if(cullVisitor)
         {
            osg::CullSettings::CullingMode mode = cullVisitor->getCullingMode();
            
            ossimPlanetLayer::traverse(nv);

            cullVisitor->setCullingMode(mode);
         }
         else
         {
            
            ossimPlanetLayer::traverse(nv);
         }
         break;
      }
      case osg::NodeVisitor::EVENT_VISITOR:
      {
#if 0
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theRefreshMutex);
         if(theRedrawFlag)
         {
            osgGA::EventVisitor* ev = dynamic_cast<osgGA::EventVisitor*>(&nv);
            if(ev)
            {
               const osgGA::EventVisitor::EventList& eventList = ev->getEvents();
               if(eventList.size())
               {
                  osgGA::EventVisitor::EventList::const_iterator iter = eventList.begin();
                  if(((*iter)->getEventType() == osgGA::GUIEventAdapter::FRAME)&&ev->getActionAdapter())
                  {
                     ev->getActionAdapter()->requestRedraw();
                     theRedrawFlag = false;
                  }
               }
            }
         }
#endif
         break;
      }
      default:
      {
         ossimPlanetLayer::traverse(nv);
         break;
      }
   }
}

void ossimPlanetLand::setLineOfSiteIntersection(const osg::Vec3d& pt)
{
   if(theCullCallback.valid())
   {
      theCullCallback->setLineOfSite(pt);
   }
}

void ossimPlanetLand::setModel(ossimPlanetGeoRefModel* model)
{
   ossimPlanetLayer::setModel(model);
   resetGraph();
   theCullCallback = theCullCallback->clone();
   theReaderWriter->setLandNodeCullCallback(theCullCallback.get());
   theReaderWriter->setModel(model);
}

bool ossimPlanetLand::getElevationEnabledFlag()const
{
   return theReaderWriter->getElevationEnabledFlag();
}

void ossimPlanetLand::setElevationEnabledFlag(bool elevationEnabledFlag)
{
   resetGraph();
   theReaderWriter->setElevationEnabledFlag(elevationEnabledFlag);
}

ossim_float64 ossimPlanetLand::getHeightExag()const
{
   return theReaderWriter->getHeightExag();
}

void ossimPlanetLand::setHeightExag(ossim_float64 heightExag)
{
   resetGraph();
   theReaderWriter->setHeightExag(heightExag);
}

ossim_uint32 ossimPlanetLand::getElevationPatchSize()const
{
   return theReaderWriter->getElevationPatchSize();
}

void ossimPlanetLand::setElevationPatchSize(ossim_uint32 patchSize)
{
   resetGraph();
   theReaderWriter->setElevationPatchSize(patchSize);
}

void ossimPlanetLand::setReaderWriter(ossimPlanetLandReaderWriter* /*readerWriter*/)
{
}

ossimPlanetLandReaderWriter* ossimPlanetLand::getReaderWriter()
{
	return theReaderWriter.get();
}

const ossimPlanetLandReaderWriter* ossimPlanetLand::getReaderWriter()const
{
	return theReaderWriter.get();
}

ossim_uint32 ossimPlanetLand::getMaxLevelDetail()const
{
   return theReaderWriter->getMaxLevelDetail();
}

void ossimPlanetLand::setMaxLevelDetail(ossim_uint32 maxLevelDetail)
{
   resetGraph();
   theReaderWriter->setMaxLevelDetail(maxLevelDetail);
}

ossimFilename ossimPlanetLand::getElevationCacheDir()const
{
   return theReaderWriter->getElevationCacheDir();
}

void ossimPlanetLand::setElevationCacheDir(const ossimFilename& cacheDir)
{
   resetGraph();
   theReaderWriter->setElevationCacheDir(cacheDir);
}

void ossimPlanetLand::resetGraph(const osg::ref_ptr<ossimPlanetExtents> extents,
                                 ossimPlanetLandRefreshType refreshType)

{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theRefreshMutex);
   
   osg::ref_ptr<ossimPlanetLand::refreshInfo> info = new ossimPlanetLand::refreshInfo;
   info->theRefreshType = refreshType;

   if(extents.valid())
   {
      info->theExtents = extents->clone();
   }
   else
   {
      info->theExtents = new ossimPlanetExtents();
   }
   ossim_uint32 idx = 0;
   bool found = false;
   for(idx = 0; idx < theExtentRefreshList.size(); ++idx)
   {
      if(theExtentRefreshList[idx]->theExtents->equal(info->theExtents))
      {
         theExtentRefreshList[idx]->theRefreshType = (ossimPlanetLandRefreshType)(theExtentRefreshList[idx]->theRefreshType |
                                                                                  refreshType);
         found = true;
      }
   }
   if(!found)
   {
      theExtentRefreshList.push_back(info);
   }
   setRedrawFlag(true);
}

void ossimPlanetLand::resetGraphLocal()
{
   theExtentRefreshList.clear();
   bool hasPagedLods = false;

   ossim_uint32 idx = 0;
   for(;((idx < getNumChildren())&&(!hasPagedLods));++idx)
   {
      hasPagedLods = (dynamic_cast<ossimPlanetPagedLandLod*>(getChild(idx))!= 0);
   }

   if(!hasPagedLods)
   {
      bool addedChild = false;
      ossim_uint32 faceCount = theReaderWriter->gridUtility()->getNumberOfFaces();
      ossim_uint32 idx = 0;
      for(idx = 0; idx < faceCount; ++idx)
      {
         osg::ref_ptr<osg::Node> node = theReaderWriter->readNode(theReaderWriter->createDbString(0,0,idx), 0).getNode();
         if(node.valid())
         {
            addedChild = true;
            addChild(node.get());
         }
      }
   }
   else
   {
     for(idx = 0;idx < getNumChildren();++idx)
      {
         ossimPlanetPagedLandLod* lod = dynamic_cast<ossimPlanetPagedLandLod*>(getChild(idx));
         if(lod)
         {
            lod->setRefreshType(ossimPlanetLandRefreshType_PRUNE);
         }
      }
   }
   setRedrawFlag(true);
}

const osg::ref_ptr<ossimPlanetGeoRefModel> ossimPlanetLand::model()const
{
   
   if(theReaderWriter.valid())
   {
      return theReaderWriter->model();
   }

   return 0;
}


void ossimPlanetLand::accept(osg::NodeVisitor& nv)
{
   if(nv.validNodeMask(*this))
   {
      nv.pushOntoNodePath(this);
      nv.apply(*this);
      nv.popFromNodePath();
   }
}

bool ossimPlanetLand::addChild( Node *child )
{
   setRedrawFlag(true);
   return Group::addChild(child);
}

// ossim_uint32 ossimPlanetLand::getNumberOfTextureLayers()const
// {
//    return theTextureLayers->getNumberOfLayers();
// }


// void ossimPlanetLand::setTextureLayer(osg::ref_ptr<ossimPlanetTextureLayer> textureLayer,
//                                       ossim_uint32 idx)
// {
//    ossim_uint32 nLayers = getNumberOfTextureLayers();
//    if(idx == nLayers)
//    {
//       theTextureLayers->addBottom(textureLayer.get());
//    }
//    else if(idx < nLayers)
//    {
//       theTextureLayers->replaceLayer(idx, textureLayer.get());
//    }
//    resetGraph();
// }

// osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetLand::getTextureLayer(ossim_uint32 idx)
// {
//    return theTextureLayers->getLayer(idx);
// }

// const osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetLand::getTextureLayer(ossim_uint32 idx)const
// {
//    return theTextureLayers->getLayer(idx);
// }

// osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetLand::removeTextureLayer(ossim_uint32 idx)
// {
//    return theTextureLayers->removeLayer(idx);
// }

ossim_uint32 ossimPlanetLand::getNumberOfOverlayLayers()const
{
   if(theOverlayLayers.valid())
   {
      return theOverlayLayers->numberOfLayers();
   }

   return 0;
}

void ossimPlanetLand::setReferenceLayer(osg::ref_ptr<ossimPlanetTextureLayerGroup> reference)
{
   if(theReferenceLayer.valid())
   {
      theReferenceLayer->removeCallback(theTextureLayerCallback);

   }
   theReferenceLayer = reference;
   if(theReaderWriter.valid())
   {
      theReaderWriter->setReferenceLayer(theReferenceLayer.get());
   }
   if(theReferenceLayer.valid())
   {
      theReferenceLayer->addCallback(theTextureLayerCallback);
   }
}

osg::ref_ptr<ossimPlanetTextureLayerGroup> ossimPlanetLand::referenceLayer()
{
   return theReferenceLayer.get();
}

const osg::ref_ptr<ossimPlanetTextureLayerGroup> ossimPlanetLand::referenceLayer()const
{
   return theReferenceLayer.get();
}

osg::ref_ptr<ossimPlanetTextureLayerGroup> ossimPlanetLand::overlayLayers()
{
   return theOverlayLayers.get();
}

const osg::ref_ptr<ossimPlanetTextureLayerGroup> ossimPlanetLand::overlayLayers()const
{
   return theOverlayLayers.get();
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetLand::overlayLayer(ossim_uint32 layerIdx)
{
   if(!theOverlayLayers.valid()) return 0;
   if(layerIdx < theOverlayLayers->numberOfLayers())
   {
      return theOverlayLayers->layer(layerIdx);
   }
   
   return 0;
}

const osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetLand::overlayLayer(ossim_uint32 layerIdx)const
{
   if(!theOverlayLayers.valid()) return 0;
   if(layerIdx < theOverlayLayers->numberOfLayers())
   {
      return theOverlayLayers->layer(layerIdx);
   }
   
   return 0;
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetLand::removeOverlayLayer(ossim_uint32 layerIdx)
{
   if(!theOverlayLayers.valid()) return 0;
   return theOverlayLayers->removeLayer(layerIdx);
}

void ossimPlanetLand::setCurrentFragmentShaderType(ossimPlanetShaderProgramSetup::ossimPlanetFragmentShaderType fragType)
{
   if(!theFragShader.valid()||!theCurrentShaderProgram.valid()) return;
   if((theCurrentShaderProgram->fragmentType() == ossimPlanetShaderProgramSetup::NO_SHADER)&&
      (fragType!=ossimPlanetShaderProgramSetup::NO_SHADER))
   {
      theReaderWriter->setMultiTextureEnableFlag(true);
   }
   switch(fragType)
   {
      case ossimPlanetShaderProgramSetup::NO_SHADER:
      {
         theReaderWriter->setMultiTextureEnableFlag(false);
         theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::NO_SHADER);
         theCurrentShaderProgram->setProgram(theNoShaderProgram.get());
         resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
         break;
      }
      case ossimPlanetShaderProgramSetup::TOP:
      {
         theFragShader->setShaderSource(theTopSource);
         theFragShader->dirtyShader();
         theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::TOP);
         theCurrentShaderProgram->setProgram(theLandShaderProgram.get());
         resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
         break;
      }
      case ossimPlanetShaderProgramSetup::REFERENCE:
      {
         theFragShader->setShaderSource(theReferenceSource);
         theFragShader->dirtyShader();
         theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::REFERENCE);
         theCurrentShaderProgram->setProgram(theLandShaderProgram.get());
         resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
         break;
      }
      case ossimPlanetShaderProgramSetup::OPACITY:
      {
         theFragShader->setShaderSource(theOpacitySource);
         theFragShader->dirtyShader();
         theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::OPACITY);
         theCurrentShaderProgram->setProgram(theLandShaderProgram.get());
         resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
         break;
      }
      case ossimPlanetShaderProgramSetup::HORIZONTAL_SWIPE:
      {
         osg::ref_ptr<osg::Uniform> uniformParam = theCurrentShaderProgram->getUniform("swipeType");
         if(uniformParam.valid())
         {
            uniformParam->set((int)0);  
         }
         theFragShader->setShaderSource(theSwipeSource);
         theFragShader->dirtyShader();
         theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::HORIZONTAL_SWIPE);
         theCurrentShaderProgram->setProgram(theLandShaderProgram.get());
         resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
         break;
      }
      case ossimPlanetShaderProgramSetup::VERTICAL_SWIPE:
      {
         osg::ref_ptr<osg::Uniform> uniformParam = theCurrentShaderProgram->getUniform("swipeType");
         if(uniformParam.valid())
         {
            uniformParam->set((int)1);  
         }
         theFragShader->setShaderSource(theSwipeSource);
         theFragShader->dirtyShader();
         theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::VERTICAL_SWIPE);
         theCurrentShaderProgram->setProgram(theLandShaderProgram.get());
         resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
         break;
      }
      case ossimPlanetShaderProgramSetup::BOX_SWIPE:
      {
         osg::ref_ptr<osg::Uniform> uniformParam = theCurrentShaderProgram->getUniform("swipeType");
         if(uniformParam.valid())
         {
            uniformParam->set((int)2);  
         }
         theFragShader->setShaderSource(theSwipeSource);
         theFragShader->dirtyShader();
         theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::BOX_SWIPE);
         theCurrentShaderProgram->setProgram(theLandShaderProgram.get());
         resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
         break;
      }
      case ossimPlanetShaderProgramSetup::CIRCLE_SWIPE:
      {
         osg::ref_ptr<osg::Uniform> uniformParam = theCurrentShaderProgram->getUniform("swipeType");
         if(uniformParam.valid())
         {
            uniformParam->set((int)3);  
         }
         theFragShader->setShaderSource(theSwipeSource);
         theFragShader->dirtyShader();
         theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::CIRCLE_SWIPE);
         theCurrentShaderProgram->setProgram(theLandShaderProgram.get());
         resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
         break;
      }
      case ossimPlanetShaderProgramSetup::ABSOLUTE_DIFFERENCE:
      {
         theFragShader->setShaderSource(theAbsoluteDifferenceSource);
         theFragShader->dirtyShader();
         theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::ABSOLUTE_DIFFERENCE);
         theCurrentShaderProgram->setProgram(theLandShaderProgram.get());
         resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);

         break;
      }
      case ossimPlanetShaderProgramSetup::FALSE_COLOR_REPLACEMENT:
      {
         theFragShader->setShaderSource(theFalseColorReplacementSource);
         theFragShader->dirtyShader();
         theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::FALSE_COLOR_REPLACEMENT);
         theCurrentShaderProgram->setProgram(theLandShaderProgram.get());
         resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
         break;
      }
      default:
      {
         break;
      }
   }// end switch
   if(!theStateSet.valid())
   {
	theStateSet = getOrCreateStateSet();
   }
   theStateSet->setAttribute(theCurrentShaderProgram->getProgram());
   setRedrawFlag(true);
}

void ossimPlanetLand::setCacheSize(ossim_uint64 maxSize, ossim_uint64 minSize)
{
   theLandCache->setCacheSize(maxSize, minSize);
}

ossim_uint64 ossimPlanetLand::getCacheSize()const
{
   return theLandCache->getCacheSize();
}


void ossimPlanetLand::setSplitMetricRatio(double ratio)
{
   theCullCallback->setSplitMetricRatio(ratio);
   resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
}

double ossimPlanetLand::getSplitMetricRatio()const
{
   return theCullCallback->getSplitMetricRatio();

}

void ossimPlanetLand::setSplitPriorityType(ossimPlanetPriorityType priorityType)
{
   theCullCallback->setSplitPriorityType(priorityType);
}

ossimPlanetPriorityType ossimPlanetLand::getSplitPrioirtyType()const
{
   return theCullCallback->getSplitPriorityType();
}

void ossimPlanetLand::setCullingFlag(bool flag)
{
   theCullCallback->setCullingFlag(flag);
}

bool ossimPlanetLand::getCullingFlag()const
{
   return theCullCallback->getCullingFlag();
}

void ossimPlanetLand::setFreezeRequestFlag(bool flag)
{
   theCullCallback->setFreezeRequestFlag(flag);
}

bool ossimPlanetLand::getFreezRequestFlag()const
{
   return theCullCallback->getFreezRequestFlag();
}

bool ossimPlanetLand::getMipMappingFlag()const
{
   return theReaderWriter->getMipMappingFlag();
}

void ossimPlanetLand::setMipMappingFlag(bool flag)
{
   theReaderWriter->setMipMappingFlag(flag);
   resetGraph(0, ossimPlanetLandRefreshType_TEXTURE);
}

void ossimPlanetLand::pagedLodAdded(osg::Node* /*parent*/ , osg::Node* /*child*/)
{
   setRedrawFlag(true);
}

void ossimPlanetLand::pagedLodRemoved(osg::Node* /*node*/)
{
   setRedrawFlag(true);
}

void ossimPlanetLand::pagedLodModified(osg::Node* /*node*/)
{
   setRedrawFlag(true);
}

osg::ref_ptr<ossimPlanetShaderProgramSetup> ossimPlanetLand::getCurrentShader()
{
   return theCurrentShaderProgram.get();
}

void ossimPlanetLand::initShaders()
{
   theGL2Extensions = new osg::GL2Extensions(0);
   theShadersInitializedFlag = false;
   if(!theGL2Extensions->isGlslSupported())
   {
      theCurrentShaderProgram = 0;
      theReaderWriter->setMultiTextureEnableFlag(false);
      return;
   }
   theShadersInitializedFlag = true;
   theReaderWriter->setMultiTextureEnableFlag(true);
   theNoShaderProgram = new osg::Program;
   
   theCurrentShaderProgram = new ossimPlanetShaderProgramSetup;
   theLandShaderProgram = new osg::Program;
   theCurrentShaderProgram->setProgram(theLandShaderProgram.get());
   ossimFilename file = ossimEnvironmentUtility::instance()->getUserOssimSupportDir();
   ossimFilename shaders = file.dirCat("planet");
   shaders = shaders.dirCat("shaders");
   if(!shaders.exists())
   {
      shaders = ossimEnvironmentUtility::instance()->getInstalledOssimSupportDir();
      shaders = shaders.dirCat("planet");
      shaders = shaders.dirCat("shaders");
   }
   bool vertexShaderExists = false;
   if(shaders.exists())
   {
      ossimFilename vertShader = shaders.dirCat("land.vert");
      if(vertShader.exists())
      {
         std::vector<char> vertBuf(vertShader.fileSize());
         std::ifstream in(vertShader.c_str());

         in.read(&vertBuf.front(), vertBuf.size());
         theLandShaderProgram->addShader(new osg::Shader(osg::Shader::VERTEX, &vertBuf.front()));
         vertexShaderExists = true;
      }
      ossimFilename fragFunctions = shaders.dirCat("land.frag");
      if(fragFunctions.exists())
      {
         std::vector<char> buf(fragFunctions.fileSize());
         std::ifstream in(fragFunctions.c_str());

         in.read(&buf.front(), buf.size());
         theLandShaderProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, &buf.front()));
      }
   }
   if(!vertexShaderExists)
   {
      char vertexShaderSource[] = 
         "varying vec2 projectedPosition;\n"
         "varying vec4 position;\n"
         "varying vec3 normal;\n"
         "void main(void)\n"
         "{\n"
         "\n"
         "   vec4 projPosition = gl_ModelViewProjectionMatrix*gl_Vertex;\n"
         "   projectedPosition = vec2(projPosition[0]/projPosition[2],projPosition[1]/projPosition[2]);\n"
         "   gl_TexCoord[0] = gl_MultiTexCoord0;\n"
         "   gl_Position = ftransform(); \n"
         "   position = gl_Position; \n"
         "   normal = gl_NormalMatrix*gl_Normal; \n"
        "}\n";
      theLandShaderProgram->addShader(new osg::Shader(osg::Shader::VERTEX, vertexShaderSource));
   }
   theFalseColorReplacementSource =
      "uniform sampler2D referenceTexture; \n"
      "uniform sampler2D topTexture; \n"
      "vec4 simpleShader(vec4 color);\n"
      "void main(void) \n"
      "{\n"
      "    vec4 color;\n"
      "    vec4 texa = vec4(0.0,0.0,0.0,0.0);\n"
      "    vec4 texb = vec4(0.0,0.0,0.0,0.0);\n"
      "    float weight = 0.0;\n"
      "    texa = texture2D(referenceTexture, gl_TexCoord[0].st);\n"
      "    texb = texture2D(topTexture, gl_TexCoord[0].st);\n"
      "    color[0] = texa[0];\n"
      "    color[1] = 0.0;\n"
      "    color[2] = texb[0];\n"
      "    color[3] = 1.0;\n"
      "    if(texb[3] < .000001)\n"
      "    {\n"
      "         color = texa;\n"
      "    }\n"
     "    gl_FragColor = (color);\n"
      "}\n";
   theAbsoluteDifferenceSource =
      "uniform sampler2D referenceTexture; \n"
      "uniform sampler2D topTexture; \n"
      "\n"
      "void main(void) \n"
      "{\n"
      "    vec4 color;\n"
      "    vec4 texa = vec4(0.0,0.0,0.0,0.0);\n"
      "    vec4 texb = vec4(0.0,0.0,0.0,0.0);\n"
      "    texa = texture2D(referenceTexture, gl_TexCoord[0].st);\n"
      "    texb = texture2D(topTexture, gl_TexCoord[0].st);\n"
      "    color = texa-texb;\n"
      "    if(texb[3] > .000001)\n"
      "    {\n"
      "       color[0] = abs(color[0]);\n"
      "       color[1] = abs(color[1]);\n"
      "       color[2] = abs(color[2]);\n"
      "       color[3] = 1.0;\n"
      "    }\n"
      "    else\n"
      "       color = texa;\n"
      "    gl_FragColor = color;\n"
      "}\n";
   
      theTopSource = 
         "uniform sampler2D topTexture; \n"
         "\n"
         "void main(void) \n"
         "{\n"
         "    vec4 texa = vec4(0.0,0.0,0.0,0.0);\n"
         "    gl_FragColor = texture2D(topTexture, gl_TexCoord[0].st);\n"
         "}\n";
      theReferenceSource = 
         "uniform sampler2D referenceTexture; \n"
         "\n"
         "void main(void) \n"
         "{\n"
         "    vec4 texa = vec4(0.0,0.0,0.0,0.0);\n"
         "    gl_FragColor = texture2D(referenceTexture, gl_TexCoord[0].st);\n"
         "}\n";
      
      theOpacitySource = 
         "uniform sampler2D referenceTexture; \n"
         "uniform sampler2D topTexture; \n"
         "uniform float param;\n"
         "\n"
         "void main(void) \n"
         "{\n"
         "    vec4 color;\n"
         "    vec4 texa = vec4(0.0,0.0,0.0,0.0);\n"
         "    vec4 texb = vec4(0.0,0.0,0.0,0.0);\n"
         "    texa = texture2D(referenceTexture, gl_TexCoord[0].st);\n"
         "    texb = texture2D(topTexture, gl_TexCoord[0].st);\n"
         "    if(texb[3] > .1)\n"
         "       if(texa[3] > .1)\n"
         "       {\n"
         "          color = mix(texa, texb, param*texb[3]);\n"
         "          color[3] = 1.0;\n"
         "       }\n"
         "       else\n"
         "          color = texb;\n"
         "    else\n"
         "       color = texa;\n"
         "    gl_FragColor = color;\n"
         "}\n";
      theSwipeSource = 
         "uniform sampler2D referenceTexture; \n"
         "uniform sampler2D topTexture; \n"
         "uniform float param;\n"
         "uniform int swipeType;\n"
         "varying vec2  projectedPosition;\n"
         "\n"
         "void main(void) \n"
         "{\n"
         "    vec4 color;\n"
         "    vec4 texa = vec4(0.0,0.0,0.0,0.0);\n"
         "    vec4 texb = vec4(0.0,0.0,0.0,0.0);\n"
         "    float dist;\n"
         "    bool selected = false;\n"
         "    if(swipeType == 0)\n"
         "    {\n"
         "       dist = (1.0 + projectedPosition[0])/2.0;\n"
         "    }\n"
         "    else if(swipeType == 1)\n"
         "    {\n"
         "       dist = (1.0 + projectedPosition[1])/2.0;\n"
         "    }\n"
         "    else if(swipeType == 2)\n"
         "    {\n"
         "       dist = max(abs(projectedPosition[0]), abs(projectedPosition[1]));\n"
         "    }\n"
         "    else \n"
         "    {\n"
         "       dist = length(projectedPosition);\n"
         "    }\n"
         "    texa = texture2D(referenceTexture, gl_TexCoord[0].st);\n"
         "    texb = texture2D(topTexture, gl_TexCoord[0].st);\n"
         "    if(texb[3] > .000001)\n"
         "       if(texa[3] > .000001)\n"
         "         if(dist < abs(param))\n"
         "            color = texa;\n"
         "         else\n"
         "            color = texb;\n"
         "       else\n"
         "          color = texb;\n"
         "    else\n"
         "       color = texa;\n"
         "    gl_FragColor = color;\n"
         "}\n";
      
      osg::ref_ptr<osg::Uniform> param = new osg::Uniform("param",
                                                          (float)1.0);
      osg::ref_ptr<osg::Uniform> swipeType = new osg::Uniform("swipeType",
                                                               0);
      theReferenceTexture = new osg::Uniform("referenceTexture",0);
      theTopTexture       = new osg::Uniform("topTexture",1);
      theFragShader = new osg::Shader(osg::Shader::FRAGMENT, theReferenceSource);
      theCurrentShaderProgram->addUniform(swipeType.get());
      theCurrentShaderProgram->addUniform(param.get());
      theCurrentShaderProgram->addUniform(theReferenceTexture.get());
      theCurrentShaderProgram->addUniform(theTopTexture.get());
      theLandShaderProgram->addShader(theFragShader.get());   
      theCurrentShaderProgram->setFragmentType(ossimPlanetShaderProgramSetup::REFERENCE);


      theStateSet->addUniform(swipeType.get());
      theStateSet->addUniform(param.get());
      theStateSet->addUniform(theReferenceTexture.get());
      theStateSet->addUniform(theTopTexture.get());
      theStateSet->setAttribute(theCurrentShaderProgram->getProgram());
   
      resetGraph();

}

void ossimPlanetLand::initElevation()
{
   ossim_uint32 idx = 0;
   ossim_uint32 numberOfDatabases = ossimElevManager::instance()->getNumberOfElevationDatabases();
   theElevationDatabase = new ossimPlanetElevationDatabaseGroup;

   for(idx = 0; idx < numberOfDatabases; ++idx)
   {
      const ossimRefPtr<ossimElevationDatabase> database = ossimElevManager::instance()->getElevationDatabase(idx);

      if(database.valid())
      {
         ossimFilename directory = ossimFilename(database->getConnectionString());
         addElevationDirectory(directory, false);
      }
   }
   
   theElevationDatabase->sortByGsd();
      
}

void ossimPlanetLand::addElevationDirectory(const ossimFilename& file, bool sortFlag)
{
   osg::ref_ptr<ossimPlanetElevationDatabase> database = ossimPlanetElevationRegistry::instance()->openDatabase(file);
   addElevation(database.get(), sortFlag);
}

void ossimPlanetLand::addElevation(osg::ref_ptr<ossimPlanetElevationDatabase> database, bool sortFlag)
{
   if(database.valid())
   {
      theElevationDatabase->addBottom(database.get());
   }
   
   if(sortFlag)
   {
      // make sure the highest accuracy is on top
      theElevationDatabase->sortByGsd();
   }   
}

bool ossimPlanetLand::shadersInitialized()const
{
   return theShadersInitializedFlag;
}

void ossimPlanetLand::clearElevation()
{
   if(theElevationDatabase->numberOfLayers() > 0)
   {
      theElevationDatabase->removeLayers(0, theElevationDatabase->numberOfLayers());
   }
}

void ossimPlanetLand::execute(const ossimPlanetAction &a)
{
   const ossimPlanetXmlAction* xmlAction = a.toXmlAction();
	
	if(xmlAction)
	{
		xmlExecute(*xmlAction);
	}
}

void ossimPlanetLand::xmlExecute(const ossimPlanetXmlAction& a)
{

	std::string command = a.command();
	if(a.xmlNode().valid())
	{
		ossim_uint32 idx = 0;
		const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();  
		if(command == "Add")
		{
         for(idx = 0;idx<children.size();++idx)
         {
				ossimString tag = children[idx]->getTag();
				osg::ref_ptr<ossimPlanetTextureLayer> layer;
				osg::ref_ptr<ossimPlanetTextureLayer> namedLayer;
				ossimString parentName  = children[idx]->getChildTextValue("Parent/name");
				ossimString parentId    = children[idx]->getChildTextValue("Parent/id");;
				ossimString description = children[idx]->getChildTextValue("description");
				ossimString id          = children[idx]->getChildTextValue("id");
				ossimString name        = children[idx]->getChildTextValue("name");
				ossimString groupType   = children[idx]->getAttributeValue("groupType");
				if(!id.empty()&&referenceLayer().valid())
				{
					namedLayer = referenceLayer()->findLayerByNameAndId("", id);
					if(namedLayer.valid())
					{
						// do not do duplicate named ID's
						return;
					}
				}
				if(tag == "Image")
				{
					ossimFilename filename  = children[idx]->getChildTextValue("filename");
					layer = ossimPlanetTextureLayerRegistry::instance()->createLayer(filename);
				}
				else if(groupType == "groundTexture")
				{
					layer = new ossimPlanetTextureLayerGroup;
				}
				if(layer.valid()&&referenceLayer().valid())
				{
					if(!parentName.empty()&&!parentId.empty())
					{
						namedLayer = referenceLayer()->findLayerByNameAndId(parentName, parentId);
					}
					else if(!parentName.empty())
					{
						namedLayer = referenceLayer()->findLayerByName(parentName);
					}
					else if(!parentId.empty())
					{
						namedLayer = referenceLayer()->findLayerById(parentId);
					}
					
					layer->setName(name);
					layer->setId(id);
					layer->setDescription(description);
					
					if(namedLayer.valid()&&namedLayer->asGroup())
					{
						ossimPlanetTextureLayerGroup* group = namedLayer->asGroup();
						group->addTop(layer.get());
					}
					else 
					{
						referenceLayer()->addTop(layer.get());
					}
				}
			}
		}
		else if(command == "Remove")
		{
			ossimString id;
			ossimString name;
         ossimString parentId;
			if(children.size() > 0)
			{
				for(idx = 0;idx<children.size();++idx)
				{
               parentId = children[idx]->getAttributeValue("parentId");;
					id       = children[idx]->getAttributeValue("id");
					name     = children[idx]->getAttributeValue("name");
					
					if(id.empty() && name.empty())
					{
						id   = children[idx]->getChildTextValue("id");
						name = children[idx]->getChildTextValue("name");
					}
					if(!id.empty()||
						!name.empty())
					{
						ossimPlanetTextureLayer* parent = 0;
                  if(!parentId.empty())
						{
							parent = referenceLayer()->findLayerById(parentId);
						}
						else 
						{
							parent = referenceLayer().get();
						}
						ossimPlanetTextureLayer* layerToRemove = 0;
						if(parent)
						{
							if(!name.empty()&&!id.empty())
							{
								layerToRemove = parent->findLayerByNameAndId(name, id);
							}
							else if(!name.empty())
							{
								layerToRemove = parent->findLayerByName(name);
							}
							else if(!id.empty())
							{
								layerToRemove = parent->findLayerById(id);
							}
						}
						if(layerToRemove)
						{
							layerToRemove->remove();
						}
					}
					
				}
			}
			else
			{
				id         = a.xmlNode()->getAttributeValue("id");
				name       = a.xmlNode()->getAttributeValue("name");
				ossimPlanetTextureLayer* layerToRemove = 0;
				if(!name.empty()&&!id.empty())
				{
					layerToRemove = referenceLayer()->findLayerByNameAndId(name, id);
				}
				else if(!name.empty())
				{
					layerToRemove = referenceLayer()->findLayerByName(name);
				}
				else if(!id.empty())
				{
					layerToRemove = referenceLayer()->findLayerById(id);
				}
				
				if(layerToRemove)
				{
					layerToRemove->remove();
				}
			}
		}
	}
}

void ossimPlanetLand::addImageObject(const ossimString& /*type*/, const ossimString& /*arg*/)
{
#if 0
   ossimString name;
   ossimString id;
   ossimString description;
   ossimString parentName;
   ossimString parentId;
   ossimString objectName;
   ossimString objectArg;
   ossim_uint32 idx;
   if(type == "Image")
   {
      ossimString filename;
      ossimPlanetAction nestedAction(": dummy " + arg);
      for(idx = 1; idx <= nestedAction.argCount(); ++idx)
      {
         if(mkUtils::extractObjectAndArg(objectName,
                                         objectArg,
                                         nestedAction.arg(idx)))
         {
            if(objectName == "Description")
            {
               description = objectArg;
            }
            else if(objectName == "Name")
            {
               name = objectArg;
            }
            else if(objectName == "Id")
            {
               id = objectArg;
            }
            else if(objectName == "Filename")
            {
               filename = objectArg;
            }
            else if(objectName == "Parent")
            {
               ossimString objectName2;
               ossimString objectArg2;
               ossimPlanetAction nestedParentAction(": dummy " + objectArg);
               ossim_uint32 idx2;
               for(idx2 = 1; idx2 <= nestedParentAction.argCount(); ++idx2)
               {
                  if(mkUtils::extractObjectAndArg(objectName2,
                                                  objectArg2,
                                                  nestedAction.arg(idx)))
                  {
                     if(objectName == "Name")
                     {
                        parentName = objectArg;
                     }
                     else if(objectName == "Id")
                     {
                        parentId = objectArg;
                     }
                  }
               }// end Parent object for loop
            }
         }
      } // end Image object for loop
      if(ossimFilename(filename).exists())
      {
         osg::ref_ptr<ossimPlanetTextureLayer> layer = ossimPlanetTextureLayerRegistry::instance()->createLayer(filename.c_str());
         
         if(layer.valid())
         {
				if(layer->isStateSet(ossimPlanetTextureLayer_NO_SOURCE_DATA)||
					layer->isStateSet(ossimPlanetTextureLayer_NO_GEOM))
				{
					// will need to notify later.
					layer = 0;
					return;
				}
            if(!description.empty())
            {
               layer->setDescription(description);
            }
            if(!name.empty())
            {
               layer->setName(name);
            }
            if(!id.empty())
            {
               layer->setId(id);
            }
            
            if(parentName.empty()&&parentId.empty())
            {
               referenceLayer()->addTop(layer);
            }
            else
            {
               osg::ref_ptr<ossimPlanetTextureLayer> tempLayer;
               if(!parentName.empty()&&!parentId.empty())
               {
                  tempLayer = referenceLayer()->findLayerByNameAndId(parentName, parentId);
               }
               else if(!parentName.empty())
               {
                  tempLayer = referenceLayer()->findLayerByNameAndId(parentName, true);                  
               }
               else if(!parentId.empty())
               {
                  tempLayer = referenceLayer()->findLayerById(parentName, true);                  
               }
               osg::ref_ptr<ossimPlanetTextureLayerGroup> layerToAddTo = tempLayer.valid()?tempLayer->asGroup():0;
                  
               
               if(layerToAddTo.valid())
               {
                  layerToAddTo->addTop(layer.get());
               }
               else
               {
               }
            }
         }
      }
   }   
   else if(type == "ImageGroup")
   {
      ossimPlanetAction nestedAction(": dummy " + arg);
      for(idx = 1; idx <= nestedAction.argCount(); ++idx)
      {
         if(mkUtils::extractObjectAndArg(objectName,
                                         objectArg,
                                         nestedAction.arg(idx)))
         {
            if(objectName == "Description")
            {
               description = objectArg;
            }
            else if(objectName == "Name")
            {
               name = objectArg;
            }
            else if(objectName == "Id")
            {
               id = objectArg;
            }
            else if(objectName == "Parent")
            {
               ossimString objectName2;
               ossimString objectArg2;
               ossimPlanetAction nestedParentAction(": dummy " + objectArg);
               ossim_uint32 idx2;
               for(idx2 = 1; idx2 <= nestedParentAction.argCount(); ++idx2)
               {
                  if(mkUtils::extractObjectAndArg(objectName2,
                                                  objectArg2,
                                                  nestedAction.arg(idx)))
                  {
                     if(objectName == "Name")
                     {
                        parentName = objectArg;
                     }
                     else if(objectName == "Id")
                     {
                        parentId = objectArg;
                     }
                  }
               }// end Parent object for loop
            }
         }
      }
      
      osg::ref_ptr<ossimPlanetTextureLayerGroup> layer = new ossimPlanetTextureLayerGroup;
      if(!description.empty())
      {
         layer->setDescription(description);
      }
      if(!name.empty())
      {
         layer->setName(name);
      }
      if(!id.empty())
      {
         layer->setId(id);
      }
      if(parentName.empty())
      {
         referenceLayer()->addTop(layer.get());
      }
      else
      {
         osg::ref_ptr<ossimPlanetTextureLayer> tempLayer;
         if(!parentName.empty()&&!parentId.empty())
         {
            tempLayer = referenceLayer()->findLayerByNameAndId(parentName, parentId);
         }
         else if(!parentName.empty())
         {
            tempLayer = referenceLayer()->findLayerByNameAndId(parentName, true);                  
         }
         else if(!parentId.empty())
         {
            tempLayer = referenceLayer()->findLayerById(parentName, true);                  
         }
         
         osg::ref_ptr<ossimPlanetTextureLayerGroup> layerToAddTo = tempLayer.valid()?tempLayer->asGroup():0;
         
         if(layerToAddTo.valid())
         {
            layerToAddTo->addTop(layer.get());
         }
         else
         {
            
         }
      }
   }
#endif
}

void ossimPlanetLand::add(const ossimPlanetAction& /*a*/)
{
#if 0
   ossimString   objectName;
   ossimString   objectArg;
   
   ossim_uint32 idx = 1;
   for(idx = 1; idx <= (ossim_uint32)a.argCount(); ++idx)
   {
      if(mkUtils::extractObjectAndArg(objectName,
                                      objectArg,
                                      a.arg(idx)))
      {
         if(objectName == "Image")
         {
            addImageObject(objectName, objectArg);
         }
      }
   }
#endif
}

void ossimPlanetLand::remove(const ossimPlanetAction& /*a*/)
{
#if 0
   ossimString   objectName;
   ossimString   objectArg;
   ossim_uint32 idx = 1;
   for(idx = 1; idx <= (ossim_uint32)a.argCount(); ++idx)
   {
      if(mkUtils::extractObjectAndArg(objectName,
                                      objectArg,
                                      a.arg(idx)))
      {
         if((objectName == "Image")||
            (objectName == "ImageGroup"))
         {
            ossimPlanetAction nestedAction(": dummy " + objectArg);
            ossim_uint32 idx2;
            ossimString layerName;
            ossimString id;
            for(idx2=1; idx2 <= nestedAction.argCount();++idx2)
            {
               ossimString   tempObjectName;
               ossimString   tempObjectArg;
               if(mkUtils::extractObjectAndArg(tempObjectName,
                                               tempObjectArg,
                                               nestedAction.arg(idx)))
               {
                  if(tempObjectName == "Name")
                  {
                     layerName = tempObjectArg;
                  }
                  else if(tempObjectName == "Id")
                  {
                     id = tempObjectArg;
                  }
               }
            }
            osg::ref_ptr<ossimPlanetTextureLayer> layer;
            if(!id.empty()&&!layerName.empty())
            {
               layer = referenceLayer()->findLayerByNameAndId(layerName, id);
            }
            else if(!id.empty())
            {
               layer = referenceLayer()->findLayerById(id, true);
            }
            else if(!layerName.empty())
            {
               layer = referenceLayer()->findLayerByName(layerName, true);
            }
            if(layer.valid())
            {
               layer->remove();
            }
         }
      }
   }
#endif
}

void ossimPlanetLand::resetGraph(const ossimPlanetAction& /*a*/)
{
#if 0
   if(a.argCount() == 0)
   {
      resetGraph();
   }
   else
   {
      ossimString  bbox = "-180,-90,180,90";
      ossimString scaleRangeString = "";
      double minLat = -90.0, minLon = -180.0, maxLat = 90.0, maxLon = 180.0;
      ossimString  refreshTypeString = "all";
      ossimString  objectName;
      ossimString  objectArg;
      ossim_uint32 idx = 1;
      for(idx = 1; idx <= (ossim_uint32)a.argCount(); ++idx)
      {
         if(mkUtils::extractObjectAndArg(objectName,
                                         objectArg,
                                         a.arg(idx)))
         {
            if(objectName == "Extents")
            {
               ossimPlanetAction nestedAction(": Extents " + objectArg);
               ossim_uint32 idx2=1;
               for(;idx2 <= (ossim_uint32)nestedAction.argCount(); ++idx2)
               {
                  if(mkUtils::extractObjectAndArg(objectName,
                                                  objectArg,
                                                  nestedAction.arg(idx2)))
                  {
                     if(objectName == "Bbox")
                     {
                        bbox = objectArg;
                     }
                     else if(objectName == "ScaleRange")
                     {
                        scaleRangeString = objectArg;
                     }
                  }
               }
            }
            else if(objectName == "Type")
            {
               refreshTypeString = objectArg;
            }
         }
      }
      osg::ref_ptr<ossimPlanetExtents> extents = new ossimPlanetExtents;
      ossimPlanetLandRefreshType refreshType = ossimPlanetLandRefreshType_PRUNE;
      std::vector<ossimString> splitVector;
      bbox.split(splitVector, ",");
      if(splitVector.size() == 4)
      {
         minLon = splitVector[0].toDouble();
         minLat = splitVector[1].toDouble();
         maxLon = splitVector[2].toDouble();
         maxLat = splitVector[3].toDouble();
      }
      refreshTypeString = refreshTypeString.downcase();
      if(refreshTypeString == "texture")
      {
         refreshType = ossimPlanetLandRefreshType_TEXTURE;
      }
      else if(refreshTypeString == "geometry")
      {
         refreshType = ossimPlanetLandRefreshType_GEOM;
      }
      extents->setMinMaxLatLon(minLat, minLon, maxLat, maxLon);
      if(!scaleRangeString.empty())
      {
         splitVector.clear();
         scaleRangeString.split(splitVector,",");
         if(splitVector.size() == 2)
         {
            extents->setMinMaxScale(splitVector[0].toDouble(),
                                    splitVector[1].toDouble());
         }
      }
      resetGraph(extents,refreshType);
   }
#endif
}
