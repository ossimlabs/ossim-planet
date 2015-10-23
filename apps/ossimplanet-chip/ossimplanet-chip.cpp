#include <ossimPlanet/ossimPlanetApi.h>
#include <ossimPlanet/ossimPlanetTextureLayerRegistry.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossim/base/ossimDirectory.h>
#include <ossim/init/ossimInit.h>
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/imaging/ossimMemoryImageSource.h>
#include <ossim/imaging/ossimJpegWriter.h>
#include <iostream>
#include <sstream>
#include <osg/ArgumentParser>
#include <osg/Texture>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/Drawable>
#include <osg/io_utils>
#include <time.h>
#include <osgViewer/Renderer>
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


class CustomRenderer : public osgViewer::Renderer
{
public:
   CustomRenderer( osg::Camera* camera )
   : osgViewer::Renderer(camera), _cullOnly(true)
   {
   }
   
   void setCullOnly(bool on) { _cullOnly = on; }
   
   virtual void operator ()( osg::GraphicsContext* )
   {
      if ( _graphicsThreadDoesCull )
      {
         if (_cullOnly) 
         {
            cull();
         }
         else
         {
            cull_draw();
         }
      }
   }
   virtual void cull()
   {
      osgUtil::SceneView* sceneView = _sceneView[0].get();
      if ( !sceneView || _done  )
         return;
      
      updateSceneView( sceneView );
      
      osgViewer::View* view = dynamic_cast<osgViewer::View*>( _camera->getView() );
      if ( view )
         sceneView->setFusionDistance( view->getFusionDistanceMode(), view->getFusionDistanceValue() );
      sceneView->inheritCullSettings( *(sceneView->getCamera()) );
      sceneView->cull();
   }
   bool _cullOnly;
};

int main(int argc, char* argv[])
{
   ossimInit::instance()->initialize(argc, argv);
   
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
   
   
   
	ossim_uint32 helpType = 0;
	
	if ((helpType = arguments.readHelpType()))
	{
		arguments.getApplicationUsage()->write(std::cout, helpType);
		return 1;
	}
	
   
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
   double minTimeToCompilePerFrame = 10000;
   ossimFilename sunTextureFile = "";
   ossimFilename moonTextureFile = "";
   ossim_float64 visibility = 1000000000.0;
   ossim_float64 fogNear = 0.0;
   ossim_int32 cloudCoverage = 20;
   ossim_float64 cloudSharpness = .95;
   ossim_float64 cloudAltitude = 20000;
   bool addCloud = false;
   
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
   
   
   planet = new ossimPlanet();
   model = planet->model().get();
   osg::ref_ptr<ossimPlanetTerrain> terrain = new ossimPlanetTerrain(grid.get());
   terrain->setTerrainTechnique(technique);
   terrain->setCullAmountType(cullAmount);
   terrain->setSplitMergeSpeedType(splitMergeSpeed);
   terrain->setTextureDensityType(textureDensity);
   terrain->setElevationDensityType(elevationDensity);
   terrain->setElevationExaggeration(elevationExaggeration);
   terrain->setMinimumTimeToCompilePerFrameInSeconds(minTimeToCompilePerFrame);
   terrain->setPrecompileEnabledFlag(false);
   //terrain->setTextureLayer(0, group);
   terrain->initElevation();
   planet->addChild(terrain.get());
   planet->addChild(annotationLayer.get());
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
   viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
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

   osg::ref_ptr<osg::GraphicsContext> pbuffer;
   osg::Image *image = new osg::Image();

   osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
   traits->readDISPLAY();
   traits->alpha = 8;
   traits->width = 1920;
   traits->height = 1080;
   traits->pbuffer = true;
   pbuffer = osg::GraphicsContext::createGraphicsContext(traits.get());
   manipulator->setEventHandlingFlag(true);
   manipulator->viewMatrixBuilder()->setLookAxis(ossimPlanetViewMatrixBuilder::LOOK_AXIS_Y);
   viewer.setCameraManipulator(manipulator.get());
   
   osg::ref_ptr<CustomRenderer> renderer; 
   if (pbuffer.valid())
   {
      osg::Camera *camera = viewer.getCamera();
      renderer = new CustomRenderer( camera );
      camera->setGraphicsContext(pbuffer.get());
      camera->setDrawBuffer(GL_FRONT);
      camera->setReadBuffer(GL_FRONT);
      camera->setViewport(new osg::Viewport(0,0,traits->width,traits->height));
      double fovy, aspectRatio, near, far;
      camera->getProjectionMatrixAsPerspective(fovy, aspectRatio, near, far);
      double newAspectRatio = double(traits->width) / double(traits->height);
      double aspectRatioChange = newAspectRatio / aspectRatio;
      if (aspectRatioChange != 1.0)
         camera->getProjectionMatrix() *=
         osg::Matrix::scale(1.0/aspectRatioChange,1.0,1.0);
      
      camera->attach(osg::Camera::COLOR_BUFFER0, image);
      if(renderer.valid()) camera->setRenderer( renderer.get() );
   
      viewer.realize();
   }
   else 
   {
      std::cout << "Invalid pbuffer!!!!!!!!!!!!!!!!!!!!" << std::endl;
      exit(1);
   }
   double time = 0.0;
   GLenum pixelFormat = GL_RGB;
   bool gotTheImage = false;
   manipulator->viewMatrixBuilder()->setLookFrom(osg::Vec3d(37,87,15000),
                                                 osg::Vec3d(0, 0.0, 0.0),
                                                 0);   
   manipulator->viewMatrixBuilder()->setLookTo(osg::Vec3d(38,87,5500));   
   
   if(!viewer.done())
   {
      osg::Timer_t lastFrameTick = osg::Timer::instance()->tick();
      viewer.frame();
      //viewer.advance();
      if(renderer.valid())renderer->setCullOnly( true );
      while(viewer.getAndSetRedrawFlag(false)) 
      {
         viewer.frame();
//         viewer.eventTraversal();
//         viewer.updateTraversal();
//         viewer.renderingTraversals();
      }
      if(renderer.valid()) renderer->setCullOnly( false );
      viewer.frame();
      osg::Timer_t frameTick = osg::Timer::instance()->tick();
      double delta = osg::Timer::instance()->delta_s(lastFrameTick, frameTick);
      std::cout << "delta = " << delta << std::endl;
      if(!gotTheImage)
      {
         unsigned char* buffer = image->data();
         ossimRefPtr<ossimImageData> imageData = new ossimImageData(0,OSSIM_UINT8, 4, image->s(), image->t());
         imageData->initialize();
         
         imageData->loadTile(image->data(), ossimIrect(0,0,image->s()-1, image->t()-1), OSSIM_BIP);
         imageData->validate();
         ossimRefPtr<ossimMemoryImageSource> memSource = new ossimMemoryImageSource();
         memSource->setImage(imageData.get());
         ossimRefPtr<ossimJpegWriter> jpegWriter = new ossimJpegWriter();
         jpegWriter->connectMyInputTo(0, memSource.get());
         jpegWriter->setFilename(ossimFilename("/tmp/output.jpg"));
         jpegWriter->execute();
         
         delta = osg::Timer::instance()->delta_s(frameTick, osg::Timer::instance()->tick());
         
         std::cout << "SAVED /tmp/output.jpg in " << delta << " seconds" <<std::endl;
      }
   }
}
