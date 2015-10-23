#include <ossim/init/ossimInit.h>
#include <ossimPlanet/ossimPlanetViewer.h>
#include <ossimPlanet/ossimPlanetManipulator.h>
#include <ossimPlanet/ossimPlanetTerrain.h>
#include <ossimPlanet/ossimPlanetTerrainGeometryTechnique.h>
#include <ossimPlanet/ossimPlanetAnimatedPointModel.h>
#include <ossimPlanet/ossimPlanetViewMatrixBuilder.h>
#include <ossimPlanet/ossimPlanetEphemeris.h>
#include <ossimPlanet/ossimPlanetOssimImageLayer.h>
#include <osgDB/ReadFile>
#include <osgGA/GUIEventHandler>
#include <osg/Camera>
#include <osg/CameraNode>
#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osgText/Text>
#include <osgText/Font>
#include <osg/PolygonMode>
#include <osgViewer/ViewerEventHandlers>
#include <ossimPlanet/ossimPlanetLatLonHud.h>
#include <ossimPlanet/ossimPlanetApi.h>
#include <ossimPlanet/ossimPlanetTextureLayerRegistry.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossim/base/ossimDirectory.h>
#include <ossim/init/ossimInit.h>
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/base/ossimGeoidEgm96.h>
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
#include <osg/LineWidth>
#include <osgUtil/CullVisitor>
#include <osg/ClipPlane>
#include <osg/Program>
#include <osg/Uniform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PolygonMode>
#include <osg/ref_ptr>
#include <osg/Texture>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osg/TextureCubeMap>
#include <ossimPlanet/ossimPlanetCloudLayer.h>

osg::Node* loadFlightGearModel(const ossimFilename& file,
                               ossimPlanetGeoRefModel* model, double scale = 1.0)
{
	osg::MatrixTransform* orientedModel = new osg::MatrixTransform;
	osg::Node* flightgearModel = osgDB::readNodeFile(file.c_str(), 0);
	osg::Matrixd mr;
	osg::Matrixd ms;
	double adjustedScale = scale/model->getNormalizationScale();
	ms.makeScale(osg::Vec3d(adjustedScale, adjustedScale, adjustedScale));
	mr.makeRotate(osg::DegreesToRadians(-90.0), osg::Vec3d(0.0,0.0,1.0),
                 osg::DegreesToRadians(-90.0), osg::Vec3d(0.0,1.0,0.0),
                 osg::DegreesToRadians(0.0), osg::Vec3d(1.0,0.0,0.0));
	orientedModel->setMatrix(ms*mr);
	if(flightgearModel)
	{
		orientedModel->addChild(flightgearModel);
	}
	return orientedModel;
}

class TestHandler : public osgGA::GUIEventHandler
{
public:
	enum TestStage
	{
		OSSIMPLANET_INIT_TERRAIN = 0,
		
		OSSIMPLANET_SET_GEOID,
		OSSIMPLANET_SET_ELEVATION_DENSITY,
		OSSIMPLANET_SET_CAP_LOCATION,
		OSSIMPLANET_ADD_IMAGE_TEXTURE,
		OSSIMPLANET_ADD_FLORIDA_DATA,
      OSSIMPLANET_ADD_REMOVE_TEXTURE_LAYER,
		OSSIMPLANET_SET_TEXTURE_DENSITY,
		OSSIMPLANET_ADD_ANNOTATION_NODE,
		OSSIMPLANET_ADJUST_HEADING,
		OSSIMPLANET_ADJUST_PITCH,
		OSSIMPLANET_ADJUST_ROLL,
		OSSIMPLANET_ADD_ELEVATION,
		OSSIMPLANET_ELEVATION_EXAGGERATION,
		OSSIMPLANET_INITIALIZE_EPHEMERIS,
		OSSIMPLANET_ADD_CLOUDS,
		OSSIMPLANET_ADD_FOG,
		OSSIMPLANET_EPHEMERIS_ANIMATE,
		OSSIMPLANET_ANIMATION_PATH,
		OSSIMPLANET_ANIMATION_PATH2,
		OSSIMPLANET_LOOKFROM,
      OSSIMPLANET_LOOKTO,
      OSSIMPLANET_LOOKTO_DISPLACEMENT,
      OSSIMPLANET_LOOKFROM_CONVERSION,
      OSSIMPLANET_LOOKFROM_CONVERSION_RANGE_FLATTEN,
      OSSIMPLANET_LOOKTO_ALONG_AXIS,
		OSSIMPLANET_LAST_TEST
	};

	enum texture_density
	{
		low_texture = 0,
		medium_low_texture,
		medium_texture,
		medium_high_texture,
		high_texture,
		last_texture
	};
	
	enum elevation_density
	{
		low_elevation = 0,
		medium_low_elevation,
		medium_elevation,
		medium_high_elevation,
		high_elevation,
		last_elevation
	};
	
	enum cap
	{
		low_cap = 0,
		medium_low_cap,
		medium_cap,
		medium_high_cap,
		high_cap,
		last_cap
	};
	
   enum AnimationFromMode
   {
      ANIMATION_FROM_INIT = 0,
      RELATIVE_FROM_GLOBAL_RANGE_NONE,
      RELATIVE_FROM_NONE,
      RELATIVE_FROM_HEADING,
      ANIMATION_FROM_STOP,
      RELATIVE_FROM_PITCH,
      RELATIVE_FROM_ROLL,
      RELATIVE_FROM_ALL,
      FROM_DISPLACEMENT_XYPLANE_RELATIVE_ALL_AXIS,
      FROM_DISPLACEMENT_XYPLANE_RELATIVE_NONE_AXIS,
      FROM_DISPLACEMENT_ZXPLANE_RELATIVE_ALL_AXIS,
      FROM_DISPLACEMENT_ZXPLANE_RELATIVE_NONE_AXIS,
      ANIMATION_FROM_LAST
   };
   
	enum AnimationToMode
   {
      ANIMATION_TO_MODE_INIT = 0,
      ANIMATION_TO_BOTH_MOVING_NO_RELATVE,
      ANIMATION_TO_LAST
   };
	enum FogVisibilityMode
   {
      FOG_CLEAR= 0,
		FOG_VERY_LIGHT,
		FOG_LIGHT,
		FOG_MEDIUM,
		FOG_MEDIUM_HEAVY,
		FOG_HEAVY,
      FOG_LAST
   };
	
	TestHandler()
	:
	theFontName("arial.ttf"),
	thePlanetManipulator(new ossimPlanetManipulator),
	theTestStage(-1),
	theSwitchModeKey('m'),
	theMaxTestStages(2),
	theUpdateHudTextFlag(true),
   theLookModeDuration(5.0)
	{
	}
   ossim_float64 simtime()const
   {
      return theSimTime;
   }
	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);
	void setRootDirectory(const ossimFilename& rootDir)
	{
		theRootDirectory = rootDir;
	}
protected:
	void updateCurrentStage(ossimPlanetViewer* viewer);
	void setupStage(ossimPlanetViewer* viewer);
	void initScene(ossimPlanetViewer* viewer);
	void updateText()
	{
		theHudText->setText(theHudTextString.c_str());
		// puts the text in the lower left corner.
		osg::BoundingBox bb = osg::BoundingBox();
		bb.expandBy(theHudText->getBound());
		theHudText->setPosition(osg::Vec3d(5.0,bb.yMax()-bb.yMin(),0.0));
	}
	void updateHudCoordinatesIfNeeded(ossimPlanetViewer* viewer)
	{
		// get master camera
		osg::ref_ptr<osg::Camera> camera           = viewer->getCamera();
		osg::ref_ptr<osg::Viewport> masterViewport = camera->getViewport();
		// now make sure the viewport is mirrored
		//
		double x = masterViewport->x();
		double y = masterViewport->y();
		double w = masterViewport->width();
		double h = masterViewport->height();
		
		if(!ossim::almostEqual(theHudViewport->x(),      x)||
		   !ossim::almostEqual(theHudViewport->y(),      y)||
		   !ossim::almostEqual(theHudViewport->width(),  w)||
		   !ossim::almostEqual(theHudViewport->height(), h))
		{
			theHudViewport->setViewport(x,y,w,h);
			theHudCamera->setProjectionMatrix(osg::Matrix::ortho2D(theHudViewport->x(),
                                                                theHudViewport->width(),
                                                                theHudViewport->y(),
                                                                theHudViewport->height()));
			updateText();
		}
	}
	osg::ref_ptr<osg::CameraNode> theHudCamera;
	osg::ref_ptr<osgText::Text> theHudText;
	std::string theFontName;
	osg::ref_ptr<osgText::Font> theFont;
	osg::ref_ptr<osg::Viewport> theHudViewport;
	osg::ref_ptr<osg::MatrixTransform> theRootScene;
	osg::ref_ptr<ossimPlanetManipulator> thePlanetManipulator;
	ossim_int32 theTestStage;
	char theSwitchModeKey;
	ossim_int32 theMaxTestStages;
	bool theUpdateHudTextFlag;
	ossimString theHudTextString;
   osg::ref_ptr<osg::Node>             theFlightGearModel;
   osg::ref_ptr<osg::MatrixTransform>  theFlightGearModelScaleTransform1;
	osg::ref_ptr<ossimPlanetPointModel> thePointModel1;
	osg::ref_ptr<ossimPlanetPointModel> thePointModel2;
	ossim_float64 theLookModeDuration;
	ossimFilename theRootDirectory;
	
	std::queue<ossimFilename> theTexturesToTest;
   
   double theSimTime;
	double theDuration;
   osg::Timer_t theStartTick;
	osg::Timer_t theStartTick2;
	osg::Timer_t lastHeadingTick;
   osg::Timer_t theLastAnimationModeTick;
   ossim_uint32 theAnimationFromMode;
   ossim_uint32 theAnimationToMode;
	ossim_uint32 texture_density;
	ossim_uint32 elevation_density;
	ossim_uint32 cap_location;
	ossim_uint32 fogMode;
   
   ossimPlanetTextureLayerGroup* theAddRemoveLayer;
   ossimString theSwapFile1;
   ossimString theSwapFile2;
#if 0
   ossimPlanetTextureLayerGroup* theSwapTestGroup;
   ossim_int32 theSwapTestIndex1;
   ossim_int32 theSwapTestIndex2;
#endif
};

bool TestHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) 
{ 
	ossimPlanetViewer* viewer = dynamic_cast<ossimPlanetViewer*>(&aa);
	if(viewer)
	{
		switch(ea.getEventType())
		{
			case osgGA::GUIEventAdapter::FRAME:
			{
				if(!theHudCamera.valid())
				{
					initScene(viewer);
				}
				updateHudCoordinatesIfNeeded(viewer);
				updateCurrentStage(viewer);
				if(theUpdateHudTextFlag)
				{
					theUpdateHudTextFlag = false;
					updateText();
				}
				break;
			}
			case osgGA::GUIEventAdapter::KEYDOWN:
			{
				if(ea.getKey() == theSwitchModeKey)
				{
					++theTestStage;
					theTestStage = theTestStage % OSSIMPLANET_LAST_TEST;
					setupStage(viewer);
					return true;
				}
			}
			default:
			{
				break;
			}
		}
	}
	return false; // go ahead and not consume any events and let them pass. 
}

void TestHandler::updateCurrentStage(ossimPlanetViewer* viewer)
{
	switch(theTestStage)
	{
		case OSSIMPLANET_SET_TEXTURE_DENSITY:
      {
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         bool textureDensityChanged = false;
         if(osg::Timer::instance()->delta_s(theLastAnimationModeTick, osg::Timer::instance()->tick()) > theDuration)
         {
            ++texture_density;
            texture_density = texture_density%last_texture;
            theLastAnimationModeTick = osg::Timer::instance()->tick();
            
            theUpdateHudTextFlag = true;
            textureDensityChanged = true;
            switch(texture_density)
            {
               case low_texture:
               {
                  viewer->terrainLayer()->setTextureDensityType(ossimPlanetTerrain::LOW_TEXTURE_DENSITY);
                  viewer->terrainLayer()->refreshImageLayers();
                  
                  theHudTextString = "setTextureDensity: Low Texture";
                  break;
               }
               case medium_low_texture:
               {
                  viewer->terrainLayer()->setTextureDensityType(ossimPlanetTerrain::MEDIUM_LOW_TEXTURE_DENSITY);
                  viewer->terrainLayer()->refreshImageLayers();
                  
                  theHudTextString = "setTextureDensity: Medium Low Texture";
                  break;
               }
               case medium_texture:
               {
                  viewer->terrainLayer()->setTextureDensityType(ossimPlanetTerrain::MEDIUM_TEXTURE_DENSITY);
                  viewer->terrainLayer()->refreshImageLayers();
                  
                  theHudTextString = "setTextureDensity: Medium Texture";
                  break;
               }
               case medium_high_texture:
               {
                  viewer->terrainLayer()->setTextureDensityType(ossimPlanetTerrain::MEDIUM_HIGH_TEXTURE_DENSITY);
                  viewer->terrainLayer()->refreshImageLayers();
                  
                  theHudTextString = "setTextureDensity: Medium High Texture";
                  break;
               }
               case high_texture:
               {
                  viewer->terrainLayer()->setTextureDensityType(ossimPlanetTerrain::HIGH_TEXTURE_DENSITY);
                  viewer->terrainLayer()->refreshImageLayers();
                  
                  theHudTextString = "setTextureDensity: High Texture";
                  break;
               }
            }
         }
         break;
      }
      case OSSIMPLANET_ADD_REMOVE_TEXTURE_LAYER:
      {
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         if(osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick()) > theDuration)
         {
            std::swap(theSwapFile1, theSwapFile2);
            theAddRemoveLayer->removeLayers(0, theAddRemoveLayer->numberOfLayers(), false);
            osg::ref_ptr<ossimPlanetTextureLayer> layer1 = ossimPlanetTextureLayerRegistry::instance()->createLayer(theSwapFile1);
            osg::ref_ptr<ossimPlanetTextureLayer> layer2 = ossimPlanetTextureLayerRegistry::instance()->createLayer(theSwapFile2);
            
            theAddRemoveLayer->addTop(layer1);
            theAddRemoveLayer->addTop(layer2);
            theHudTextString = "Swapping data";
            theStartTick = osg::Timer::instance()->tick();
         }
#if 0
         if(theSwapTestGroup&&
            (theSwapTestIndex1>=0)&&
            (theSwapTestIndex2>=0))
         {
            double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
            if(osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick()) > theDuration)
            {
               osg::ref_ptr<ossimPlanetTextureLayer> savedLayer = theSwapTestGroup->layer(theSwapTestIndex1);
               osg::ref_ptr<ossimPlanetTextureLayer> savedLayer2 = theSwapTestGroup->layer(theSwapTestIndex2);
               theSwapTestGroup->swapLayers(theSwapTestIndex1, theSwapTestIndex2);
               std::swap(theSwapTestIndex1, theSwapTestIndex2);
               theStartTick = osg::Timer::instance()->tick();
               theUpdateHudTextFlag = true;
               theHudTextString = "Swap texture: " + ossimString::toString(theSwapTestIndex1) + " with " 
               +ossimString::toString(theSwapTestIndex2);
#endif
//               viewer->terrainLayer()->refreshImageLayers(savedLayer->getExtents().get());
//               viewer->terrainLayer()->refreshImageLayers(savedLayer2->getExtents().get());
#if 0
               // now swap layers
               //
               osg::ref_ptr<ossimPlanetTextureLayer> savedLayer = theSwapTestGroup->layer(theSwapTestIndex1);
               osg::ref_ptr<ossimPlanetTextureLayer> savedLayer2 = theSwapTestGroup->layer(theSwapTestIndex2);
               theSwapTestGroup->replaceLayer(theSwapTestIndex1, savedLayer2.get());
               theSwapTestGroup->replaceLayer(theSwapTestIndex2, savedLayer.get());
               theHudTextString = "Swap texture: " + ossimString::toString(theSwapTestIndex1) + " with " 
                                   +ossimString::toString(theSwapTestIndex2);
               std::swap(theSwapTestIndex1, theSwapTestIndex2);
               
               theStartTick = osg::Timer::instance()->tick();
               theUpdateHudTextFlag = true;
               viewer->terrainLayer()->refreshImageLayers(savedLayer->getExtents().get());
               viewer->terrainLayer()->refreshImageLayers(savedLayer2->getExtents().get());
#endif
//           }
//         }
         break;
      }
      case OSSIMPLANET_SET_ELEVATION_DENSITY:
      {
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         if(osg::Timer::instance()->delta_s(theLastAnimationModeTick, osg::Timer::instance()->tick()) > theDuration)
         {
            ++elevation_density;
            elevation_density = elevation_density%last_elevation;
            theLastAnimationModeTick = osg::Timer::instance()->tick();
            
            theUpdateHudTextFlag = true;
            switch(elevation_density)
            {
               case low_elevation:
               {                  
                  viewer->terrainLayer()->setElevationDensityType(ossimPlanetTerrain::LOW_ELEVATION_DENSITY);
                  viewer->terrainLayer()->refreshElevationLayers();
                  
                  theHudTextString = "setTextureDensity: Low Elevation";
                  break;
               }
               case medium_low_elevation:
               {
                  viewer->terrainLayer()->setElevationDensityType(ossimPlanetTerrain::MEDIUM_LOW_ELEVATION_DENSITY);
                  viewer->terrainLayer()->refreshElevationLayers();
                  
                  theHudTextString = "setTextureDensity: Medium Low Elevation";
                  break;
               }				
               case medium_elevation:
               {
                  viewer->terrainLayer()->setElevationDensityType(ossimPlanetTerrain::MEDIUM_ELEVATION_DENSITY);
                  viewer->terrainLayer()->refreshElevationLayers();
                  
                  theHudTextString = "setTextureDensity: Medium Elevation";
                  break;
               }
               case medium_high_elevation:
               {
                  viewer->terrainLayer()->setElevationDensityType(ossimPlanetTerrain::MEDIUM_HIGH_ELEVATION_DENSITY);
                  viewer->terrainLayer()->refreshElevationLayers();
                  
                  theHudTextString = "setTextureDensity: Medium High Elevation";
                  break;
               }
               case high_elevation:
               {
                  viewer->terrainLayer()->setElevationDensityType(ossimPlanetTerrain::HIGH_ELEVATION_DENSITY);
                  viewer->terrainLayer()->refreshElevationLayers();
                  
                  theHudTextString = "setTextureDensity: High Elevation";
                  break;
               }
            }
         }
         break;
      }
		
		case OSSIMPLANET_SET_CAP_LOCATION:
      {
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         if(osg::Timer::instance()->delta_s(theLastAnimationModeTick, osg::Timer::instance()->tick()) > theDuration)
         {
            ++elevation_density;
            elevation_density = elevation_density%last_elevation;
            theLastAnimationModeTick = osg::Timer::instance()->tick();
            
            theUpdateHudTextFlag = true;
            switch(elevation_density)
            {
               case low_cap:
               {                  
						ossimPlanetAdjustableCubeGrid* grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::LOW_CAP);
						viewer->terrainLayer()->setGrid(grid);
						viewer->terrainLayer()->refreshImageLayers();
						viewer->terrainLayer()->refreshElevationLayers();
                  
                  theHudTextString = "setCapLocation: Low Cap";
                  break;
               }
					case medium_low_cap:
               {                  
						ossimPlanetAdjustableCubeGrid* grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::MEDIUM_LOW_CAP);
						viewer->terrainLayer()->setGrid(grid);
						viewer->terrainLayer()->refreshImageLayers();
						viewer->terrainLayer()->refreshElevationLayers();
                  
                  theHudTextString = "setCapLocation: Medium Low Cap";
                  break;
               }
					case medium_cap:
               {                  
						ossimPlanetAdjustableCubeGrid* grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::MEDIUM_CAP);
						viewer->terrainLayer()->setGrid(grid);
						viewer->terrainLayer()->refreshImageLayers();
						viewer->terrainLayer()->refreshElevationLayers();
                  
                  theHudTextString = "setCapLocation: Medium Cap";
                  break;
               }	
					case medium_high_cap:
               {                  
						ossimPlanetAdjustableCubeGrid* grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::MEDIUM_HIGH_CAP);
						viewer->terrainLayer()->setGrid(grid);
						viewer->terrainLayer()->refreshImageLayers();
						viewer->terrainLayer()->refreshElevationLayers();
                  
                  theHudTextString = "setCapLocation: Medium High Cap";
                  break;
               }
						
					case high_cap:
               {                  
						ossimPlanetAdjustableCubeGrid* grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::HIGH_CAP);
						viewer->terrainLayer()->setGrid(grid);
						viewer->terrainLayer()->refreshImageLayers();
						viewer->terrainLayer()->refreshElevationLayers();
                  
                  theHudTextString = "setCapLocation: High Cap";
                  break;
               }
            }
         }
         break;
      }
			
		case OSSIMPLANET_ADJUST_HEADING:
      {
         double currentHeading = thePointModel1->lsrSpace()->heading();
         const double timeDelta60Hertz = 1.0/60.0;
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         if(osg::Timer::instance()->delta_s(theLastAnimationModeTick, osg::Timer::instance()->tick()) > timeDelta60Hertz)
         {
            currentHeading += (360*timeDelta60Hertz);
            currentHeading = fmod(currentHeading, 360.0);
            thePointModel1->lsrSpace()->setHeadingPitchRoll(osg::Vec3d(currentHeading,0,0));
            theHudTextString = "Heading: " + ossimString::toString(currentHeading) + " degrees";
            theUpdateHudTextFlag = true;
            theLastAnimationModeTick = osg::Timer::instance()->tick();
         }
         break;
      }
			
		case OSSIMPLANET_ADJUST_PITCH:
      {
         double currentPitch = thePointModel1->lsrSpace()->pitch()+90.0;
         const double timeDelta60Hertz = 1.0/60.0;
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         if(osg::Timer::instance()->delta_s(theLastAnimationModeTick, osg::Timer::instance()->tick()) > timeDelta60Hertz)
         {
            currentPitch += (180*timeDelta60Hertz);
            currentPitch = fmod(currentPitch, 180.0);
            thePointModel1->lsrSpace()->setHeadingPitchRoll(osg::Vec3d(0.0,currentPitch-90.0,0));
            theHudTextString = "Pitch: " + ossimString::toString(currentPitch-90.0) + " degrees";
            theLastAnimationModeTick = osg::Timer::instance()->tick();
            theUpdateHudTextFlag = true;
         }
         break;
      }
		
		case OSSIMPLANET_ADJUST_ROLL:
      {
         double currentRoll = thePointModel1->lsrSpace()->roll()+90.0;
         const double timeDelta60Hertz = 1.0/60.0;
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         if(osg::Timer::instance()->delta_s(theLastAnimationModeTick, osg::Timer::instance()->tick()) > timeDelta60Hertz)
         {
            currentRoll += (180*timeDelta60Hertz);
            currentRoll = fmod(currentRoll, 180.0);
            thePointModel1->lsrSpace()->setHeadingPitchRoll(osg::Vec3d(0.0,0.0,currentRoll-90.0));
            theHudTextString = "Roll: " + ossimString::toString(currentRoll-90.0) + " degrees";
            theLastAnimationModeTick = osg::Timer::instance()->tick();
            theUpdateHudTextFlag = true;
         }
         break;
      }
		
		case OSSIMPLANET_EPHEMERIS_ANIMATE:
      {
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec*3600;
         theUpdateHudTextFlag = true;
         theHudTextString = "Initialize ephemeris: 3600x real time";
         
         break;
      }
      case OSSIMPLANET_ANIMATION_PATH:
      {
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         break;
      }
		case OSSIMPLANET_ANIMATION_PATH2:
      {
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         break;
      }
      case OSSIMPLANET_LOOKFROM:
      {
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         double range = (viewer->model()->calculateUnnormalizedLength(thePointModel1->getBound().radius()));
         if((osg::Timer::instance()->delta_s(theLastAnimationModeTick, osg::Timer::instance()->tick()) > theLookModeDuration)||
            theAnimationFromMode == ANIMATION_FROM_INIT)
         {
            ++theAnimationFromMode;
            theAnimationFromMode = theAnimationFromMode%ANIMATION_FROM_LAST;
            theLastAnimationModeTick = osg::Timer::instance()->tick();
            
            switch(theAnimationFromMode)
            {
               case RELATIVE_FROM_GLOBAL_RANGE_NONE:
               {
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   -range*10,
                                                                                   ossimPlanetViewMatrixBuilder::NO_ORIENTATION);
//                  thePlanetManipulator->viewMatrixBuilder()->setRange(-range*10);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(osg::Vec3d(0.0,0.0,0.0));
                  theHudTextString = "No realtive from global range mode";
                  break;
               }
               case RELATIVE_FROM_NONE:
               {
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   -range*10,
                                                                                   ossimPlanetViewMatrixBuilder::NO_ORIENTATION);
//                  thePlanetManipulator->viewMatrixBuilder()->setRange(0.0);
                  
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(osg::Vec3d(0.0,0.0,0.0));
                  
                  theHudTextString = "No realtive from mode";
                  break;
               }
               case RELATIVE_FROM_HEADING:
               {
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   -range*10,
                                                                                   ossimPlanetViewMatrixBuilder::HEADING);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(osg::Vec3d(0.0,0.0,0.0));
                  theHudTextString = "Relative Heading";
                  break;
               }
               case ANIMATION_FROM_STOP:
               {
                  thePlanetManipulator->viewMatrixBuilder()->invalidate();
                  theHudTextString = "Stop Animation";
                 break;
               }
               case RELATIVE_FROM_PITCH:
               {
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   -range*10,
                                                                                   ossimPlanetViewMatrixBuilder::PITCH);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(osg::Vec3d(0.0,0.0,0.0));
                  
                  theHudTextString = "Relative Pitch";
                  break;
               }
               case RELATIVE_FROM_ROLL:
               {
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   -range*10,
                                                                                   ossimPlanetViewMatrixBuilder::ROLL);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(osg::Vec3d(0.0,0.0,0.0));
                  theHudTextString = "Relative Roll";
                  break;
               }
               case RELATIVE_FROM_ALL:
               {
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   -range*10,
                                                                                   ossimPlanetViewMatrixBuilder::ALL_ORIENTATION);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(osg::Vec3d(0.0,0.0,0.0));
                  theHudTextString = "Relative All";
                  break;
               }
               case FROM_DISPLACEMENT_XYPLANE_RELATIVE_ALL_AXIS:
               {
                  double rotateAmount = osg::DegreesToRadians(360.0*(fmod(deltaSec, 3.0)/3.0));
                  osg::Matrixd rotate = osg::Matrixd::rotate(rotateAmount, 0.0,0.0,rotateAmount);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   0.0,
                                                                                   ossimPlanetViewMatrixBuilder::ALL_ORIENTATION);
                  osg::Vec3d displace = osg::Vec3d(0.0,1.0,0.0)*rotate;
                  displace = displace*(range*2);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(displace);
                  theHudTextString = "Displacement In XY Plane Relative All Axis";
                  break;
               }
               case FROM_DISPLACEMENT_XYPLANE_RELATIVE_NONE_AXIS:
               {
                  double rotateAmount = osg::DegreesToRadians(360.0*(fmod(deltaSec, 3.0)/3.0));
                  osg::Matrixd rotate = osg::Matrixd::rotate(rotateAmount, 0.0,0.0,rotateAmount);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   0.0,
                                                                                   ossimPlanetViewMatrixBuilder::NO_ORIENTATION);
                  osg::Vec3d displace = osg::Vec3d(0.0,1.0,0.0)*rotate;
                  displace = displace*(range*2);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(displace);
                  theHudTextString = "Displacement In XY Plane Relative No Axis (North aligned)";
                  break;
               }
               case FROM_DISPLACEMENT_ZXPLANE_RELATIVE_ALL_AXIS:
               {
                  double rotateAmount = osg::DegreesToRadians(360.0*(fmod(deltaSec, 3.0)/3.0));
                  osg::Matrixd rotate = osg::Matrixd::rotate(rotateAmount, 0.0,rotateAmount,0.0);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   0.0,
                                                                                   ossimPlanetViewMatrixBuilder::ALL_ORIENTATION);
                  osg::Vec3d displace = osg::Vec3d(0.0,0.0,1.0)*rotate;
                  displace = displace*(range*2);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(displace);
                  theHudTextString = "Displacement In XZ Plane Relative All Axis";
                  break;
               }
               case FROM_DISPLACEMENT_ZXPLANE_RELATIVE_NONE_AXIS:
               {
                  double rotateAmount = osg::DegreesToRadians(360.0*(fmod(deltaSec, 3.0)/3.0));
                  osg::Matrixd rotate = osg::Matrixd::rotate(rotateAmount, 0.0,rotateAmount,0.0);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   0.0,
                                                                                   ossimPlanetViewMatrixBuilder::NO_ORIENTATION);
                  osg::Vec3d displace = osg::Vec3d(0.0,0.0,1.0)*rotate;
                  displace = displace*(range*2);
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(displace);
                  theHudTextString = "Displacement In XZ Plane Relative No Axis (North aligned)";
                  break;
               }
            }
            
            theUpdateHudTextFlag = true;
         }
         switch(theAnimationFromMode)
         {
            case FROM_DISPLACEMENT_XYPLANE_RELATIVE_ALL_AXIS:
            {
               double rotateAmount = osg::DegreesToRadians(360.0*(fmod(deltaSec, 3.0)/3.0));
               osg::Matrixd rotate = osg::Matrixd::rotate(rotateAmount, 0.0,0.0,rotateAmount);
               osg::Vec3d displace = osg::Vec3d(0.0,1.0,0.0)*rotate;
               displace = displace*(range*2);
               thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(displace);
               break;
            }
            case FROM_DISPLACEMENT_XYPLANE_RELATIVE_NONE_AXIS:
            {
               double rotateAmount = osg::DegreesToRadians(360.0*(fmod(deltaSec, 3.0)/3.0));
               osg::Matrixd rotate = osg::Matrixd::rotate(rotateAmount, 0.0,0.0,rotateAmount);
               osg::Vec3d displace = osg::Vec3d(0.0,1.0,0.0)*rotate;
               displace = displace*(range*2);
               thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(displace);
               break;
            }
            case FROM_DISPLACEMENT_ZXPLANE_RELATIVE_ALL_AXIS:
            {
               double rotateAmount = osg::DegreesToRadians(360.0*(fmod(deltaSec, 3.0)/3.0));
               osg::Matrixd rotate = osg::Matrixd::rotate(rotateAmount, 0.0,rotateAmount,0.0);
               osg::Vec3d displace = osg::Vec3d(0.0,0.0,1.0)*rotate;
               displace = displace*(range*2);
               thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(displace);
               break;
            }
            case FROM_DISPLACEMENT_ZXPLANE_RELATIVE_NONE_AXIS:
            {
               double rotateAmount = osg::DegreesToRadians(360.0*(fmod(deltaSec, 3.0)/3.0));
               osg::Matrixd rotate = osg::Matrixd::rotate(rotateAmount, 0.0,rotateAmount,0.0);
               osg::Vec3d displace = osg::Vec3d(0.0,0.0,1.0)*rotate;
               displace = displace*(range*2);
               thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(displace);
               break;
            }
               
         }
         break;
      }
		case OSSIMPLANET_LOOKTO:
      {
         double range = (viewer->model()->calculateUnnormalizedLength(thePointModel1->getBound().radius()))*10;
         
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         if((osg::Timer::instance()->delta_s(theLastAnimationModeTick, osg::Timer::instance()->tick()) > 3)||
            (theAnimationToMode == ANIMATION_TO_MODE_INIT))
         {
            ++theAnimationToMode;
            theAnimationToMode = theAnimationToMode%ANIMATION_TO_LAST;
            theLastAnimationModeTick = osg::Timer::instance()->tick();
            switch(theAnimationToMode)
            {
               case ANIMATION_TO_BOTH_MOVING_NO_RELATVE:
               {
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromLocalDisplacement(osg::Vec3d(0.0,0.0,0.0));
                  thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
                                                                                   osg::Vec3d(0.0, 0.0, 0.0),
                                                                                   -range,
                                                                                   ossimPlanetViewMatrixBuilder::NO_ORIENTATION);
                  thePlanetManipulator->viewMatrixBuilder()->setLookToNode(thePointModel2.get());
                  thePlanetManipulator->viewMatrixBuilder()->setLookToLocalDisplacement(osg::Vec3d(0.0,0.0,0.0));
                  
//                  thePlanetManipulator->viewMatrixBuilder()->setRange(-range);
                  thePlanetManipulator->viewMatrixBuilder()->setAttitudeHpr(osg::Vec3d(0,0,0));
                  theHudTextString = "Animation To Both Moving No Relative";               
                  break;
               }
            }
            
            theUpdateHudTextFlag = true;
         }
         
         break;
      }
      case OSSIMPLANET_LOOKTO_DISPLACEMENT:
      case OSSIMPLANET_LOOKFROM_CONVERSION:
      case OSSIMPLANET_LOOKFROM_CONVERSION_RANGE_FLATTEN:
      {
         double deltaSec = osg::Timer::instance()->delta_s(theStartTick, osg::Timer::instance()->tick());
         theSimTime = deltaSec;
         break;
      }
      case OSSIMPLANET_LOOKTO_ALONG_AXIS:
      {
         break;
      }
      case OSSIMPLANET_ADD_FOG:
      {
         if(osg::Timer::instance()->delta_s(theLastAnimationModeTick, osg::Timer::instance()->tick()) > theDuration)
         {
            theLastAnimationModeTick = osg::Timer::instance()->tick();
            ++fogMode;
            fogMode = fogMode%FOG_LAST;
            
            switch(fogMode)
            {
               case FOG_CLEAR:
               {
                  viewer->ephemeris()->setFogEnableFlag(false);
                  theHudTextString = "Fog: Clear Day";
                  theUpdateHudTextFlag = true;
                  break;
               }
               case FOG_VERY_LIGHT:
               {
                  viewer->ephemeris()->setFogEnableFlag(true);
                  viewer->ephemeris()->setVisibility(1000000);
                  theHudTextString = "Fog: Very Light";
                  theUpdateHudTextFlag = true;
                  break;
               }
               case FOG_LIGHT:
               {
                  theHudTextString = "Fog: Light";
                  theUpdateHudTextFlag = true;
                  viewer->ephemeris()->setVisibility(500000);
                 break;
               }
               case FOG_MEDIUM:
               {
                  theHudTextString = "Fog: Medium Fog";
                  theUpdateHudTextFlag = true;
                  viewer->ephemeris()->setVisibility(200000);
                  break;
               }
               case FOG_MEDIUM_HEAVY:
               {
                  theHudTextString = "Fog: Medium Heavy Fog";
                  theUpdateHudTextFlag = true;
                  viewer->ephemeris()->setVisibility(100000);
                  break;
               }
               case FOG_HEAVY:
               {
                  theHudTextString = "Fog: Heavy Fog";
                  theUpdateHudTextFlag = true;
                  viewer->ephemeris()->setVisibility(50000);
                  break;
               }
            }
         }
         break;
      }
   }
}

void TestHandler::setupStage(ossimPlanetViewer* viewer)
{
	theUpdateHudTextFlag = true;
	switch (theTestStage)
	{
		// Init Terrain
		case OSSIMPLANET_INIT_TERRAIN:
		{
				// clear pointers that need clearing
				//
				thePointModel1 = 0;
				thePointModel2 = 0;
				theFlightGearModel = 0;
				theFlightGearModelScaleTransform1 = 0;
				thePlanetManipulator = 0;
				if(viewer->planet())
				{
					viewer->planet()->removeChildren(0, viewer->planet()->getNumChildren());
				}
				viewer->setSceneData(0);
				viewer->setCameraManipulator(0);
				viewer->removeEphemeris();
				
				// done clearing pointers
				
				// Now init the new pointers
				//
				thePlanetManipulator = new ossimPlanetManipulator;
				ossimPlanet* planet = new ossimPlanet;
				theRootScene = new osg::MatrixTransform();
				theRootScene->addChild(planet);
				theRootScene->addChild(theHudCamera.get());
				viewer->setSceneData(theRootScene.get());
				viewer->setCameraManipulator(thePlanetManipulator.get());
				viewer->planet()->addChild(new ossimPlanetTerrain);
				
				//  latLon Hud Overlay
				//ossimPlanetLatLonHud* latLonHud = new ossimPlanetLatLonHud();
				//viewer->planet()->addChild(latLonHud);
				
				osg::ref_ptr<ossimPlanetLookAt> lookAt = new ossimPlanetLookAt(0.0,
																									-89.999999999999,
																									20410038.400000,
																									0,
																									0,
																									0,
																									0.0,
																									ossimPlanetAltitudeMode_ABSOLUTE);
				thePlanetManipulator->navigator()->gotoLookAt(*lookAt, false);
				
				theHudTextString = "Terrain Test";
				theUpdateHudTextFlag = true;
				viewer->planet()->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::ON);
			
				// for any other stage we should have a root scene
				if(!theRootScene.valid()) return;
			
         viewer->addEphemeris(ossimPlanetEphemeris::SUN_LIGHT|
                              ossimPlanetEphemeris::SKY);
				
				break;
		}
		
		// Set Geoid
		case OSSIMPLANET_SET_GEOID:
		{
			ossimGeoidEgm96* geoid = new ossimGeoidEgm96(theRootDirectory.dirCat("geoid/egm96.grd"));
			viewer->planet()->model()->setGeoid(geoid);
			
			viewer->terrainLayer()->setElevationExaggeration(10000);
			viewer->terrainLayer()->refreshElevationLayers();
			
			theHudTextString = "setGeoid: " + theRootDirectory + "/geoid/egm96.grd\n";
			theHudTextString += "setElevationExaggeration: 10000x";
			
			break;
		}
		
		// Set Elevation Density
		case OSSIMPLANET_SET_ELEVATION_DENSITY:
		{
			viewer->terrainLayer()->setElevationExaggeration(1); // set elevation exaggeration back to 1x
			viewer->terrainLayer()->refreshElevationLayers();
			
			osg::StateSet* state = viewer->terrainLayer()->getOrCreateStateSet();
			osg::PolygonMode* polygonMode = new osg::PolygonMode(osg::PolygonMode::FRONT,osg::PolygonMode::LINE);
			state->setAttributeAndModes(polygonMode);
			
			theSimTime = 0.0;
			theDuration = 5.0; // duration between elevation density modes
			
			theStartTick = osg::Timer::instance()->tick();
			theLastAnimationModeTick = theStartTick;
			elevation_density = low_elevation;
			
			viewer->terrainLayer()->setElevationDensityType(ossimPlanetTerrain::LOW_ELEVATION_DENSITY);
			viewer->terrainLayer()->refreshElevationLayers();
			
			theUpdateHudTextFlag = true;
			theHudTextString = "setElevationDensity: Low Elevation Density";
			
			break;
		}
		
		// Set Cap Position
		case OSSIMPLANET_SET_CAP_LOCATION:
		{
			viewer->terrainLayer()->setElevationDensityType(ossimPlanetTerrain::LOW_ELEVATION_DENSITY); // set elevation density back to low
			viewer->terrainLayer()->refreshElevationLayers();
			
			osg::ref_ptr<ossimPlanetLookAt> lookAt = new ossimPlanetLookAt(45.000000000000,
                                                                        -89.999999999999,
                                                                        20410038.400000,
                                                                        0,
                                                                        0,
                                                                        0,
                                                                        ossimPlanetAltitudeMode_ABSOLUTE);
			thePlanetManipulator->navigator()->gotoLookAt(*lookAt, false);
			
			theSimTime = 0.0;
			theDuration = 5.0; // duration between cap location modes
			
			theStartTick = osg::Timer::instance()->tick();
			theLastAnimationModeTick = theStartTick;
			cap_location = low_cap;
			
			ossimPlanetAdjustableCubeGrid* grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::LOW_CAP);
			viewer->terrainLayer()->setGrid(grid);
			viewer->terrainLayer()->refreshImageLayers();
			viewer->terrainLayer()->refreshElevationLayers();
			
			theUpdateHudTextFlag = true;
			theHudTextString = "setCapLocation: Low Cap Location";
			
			break;	
		}
		
		// Add Image Texture
		case OSSIMPLANET_ADD_IMAGE_TEXTURE:
		{
			ossimPlanetAdjustableCubeGrid* grid = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::LOW_CAP); // set cap position back to low
			viewer->terrainLayer()->setGrid(grid);
			viewer->terrainLayer()->refreshImageLayers();
			viewer->terrainLayer()->refreshElevationLayers();
			
			osg::ref_ptr<ossimPlanetLookAt> lookAt = new ossimPlanetLookAt(0.000000000000,
                                                                        -89.999999999999,
                                                                        20410038.400000,
                                                                        0,
                                                                        0,
                                                                        0,
                                                                        ossimPlanetAltitudeMode_ABSOLUTE);
			thePlanetManipulator->navigator()->gotoLookAt(*lookAt, false);
			
			osg::StateSet* state = viewer->terrainLayer()->getOrCreateStateSet();
			osg::PolygonMode* polygonMode = new osg::PolygonMode(osg::PolygonMode::FRONT,osg::PolygonMode::FILL);
			state->setAttributeAndModes(polygonMode);
			
			viewer->addImageTexture(theRootDirectory.dirCat("images/textures/reference/earth.jpg"));
			viewer->terrainLayer()->refreshImageLayers();
			
			theHudTextString = "addImageTexture: " + theRootDirectory + "/images/textures/reference/earth.jpg";
			
			break;
		}
			
		// Add Florida Data
		case OSSIMPLANET_ADD_FLORIDA_DATA:
		{
			osg::ref_ptr<ossimPlanetLookAt> lookAt = new ossimPlanetLookAt(28.239487609273,
                                                                        -80.607619385637,
                                                                        1938857.036673,
                                                                        0,
                                                                        0,
                                                                        0,
                                                                        ossimPlanetAltitudeMode_ABSOLUTE);
			thePlanetManipulator->navigator()->gotoLookAt(*lookAt, false);
			
			viewer->addImageTexture(theRootDirectory.dirCat("images/textures/florida/brevard.tif"));
			viewer->terrainLayer()->refreshImageLayers();
			
			theHudTextString = "addImageTexture: adding Florida data";
			
			break;	
		}
		case OSSIMPLANET_ADD_REMOVE_TEXTURE_LAYER:
		{
         theAddRemoveLayer = new ossimPlanetTextureLayerGroup;
         viewer->addImageTexture(theAddRemoveLayer);
         theSwapFile1 = theRootDirectory.dirCat("images/textures/sanfran/sanfran.tif").c_str();
         theSwapFile2 = theRootDirectory.dirCat("images/textures/sanfran/sanfran_map.tif").c_str();
         
         osg::ref_ptr<ossimPlanetTextureLayer> layer1 = ossimPlanetTextureLayerRegistry::instance()->createLayer(theSwapFile1);
         osg::ref_ptr<ossimPlanetTextureLayer> layer2 = ossimPlanetTextureLayerRegistry::instance()->createLayer(theSwapFile2);
         if(layer1.valid())
         {
            layer1->resetLookAt();
            thePlanetManipulator->navigator()->gotoLookAt(*layer1->getLookAt(), false);
         }
         
         theAddRemoveLayer->addTop(layer1.get(), false);
         theAddRemoveLayer->addTop(layer2.get(), false);
			theHudTextString = "addImageTexture: adding Sanfran data for swap test";
			theStartTick = osg::Timer::instance()->tick();
			theDuration = .1; // duration between elevation density modes
#if 0
         osg::ref_ptr<ossimPlanetTextureLayer> layer1 = viewer->addImageTexture(theRootDirectory.dirCat("images/textures/sanfran/sanfran.tif"));
         theSwapTestGroup = 0;
         if(layer1.valid())
         {
            theSwapTestGroup = layer1->parent(0);
            layer1->resetLookAt();
            thePlanetManipulator->navigator()->gotoLookAt(*layer1->getLookAt(), false);
         }
			
         osg::ref_ptr<ossimPlanetTextureLayer> layer2 = viewer->addImageTexture(theRootDirectory.dirCat("images/textures/sanfran/sanfran_map.tif"));
         if(layer1.valid()&&layer2.valid()&&theSwapTestGroup)
         {
            theSwapTestIndex1 = theSwapTestGroup->findLayerIndex(layer1.get());
            theSwapTestIndex2 = theSwapTestGroup->findLayerIndex(layer2.get());
         }
         else
         {
            theSwapTestIndex1 = -1;
            theSwapTestIndex2 = -1;
         }
			theUpdateHudTextFlag = true;
			theHudTextString = "addImageTexture: adding Sanfran data for swap test";
			theStartTick = osg::Timer::instance()->tick();
			theDuration = .1; // duration between elevation density modes
#endif
			break;	
		}
         
		// Set Texture Density
		case OSSIMPLANET_SET_TEXTURE_DENSITY:
		{
			osg::ref_ptr<ossimPlanetLookAt> lookAt = new ossimPlanetLookAt(0.000000000000,
                                                                        -89.999999999999,
                                                                        20410038.400000,
                                                                        0,
                                                                        0,
                                                                        0,
                                                                        ossimPlanetAltitudeMode_ABSOLUTE);
			thePlanetManipulator->navigator()->gotoLookAt(*lookAt, false);
						
			theSimTime = 0.0;
			theDuration = 5.0; // duration between texture density modes
			
			theStartTick = osg::Timer::instance()->tick();
			theLastAnimationModeTick = theStartTick;
			texture_density = low_texture;
			
			viewer->terrainLayer()->setTextureDensityType(ossimPlanetTerrain::LOW_TEXTURE_DENSITY);
			viewer->terrainLayer()->refreshImageLayers();
			
			theUpdateHudTextFlag = true;
			theHudTextString = "setTextureDensity: Low Texture Density";
			
			break;
		}
		
		// Add Annotation Node
		case OSSIMPLANET_ADD_ANNOTATION_NODE:
		{
			viewer->terrainLayer()->setTextureDensityType(ossimPlanetTerrain::MEDIUM_TEXTURE_DENSITY); // set texture density back to medium
			viewer->terrainLayer()->refreshImageLayers();
			
			osg::ref_ptr<ossimPlanetLookAt> lookAt = new ossimPlanetLookAt(28.2395,
                                                                        -80.6076,
                                                                        100.9824,
                                                                        0,
                                                                        0,
                                                                        0,
                                                                        ossimPlanetAltitudeMode_ABSOLUTE);
			thePlanetManipulator->navigator()->gotoLookAt(*lookAt, false);
			
			if(!viewer->annotationLayer())
			{
				viewer->planet()->addChild(new ossimPlanetAnnotationLayer);
			}
			
			else
			{
				viewer->annotationLayer()->removeChildren(0, viewer->annotationLayer()->getNumChildren());
			}
			
			if(!theFlightGearModel.valid())
         {
          //  theFlightGearModel = loadFlightGearModel(theRootDirectory.dirCat("flight_models/f16/f16.osg"),viewer->planet()->model().get());
            theFlightGearModel = loadFlightGearModel(theRootDirectory.dirCat("flight_models/flightgear/Aircraft/f16/Models/f16.ac"),viewer->planet()->model().get());
            theFlightGearModelScaleTransform1 = new osg::MatrixTransform;
            theFlightGearModelScaleTransform1->addChild(theFlightGearModel.get());
				theFlightGearModelScaleTransform1->setMatrix(osg::Matrixd());
				
				thePointModel1 = new ossimPlanetPointModel();
            viewer->annotationLayer()->addChild(thePointModel1.get());
            thePointModel1->setNode(theFlightGearModelScaleTransform1.get());
				thePointModel1->lsrSpace()->setLatLonAltitude(osg::Vec3d(28.2395, -80.6076, 50.9824));
			}
			
			theHudTextString = "setNode: " + theRootDirectory + "flight_models/flightgear/Aircraft/f16/Models/f16.ac";
			
			break;	
		}
		
		// Adjust Heading
		case OSSIMPLANET_ADJUST_HEADING:
		{
			theSimTime = 0.0;
			
			theStartTick = osg::Timer::instance()->tick();
			theLastAnimationModeTick = theStartTick;
			
			break;	
		}
		
		// Adjust Pitch
		case OSSIMPLANET_ADJUST_PITCH:
		{
			theSimTime = 0.0;
			
			theStartTick = osg::Timer::instance()->tick();
			theLastAnimationModeTick = theStartTick;
			
			break;	
		}
		
		// Adjust Roll
		case OSSIMPLANET_ADJUST_ROLL:
		{
			theSimTime = 0.0;
			
			theStartTick = osg::Timer::instance()->tick();
			theLastAnimationModeTick = theStartTick;
			
			break;	
		}
		
		// Add the following...
		// lat jog
		// lon jog
		// alt jog
		// scale .5x 1x 2x
		
		// Add Elevation
		case OSSIMPLANET_ADD_ELEVATION:
		{
			osg::ref_ptr<ossimPlanetLookAt> lookAt = new ossimPlanetLookAt(35.421021580041,
																								-83.080005227486,
																								4062.174155,
																								0,
																								90,
																								0,
																								0.0,
																								ossimPlanetAltitudeMode_ABSOLUTE);
			thePlanetManipulator->navigator()->gotoLookAt(*lookAt, false);
			
			viewer->terrainLayer()->addElevation(theRootDirectory.dirCat("srtm30plus/"), 1);
			viewer->terrainLayer()->refreshElevationLayers();
			
			theHudTextString = "addElevation: " + theRootDirectory + "/srtm30plus/";
			
			break;
		}
		
		case OSSIMPLANET_ELEVATION_EXAGGERATION:
		{
			viewer->terrainLayer()->setElevationExaggeration(1.5);
			viewer->terrainLayer()->refreshElevationLayers();
			
			theHudTextString = "setElevationExaggeration: 1.5x";
			
			break;
		}
		
		case OSSIMPLANET_INITIALIZE_EPHEMERIS:
		{
			viewer->terrainLayer()->setElevationExaggeration(1); // set elevation exaggeration back to 1x
			viewer->terrainLayer()->refreshElevationLayers();
			
			osg::ref_ptr<ossimPlanetLookAt> lookAt = new ossimPlanetLookAt(35.421021580041,
																								-83.080005227486,
																								3562.174155,
																								0,
																								90,
																								0,
																								0.0,
																								ossimPlanetAltitudeMode_ABSOLUTE);
			thePlanetManipulator->navigator()->gotoLookAt(*lookAt, false);
						
			viewer->planet()->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::ON);
			
			theSimTime = 0.0;
			
			ossimFilename sunTextureFile = theRootDirectory.dirCat("images/icons/sun.png");
			ossimFilename moonTextureFile = theRootDirectory.dirCat("images/icons/moon.png");
			ossim_float64 visibility = 1000000000.0;
			ossim_float64 fogNear = 20.0;
			
			viewer->addEphemeris(ossimPlanetEphemeris::SUN_LIGHT
										//|ossimPlanetEphemeris::MOON_LIGHT
										//|ossimPlanetEphemeris::AMBIENT_LIGHT
										|ossimPlanetEphemeris::SUN
										|ossimPlanetEphemeris::MOON
										|ossimPlanetEphemeris::SKY
										|ossimPlanetEphemeris::FOG
										);
			
			ossimLocalTm date;
			date.now();
			viewer->ephemeris()->setDate(date);
			
			viewer->ephemeris()->setApplySimulationTimeOffsetFlag(false);
			viewer->ephemeris()->setSunTextureFromFile(sunTextureFile);
			viewer->ephemeris()->setMoonTextureFromFile(moonTextureFile);
			//viewer->ephemeris()->setGlobalAmbientLight(osg::Vec3d(0.1, 0.1, 0.1));
			viewer->ephemeris()->setVisibility(visibility);
			viewer->ephemeris()->setFogNear(fogNear);
			viewer->ephemeris()->setFogMode(ossimPlanetEphemeris::LINEAR);
			viewer->ephemeris()->setFogEnableFlag(true);
			
			theHudTextString = "addEphemeris: Real Time";
			
			break;	
		}
		
		// Add Clouds
		case OSSIMPLANET_ADD_CLOUDS:
		{
			ossim_int32 cloudCoverage = 20;
			ossim_float64 cloudSharpness = .95;
			ossim_float64 cloudAltitude = 20000;
			
			osg::ref_ptr<ossimPlanetCloudLayer> cloud = new ossimPlanetCloudLayer;
			viewer->planet()->addChild(cloud.get());
			
			cloud->computeMesh(cloudAltitude, 32,32,1);
			cloud->updateTexture(time(0), cloudCoverage, cloudSharpness);
			cloud->setSpeedPerHour(60.0, OSSIM_MILES);
			cloud->setScale(4);
			
			theHudTextString = "Enable Clouds";
			
			break;	
		}
			
		// Add Fog
		case OSSIMPLANET_ADD_FOG:
		{
			theSimTime = 0.0;
			theDuration = 5.0; // duration between fog modes
			
			theStartTick = osg::Timer::instance()->tick();
			theLastAnimationModeTick = theStartTick;
			fogMode = FOG_CLEAR;
			
			viewer->ephemeris()->setFogEnableFlag(false);
			
			theUpdateHudTextFlag = true;
			theHudTextString = "Enable Fog: Clear Day";
			
			break;	
		}
		
		case OSSIMPLANET_EPHEMERIS_ANIMATE:
			{
				viewer->ephemeris()->setApplySimulationTimeOffsetFlag(true);
				
				theStartTick = osg::Timer::instance()->tick();
				
				theUpdateHudTextFlag = true;
				theHudTextString = "addEphemeris: Sim time = 3600x";
				
				break;
			}
		
		case OSSIMPLANET_ANIMATION_PATH:
		{
			viewer->planet()->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OVERRIDE|osg::StateAttribute::OFF);
			
			osg::ref_ptr<ossimPlanetAnimationPath> animationPath = new ossimPlanetAnimationPath;
			
			theStartTick = osg::Timer::instance()->tick();
			
			theFlightGearModelScaleTransform1->setMatrix(osg::Matrixd::scale(50.0,50.0,50.0));
			theFlightGearModelScaleTransform1->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL, osg::StateAttribute::ON);
			
			animationPath->openAnimationPathByXmlDocument(ossimFilename(theRootDirectory.dirCat("flight_paths/flight1.xml")));
			thePointModel1->setCullingActive(false);
			theFlightGearModelScaleTransform1->setCullingActive(false);
			
			if(animationPath->geospatialPath()&&(!animationPath->geospatialPath()->empty()))
			{
				ossimPlanetAnimationPath::Tuple value = animationPath->geospatialPath()->timeTupleMap().begin()->second;
				osg::ref_ptr<ossimPlanetLookAt> lookAt = new ossimPlanetLookAt(value.position()[0],
																									value.position()[1],
																									value.position()[2],
																									0.0,
																									45.0,
																									0.0,
																									5000,
																									ossimPlanetAltitudeMode_ABSOLUTE);
				thePlanetManipulator->navigator()->gotoLookAt(*lookAt, false);
				
				osg::ref_ptr<ossimPlanetAnimatedPointModel> animatedPath = new ossimPlanetAnimatedPointModel();
				
				animationPath->setGeoRefModel(viewer->planet()->model().get());
				animationPath->setLoopMode(osg::AnimationPath::LOOP);
				animatedPath->setPointModel(thePointModel1.get());
				animatedPath->setAnimationPath(animationPath.get());
				animatedPath->setTimeScale(10.0);
				animatedPath->setTimeOffset(0.0);
				animatedPath->setAnimationPathColor(osg::Vec4f(2.0,0.0, 2.0, .5));
				
				viewer->annotationLayer()->addChild(animatedPath.get());
				theSimTime = 0.0;
				theHudTextString = "Add Animation Path 1";		
			}
			else
			{
				theHudTextString = "No points in flightpath file";		
			}	
			
		break;
		}
		
		case OSSIMPLANET_ANIMATION_PATH2:
		{
			theStartTick = osg::Timer::instance()->tick();
				if(!thePointModel2.valid())
				{
					thePointModel2 = new ossimPlanetPointModel;
					thePointModel2->setNode(theFlightGearModelScaleTransform1.get());
				}
				thePointModel2->setCullingActive(false);
				osg::ref_ptr<ossimPlanetAnimationPath> animationPath2 = new ossimPlanetAnimationPath;;
				
				animationPath2->openAnimationPathByXmlDocument(ossimFilename(theRootDirectory.dirCat("flight_paths/flight2.xml")));
				
				osg::ref_ptr<ossimPlanetAnimatedPointModel> animatedPath2 = new ossimPlanetAnimatedPointModel();
				
				animationPath2->setGeoRefModel(viewer->planet()->model().get());
				animationPath2->setLoopMode(osg::AnimationPath::LOOP);
				animatedPath2->setPointModel(thePointModel2.get());
				animatedPath2->setAnimationPath(animationPath2.get());
				animatedPath2->setTimeScale(62.0);
				animatedPath2->setTimeOffset(0.0);
				viewer->annotationLayer()->addChild(animatedPath2.get());
				theHudTextString = "Add Animation Path 2";		
		
				break;
			}
			
		case OSSIMPLANET_LOOKFROM:
		{
			
			thePlanetManipulator->setEventHandlingFlag(true);
			thePlanetManipulator->viewMatrixBuilder()->setLookAxis(ossimPlanetViewMatrixBuilder::LOOK_AXIS_Y);
			theSimTime = 0.0;
			theStartTick = osg::Timer::instance()->tick();
			theLastAnimationModeTick = theStartTick;
			theAnimationFromMode = ANIMATION_FROM_INIT;
			theHudTextString     = "No realtive from mode";
			theUpdateHudTextFlag = true;
			break;	
		}
			
		case OSSIMPLANET_LOOKTO:
		{
			thePlanetManipulator->setEventHandlingFlag(true);
			thePlanetManipulator->viewMatrixBuilder()->setLookAxis(ossimPlanetViewMatrixBuilder::LOOK_AXIS_Y);
			theSimTime = 0.0;
			theStartTick = osg::Timer::instance()->tick();
			theLastAnimationModeTick = theStartTick;
			theAnimationToMode = ANIMATION_TO_MODE_INIT;
			theHudTextString = "Animation To Both Moving No Relative";
			theUpdateHudTextFlag = true;
			
			break;
		}
			
		case OSSIMPLANET_LOOKTO_DISPLACEMENT:
		{
			
			thePlanetManipulator->setEventHandlingFlag(true);
			thePlanetManipulator->viewMatrixBuilder()->setLookAxis(ossimPlanetViewMatrixBuilder::LOOK_AXIS_Y);
			theSimTime = 0.0;
			theStartTick = osg::Timer::instance()->tick();
			theLastAnimationModeTick = theStartTick;
			double range = (viewer->model()->calculateUnnormalizedLength(thePointModel1->getBound().radius()))*10;
			
			// setup Local LSR to calculate displacement
			//
			thePlanetManipulator->viewMatrixBuilder()->setLookFromNodeOffset(thePointModel1.get(),
																								  osg::Vec3d(0.0, 0.0, 0.0),
																								  0.0,
																								  ossimPlanetViewMatrixBuilder::HEADING);
			
			thePlanetManipulator->viewMatrixBuilder()->setLookToNode(thePointModel2.get());
			thePlanetManipulator->viewMatrixBuilder()->setAttitudeHpr(osg::Vec3d(0.0,0.0, 0.0));
			thePlanetManipulator->viewMatrixBuilder()->setLookToLocalDisplacement(osg::Vec3d(0.0,range*.1,-range*2));
			theHudTextString = "Animation Look To displacement";
			theUpdateHudTextFlag = true;
			
			break;
		}
      case OSSIMPLANET_LOOKFROM_CONVERSION:
      {
         thePlanetManipulator->viewMatrixBuilder()->convertToAFromViewMatrix(false);
			theHudTextString = "Converted to a look from without flatten";
			theUpdateHudTextFlag = true;
         break;
      }
      case OSSIMPLANET_LOOKFROM_CONVERSION_RANGE_FLATTEN:
      {
         //thePlanetManipulator->viewMatrixBuilder()->convertToAFromViewMatrix(true);
         thePlanetManipulator->viewMatrixBuilder()->setParametersByMatrix(thePlanetManipulator->viewMatrixBuilder()->viewMatrix());
			theHudTextString = "Converted to a look from with range flattened";
			theUpdateHudTextFlag = true;
         break;
      }
      case OSSIMPLANET_LOOKTO_ALONG_AXIS:
      {
 			double range = (viewer->model()->calculateUnnormalizedLength(thePointModel1->getBound().radius()))*10;
         osg::Vec3d from(thePointModel1->lsrSpace()->latLonAltitude());
         osg::Vec3d to(thePointModel1->lsrSpace()->latLonAltitude());
         from[2]+=range;
         thePlanetManipulator->viewMatrixBuilder()->setLookFrom(from,
                                                                osg::Vec3d(0.0, 0.0, 0.0),
                                                                0.0);
         thePlanetManipulator->viewMatrixBuilder()->updateFromLocalDisplacement(osg::Vec3d(0.0,0.0,0.0));
         thePlanetManipulator->viewMatrixBuilder()->setLookTo(to);
			theHudTextString = "Looking down Z to a point";
			theUpdateHudTextFlag = true;
         break;
      }
		default:
			break;
	}
	
}

void TestHandler::initScene(ossimPlanetViewer* viewer)
{
	theFont = osgText::readFontFile(theFontName.c_str());
	osg::ref_ptr<osg::Geode> geode = new osg::Geode();
	osg::StateSet* stateset = geode->getOrCreateStateSet();
	stateset->setMode(GL_LIGHTING,
                     osg::StateAttribute::OFF);
	stateset->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
	stateset->setRenderBinDetails(11,"RenderBin");
	stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
	stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
	stateset->setAttribute(new osg::PolygonMode(), 
                          osg::StateAttribute::PROTECTED); // don't allow to go to wireframe
	theHudCamera = new osg::CameraNode;
	theHudViewport = new osg::Viewport(0,0,1024,1024);
	theHudText   = new osgText::Text;
	theHudText->setSupportsDisplayList(false);
	theHudText->setFont(theFont.get());
	geode->addDrawable(theHudText.get());
	theHudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	theHudCamera->setProjectionMatrix(osg::Matrix::ortho2D(theHudViewport->x(),
                                                          theHudViewport->width(),
                                                          theHudViewport->y(),
                                                          theHudViewport->height()));
	theHudCamera->setViewMatrix(osg::Matrix::identity());
	theHudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
	theHudCamera->setRenderOrder(osg::CameraNode::POST_RENDER);
	theHudCamera->addChild(geode.get());
	osg::BoundingBox bb = osg::BoundingBox();
	bb.expandBy(theHudText->getBound());	
	theHudText->setPosition(osg::Vec3d(5.0,bb.yMax()-bb.yMin(),0.0));
}

int main(int argc, char* argv[])
{
	ossimInit::instance()->initialize(argc, argv);
	
	osg::ArgumentParser arguments(&argc,argv);
	arguments.getApplicationUsage()->addCommandLineOption("--root-dir", "Root directory for testing");
	//add more arguments
	
	ossim_uint32 helpType = 0;
	
	if ((helpType = arguments.readHelpType()))
	{
		arguments.getApplicationUsage()->write(std::cout, helpType);
		return 1;
	}
	
	ossimString tempString;
	osg::ArgumentParser::Parameter stringParam(tempString);
	
	//Viewer and Test Handler Objects
	osg::ref_ptr<ossimPlanetViewer> viewer = new ossimPlanetViewer(arguments); // object for viewer
	osg::ref_ptr<TestHandler> testHandler = new TestHandler;
	//osg::ref_ptr<ossimPlanetTerrain> terrain = new ossimPlanetTerrain();
	
	//Event Handlers
	
	viewer->addEventHandler(testHandler.get());
	viewer->addEventHandler(new osgViewer::WindowSizeHandler);
	viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
	viewer->addEventHandler(new osgViewer::StatsHandler);
	
	if(arguments.read("--root-dir", stringParam))
	{
		testHandler->setRootDirectory(ossimFilename(tempString));
	}
	while(!viewer->done())
	{
		viewer->frame(testHandler->simtime());
	}
	
	return 0;
}
