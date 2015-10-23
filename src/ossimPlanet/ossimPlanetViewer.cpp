#include <ossimPlanet/ossimPlanetViewer.h>
#include <ossimPlanet/ossimPlanetLookAt.h>
#include <ossimPlanet/ossimPlanetManipulator.h>

#include <ossimPlanet/ossimPlanetTextureLayerRegistry.h>
#include <osg/CoordinateSystemNode>
#include <osg/io_utils>
#include <OpenThreads/ScopedLock>
#include <osgUtil/IncrementalCompileOperation>

class ossimPlanetViewerFindNodesVisitor : public osg::NodeVisitor
{
public:
   ossimPlanetViewerFindNodesVisitor(const ossimString& id,
                                     ossimPlanetViewer::PlanetNodeList* nodeList,
                                     bool findFirstNodeOnlyFlag=false)
   :osg::NodeVisitor(TRAVERSE_ALL_CHILDREN),
   theListIsSharedFlag(true),
   theFindFirstNodeOnlyFlag(findFirstNodeOnlyFlag),
   theId(id),
   theNodeList(nodeList)
   {
      theNodeList->clear();
   }
   ossimPlanetViewerFindNodesVisitor(const ossimString& id,
                                     bool findFirstNodeOnlyFlag=false)
   :theListIsSharedFlag(false),
   theFindFirstNodeOnlyFlag(findFirstNodeOnlyFlag),
   theId(id),
   theNodeList(new ossimPlanetViewer::PlanetNodeList)
   {
      
   }
   virtual ~ossimPlanetViewerFindNodesVisitor()
   {
      if(!theListIsSharedFlag&&theNodeList&&theNodeList)
      {
         delete theNodeList;
      }
      theNodeList = 0;
   }
   ossimPlanetViewer::PlanetNodeList* nodeList()
   {
      return theNodeList;
   }
   const ossimPlanetViewer::PlanetNodeList* nodeList()const
   {
      return theNodeList;
   }
   virtual void apply(osg::Node& node)
   {
      ossimPlanetNode* planetNode = dynamic_cast<ossimPlanetNode*>(&node);
      
      if(planetNode)
      {
         if(planetNode->id() == theId)
         {
            if(theNodeList)
            {
               theNodeList->push_back(planetNode);
            }
         }
      }
      // keep going even if we find a node
      // if the flag is not set
      //
      if(theNodeList&&theNodeList->size() && !theFindFirstNodeOnlyFlag)
      {
         traverse(node);
      }
   }
protected:
   bool theListIsSharedFlag;
   bool theFindFirstNodeOnlyFlag;
   ossimString theId;
   ossimPlanetViewer::PlanetNodeList* theNodeList;
};

class ossimPlanetViewer::InitializePointersVisitor : public osg::NodeVisitor
{
public:
   InitializePointersVisitor(ossimPlanetViewer* viewer)
   :osg::NodeVisitor(TRAVERSE_ALL_CHILDREN),
   theViewer(viewer)
   {
      if(theViewer)
      {
         if(theViewer->thePlanet.valid())
         {
            theViewer->thePlanet->setComputeIntersectionFlag(true);
            theViewer->thePlanet->removeCallback(theViewer->theCallback.get());
         }
         theViewer->thePlanet = 0;
         theViewer->theTerrainLayer = 0;
         theViewer->theAnnotationLayer = 0;
      }
   }
   virtual void apply(osg::Node& node)
   {
      
      if(theViewer)
      {
         ossimPlanet*                planet       = dynamic_cast<ossimPlanet*>(&node);
         ossimPlanetLand*            landLayer       = dynamic_cast<ossimPlanetLand*>(&node);
         ossimPlanetTerrain*         terrainLayer       = dynamic_cast<ossimPlanetTerrain*>(&node);
         ossimPlanetAnnotationLayer* annotationLayer = dynamic_cast<ossimPlanetAnnotationLayer*>(&node);
         ossimPlanetKmlLayer*        kmlLayer = dynamic_cast<ossimPlanetKmlLayer*>(&node);
         if(planet)
         {
            if(!theViewer->thePlanet.valid())
            {
               theViewer->thePlanet = planet;
               if(theViewer->theEphemerisLayer.valid())
               {
                  theViewer->theEphemerisLayer->setModel(planet->model().get());
               }
               if(theViewer->thePlanet.valid())
               {
                  // the viewer will manage setting a valid intersection point
                  // for any who need it.  This is the current intersection for
                  // current traverse.
                  //
                  theViewer->thePlanet->setComputeIntersectionFlag(false);
                  theViewer->thePlanet->addCallback(theViewer->theCallback.get());
               }
            }
         }
         else if(kmlLayer)
         {
            theViewer->theKmlLayer = kmlLayer;
         }
         else if(annotationLayer)
         {
            theViewer->theAnnotationLayer = annotationLayer;
         }
         else if(terrainLayer)
         {
           theViewer->theTerrainLayer = terrainLayer;
         }
         else if(landLayer)
         {
            theViewer->theTerrainLayer = landLayer;
         }
         if(!theViewer->theAnnotationLayer.valid() ||
            !(theViewer->theTerrainLayer.valid()||theViewer->thePlanet.valid()))
         { 
            traverse(node);
         }
      }
   }
   
protected:  
   ossimPlanetViewer* theViewer;
};

void ossimPlanetViewer::NodeListener::nodeAdded(osg::Node* node)
{
   ossimPlanetLand* land = dynamic_cast<ossimPlanetLand*>(node);
   ossimPlanetTerrain* terrain = dynamic_cast<ossimPlanetTerrain*>(node);
   if(land)
   {
      if(!theViewer->theTerrainLayer.valid())
      {
         theViewer->theTerrainLayer = land;
      }
   }
   else if(terrain)
   {
      if(!theViewer->theTerrainLayer.valid())
      {
         theViewer->theTerrainLayer = terrain;
      }
   }
   else 
   {
      ossimPlanetAnnotationLayer* annotationLayer = dynamic_cast<ossimPlanetAnnotationLayer*>(node);
      if(annotationLayer)
      {
         if(!theViewer->theAnnotationLayer.valid())
         {
            theViewer->theAnnotationLayer = annotationLayer;
         }
      }
      else
      {
         ossimPlanetKmlLayer* kmlLayer = dynamic_cast<ossimPlanetKmlLayer*>(node);
         if(kmlLayer)
         {
            theViewer->theKmlLayer = kmlLayer;
         }
      }
   }
}

void ossimPlanetViewer::NodeListener::nodeRemoved(osg::Node* node)
{
   if(node == theViewer->theTerrainLayer.get())
   {
      theViewer->theTerrainLayer = 0;
   }
   else if(node == theViewer->theAnnotationLayer.get())
   {
      theViewer->theAnnotationLayer = 0;
   }
}

void ossimPlanetViewer::NodeListener::needsRedraw(ossimPlanetNode* node)
{
   if(theViewer)
   {
      theViewer->requestRedraw();
   }
}


ossimPlanetNode* ossimPlanetViewer::PickObject::firstPlanetNode()
{
   ossimPlanetNode* result = 0;
   if(theNodePath.empty()) return result;
   ossim_int32 idx=0;
   for(idx = theNodePath.size()-1; ((idx >=0)&&(!result)); --idx)
   {
      result = dynamic_cast<ossimPlanetNode*>(theNodePath[idx].get());
   }
   
   return result;
}

#if 0
void ossimPlanetViewer::Renderer::flushAndCompile(double currentElapsedFrameTime, 
                                                  osgUtil::SceneView* sceneView, 
                                                  osgDB::DatabasePager* databasePager, 
                                                  osg::GraphicsThread* compileThread)
{
   osg::Timer_t t = osg::Timer::instance()->tick();
   osgViewer::Renderer::flushAndCompile(currentElapsedFrameTime, sceneView, databasePager, compileThread);
   double delta = osg::Timer::instance()->delta_s(t, osg::Timer::instance()->tick());
   if(theCallback.valid())
   {
      theCallback->flushAndCompile(currentElapsedFrameTime + delta,
                                   sceneView,
                                   databasePager,
                                   compileThread);
   }
}

void ossimPlanetViewer::DrawCallback::operator () (osg::RenderInfo& renderInfo) const
{
   if(!theViewer) return;
   if(theViewer->terrainLayer()&&renderInfo.getState())
   {
      //double delta = osg::Timer::instance()->delta_s(theViewer->theFrameStartTimeStamp, 
      //                                               osg::Timer::instance()->tick());
      theViewer->terrainLayer()->compileGLObjects(*renderInfo.getState(), 
                                                  theViewer->terrainLayer()->minimumTimeToCompilePerFrame());
   }
}

void ossimPlanetViewer::FlushAndCompileCallback::flushAndCompile(double currentElapsedFrameTime, 
                                                                   osgUtil::SceneView* sceneView, 
                                                                   osgDB::DatabasePager* databasePager, 
                                                                   osg::GraphicsThread* compileThread)
{
   if(theViewer)
   {
      if(theViewer->terrainLayer())
      {
         // let's calculate based on target frame rate later.  For now give
         // 2 milliseconds
         //
         theViewer->terrainLayer()->compileGLObjects(*(sceneView->getState()), .002);
      }
   }
}
#endif

ossimPlanetViewer::ossimPlanetViewer()
:osgViewer::Viewer()
{
   init();
}

ossimPlanetViewer::ossimPlanetViewer(osg::ArgumentParser& arguments)
:osgViewer::Viewer(arguments)
{
   init();
}

ossimPlanetViewer::ossimPlanetViewer(const osgViewer::Viewer& viewer, const osg::CopyOp& copyop)
:osgViewer::Viewer(viewer, copyop)
{
   init();
}

ossimPlanetViewer::~ossimPlanetViewer()
{
   if(thePlanet.valid())
   {
      thePlanet->removeCallback(theCallback.get());
   }
   thePlanet = 0;
}

void ossimPlanetViewer::init()
{
   theRootNode = new osg::Group();
   theRootNode->setUpdateCallback(new ossimPlanetTraverseCallback);
   theRootNode->setEventCallback(new ossimPlanetTraverseCallback);
   theRootNode->setCullCallback(new ossimPlanetTraverseCallback);
   
//   getCamera()->setPreDrawCallback(new ossimPlanetViewer::DrawCallback(this));
   setThreadSafeRefUnref(true);
   osgDB::DatabasePager* pager = osgDB::DatabasePager::create();
   setDatabasePager(pager);
   theUpdateVisitor = new ossimPlanetUpdateVisitor();
   //theUpdateVisitor->setDatabaseRequestHandler(pager);
   setUpdateVisitor(theUpdateVisitor.get());
   theCallback = new ossimPlanetViewer::NodeListener(this);
   theCurrentCamera = new ossimPlanetLookAt;
   theCurrentLookAt = new ossimPlanetLookAt;
   getCamera()->setClearColor(osg::Vec4(0.0,0.0,0.0,1.0));
   //getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
   //getCamera()->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
   //getCamera()->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_PRIMITIVES);
   theRedrawFlag          = false;
   theContinousUpdateFlag = false;
   theIntersectWithMasterIfNotWithinAnyViewFlag = false;
   theCalculateNearFarRatioFlag = true;

   //setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation());

}

void ossimPlanetViewer::setTerrainMaxNumberOfOperationsToApplyToGraphPerFrame(ossim_uint32 value)
{
   ossimPlanetTerrain* terrain = dynamic_cast<ossimPlanetTerrain*>(theTerrainLayer.get());
   
   if(terrain)
   {
      terrain->setMaxNumberOfOperationsToApplyToGraphPerFrame(value);
   }
}

ossimPlanetManipulator* ossimPlanetViewer::planetManipulator()
{
   return dynamic_cast<ossimPlanetManipulator*>(getCameraManipulator());
}

ossimPlanetTerrain* ossimPlanetViewer::terrainLayer()
{
   return dynamic_cast<ossimPlanetTerrain*>(theTerrainLayer.get());
}

ossimPlanetAnnotationLayer* ossimPlanetViewer::annotationLayer()
{
   return theAnnotationLayer.get();
}

ossimPlanetKmlLayer* ossimPlanetViewer::kmlLayer()
{
   return theKmlLayer.get();
}

void ossimPlanetViewer::addEphemeris(ossim_uint32 memberBitMask)
{
   if(!theEphemerisLayer.valid())
   {
      if(getLight())
      {
         theSavedLight = (osg::Light*)getLight()->clone(osg::CopyOp::DEEP_COPY_ALL);
      }
      ossimPlanet* tempPlanet = new ossimPlanet;
      tempPlanet->setComputeIntersectionFlag(false);
      theEphemerisRoot = tempPlanet;
      theEphemerisLayer = new ossimPlanetEphemeris();
      theEphemerisLayer->setRoot(theRootNode.get());
      theEphemerisLayer->setMembers(memberBitMask);
//      planet()->addChild(theEphemerisLayer.get());
      tempPlanet->addChild(theEphemerisLayer.get());
      theRootNode->addChild(tempPlanet);
      theEphemerisCamera = new osg::Camera;
      theEphemerisCamera->setProjectionResizePolicy(getCamera()->getProjectionResizePolicy());
      theEphemerisCamera->setClearColor(getCamera()->getClearColor());
      theEphemerisCamera->setRenderOrder(osg::Camera::PRE_RENDER);
      theEphemerisCamera->setRenderTargetImplementation( getCamera()->getRenderTargetImplementation() );
      //      theEphemerisCamera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      //      getCamera()->setClearMask(getCamera()->getClearMask() & ~GL_COLOR_BUFFER_BIT & ~GL_DEPTH_BUFFER_BIT);
      theEphemerisCamera->setClearMask(GL_COLOR_BUFFER_BIT);
      getCamera()->setClearMask(getCamera()->getClearMask() & ~GL_COLOR_BUFFER_BIT);
      if(getCamera()->getViewport())
      {
         theEphemerisCamera->setViewport(new osg::Viewport(*getCamera()->getViewport()));
      }
      else
      {
         theEphemerisCamera->setViewport(new osg::Viewport());
      }
      addSlave(theEphemerisCamera.get(), false);
      theEphemerisLayer->setCamera(theEphemerisCamera.get());
      theEphemerisCamera->setEventCallback(new ossimPlanetTraverseCallback());
      theEphemerisCamera->setUpdateCallback(new ossimPlanetTraverseCallback());
      theEphemerisCamera->setCullCallback(new ossimPlanetTraverseCallback());
      //theEphemerisCamera->addChild(theRootNode.get());
      //getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
      //theEphemerisCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
      //double fov, aspectRatio, near, far;
      //osg::Matrixd& m = getCamera()->getProjectionMatrix();
      //m.getPerspective(fov, aspectRatio, near, far);
      //theEphemerisCamera->setProjectionMatrixAsPerspective(fov, aspectRatio, .2, 5.0);
      //getCamera()->setProjectionMatrixAsPerspective(fov, aspectRatio, .0000001, .2);
   }
}

void ossimPlanetViewer::removeEphemeris()
{
   if(theEphemerisLayer.valid())
   {
      if(theRootNode->getNumChildren() > 1)
      {
         theRootNode->removeChild(1, theRootNode->getNumChildren());
      }
      theEphemerisLayer  = 0;
      theEphemerisRoot   = 0;
      int slaveCameraIdx = findSlaveIndexForCamera(theEphemerisCamera.get());
      if(slaveCameraIdx >= 0)
      {
         removeSlave(slaveCameraIdx);
         theEphemerisCamera = 0;
      }
      getCamera()->setClearMask(getCamera()->getClearMask()|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      //setLight(new osg::Light());
      requestRedraw();
   }
}

ossimPlanetEphemeris* ossimPlanetViewer::ephemeris()
{
   return theEphemerisLayer.get();
}

void ossimPlanetViewer::requestRedraw()
{
   setRedrawFlag(true);
}

void ossimPlanetViewer::requestContinuousUpdate(bool needed)
{  
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionAdapterMutex);
  theContinousUpdateFlag=needed;
}

void ossimPlanetViewer::requestWarpPointer(float x,float y)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionAdapterMutex);
   theWarpPointerFlag = true; theWarpX=x; theWarpY=y;
}

bool ossimPlanetViewer::getAndSetRedrawFlag(bool newValue)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionAdapterMutex);
   bool result = theRedrawFlag;
   theRedrawFlag = newValue;
   return result;
}

bool ossimPlanetViewer::getRedrawFlag()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionAdapterMutex);
   return theRedrawFlag;
}

void ossimPlanetViewer::setRedrawFlag(bool flag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionAdapterMutex);
   theRedrawFlag=flag;
}

bool ossimPlanetViewer::redrawFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionAdapterMutex);
   return theRedrawFlag;
}

void ossimPlanetViewer::setContinuousUpdateFlag(bool flag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionAdapterMutex);
   theContinousUpdateFlag=flag;
}

bool ossimPlanetViewer::continuousUpdateFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionAdapterMutex);
   return theContinousUpdateFlag;
}

bool ossimPlanetViewer::warpPointerFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionAdapterMutex);
   return theWarpPointerFlag;
}

void ossimPlanetViewer::getWarpPoints(float& x, float& y)const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionAdapterMutex);
   x=theWarpX; 
   y=theWarpY;
}

void ossimPlanetViewer::setSceneData(osg::Node* p)
{
   InitializePointersVisitor initializePointersVisitor(this);
   theRootNode->removeChildren(0, theRootNode->getNumChildren());
   if(p)
   {
      p->accept(initializePointersVisitor);
      theRootNode->addChild(p);
      if(theEphemerisLayer.valid())
      {
         ossimPlanet* tempPlanet = new ossimPlanet;
         tempPlanet->addChild(theEphemerisLayer.get());
         tempPlanet->setComputeIntersectionFlag(false);
         theRootNode->addChild(tempPlanet);
      }
   }
   osgViewer::Viewer::setSceneData(theRootNode.get());
   requestRedraw();
}

ossimPlanet* ossimPlanetViewer::planet()
{
   return thePlanet.get();
}

const ossimPlanet* ossimPlanetViewer::planet()const
{
   return thePlanet.get();
}

const ossimPlanetGeoRefModel* ossimPlanetViewer::model()const
{
   if(thePlanet.get())
   {
      return thePlanet->model().get();
   }
   
   return 0;
}

ossimPlanetGeoRefModel* ossimPlanetViewer::model()
{
   if(thePlanet.get())
   {
      return thePlanet->model().get();
   }
   
   return 0;
}

void ossimPlanetViewer::advance(double simulationTime)
{
   theFrameStartTimeStamp = osg::Timer::instance()->tick();
   osgViewer::Viewer::advance(simulationTime);
}

void ossimPlanetViewer::eventTraversal()
{
   osgViewer::Viewer::eventTraversal();
}

void ossimPlanetViewer::updateTraversal()
{
   ossimPlanetTerrain* terrain = dynamic_cast<ossimPlanetTerrain*>(terrainLayer());
   if(terrain&&!terrain->getDatabasePager()) terrain->setDatabasePager(getDatabasePager());

   if(!mkUtils::almostEqual(theCurrentViewMatrix,
                            getCamera()->getViewMatrix()))
   {
      theCurrentViewMatrix        = getCamera()->getViewMatrix();
      theCurrentViewMatrixInverse = theCurrentViewMatrix.inverse(theCurrentViewMatrix);
      computeCurrentCameraInfo();
      const ossimPlanetGeoRefModel* landModel = model();

      // let's do a crude normalize distance to do an estimate NearFarRatio
      //
      if(getCamera()&&landModel&&theCurrentLookAt.valid()&&theCurrentCamera.valid()&&theCalculateNearFarRatioFlag)
      {
         double dist = ossim::min(theCurrentLookAt->range(),
                                  theCurrentCamera->altitude())/osg::WGS_84_RADIUS_EQUATOR;
         double t = log(1+dist);
         t = ossim::clamp(t, 0.0, 1.0);
         double ratio = .1*(t) + (.0000001)*(1.0-t);
         getCamera()->setNearFarRatio(ossim::min(.001, ratio));
      }
      notifyViewChanged();
   }
   osgViewer::Viewer::updateTraversal();
   if(theEphemerisCamera.valid())
   {
      if(theEphemerisCamera.valid())
      {
         theEphemerisCamera->setGraphicsContext(getCamera()->getGraphicsContext());
         theEphemerisCamera->setRenderTargetImplementation( getCamera()->getRenderTargetImplementation() );
      }
      osg::Viewport* viewport    = getCamera()->getViewport();
      osg::Viewport* ephViewport = theEphemerisCamera->getViewport();
      if(viewport&&ephViewport)
      {
         if(!ossim::almostEqual(viewport->x(), ephViewport->x())||
            !ossim::almostEqual(viewport->y(), ephViewport->y())||
            !ossim::almostEqual(viewport->width(), ephViewport->width())||
            !ossim::almostEqual(viewport->height(), ephViewport->height()))
         {            
            ephViewport->setViewport(viewport->x(), 
                                     viewport->y(), 
                                     viewport->width(), 
                                     viewport->height());
         }
      }
      theEphemerisCamera->setProjectionMatrix(getCamera()->getProjectionMatrix());
      theEphemerisCamera->setViewMatrix(getCamera()->getViewMatrix());
   }
   bool databasePagerHasRequests = getDatabasePager()?getDatabasePager()->requiresUpdateSceneGraph():false;//||
                                                       //getDatabasePager()->requiresCompileGLObjects()):false;
   if(databasePagerHasRequests)
   {
      requestRedraw();
   }
   
}

bool ossimPlanetViewer::addAnnotation(osg::ref_ptr<ossimPlanetAnnotationLayerNode> annotation)
{
   bool result = false;
   
   if(theAnnotationLayer.valid())
   {
      result = theAnnotationLayer->addChild(annotation.get());
   }
   
   return result;
}

bool ossimPlanetViewer::addImageTexture(osg::ref_ptr<ossimPlanetTextureLayer> imageTexture)
{
   bool result = false;
   
   if(theTerrainLayer.valid()&&imageTexture.valid())
   {
      ossimPlanetTerrain* terrain = dynamic_cast<ossimPlanetTerrain*>(theTerrainLayer.get());
      if(terrain)
      {
         if(terrain->numberOfTextureLayers() > 0)
         {
            ossimPlanetTextureLayerGroup* group = terrain->textureLayer(0)->asGroup();
            if(group)
            {
               result = group->addTop(imageTexture.get());
            }
         }
      }
      else
      {
         ossimPlanetLand* land = dynamic_cast<ossimPlanetLand*>(theTerrainLayer.get());
         if(land)
         {
            result = land->referenceLayer()->addTop(imageTexture.get());
         }
      }
   }
   
   return result;
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetViewer::addImageTexture(const ossimString& file)
{
   osg::ref_ptr<ossimPlanetTextureLayer> layer = ossimPlanetTextureLayerRegistry::instance()->createLayer(file);
   
   if(!addImageTexture(layer.get()))
   {
      layer = 0;
   }
   
   return layer.get();
}

bool ossimPlanetViewer::addElevation(osg::ref_ptr<ossimPlanetElevationDatabase> database, bool sortFlag)
{
   bool result = false;
   
   if(theTerrainLayer.valid()&&database.valid())
   {
      result = true;
      ossimPlanetTerrain* terrain = dynamic_cast<ossimPlanetTerrain*>(theTerrainLayer.get());
      if(terrain)
      {
         result = terrain->addElevation(database.get(), sortFlag);
      }
      else
      {
         ossimPlanetTerrain* land = dynamic_cast<ossimPlanetTerrain*>(theTerrainLayer.get());
         if(land)
         {
            result = land->addElevation(database.get(), sortFlag);
         }
      }
   }
   
   return result;
}

void ossimPlanetViewer::addKml(const ossimFilename& file)
{
   if(theKmlLayer.valid())
   {
      theKmlLayer->addKml(file);
   }
}

void ossimPlanetViewer::computeCurrentCameraInfo()
{
   const ossimPlanetGeoRefModel* landModel = model();
   if(!landModel) return;
   
   // First solve the camera in lat lon hgt and roll pitch heading format
   //
   osg::Matrixd eyeLsrMatrix;
   osg::Vec3d eyeLlh;
   osg::Vec3d hpr;
   osg::Vec3d eye = osg::Vec3d(0.0,0.0,0.0)*theCurrentViewMatrixInverse;
   landModel->inverse(eye, eyeLlh);
   landModel->lsrMatrix(eyeLlh, eyeLsrMatrix);
   mkUtils::matrixToHpr(hpr, eyeLsrMatrix, theCurrentViewMatrixInverse);
   theCurrentCamera->setAll(eyeLlh[0], eyeLlh[1], eyeLlh[2],
                            hpr[0], hpr[1], hpr[2],
                            0.0, ossimPlanetAltitudeMode_ABSOLUTE);
   
   // now compute the current lookat point
   // if no intersection then we will fake one by extruding a lookat range meters out
   // from current line of site
   osg::Vec3d llh;
   osg::Viewport* viewPort = getCamera()->getViewport();
   if(!viewPort) return;
   double midX=(viewPort->x()+(viewPort->width()/2.0));
   double midY=(viewPort->y()+(viewPort->height()/2.0));
   
   // if we do not intersect then use the range from the previous look at and create a point
   if(!getLatLonHeightAtWindowCoordinate(llh, midX, midY))
   {
      osg::Vec3d origin, ray;
      if(makeRayAtWindowCoordinate(origin, ray, midX, midY))
      {
         osg::Vec3d pt = origin + ray*(theCurrentLookAt->range()/landModel->getNormalizationScale());
         landModel->inverse(pt, llh);
      }
   }
   osg::Matrixd losLsrMatrix;
   osg::Vec3d tempHpr;
   landModel->orientationLsrMatrix(losLsrMatrix, llh, 0.0, 0.0, 0.0);
   
   mkUtils::matrixToHpr(tempHpr, losLsrMatrix, theCurrentViewMatrixInverse);
   osg::Vec3d eyeXyz;
   osg::Vec3d losXyz;
   landModel->forward(llh, losXyz);
   double range = (losXyz-eye).length()*landModel->getNormalizationScale();
   
   theCurrentLookAt->setAll(llh[0], llh[1], llh[2],
                            tempHpr[0], tempHpr[1], tempHpr[2],
                            range, ossimPlanetAltitudeMode_ABSOLUTE);
   
   
#if 0  
   if(thePlanet.valid())
   {
      thePlanet->setLookAt(theCurrentLookAt.get());
      thePlanet->setEyePosition(theCurrentCamera.get());
   }
#endif
   //   std::cout << "<lat,lon,hgt> = <" << theCurrentLookAt->lat() << ", " << theCurrentLookAt->lon() << ", " << theCurrentLookAt->altitude() << ">" << std::endl
   //             << "<h, p, r>     = <" << theCurrentLookAt->heading() << ", " << theCurrentLookAt->pitch() << ", " << theCurrentLookAt->roll() << ">" << std::endl
   //   << "range         =  " << theCurrentLookAt->range() << std::endl;
}

void ossimPlanetViewer::findNodesWithId(ossimPlanetViewer::PlanetNodeList& nodeList,
                                        const ossimString& id)
{
   ossimPlanetViewerFindNodesVisitor visitor(id, &nodeList, false);
   
   if(getSceneData())
   {
      getSceneData()->accept(visitor);
   }
}

osg::ref_ptr<ossimPlanetNode> ossimPlanetViewer::findFirstNodeWithId(const ossimString& id)
{
   osg::ref_ptr<ossimPlanetNode> result = 0;
   ossimPlanetViewer::PlanetNodeList nodeList;
   ossimPlanetViewerFindNodesVisitor visitor(id, &nodeList, true);
   
   if(getSceneData())
   {
      getSceneData()->accept(visitor);
   }
   
   if(nodeList.size())
   {
      result = nodeList[0].get();
   }
   
   return result;
}

bool ossimPlanetViewer::makeRayAtWindowCoordinate(osg::Vec3d& origin,
                                                  osg::Vec3d& ray,
                                                  double wx, double wy)
{
   float local_x = wx, local_y = wy;    
   osg::Camera* camera = const_cast<osg::Camera*>(getCameraContainingPosition(wx, wy, local_x, local_y));
   if (!camera) camera = getCamera();
   return makeRayAtWindowCoordinate(origin, ray, camera, local_x, local_x);
}

bool ossimPlanetViewer::makeRayAtWindowCoordinate(osg::Vec3d& origin,
                                                  osg::Vec3d& ray,
                                                  osg::Camera* camera,
                                                  double wx, double wy)
{
   osg::Vec3d eye(0.0,0.0,0.0);
   osg::Vec3d center(0.0,0.0,0.0);
   osg::Vec3d up(0.0,0.0,0.0);
   osg::Vec3d direction;
   double fov;
   double aspectRatio;
   double znear;
   double zfar;
   osg::Matrixd pm = camera->getProjectionMatrix();
   ray[0] = 0;
   ray[1] = 0;
   ray[2] = 0;
   
   if(!pm.getPerspective(fov, aspectRatio, znear, zfar))
   {
      return false;
   }
   camera->getViewMatrixAsLookAt(eye, center, up, 1000.0);
   osg::Matrixd iv = camera->getViewMatrix();
   
   const osg::ref_ptr<osg::Viewport> viewport = camera->getViewport();
   osg::Matrix wm;
   wm.invert(viewport->computeWindowMatrix());
   osg::Vec3d normalizedPoint = osg::Vec3d(wx,wy,0.0)*wm;
  
   double angleY = (fov*.5)*normalizedPoint[1];
   double angleX = (fov*.5)*normalizedPoint[0]*aspectRatio;
   direction = center - eye;
   direction.normalize();
   osg::Vec3d crossVec = direction^up;
   osg::Vec3d newUp = crossVec^direction;
   osg::Matrixd m = (osg::Matrixd::rotate(-angleX*(M_PI/180.0), newUp)*
                     osg::Matrixd::rotate(angleY*(M_PI/180.0), crossVec));
   ray    = direction*m;
   origin = eye;
   
   return true;
}

bool ossimPlanetViewer::pickAtWindowCoordinate(PickList& result,
                                               double wx, double wy,
                                               osg::Node::NodeMask traversalMask)
{
   result.clear();
   
   osgUtil::LineSegmentIntersector::Intersections intersections;
   if (computeIntersections(wx, wy, intersections, traversalMask))
   {
      osgUtil::LineSegmentIntersector::Intersections::iterator hitr = intersections.begin();
      
      while(hitr != intersections.end())
      {
         osg::Vec3d wpt = hitr->getWorldIntersectPoint();
         osg::Vec3d llh;
         if(model())
         {
            model()->inverse(wpt, llh);
         }
         
         result.push_back(new PickObject(hitr->nodePath, hitr->getLocalIntersectPoint(), hitr->getWorldIntersectPoint(), llh));
         ++hitr;
      }
   }
   
   return !result.empty();
}

bool ossimPlanetViewer::getLatLonHeightAtWindowCoordinate(osg::Vec3d& llh,
                                                          double wx, double wy,
                                                          osg::Node::NodeMask traversalMask)
{
   bool resultFlag = false;
   osgUtil::LineSegmentIntersector::Intersections intersections;
   if (computeIntersections(wx, wy, intersections, traversalMask))
   {
      osgUtil::LineSegmentIntersector::Intersections::iterator hitr = intersections.begin();
      
      if(hitr != intersections.end())
      {
         osg::Vec3d wpt = hitr->getWorldIntersectPoint();
         if(model())
         {
            model()->inverse(wpt, llh);
            resultFlag = true;
         }
      }
   }
   
   return resultFlag;
}

void ossimPlanetViewer::notifyViewChanged()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx = 0;
   ossim_uint32 upper = theCallbackList.size();
   
   for(idx = 0; idx < upper; ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->viewChanged(this); 
      }
   }
   
}


#if 0
bool ossimPlanetViewer::AddHudOverlay(osg::ref_ptr<osg::Node> hudNode)
{
   bool result =  false;
   if(thePlanet.valid())
   {
      result = true;
      
      hudNode->getOrCreateStateSet()->setRenderBinDetails(11,"RenderBin");
      theHudOverlayList.push_back(hudNode.get());
      
      thePlanet->addChild(hudNode.get());
   }
   
   return result;
}

ossim_uint32 ossimPlanetViewer::getNumberOfHudOverlays()const
{
   return static_cast<ossim_uint32>(theHudOverlayList.size());
}
#endif

const osg::Camera* ossimPlanetViewer::forceAdjustToMasterCamera(float x, float y, float& local_x, float& local_y) const
{
   
   const osg::Camera* result = _camera.get(); // get and use the master camera
   if(!result) return 0;
   const osgGA::GUIEventAdapter* eventState = getEventQueue()->getCurrentEventState(); 
   const osgViewer::GraphicsWindow* gw = dynamic_cast<const osgViewer::GraphicsWindow*>(eventState->getGraphicsContext());
   
   bool view_invert_y = eventState->getMouseYOrientation()==osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS;
   
   double epsilon = 0.5;
   
   if (_camera->getGraphicsContext() &&
       (!gw || _camera->getGraphicsContext()==gw) &&
       _camera->getViewport())
   {
      const osg::Viewport* viewport = _camera->getViewport();
      
      double new_x = x;
      double new_y = y;
      
      if (!gw)
      {
         new_x = static_cast<double>(_camera->getGraphicsContext()->getTraits()->width) * (x - eventState->getXmin())/(eventState->getXmax()-eventState->getXmin());
         new_y = view_invert_y ?
         static_cast<double>(_camera->getGraphicsContext()->getTraits()->height) * (1.0 - (y- eventState->getYmin())/(eventState->getYmax()-eventState->getYmin())) :
         static_cast<double>(_camera->getGraphicsContext()->getTraits()->height) * (y - eventState->getYmin())/(eventState->getYmax()-eventState->getXmin());
      }
      
      // lets still allow for valid camera positioning even if outside the frustum if no camera is found
      if (viewport)
      {
         local_x = new_x;
         local_y = new_y;
         
         result = _camera.get();
      }
   }
   
   return result;
}

#if 1
/** Compute intersections between a ray through the specified master cameras window/eye coords and a specified node.
 * Note, when a master cameras has slaves and no viewport itself its coordinate frame will be in clip space i.e. -1,-1 to 1,1,
 * while if its has a viewport the coordintates will be relative to its viewport dimensions. 
 * Mouse events handled by the view will automatically be attached into the master camera window/clip coords so can be passed
 * directly on to the computeIntersections method. */
bool ossimPlanetViewer::computeIntersections(float x,
                                                     float y, 
                                                     osgUtil::LineSegmentIntersector::Intersections& intersections,
                                                     osg::Node::NodeMask traversalMask)
{
   if (!_camera.valid()) return false;
   
   float local_x, local_y = 0.0;    
   const osg::Camera* camera = getCameraContainingPosition(x, y, local_x, local_y);
   if (!camera)
   {
      if(theIntersectWithMasterIfNotWithinAnyViewFlag)
      {
         camera = forceAdjustToMasterCamera(x, y, local_x, local_y);
         if(!camera) return false;
      }
      else
      {
         return false;
      }
   }
   
   osgUtil::LineSegmentIntersector::CoordinateFrame cf = camera->getViewport() ? osgUtil::Intersector::WINDOW : osgUtil::Intersector::PROJECTION;
   osg::ref_ptr< osgUtil::LineSegmentIntersector > picker = new osgUtil::LineSegmentIntersector(cf, local_x, local_y);
   osg::Vec3d adjustedStart(picker->getStart());
#if 0
   if(thePlanet.valid())
   {
      adjustedStart[2]-=(5*thePlanet->model()->getInvNormalizationScale());
   }
   picker->setStart(adjustedStart);
#endif
   osgUtil::IntersectionVisitor iv(picker.get());
   iv.setTraversalMask(traversalMask);
   iv.setUseKdTreeWhenAvailable(true);
   
   const_cast<osg::Camera*>(camera)->accept(iv);
   
   if (picker->containsIntersections())
   {
      intersections = picker->getIntersections();
      return true;
   }
   intersections.clear();
   return false;
}

/** Compute intersections between a ray through the specified master cameras window/eye coords and a specified nodePath's subgraph. */
bool ossimPlanetViewer::computeIntersections(float x,
                                                     float y, 
                                                     const osg::NodePath& nodePath, 
                                                     osgUtil::LineSegmentIntersector::Intersections& intersections,
                                                     osg::Node::NodeMask traversalMask)
{
   if (!_camera.valid() || nodePath.empty()) return false;
   
   float local_x, local_y = 0.0;    
   const osg::Camera* camera = getCameraContainingPosition(x, y, local_x, local_y);
   if (!camera)
   {
      if(theIntersectWithMasterIfNotWithinAnyViewFlag)
      {
         camera = forceAdjustToMasterCamera(x, y, local_x, local_y);
         if(!camera) return false;
      }
      else
      {
         return false;
      }
   }
   
   osg::Matrixd matrix;
   if (nodePath.size()>1)
   {
      osg::NodePath prunedNodePath(nodePath.begin(),nodePath.end()-1);
      matrix = osg::computeLocalToWorld(prunedNodePath);
   }
   
   matrix.postMult(camera->getViewMatrix());
   matrix.postMult(camera->getProjectionMatrix());
   
   double zNear = -1.0;
   double zFar = 1.0;
   if (camera->getViewport())
   {
      matrix.postMult(camera->getViewport()->computeWindowMatrix());
      zNear = 0.0;
      zFar = 1.0;
   }
   
   osg::Matrixd inverse;
   inverse.invert(matrix);
   
   osg::Vec3d startVertex = osg::Vec3d(local_x,local_y,zNear) * inverse;
   osg::Vec3d endVertex = osg::Vec3d(local_x,local_y,zFar) * inverse;
   
   osg::ref_ptr< osgUtil::LineSegmentIntersector > picker = new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, startVertex, endVertex);
   
   osgUtil::IntersectionVisitor iv(picker.get());
   iv.setTraversalMask(traversalMask);
   nodePath.back()->accept(iv);
   
   if (picker->containsIntersections())
   {
      intersections = picker->getIntersections();
      return true;
   }
   else
   {
      intersections.clear();
      return false;
   }
}
#endif

void ossimPlanetViewer::execute(const ossimPlanetAction& /*action*/)
{
}


