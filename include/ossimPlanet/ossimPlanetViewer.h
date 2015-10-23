#ifndef ossimPlanetViewer_HEADER
#define ossimPlanetViewer_HEADER
#include <osgDB/DatabasePager>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetElevationDatabase.h>
#include <ossimPlanet/ossimPlanetCallback.h>
#include <ossimPlanet/ossimPlanetLookAt.h>
#include <ossimPlanet/ossimPlanetNode.h>
#include <osgViewer/Viewer>
//#include <osgViewer/Renderer>
#include <ossimPlanet/ossimPlanet.h>
#include <osg/ref_ptr>
#include <osg/Node>
#include <ossimPlanet/ossimPlanetNode.h>
#include <ossimPlanet/ossimPlanetLookAt.h>
#include <osgUtil/LineSegmentIntersector>
#include <ossimPlanet/ossimPlanetLand.h>
#include <ossimPlanet/ossimPlanetTerrain.h>
#include <ossimPlanet/ossimPlanetAnnotationLayer.h>
#include <ossimPlanet/ossimPlanetKmlLayer.h>
#include <ossimPlanet/ossimPlanetVisitors.h>
#include <ossimPlanet/ossimPlanetEphemeris.h>
#include <OpenThreads/Mutex>

class ossimPlanetManipulator;
class ossimPlanetViewer;
class OSSIMPLANET_DLL ossimPlanetViewerCallback : public ossimPlanetCallback
{
public:
   virtual void viewChanged(ossimPlanetViewer* /*viewer*/){}
};

/**
 *
 * ossimPlanetViewer will serve as a higher level interface to most of the ossimPlanet's inner workings.  During the update phase it will determine if
 * the scnee graph is in a static state ad will notify the user through a virtual method callback.  At the time of this documentation we have not written then API
 * to auto setup the planet node.  This is an example to setup the planet node and then set it to the Viewer.  Note, this code will eventually be moved into 
 * a single setup routine on ossimPlanetViewer.
 *
 * <pre>
 *    viwer->addEventHandler( new osgGA::StateSetManipulator(theCanvas->getCamera()->getOrCreateStateSet()));
 *    viewer->addEventHandler(new osgViewer::StatsHandler());
 *    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
 *    osg::ref_ptr<ossimPlanet> planet = new ossimPlanet;
 *    planet->setupDefaults();
 *    planet->land()->setCurrentFragementShaderType(ossimPlanetShaderProgramSetup::NO_SHADER);
 *    viewer->setSceneData(planet.get());
 *    viewer->addImageTexture("/data/earth/earth.jpg");
 *
 *    viewer->setCameraManipulator(new ossimPlanetManipulator());
 * </pre>
 *
 * this example sets some GUI action handlers supported by OSG.  We allocate a planet and then call planets setupDefaults().
 * I then set the scene to the viewer and then add an image to the earth.  The last thing we did was setup the manipulator that will
 * manipulate the eye matrix of OSG.  We have our own manipulator called ossimPlanetManipulator.
 */
class OSSIMPLANET_DLL ossimPlanetViewer : public osgViewer::Viewer, 
                                          public ossimPlanetCallbackListInterface<ossimPlanetViewerCallback>,
                                          public ossimPlanetActionReceiver

{
public:
   class InitializePointersVisitor;
   friend class InitializePointersVisitor;

   /**
    * PickObject is used to hold the node path information when picking objects
    * in the scene.  For each object intersected a complete node path is stored.  
    * A higher level api is placed on PickObject to give access to the most common attributes.
    *
    * <pre>
    * Exmaple usage:
    *  assume we have a pointer to ossimPlanetViewer called viewer and we have variables wx and wy at the window location
    *  to pick.
    *
    *  std::vector<osg::ref_ptr<PickObject> > pickResult,
    *  viewer->pickAtWindowCoordinate(pickResult, wx,wy);
    *
    *  if(pickResult.size() > 0)
    *  {
    *    // now access the Picked objects.
    *  }
    * </pre>
    */
   class OSSIMPLANET_DLL PickObject : public osg::Referenced
   {
   public:
      typedef std::vector<osg::ref_ptr<osg::Node> > Path;
      /**
       * Used by the intersection routines to construct all the default values for the object.
       */
      PickObject(const osg::NodePath& nodePath, 
                 const osg::Vec3d& localPoint,
                 const osg::Vec3d& worldPoint,
                 const osg::Vec3d& llh)
      :theNodePath(nodePath.begin(), nodePath.end()),
       theLocalPoint(localPoint),
       theWorldPoint(worldPoint),
       theLatLonHeightPoint(llh)
      {}
      
      /**
       * @return the objects local point
       */
      const osg::Vec3d& localPoint()const{return theLocalPoint;}
      
      /**
       * @param value local point
       */
      void setLocalPoint(osg::Vec3d& value){theLocalPoint = value;}
      
      /**
       * This will return the objects world point.  Note this is in the space that the x,y,z coordinates are in
       * So if normalized ellipsoidalmode then values are in normalized ellipsoid. 
       */
      const osg::Vec3d& worldPoint()const{return theWorldPoint;}
      
      /**
       * @param value world point
       */
      void setWorldPoint(osg::Vec3d& value){theWorldPoint = value;}
      
      /**
       * This returns the lat, lon, height in the mode of the model.  By default the model uses wgs84 horizontal
       * datum.  The height is currently not shifted relative to any geoid so it is an ellipsoidal height at this point.
       */
      const osg::Vec3d& llh()const{return theLatLonHeightPoint;}
      void setllh(const osg::Vec3d& value){theLatLonHeightPoint = value;}
      
      /**
       * Will search the node path and return a pointer to the ossimPlanetNode that is first encountered. If
       * no ossimPlanetNode is encountered then null is returned
       */
      ossimPlanetNode* firstPlanetNode();
      
   protected:
      ossimPlanetViewer::PickObject::Path theNodePath;
      osg::Vec3d theLocalPoint;
      osg::Vec3d theWorldPoint;
      osg::Vec3d theLatLonHeightPoint;
   };
   
   /**
    * The NodeListener is used internally to ossimPlanetViewer to listen to things in the graph so it can effectively 
    * determine if a graph is at a static state.
    */
   class OSSIMPLANET_DLL NodeListener: public ossimPlanetNodeCallback
      {
      public:
         NodeListener(ossimPlanetViewer* viewer)
         :theViewer(viewer)
         {
            
         }
         virtual void nodeAdded(osg::Node* /*node*/);
         virtual void nodeRemoved(osg::Node* /*node*/);
         virtual void needsRedraw(ossimPlanetNode* node);
         
      protected:
         ossimPlanetViewer* theViewer;
      };
   friend class NodeListener;
   
   
   /**
    * Shorthand typedef of a list of Pick objects.
    */
   typedef std::vector<osg::ref_ptr<PickObject> > PickList;
   /**
    * Shorthand typedef of a PlanetNode list.
    */
   typedef std::vector<osg::ref_ptr<ossimPlanetNode> > PlanetNodeList;
   /**
    * Shorthand typedef to hold a list of hud nodes.
    */
   typedef std::vector<osg::ref_ptr<osg::Node> > HudList;
   
   /**
    * constructor for ossimPlanetViewer.  Initializing internal pointers and sets up the Update visitor
    * that will be used to determine if the graph needs refreshing.  
    */
   ossimPlanetViewer();
   /**
    * Constructs with arguments.  Note this is bridged for osgViewer since ossimPlanetViewer derives from it.
    */
   ossimPlanetViewer(osg::ArgumentParser& arguments);
   
   /**
    * Copy constructor.  Currently ossimPlanetViewer ignores this and passes on up to osgViewer.
    */
   ossimPlanetViewer(const osgViewer::Viewer& viewer, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
   
   /**
    * destorys and allocated data and removes callbacks form any internal nodes in the scene graph that ossim
    * PlanetViewer was listneeing to
    */
   virtual ~ossimPlanetViewer();
   
   /**
    *
    * Note only 1 ossimPlanet should exist in a graph.  ossimPlanetViewer searches for the ossimPlanet node and
    * will unset any previous setup pointers to the newly found ossimPlanet object.
    *
    */
   virtual void setSceneData(osg::Node* node);
 
   void initializeIncrementalCompile();

   /**
    * Gives acdcess to any allocated ossimPlanet object found in the setSceneData call.
    */
   ossimPlanet* planet();
   
   /**
    * Gives acdcess to any allocated ossimPlanet object found in the setSceneData call.
    */
   const ossimPlanet* planet()const;
   
   /**
    * Override this to mark the start of the framing
    */
   virtual void advance(double simulationTime=USE_REFERENCE_TIME);

   /**
    * virtual method override form base class.  We check the ossimPlanetUpdte visitor to see if any need refresh flags were found 
    * in the scnee graph.
    */
   virtual void updateTraversal();
      
   virtual void eventTraversal();
   /**
    * Utility method to give you access to the continuous equation model used for the ellipsoidal tesselation.
    */
   const ossimPlanetGeoRefModel* model()const;
   ossimPlanetGeoRefModel* model();
   
   /**
    * This will be reserved for executing messages routed to the view object.  No implementation yet for this method.
    */
   virtual void execute(const ossimPlanetAction& action);

   /**
    * This will return the eye origin and a normalized ray direction given a window coordinate.  This is unnormalized
    * window coordinate and assumes Y is up.  So if you are in a GUI where Y is down (left handed) then you must flip and make Y up
    * using some formula like: height() - Y.
    */
   virtual bool makeRayAtWindowCoordinate(osg::Vec3d& origin,
                                          osg::Vec3d& ray,
                                          double wx, double wy);
   
   
   /**
    * This is a utility method for picking objects.  The passed in wx and wy are in unnormalized window coordinates.  Assumed that
    * the passed in coordinates are right handed where Y is up.  So if you are in a GUI where Y is down (left haded) then you must flip and make Y up
    * using some formula like: height() - Y.
    */
   virtual bool pickAtWindowCoordinate(PickList& result,
                                       double wx, double wy, 
                                       osg::Node::NodeMask traversalMask = 0xffffffff);
   /**
    * This is a utility method for picking objects.  The passed in wx and wy are in unnormalized window coordinates.  Assumed that
    * the passed in coordinates are right handed where Y is up.  So if you are in a GUI where Y is down (left haded) then you must flip and make Y up
    * using some formula like: height() - Y.
    */
   virtual bool getLatLonHeightAtWindowCoordinate(osg::Vec3d& llh,
                                                  double wx, double wy,
                                                  osg::Node::NodeMask traversalMask=0xffffffff);

   /**
    * will add and manage the hud.
    */
//   bool AddHudOverlay(osg::ref_ptr<osg::Node> hudNode);
//   ossim_uint32 getNumberOfHudOverlays()const;
   
   /**
    * This will take the passed in anotation node and add it to the annotation layer in planet.
    *
    * @param annotation Annotation node to add to planet.
    */
   bool addAnnotation(osg::ref_ptr<ossimPlanetAnnotationLayerNode> annotation);
   
   /**
    *
    * This will take a texture layer and add it to the top of the reference layer.
    *
    * @param imageLayer Texture layer to be added.
    * @return true if the layer was added successfully.
    *
    */
   bool addImageTexture(osg::ref_ptr<ossimPlanetTextureLayer> imageTexture);
   
   /**
    * Will open all entries and return as a root image layer.  If this image file only contains a single
    * image then that layer is returned.  If multipl images are found then it will open each image and 
    * create a Layer group and return the images all grouped together.
    *
    * @param file The file to open adn add to the graph
    *
    * @return the Root layer node.
    */
   osg::ref_ptr<ossimPlanetTextureLayer> addImageTexture(const ossimString& file);
   
   /**
    * Allows one to add elevation database.
    *
    * @param the elevation database to add.
    * @param sortFlag will specify if you want to re-sort the database based on resolution.
    *
    */
   bool addElevation(osg::ref_ptr<ossimPlanetElevationDatabase> database, bool sortFlag=false);
   
   /**
    * Allows one to add a kml resource to the kml layer
    *
    **/
   void addKml(const ossimFilename& file);
   
   /**
    * This will take the current active camera and calculate ossimPlanetLookAt representations
    * for the eye and the lookat point.   
    *
    * You should not have to call this for it is already called after each update traversal.
    */
   void computeCurrentCameraInfo();
   
   /**
    * Utility to search the scene graph for objects of type ossimPlanetNode that are in the scene graph and has
    * the passed in id
    */
   void findNodesWithId(ossimPlanetViewer::PlanetNodeList& nodeList,
                        const ossimString& id);
   /**
    * Utility to search the scene graph for objects of type ossimPlanetNode that are in the scene graph and has
    * the passed in id
    */
   osg::ref_ptr<ossimPlanetNode> findFirstNodeWithId(const ossimString& id);
   
   
   /**
    * Lookat object is maintained at any view change within the ossimPlanetViewer.  You can at anytime obtain
    * the current lookat.
    */
   const ossimPlanetLookAt* currentLookAt()const{return theCurrentLookAt.get();}
   
   /**
    * Camera object is stored in the format of a lookat object where the range value is 0.  The Eye position and oritentation is
    * a special case of a look at object.  This is maintained at any view change automatically by ossimPlanetViewer
    * and can be queried at any time.
    *
    * @return Eye position and orientation in the form of a LookAt object.
    */
   const ossimPlanetLookAt* currentCamera()const{return theCurrentCamera.get();}
   
   /**
    * This currently may change.  We are exposing some control of how fast objects are added to the graph
    * at runtime.  This in particular is an interface added for ossimPlanetTerrain engine.  It will pass
    * the value on to the terrain and will indicate how many tile request operation to apply to the graph 
    * per frame.  So the higher the number the more requests that are finalized will be applied to the graph.
    *
    * Note:  Requests can be a split, update a mesh, or update a texture.  Each of these adds content to the 
    *        scene and how fast it's added can be controlled by this variable.
    */
   void setTerrainMaxNumberOfOperationsToApplyToGraphPerFrame(ossim_uint32 value);
   
   ossimPlanetManipulator* planetManipulator();
   ossimPlanetTerrain* terrainLayer();
   ossimPlanetAnnotationLayer* annotationLayer();
   ossimPlanetKmlLayer* kmlLayer();
   void addEphemeris(ossim_uint32 memberBitMask);
   void removeEphemeris();
   ossimPlanetEphemeris* ephemeris();
   
   /**
    * We will bridge the GUI action adapter so these values can be poled
    */
   virtual void requestRedraw();
   virtual void requestContinuousUpdate(bool needed=true);
   virtual void requestWarpPointer(float x,float y);
   
   
   /**
    * This is an atomic operation so if another request redraw comes in while you are querying
    * they will be in synch.  Instead of doing a separate query of the redraw flag and then 
    * a set on it for a new request could come in while in between those 2 calls and therefore you will
    * miss a frame.
    */
   bool getAndSetRedrawFlag(bool newValue);
   bool getRedrawFlag();
   void setRedrawFlag(bool flag);
   bool redrawFlag()const;
   void setContinuousUpdateFlag(bool flag);
   bool continuousUpdateFlag()const;
   bool warpPointerFlag()const;
   void getWarpPoints(float& x, float& y)const;

   /** 
    * All code contained in here was copied from OSG.  We will be adding fudge factor shifts
    * for close to earth intersections.
    *
    * Compute intersections between a ray through the specified master cameras window/eye coords and a specified node.
    * Note, when a master cameras has slaves and no viewport itself its coordinate frame will be in clip space i.e. -1,-1 to 1,1,
    * while if its has a viewport the coordintates will be relative to its viewport dimensions. 
    * Mouse events handled by the view will automatically be attached into the master camera window/clip coords so can be passed
    * directly on to the computeIntersections method. */
   bool computeIntersections(float x,
                             float y, 
                             osgUtil::LineSegmentIntersector::Intersections& intersections,
                             osg::Node::NodeMask traversalMask = 0xffffffff);
   
   /** 
    * All code contained in here was copied from OSG.  We will be adding fudge factor shifts
    * for close to earth intersections.
    *
    * Compute intersections between a ray through the specified master cameras 
    * window/eye coords and a specified nodePath's subgraph. 
    **/
   bool computeIntersections(float x,
                             float y, 
                             const osg::NodePath& nodePath, 
                             osgUtil::LineSegmentIntersector::Intersections& intersections,
                             osg::Node::NodeMask traversalMask = 0xffffffff);
   /**
    * This will return the eye origin and a normalized ray direction given a window coordinate.  This is unnormalized
    * window coordinate and assumes Y is up.  So if you are in a GUI where Y is down (left haded) then you must flip and make Y up
    * using some formula like: height() - Y.
    */
   virtual bool makeRayAtWindowCoordinate(osg::Vec3d& origin,
                                          osg::Vec3d& ray,
                                          osg::Camera* camera,
                                          double wx, double wy);
   
   void setIntersectWithMasterIfNotWithinAnyViewFlag(bool flag)
   {
      theIntersectWithMasterIfNotWithinAnyViewFlag = flag;
   }
   bool intersectWithMasterIfNotWithinAnyViewFlag()const
   {
      return theIntersectWithMasterIfNotWithinAnyViewFlag;
   }
   void setCalculateNearFarRatioFlag(bool flag)
   {
      theCalculateNearFarRatioFlag = flag;
   }
   bool calculateNearFarRatioFlag()const
   {
      return theCalculateNearFarRatioFlag;
   }
protected:
   friend class DrawCallback;
   const osg::Camera* forceAdjustToMasterCamera(float x, float y, float& local_x, float& local_y) const;
   
   void init();
   void notifyViewChanged();

   HudList theHudOverlayList;
   osg::ref_ptr<NodeListener> theCallback;
   osg::ref_ptr<ossimPlanet> thePlanet;
   osg::ref_ptr<ossimPlanetUpdateVisitor> theUpdateVisitor;
   
   osg::Matrixd theCurrentViewMatrix;
   osg::Matrixd theCurrentViewMatrixInverse;
   osg::ref_ptr<ossimPlanetLookAt> theCurrentCamera;
   osg::ref_ptr<ossimPlanetLookAt> theCurrentLookAt;
   osg::ref_ptr<ossimPlanetAnnotationLayer> theAnnotationLayer;
   osg::ref_ptr<ossimPlanetKmlLayer>  theKmlLayer;
   osg::ref_ptr<ossimPlanetLayer>     theTerrainLayer;
   osg::ref_ptr<ossimPlanetEphemeris> theEphemerisLayer;
   osg::ref_ptr<osg::Camera>          theEphemerisCamera;
   osg::ref_ptr<osg::Node>            theEphemerisRoot;
  
   osg::ref_ptr<osg::Group> theRootNode;
   osg::ref_ptr<osg::Light> theSavedLight;
   osg::Timer_t theFrameStartTimeStamp;
   /***  These are Gui Action adapter bridge ****/
   mutable OpenThreads::Mutex theActionAdapterMutex;
   bool theContinousUpdateFlag;
   bool theRedrawFlag;
   bool theWarpPointerFlag;
   float theWarpX, theWarpY;
   /*** Done with GUI Action adapter bridge ****/
   
   bool theIntersectWithMasterIfNotWithinAnyViewFlag;
   bool theCalculateNearFarRatioFlag;
   
};
#endif
