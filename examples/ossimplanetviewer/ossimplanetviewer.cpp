#include <ossimPlanet/ossimPlanetTextureLayerRegistry.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossim/base/ossimDirectory.h>
#include <ossim/init/ossimInit.h>
#include <ossim/elevation/ossimElevManager.h>
#include <iostream>
#include <sstream>
#include <osg/ArgumentParser>
#include <osg/Texture>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osg/Drawable>
#include <osg/io_utils>
#include <time.h>
#include <osgViewer/ViewerEventHandlers>
#include <ossimPlanet/ossimPlanetViewer.h>
#include <ossimPlanet/ossimPlanetManipulator.h>
#include <ossimPlanet/ossimPlanetLatLonHud.h>
#include <ossimPlanet/ossimPlanetTerrain.h>
#include <ossimPlanet/ossimPlanetTerrainGeometryTechnique.h>
#include <ossimPlanet/ossimPlanetAnimatedPointModel.h>
#include <ossimPlanet/ossimPlanetViewMatrixBuilder.h>
#include <ossimPlanet/ossimPlanetCloudLayer.h>
#include <ossimPlanet/ossimPlanetEphemeris.h>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/AnimationPathManipulator>
#include <osgGA/TerrainManipulator>
#include <ossimPlanet/ossimPlanetAnimationPath.h>
#include <ossimPlanet/ossimPlanetKmlLayer.h>
#include <ossimPlanet/ossimPlanetAnimatedPointModel.h>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/LineWidth>
#include <osgText/Text>
#include <osgUtil/CullVisitor>
#include <osg/ClipPlane>

#include <osg/Program>
#include <osg/Uniform>

#include <osg/Geode>
#include <osg/Geometry>
void setUpViewForMultipipeTest(ossimPlanetViewer& viewer, ossim_uint32 numberOfPipes)
{
   //unsigned int width = 640, height = 480, aspectRatio = 1.33;
   
   osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
   if (!wsi)
   {
      osg::notify(osg::NOTICE)<<"setUpViewForMultipipeTest() : Error, no WindowSystemInterface available, cannot create windows."<<std::endl;
      return;
   }
   
   osg::DisplaySettings* ds = osg::DisplaySettings::instance();
   osg::GraphicsContext::ScreenIdentifier si;
   si.readDISPLAY();
   unsigned int screenWidth, screenHeight;
   wsi->getScreenResolution(si, screenWidth, screenHeight);
   ossim_float64 screenAspectRatio = (double)screenWidth/(double)screenHeight;
   ossim_float64 targetWidth = screenWidth/numberOfPipes;
   ossim_float64 targetHeight = screenHeight*.5;
   ossim_float64 aspectRatio  = targetWidth/targetHeight;//targetWidth/targetHeight;
   // displayNum has not been set so reset it to 0.
   if (si.displayNum<0) si.displayNum = 0;
   
   double translate_x = 0.0;
   for(unsigned int i=0; i<numberOfPipes; ++i)
   {
      translate_x += double(targetWidth) / (double(targetHeight) * aspectRatio);
   }
   
   for(unsigned int i=0; i<numberOfPipes; ++i)
   {
      osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
      traits->hostName = si.hostName;
      traits->displayNum = si.displayNum;
      traits->screenNum = si.screenNum;
      traits->x = i*targetWidth;
      traits->y = 0;
      traits->width = targetWidth;
      traits->height = targetHeight;
      traits->alpha = ds->getMinimumNumAlphaBits();
      traits->stencil = ds->getMinimumNumStencilBits();
      traits->windowDecoration = false;
      traits->doubleBuffer = true;
      traits->sharedContext = 0;
      traits->sampleBuffers = ds->getMultiSamples();
      traits->samples = ds->getNumMultiSamples();
      
      osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
      
      osg::ref_ptr<osg::Camera> camera = new osg::Camera;
      camera->setGraphicsContext(gc.get());
      
      osgViewer::GraphicsWindow* gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc.get());
      if (gw)
      {
         osg::notify(osg::INFO)<<"  GraphicsWindow has been created successfully."<<gw<<std::endl;
         
         gw->getEventQueue()->getCurrentEventState()->setWindowRectangle(traits->x, traits->y, traits->width, traits->height );
      }
      else
      {
         osg::notify(osg::NOTICE)<<"  GraphicsWindow has not been created successfully."<<std::endl;
      }
      
      camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
      
      GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
      camera->setDrawBuffer(buffer);
      camera->setReadBuffer(buffer);
#if 0      
      if(i==0)
      {
         // we will set a predraw callback here because the master camera is loosing the callback for some reason
         //
         camera->setPreDrawCallback(new ossimPlanetViewer::DrawCallback(&viewer));
      }
#endif
      double newAspectRatio = double(traits->width) / double(traits->height);
      double aspectRatioChange = newAspectRatio / aspectRatio;
      //double aspectRatioChange = newAspectRatio / screenAspectRatio;
      
//      viewer.addSlave(camera.get(), 
//                      (osg::Matrixd::translate( translate_x - aspectRatioChange, 0.0, 0.0) * 
//                       osg::Matrix::scale(1.0/aspectRatioChange,1.0,1.0)),
//                      osg::Matrixd(), true );
      viewer.addSlave(camera.get(), 
                      (osg::Matrixd::translate( translate_x - aspectRatioChange, 0.0, 0.0) * 
                      osg::Matrix::scale(1.0/aspectRatioChange,1.0,1.0)),
                      osg::Matrixd(), true );
      translate_x -= aspectRatioChange * 2.0;
   }
   viewer.assignSceneDataToCameras();
}
osg::Node* loadFlightGearModel(ossimFilename& file,
                               ossimPlanetGeoRefModel* model)
{
   osg::MatrixTransform* orientedModel = new osg::MatrixTransform;
   osg::Node* flightgearModel = osgDB::readNodeFile(file.c_str(), 0); 
   osg::Matrixd mr;
   osg::Matrixd ms;
   double scale = 50.0/model->getNormalizationScale();
   ms.makeScale(osg::Vec3d(scale, scale, scale));
   mr.makeRotate(osg::DegreesToRadians(-90.0), osg::Vec3d(0.0,0.0,1.0),
                 osg::DegreesToRadians(-90.0), osg::Vec3d(0.0,1.0,0.0),
                 osg::DegreesToRadians(0.0), osg::Vec3d(1.0,0.0,0.0));
   orientedModel->setMatrix(ms*mr);
   if(flightgearModel)
   {
      orientedModel->addChild(flightgearModel);
   }
  // orientedModel->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL, osg::StateAttribute::ON);
   return orientedModel;
}

void test(ossimPlanetGeoRefModel* model)
{
   osg::ref_ptr<ossimPlanetViewMatrixBuilder> builder = new ossimPlanetViewMatrixBuilder(model);
   osg::Vec3d llh, hpr;
   builder->setLookFrom(osg::Vec3d(45, -28, 3000),
                        osg::Vec3d(45,45,0.0),
                        0.0);
   
   osg::ref_ptr<ossimPlanetViewMatrixBuilder> builder2 = new ossimPlanetViewMatrixBuilder(builder->viewMatrix(),
                                                                                          model);
   
   builder->extractCompositedLlhHprParameters(llh, hpr);
   std::cout << "llh: " << llh << "\n" << "hpr: " << hpr << std::endl;
   
}

void testHprInvert()
{
   osg::ref_ptr<ossimPlanetViewMatrixBuilder> builder = new ossimPlanetViewMatrixBuilder;
   
   builder->setGeoRefModel(new ossimPlanetNormalizedEllipsoidModel());
   builder->setLookFrom(osg::Vec3d(56.7725, -3.84375, 6224.26),
                        osg::Vec3d(-7.69345, -10.4819, 0.0),
                        0.0);
   builder->setLookTo(osg::Vec3d(57.0337, -3.90847, 860.493));
   
   osg::Matrix targetOrientation = builder->viewMatrix();
   osg::Vec3d targetXaxis(targetOrientation(0,0), targetOrientation(0,1), targetOrientation(0,2));
   osg::Vec3d targetYaxis(targetOrientation(1,0), targetOrientation(1,1), targetOrientation(1,2));
   osg::Vec3d targetZaxis(targetOrientation(2,0), targetOrientation(2,1), targetOrientation(2,2));
   builder->setParametersByMatrix(builder->viewMatrix());
   //builder->convertToAFromViewMatrix();
   //builder->convertToAFromViewMatrix(true);

   osg::Matrix tempM=  builder->viewMatrix();
   
   osg::Vec3d xAxis(tempM(0,0), tempM(0,1), tempM(0,2));
   osg::Vec3d yAxis(tempM(1,0), tempM(1,1), tempM(1,2));
   osg::Vec3d zAxis(tempM(2,0), tempM(2,1), tempM(2,2));
   
   std::cout << "Result X = " << xAxis*targetXaxis << std::endl;
   std::cout << "Result Y = " << yAxis*targetYaxis << std::endl;
   std::cout << "Result Z = " << zAxis*targetZaxis << std::endl;
   
   
   std::cout << builder->fromPositionLlh() << std::endl;
   std::cout << builder->fromHpr() << std::endl;
   std::cout << builder->fromRange() << std::endl;
}
int main(int argc, char* argv[])
{
   ossimInit::instance()->initialize(argc, argv);
   
//   testHprInvert();
//   return 0;
   osg::ref_ptr<ossimPlanetGrid> grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::MEDIUM_CAP);  
   
   ossimString tempString;
   osg::ArgumentParser::Parameter stringParam(tempString);
   
   osg::ArgumentParser arguments(&argc,argv);
   arguments.getApplicationUsage()->addCommandLineOption("--terrain-requests-per-frame", 
                                                         "Specify the number of requests to apply to the terrain graph per frame");
   arguments.getApplicationUsage()->addCommandLineOption("--terrain-threads", 
                                                         "Specify the number of terrain threads to use  Specify between 1 and 3 for now");
	arguments.getApplicationUsage()->addCommandLineOption("--add-image", "Image to add");
	arguments.getApplicationUsage()->addCommandLineOption("--polar-cap", "Values can be LOW, MEDIUM_LOW, MEDIUM, MEDIUM_HIGH, HIGH");
	arguments.getApplicationUsage()->addCommandLineOption("--osg-view", "no ossimPlanet just use osg's readNode to add to scene");
	arguments.getApplicationUsage()->addCommandLineOption("--split-merge-speed", "enumeration that maps to split merge ratio.  Values can be LOW, MEDIUM_LOW, MEDIUM, MEDIUM_HIGH, and HIGH");
	arguments.getApplicationUsage()->addCommandLineOption("--elevation-density-type", "enumeration that maps amount of elevation posts to use per patch.  Values can be LOW, MEDIUM_LOW, MEDIUM, MEDIUM_HIGH, and HIGH");
	arguments.getApplicationUsage()->addCommandLineOption("--texture-density-type", "enumeration that maps amount of texture samples to use per patch.  Values can be LOW, MEDIUM_LOW, MEDIUM, MEDIUM_HIGH, and HIGH");
	arguments.getApplicationUsage()->addCommandLineOption("--elevation-exaggeration", "Scales the elevation by this factor");
	arguments.getApplicationUsage()->addCommandLineOption("--cull-amount", "Sets the amount of culling in the graph.  Values can be low, medium, high");
	arguments.getApplicationUsage()->addCommandLineOption("--min-time-compile", "sets the minimum time to compile per frame in seconds.  So 3 milliseconds is .003");
	arguments.getApplicationUsage()->addCommandLineOption("--animation-path", "Reads in an animation path");
	arguments.getApplicationUsage()->addCommandLineOption("--animation-node", "Reads in an animation node");
	arguments.getApplicationUsage()->addCommandLineOption("--add-kml", "Adds a kml layer to planet");
	arguments.getApplicationUsage()->addCommandLineOption("--moon-image", "Moon image used for the moon sprite for the Ephemeris");
	arguments.getApplicationUsage()->addCommandLineOption("--sun-image", "Moon image used for the moon sprite for the Ephemeris");
	arguments.getApplicationUsage()->addCommandLineOption("--visibility", "Visibility in meters");
	arguments.getApplicationUsage()->addCommandLineOption("--fog-near", "near plane for fog attenuation in meters");
	arguments.getApplicationUsage()->addCommandLineOption("--cloud-coverage", "integer value used to adjust the perlin noise");
	arguments.getApplicationUsage()->addCommandLineOption("--cloud-sharpness", "float value (0..1) used to adjust the perlin noise");
	arguments.getApplicationUsage()->addCommandLineOption("--cloud-altitude", "Altitude of the cloud");
	arguments.getApplicationUsage()->addCommandLineOption("--add-cloud", "float value (0..1) used to adjust the perlin noise");
   arguments.getApplicationUsage()->addCommandLineOption("--fog-near", "near plane for fog attenuation in meters");
   arguments.getApplicationUsage()->addCommandLineOption("--time-scale", "time scale for animation path locking");
   arguments.getApplicationUsage()->addCommandLineOption("--time-offset", "time offset");
   arguments.getApplicationUsage()->addCommandLineOption("--lockview-path", "lock the view to the path");
   arguments.getApplicationUsage()->addCommandLineOption("--add-ephemeris", "Adds the ephemeris defaults");



	ossim_uint32 helpType = 0;
	
	if ((helpType = arguments.readHelpType()))
	{
		arguments.getApplicationUsage()->write(std::cout, helpType);
		return 1;
	}
	
#if 1
   
   osg::MatrixTransform* rootScene = new osg::MatrixTransform();
   
  // osg::DisplaySettings::instance()->setRGB(true);
  // osg::DisplaySettings::instance()->setMaxNumberOfGraphicsContexts(1);
   ossimPlanetTerrainGeometryTechnique* technique = new ossimPlanetTerrainGeometryTechnique();
   ossimPlanetViewer viewer(arguments);
   ossimPlanetTerrain::CullAmountType cullAmount = ossimPlanetTerrain::HIGH_CULL;
   ossimPlanetTerrain::SplitMergeSpeedType splitMergeSpeed = ossimPlanetTerrain::LOW_SPEED;
   ossimPlanetTerrain::ElevationDensityType elevationDensity = ossimPlanetTerrain::LOW_ELEVATION_DENSITY;
   ossimPlanetTerrain::TextureDensityType textureDensity = ossimPlanetTerrain::MEDIUM_TEXTURE_DENSITY;
   std::vector<osg::ref_ptr<ossimPlanetAnimationPath> > animationPathArray;
   std::vector<osg::ref_ptr<ossimPlanetPointModel> > pointModels;
   std::vector<osg::ref_ptr<ossimPlanetAnimatedPointModel> > animatedPointModels;
   osg::ref_ptr<ossimPlanetGeoRefModel> model;
   osg::ref_ptr<ossimPlanetManipulator> manipulator = new ossimPlanetManipulator();
   osg::ref_ptr<ossimPlanetAnnotationLayer> annotationLayer =new ossimPlanetAnnotationLayer();
   osg::ref_ptr<ossimPlanetKmlLayer> kmlLayer;
   std::vector<ossimFilename> kmlFiles;
   ossimFilename flightgearFile = "";
   osg::ref_ptr<ossimPlanet> planet;
   double elevationExaggeration = 1.0;
   double minTimeToCompilePerFrame = .003;
   ossimFilename sunTextureFile = "";
   ossimFilename moonTextureFile = "";
   ossim_float64 visibility = 1000000000.0;
   ossim_float64 fogNear = 0.0;
   ossim_int32 cloudCoverage = 20;
   ossim_float64 cloudSharpness = .95;
   ossim_float64 cloudAltitude = 20000;
   double timeScale = 1.0;
   double timeOffset = 0.0;
   bool addCloud = false;
   bool lockViewToPathFlag = false;
   bool addEphemerisFlag = false;
   if(arguments.read("--lockview-path", stringParam))
   {
      lockViewToPathFlag =tempString.toBool();
   }
   if(arguments.read("--time-scale", stringParam))
   {
      timeScale = tempString.toDouble();
   }
   if(arguments.read("--time-offset", stringParam))
   {
      timeOffset = tempString.toDouble();
   }
   if(arguments.read("--cloud-coverage", stringParam))
   {
      cloudCoverage = tempString.toDouble();
   }
   if(arguments.read("--cloud-sharpness", stringParam))
   {
      cloudSharpness = tempString.toDouble();
   }
   if(arguments.read("--cloud-altitude", stringParam))
   {
      cloudAltitude = tempString.toDouble();
   }
   if(arguments.read("--add-ephemeris"))
   {
      addEphemerisFlag = true;
   }
   if(arguments.read("--add-cloud"))
   {
      addCloud = true;
   }
   if(arguments.read("--visibility", stringParam))
   {
      visibility = tempString.toDouble();
   }
   if(arguments.read("--fog-near", stringParam))
   {
      fogNear = tempString.toDouble();
   }
   while(arguments.read("--add-kml", stringParam))
   {
      kmlFiles.push_back(ossimFilename(tempString));
   }
   if(arguments.read("--animation-node", stringParam))
   {
      flightgearFile = ossimFilename(tempString);
   }
   while(arguments.read("--animation-path", stringParam))
   {
      osg::ref_ptr<ossimPlanetAnimationPath> animationPath = new ossimPlanetAnimationPath;
      
      if(animationPath->openAnimationPathByXmlDocument(ossimFilename(tempString)))
      {
         animationPathArray.push_back(animationPath.get());
      }
   }
   if(arguments.read("--moon-image", stringParam))
   {
      moonTextureFile = ossimFilename(tempString);
   }
   if(arguments.read("--sun-image", stringParam))
   {
      sunTextureFile = ossimFilename(tempString);
   }
   if(arguments.read("--min-time-compile", stringParam))
   {
      minTimeToCompilePerFrame = tempString.toDouble();
   }
   if(arguments.read("--elevation-exaggeration", stringParam))
   {
      elevationExaggeration = tempString.toDouble();
   }
   if(arguments.read("--texture-density-type", stringParam))
   {
      tempString = tempString.downcase();
      if(tempString == "low")
      {
         textureDensity = ossimPlanetTerrain::LOW_TEXTURE_DENSITY;
      }
      else if(tempString == "medium_low")
      {
         textureDensity = ossimPlanetTerrain::MEDIUM_LOW_TEXTURE_DENSITY;
      }
      else if(tempString == "medium")
      {
         textureDensity = ossimPlanetTerrain::MEDIUM_TEXTURE_DENSITY;
      }
      else if(tempString == "medium_high")
      {
         textureDensity = ossimPlanetTerrain::MEDIUM_HIGH_TEXTURE_DENSITY;
      }
      else if(tempString == "high")
      {
         textureDensity = ossimPlanetTerrain::HIGH_TEXTURE_DENSITY;
      }
   }
   if(arguments.read("--elevation-density-type", stringParam))
   {
      tempString = tempString.downcase();
      if(tempString == "low")
      {
         elevationDensity = ossimPlanetTerrain::LOW_ELEVATION_DENSITY;
      }
      else if(tempString == "medium_low")
      {
         elevationDensity = ossimPlanetTerrain::MEDIUM_LOW_ELEVATION_DENSITY;
      }
      else if(tempString == "medium")
      {
         elevationDensity = ossimPlanetTerrain::MEDIUM_ELEVATION_DENSITY;
      }
      else if(tempString == "medium_high")
      {
         elevationDensity = ossimPlanetTerrain::MEDIUM_HIGH_ELEVATION_DENSITY;
      }
      else if(tempString == "high")
      {
         elevationDensity = ossimPlanetTerrain::HIGH_ELEVATION_DENSITY;
      }
   }
   if(arguments.read("--split-merge-speed", stringParam))
   {
      tempString = tempString.downcase();
      if(tempString == "low")
      {
         splitMergeSpeed = ossimPlanetTerrain::LOW_SPEED;
      }
      
      else if(tempString == "medium_low")
      {
         splitMergeSpeed = ossimPlanetTerrain::MEDIUM_LOW_SPEED;
      }
      else if(tempString == "medium")
      {
         splitMergeSpeed = ossimPlanetTerrain::MEDIUM_SPEED;
      }
      else if(tempString == "medium_high")
      {
         splitMergeSpeed = ossimPlanetTerrain::MEDIUM_HIGH_SPEED;
      }
      else if(tempString == "high")
      {
         splitMergeSpeed = ossimPlanetTerrain::HIGH_SPEED;
      }
   }
   if(arguments.read("--polar-cap", stringParam))
   {
      tempString = tempString.downcase();
      if(tempString == "low")
      {
         grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::LOW_CAP);
      }
      else if(tempString == "medium_low")
      {
         grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::MEDIUM_LOW_CAP);
      }
      else if(tempString == "medium")
      {
         grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::MEDIUM_CAP);
      }
      else if(tempString == "medium_high")
      {
         grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::MEDIUM_HIGH_CAP);
      }
      else if(tempString == "high")
      {
         grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::HIGH_CAP);
      }
   }
   if(arguments.read("--cull-amount", stringParam))
   {
      tempString = tempString.downcase();
      if(tempString == "none")
      {
         cullAmount = ossimPlanetTerrain::NO_CULL;
      }
      if(tempString == "low")
      {
         cullAmount = ossimPlanetTerrain::LOW_CULL;
      }
      else if(tempString == "medium_low")
      {
         cullAmount = ossimPlanetTerrain::MEDIUM_LOW_CULL;
      }
      else if(tempString == "medium")
      {
         cullAmount = ossimPlanetTerrain::MEDIUM_CULL;
      }
      else if(tempString == "medium_high")
      {
         cullAmount = ossimPlanetTerrain::MEDIUM_HIGH_CULL;
      }
      else if(tempString == "high")
      {
         cullAmount = ossimPlanetTerrain::HIGH_CULL;
      }
   }
   
   viewer.addEventHandler( new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()) );
   
   // add the thread model handler
   viewer.addEventHandler(new osgViewer::ThreadingHandler);
   
   // add the window size toggle handler
   viewer.addEventHandler(new osgViewer::WindowSizeHandler);
   
   // add the stats handler
   viewer.addEventHandler(new osgViewer::StatsHandler);
   
   // add the help handler
   viewer.addEventHandler(new osgViewer::HelpHandler(arguments.getApplicationUsage()));
   
   // add the record camera path handler
   viewer.addEventHandler(new osgViewer::RecordCameraPathHandler);
   
   // add the LOD Scale handler
   // viewer.addEventHandler(new osgViewer::LODScaleHandler);
   
   // add the screen capture handler
   //   viewer.addEventHandler(new osgViewer::ScreenCaptureHandler);
   osg::ref_ptr<osg::Node> lockNode;
   if(arguments.read("--osg-view", stringParam))
   {
      osg::ref_ptr<osg::Node> n = osgDB::readNodeFile(tempString);
      if(n.valid())
      {
         osg::Group* group = new osg::Group();
         group->addChild(n.get());
         viewer.setSceneData(group);
      }
      osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> keyswitchManipulator = new osgGA::KeySwitchMatrixManipulator;
      keyswitchManipulator->addMatrixManipulator( '1', "Trackball", new osgGA::TrackballManipulator() );
      keyswitchManipulator->addMatrixManipulator( '2', "Flight", new osgGA::FlightManipulator() );
      keyswitchManipulator->addMatrixManipulator( '3', "Drive", new osgGA::DriveManipulator() );
      keyswitchManipulator->addMatrixManipulator( '4', "Terrain", new osgGA::TerrainManipulator() );
      viewer.setCameraManipulator(keyswitchManipulator.get());
      viewer.realize();
      
      while(!viewer.done())
      {
         viewer.frame();//time);
      }
      
      return 0;
   }
   else
   {
      planet = new ossimPlanet();
//      osg::ref_ptr<ossimPlanetLatLonHud> latLonHud = new ossimPlanetLatLonHud;
      model = planet->model().get();
      osg::ref_ptr<ossimPlanetTerrain> terrain = new ossimPlanetTerrain(grid.get());
      terrain->setTerrainTechnique(technique);
      terrain->setCullAmountType(cullAmount);
      terrain->setSplitMergeSpeedType(splitMergeSpeed);
      terrain->setTextureDensityType(textureDensity);
      terrain->setElevationDensityType(elevationDensity);
      terrain->setElevationExaggeration(elevationExaggeration);
      terrain->setMinimumTimeToCompilePerFrameInSeconds(minTimeToCompilePerFrame);
      //terrain->setTextureLayer(0, group);
      terrain->initElevation();
      planet->addChild(terrain.get());
      planet->addChild(annotationLayer.get());
//      planet->addChild(latLonHud.get());
      
      //     ossimPlanetEphemeris* ephemeris = new ossimPlanetEphemeris(ossimPlanetEphemeris::SUN_LIGHT);//|
      //ossimPlanetEphemeris::MOON_LIGHT);
      //planet->addChild(ephemeris);
      if(kmlFiles.size() > 0)
      {
         kmlLayer = new ossimPlanetKmlLayer();
         planet->addChild(kmlLayer.get());
         ossim_uint32 idx = 0;
         for(idx = 0; idx < kmlFiles.size(); ++idx)
         {
            kmlLayer->addKml(kmlFiles[idx]);
         }
      }
      if(!flightgearFile.empty())
      {
         osg::ref_ptr<osg::Node> flightgearNode = loadFlightGearModel(flightgearFile,
                                                                      planet->model().get());
         ossim_uint32 idx = 0;
         for(idx = 0; idx < animationPathArray.size(); ++idx)
         {
            osg::ref_ptr<ossimPlanetAnimatedPointModel> animatedNode = new ossimPlanetAnimatedPointModel();
            osg::ref_ptr<ossimPlanetPointModel> pointModel = new ossimPlanetPointModel;
            pointModel->setCullingActive(false);
            pointModel->setNode(flightgearNode.get());
            animationPathArray[idx]->setLoopMode(osg::AnimationPath::LOOP);
            animatedNode->setPointModel(pointModel.get());
            animatedNode->setAnimationPath(animationPathArray[idx].get());
            animatedNode->setTimeScale(timeScale);
            animatedNode->setTimeOffset(timeOffset);
            animatedNode->setShowModelFlag(true);
            annotationLayer->addChild(animatedNode.get());
            pointModels.push_back(pointModel.get());
            animatedPointModels.push_back(animatedNode.get());
            if(lockViewToPathFlag)
            {
               lockNode = pointModel.get(); 
            }   
         }
      }
      else if(!animationPathArray.empty()){
            osg::ref_ptr<ossimPlanetAnimatedPointModel> animatedNode = new ossimPlanetAnimatedPointModel();
            osg::ref_ptr<ossimPlanetPointModel> pointModel = new ossimPlanetPointModel;
            pointModel->setCullingActive(false);
            animationPathArray[0]->setLoopMode(osg::AnimationPath::LOOP);
            animatedNode->setPointModel(pointModel.get());
            animatedNode->setAnimationPath(animationPathArray[0].get());
            animatedNode->setTimeScale(timeScale);
            animatedNode->setTimeOffset(timeOffset);
            animatedNode->setShowModelFlag(false);
            annotationLayer->addChild(animatedNode.get());
            pointModels.push_back(pointModel.get());
            animatedPointModels.push_back(animatedNode.get());
            if(lockViewToPathFlag)
            {
               lockNode = pointModel.get(); 
            }   
      } 
      rootScene->addChild(planet.get());
      viewer.setSceneData( rootScene );
      
      osg::ref_ptr<osg::StateSet> sset = planet->getOrCreateStateSet();
      if(arguments.read("--terrain-requests-per-frame", stringParam))
      {
         ossim_int32 requestsPerFrame = ossimString(tempString).toInt32();
         if(requestsPerFrame <= 0)
         {
            requestsPerFrame = 1;
         }
         viewer.setTerrainMaxNumberOfOperationsToApplyToGraphPerFrame(requestsPerFrame);
      }
      while(arguments.read("--add-image", stringParam))
      {
         osg::ref_ptr<ossimPlanetTextureLayer> layer = ossimPlanetTextureLayerRegistry::instance()->createLayer(tempString);
         if(layer.valid())
         {
            viewer.addImageTexture(layer.get());
            
         }
      }	
      osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> keyswitchManipulator = new osgGA::KeySwitchMatrixManipulator;
      keyswitchManipulator->addMatrixManipulator( '1', "Standard", manipulator.get() );
      keyswitchManipulator->addMatrixManipulator( '2', "Trackball", new osgGA::TrackballManipulator() );
      keyswitchManipulator->addMatrixManipulator( '3', "Flight", new osgGA::FlightManipulator() );
      keyswitchManipulator->addMatrixManipulator( '4', "Drive", new osgGA::DriveManipulator() );
      keyswitchManipulator->addMatrixManipulator( '5', "Terrain", new osgGA::TerrainManipulator() );
      viewer.setCameraManipulator(keyswitchManipulator.get());
   }
  // test(planet->model().get());
  // return 0;
   viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
   
if(addEphemerisFlag)
{

   viewer.addEphemeris(ossimPlanetEphemeris::SUN_LIGHT
                       //|ossimPlanetEphemeris::MOON_LIGHT
                       //|ossimPlanetEphemeris::AMBIENT_LIGHT
                      // |ossimPlanetEphemeris::SUN
                       |ossimPlanetEphemeris::MOON
                       |ossimPlanetEphemeris::SKY
                       |ossimPlanetEphemeris::FOG
                       );
   ossimLocalTm date;
   date.now();
   viewer.ephemeris()->setDate(date);
   //viewer.ephemeris()->setApplySimulationTimeOffsetFlag(true);
   viewer.ephemeris()->setApplySimulationTimeOffsetFlag(true);
   viewer.ephemeris()->setSunTextureFromFile(sunTextureFile);
   viewer.ephemeris()->setMoonTextureFromFile(moonTextureFile);
   viewer.ephemeris()->setMoonScale(osg::Vec3d(1.0, 1.0, 1.0));
//   viewer.ephemeris()->setGlobalAmbientLight(osg::Vec3d(0.1, 0.1, 0.1));
   viewer.ephemeris()->setVisibility(visibility);
   viewer.ephemeris()->setFogNear(fogNear);
   viewer.ephemeris()->setFogMode(ossimPlanetEphemeris::LINEAR);
   //viewer.ephemeris()->setSunLightCallback(new ossimPlanetEphemeris::LightingCallback());
   double fov_y, aspect_ratio, z_near, z_far;
   viewer.getCamera()->getProjectionMatrixAsPerspective(fov_y, aspect_ratio, z_near, z_far);
   viewer.getCamera()->setProjectionMatrixAsPerspective(45, aspect_ratio, 1.0, 20.0);
   //osg::ref_ptr<ossimPlanetCloudLayer> cloud;
   if(addCloud)
   {
//      viewer.ephemeris()->setNumberOfCloudLayers(1);
#if 0
      viewer.ephemeris()->createCloudPatch(0,
                                           osg::Vec3d(28, -90, cloudAltitude),
                                           64,
                                           45,
                                           (ossim_uint64)time(0),
                                           cloudCoverage,
                                           cloudSharpness);
      //   viewer.ephemeris()->cloudLayer(0)->setMaxAltitudeToShowClouds(cloudAltitude*2.0);
#else
      //      viewer.ephemeris()->cloudLayer(0)->setGrid(new ossimPlanetCloudLayer::Patch(45.0, 45.0));
      viewer.ephemeris()->setNumberOfCloudLayers(1);
      viewer.ephemeris()->cloudLayer(0)->computeMesh(cloudAltitude, 128, 128, 0);
      viewer.ephemeris()->cloudLayer(0)->updateTexture(time(0), cloudCoverage, cloudSharpness);
      viewer.ephemeris()->cloudLayer(0)->setSpeedPerHour(1000, OSSIM_MILES);
      viewer.ephemeris()->cloudLayer(0)->setScale(3);
      viewer.ephemeris()->cloudLayer(0)->setMaxAltitudeToShowClouds(cloudAltitude*2.0);
#endif
   }
   viewer.terrainLayer()->setElevationMemoryCache(new ossimPlanetMemoryImageCache);
   viewer.terrainLayer()->elevationCache()->setMinMaxCacheSizeInMegaBytes(16, 20);
}
   if(arguments.read("--multi-pipe-test", stringParam))
   {
      setUpViewForMultipipeTest(viewer, tempString.toUInt32());
   }
   viewer.realize();
   
   //   viewer.setLightingMode(osg::View::HEADLIGHT);
   //   if(viewer.getLight())
   //   {
   //      osg::Vec4f amb(.3,.3,.3,1.0);
   //    viewer.getLight()->setAmbient(amb);
   //   }
   
   //   osg::ref_ptr<ossimPlanetViewMatrixBuilder> viewMatrixBuilder;
   //   if(planet.valid())
   //   {
   //      viewMatrixBuilder = new ossimPlanetViewMatrixBuilder(planet->model().get());
   //   }
   
   //animatedNode->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
   //viewer.setCameraManipulator(0);
   double time = 0.0;
   manipulator->setEventHandlingFlag(true);
   manipulator->viewMatrixBuilder()->setLookAxis(ossimPlanetViewMatrixBuilder::LOOK_AXIS_Y);
   if(lockNode.get())
   {
      manipulator->viewMatrixBuilder()->updateFromNode(lockNode.get());
   }
   while(!viewer.done())
   {
      viewer.frame();//time);
      time += 1.0/60.0;//60;//.016;
    }
   //return viewer.run();
#endif
}
