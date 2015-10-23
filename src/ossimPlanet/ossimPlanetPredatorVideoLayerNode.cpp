#include <ossimPlanet/ossimPlanetPredatorVideoLayerNode.h>
#include <iostream>
#include <osgUtil/CullVisitor>
#include <osgGA/EventVisitor>
#include <osgGA/GUIActionAdapter>
#include <ossimPlanet/ossimPlanetVisitors.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetLand.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetVideoLayer.h>
#include <ossim/base/ossimMatrix4x4.h>
#include <ossim/base/ossimLsrSpace.h>
#include <sstream>
#include <ossim/base/ossimDate.h>
#include <ossim/elevation/ossimElevManager.h>

class ossimPlanetPredatorVideoLayerNodeTraverseCallback : public osg::NodeCallback
{
public:
   ossimPlanetPredatorVideoLayerNodeTraverseCallback()
      {
      }
   virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
      {
         if(node)
         {
            node->traverse(*nv);
         }
      }
};

ossimPlanetFFMpegImageStream::ossimPlanetFFMpegImageStream()
   :theBlock(new ossimPlanetRefBlock),
    theFrameRate(0.0),
    theSecondsToUpdate(0.0),
    theEnableFlag(true)
{
   _status = osg::ImageStream::PAUSED;
   _loopingMode = LOOPING;
}

ossimPlanetFFMpegImageStream::~ossimPlanetFFMpegImageStream()
{
}

void ossimPlanetFFMpegImageStream::setVideo(ossimRefPtr<ossimPredatorVideo> video)
{
   if(isRunning())
   {
      cancel();
   }
   theVideo = video;
   if(theVideo.valid())
   {
      theFrameRate = theVideo->videoFrameRate();
      theSecondsToUpdate = 1.0/theFrameRate;
      allocateImage(theVideo->imageWidth(),
                    theVideo->imageHeight(),
                    1,
                    GL_RGB,
                    GL_UNSIGNED_BYTE);  
      memset(data(),
             0,
             theVideo->imageWidth()*
             theVideo->imageHeight()*3);
      dirty();
      theDoneFlag = false;
      pause();
      theVideo->setFirstFrameFlag(false);
   }
}

int ossimPlanetFFMpegImageStream::cancel()
{
   int result = 0;

   if( isRunning() )
   {
      theDoneFlag = true;
      // release the frameBlock and _databasePagerThreadBlock incase its holding up thread cancelation.
      theBlock->release();
      
      // then wait for the the thread to stop running.
      while(isRunning())
      {
         OpenThreads::Thread::YieldCurrentThread();
      }
   }
   
   return result;
}
void ossimPlanetFFMpegImageStream::CoordinateInfo::setDescriptionToKlvMetadata()
{
   if(!theKlvTable.valid()) return;
   ossimString value;
   theDescription = "";
   if(theKlvTable->valueAsString(value, KLV_KEY_ORGANIZATIONAL_PROGRAM_NUMBER))
   {
      theDescription += ("Orgnizational Program Number: " + value + "<br>");
   }
   ossimDate date;
   if(theKlvTable->getDate(date, false))
   {
      std::stringstream temp;
      temp << std::setw(4) << date.getYear() <<"-"
      << std::setw(2) << std::setfill('0') << date.getMonth() << "-"
      << std::setw(2) << std::setfill('0') << date.getDay() << " "
      << std::setw(2) << std::setfill('0') << date.getHour() << ":"
      << std::setw(2) << std::setfill('0') << date.getMin() << ":"
      << std::setw(2) << std::setfill('0') << date.getSec() << "<br>";
      theDescription += ("Date: " + temp.str());
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_MISSION_NUMBER))
   {
      theDescription += "Mission ID: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_SECURITY_CLASSIFICATION_SET))
   {
      theDescription += "Security Classification: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_CLASSIFICATION_COMMENT))
   {
      theDescription += "Classification Comment: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_SECURITY_CAVEATS))
   {
      theDescription += "Security Caveats: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_ORIGINAL_PRODUCER_NAME))
   {
      theDescription += "Original Producer Name: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_PLATFORM_HEADING_ANGLE))
   {
      theDescription += "Platform Heading: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_PLATFORM_PITCH_ANGLE))
   {
      theDescription += "Platform Pitch: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_PLATFORM_ROLL_ANGLE))
   {
      theDescription += "Platform Roll: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_PLATFORM_DESIGNATION))
   {
      theDescription += "Designation: " + value + "<br>";
   }
   else if(theKlvTable->valueAsString(value, KLV_KEY_PLATFORM_DESIGNATION2))
   {
      theDescription += "Designation: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_IMAGE_SOURCE_SENSOR))
   {
      theDescription += "Image Source Sensor: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_IMAGE_COORDINATE_SYSTEM))
   {
      theDescription += "Coordinate System: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_SENSOR_LATITUDE))
   {
      theDescription += "Sensor Latitude: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_SENSOR_LONGITUDE))
   {
      theDescription += "Sensor Longitude: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_SENSOR_TRUE_ALTITUDE))
   {
      theDescription += "Sensor Altitude: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_SENSOR_HORIZONTAL_FOV))
   {
      theDescription += "Horizontal Field Of View: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_SENSOR_VERTICAL_FOV1))
   {
      theDescription += "Vertical Field Of View: " + value + "<br>";
   }
   else if(theKlvTable->valueAsString(value, KLV_KEY_SENSOR_VERTICAL_FOV2))
   {
      theDescription += "Vertical Field Of View: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_SLANT_RANGE))
   {
      theDescription += "Slant Range: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_OBLIQUITY_ANGLE))
   {
      theDescription += "Obliquity Angle: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_ANGLE_TO_NORTH))
   {
      theDescription += "Angle To North: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_TARGET_WIDTH))
   {
      theDescription += "Target Width: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_FRAME_CENTER_LATITUDE))
   {
      theDescription += "Frame Center Latitude: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_FRAME_CENTER_LONGITUDE))
   {
      theDescription += "Frame Center Longitude: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_FRAME_CENTER_ELEVATION))
   {
      theDescription += "Frame Center Elevation: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_CORNER_LATITUDE_POINT_1))
   {
      theDescription += "Corner Latitude 1: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_CORNER_LONGITUDE_POINT_1))
   {
      theDescription += "Corner Longitude 1: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_CORNER_LATITUDE_POINT_2))
   {
      theDescription += "Corner Latitude 2: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_CORNER_LONGITUDE_POINT_2))
   {
      theDescription += "Corner Longitude 2: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_CORNER_LATITUDE_POINT_3))
   {
      theDescription += "Corner Latitude 3: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_CORNER_LONGITUDE_POINT_3))
   {
      theDescription += "Corner Longitude 3: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_CORNER_LATITUDE_POINT_4))
   {
      theDescription += "Corner Latitude 4: " + value + "<br>";
   }
   if(theKlvTable->valueAsString(value, KLV_KEY_CORNER_LONGITUDE_POINT_4))
   {
      theDescription += "Corner Longitude 4: " + value + "<br>";
   }
   
}

void ossimPlanetFFMpegImageStream::run()
{
   if(!theVideo.valid()) return;
   
   while(!theDoneFlag)
   {
      theBlock->block();
      if(theDoneFlag) return;
      ossimRefPtr<ossimPredatorVideo::FrameInfo> frame = theVideo->nextFrame();
      osg::Timer_t lastFrameTick;
      lastFrameTick = osg::Timer::instance()->tick();
      if(frame.valid())
      {
         memcpy(data(),
                frame->rgbBuffer(),
                frame->rgbBufferSizeInBytes());
         dirty();
         ossimRefPtr<ossimPredatorKlvTable> klvTable = frame->klvTable();
         if(klvTable.valid())
         {
//             ossimPlanetFFMpegImageStream::CoordinateInfo coord;
            ossim_float64 lat=0.0, lon=0.0, elev=0.0;
            ossim_float64 flat=0.0, flon=0.0, felev=0.0;
//             ossim_float32 h=0.0, p=0.0, r=0.0;
            ossim_float32 obliquityAngle=0.0;
//             ossim_float32 slantRange=0.0;
//             ossim_float32 hfov=0.0;
//             ossim_float32 vfov=0.0;
            ossim_float32 targetWidthInMeters=1.0;
            ossim_float32 angleToNorth=0.0;
//             ossim_float32 roll=0.0;

            // if we at least have a frame center and a target width let's go on that
            
            if(klvTable->getTargetWidthInMeters(targetWidthInMeters)&&
               klvTable->getFrameCenter(flat, flon, felev))
            {
               osg::ref_ptr<CoordinateInfo> coordInfo = new CoordinateInfo;
               coordInfo->theKlvTable = klvTable->dup();
               coordInfo->theReferenceTime = theVideo->referenceTime();
               coordInfo->theClock = theVideo->videoClock();
               ossim_float32 slantRange=0.0;
               ossimGpt pt1, pt2, pt3, pt4;

               if(theModel.valid())
               {
                  felev = theModel->getHeightAboveEllipsoid(flat, flon);
               }
               else
               {
                  // this is a hack for now until I fix the model synching problem.
                  //
                  felev = ossimElevManager::instance()->getHeightAboveEllipsoid(ossimGpt(flat, flon));
                  if(ossim::isnan(felev))
                  {
                     felev = 0;
                  }
               }
               coordInfo->theFrameCenterLlhValidFlag = true;
               coordInfo->theFrameCenterLlh = osg::Vec3d(flat, flon, felev);
               coordInfo->theTargetWidthInMetersValidFlag = true;
               coordInfo->theTargetWidthInMeters = targetWidthInMeters;
               if(klvTable->getCornerPoints(pt1, pt2, pt3, pt4))
               {
                  coordInfo->theCornerPoints.resize(4);
                  coordInfo->theCornerPoints[0] = osg::Vec3d(pt1.latd(), pt1.lond(), pt1.height());
                  coordInfo->theCornerPoints[1] = osg::Vec3d(pt2.latd(), pt2.lond(), pt2.height());
                  coordInfo->theCornerPoints[2] = osg::Vec3d(pt3.latd(), pt3.lond(), pt3.height());
                  coordInfo->theCornerPoints[3] = osg::Vec3d(pt4.latd(), pt4.lond(), pt4.height());
               }
               else
               {
                  coordInfo->theCornerPoints.clear();
               }
               // now let's see if we can have full orientation.  This will be the best we can do fr now.
               // The meta data doesn't seem to fit line of site.  So, we will get what we can and then rotate that
               // into the platform center to frame center vector so at least the center pixel projects to the meta 
               // data frame center.
               //
               if(klvTable->getSlantRange(slantRange))
               {
                  coordInfo->theSlantRange           = slantRange;
               }
               else
               {
                  coordInfo->theSlantRangeValidFlag = false;
               }
               if(klvTable->getAngleToNorth(angleToNorth)&&
                  klvTable->getObliquityAngle(obliquityAngle)&&
                  klvTable->getSensorPosition(lat, lon, elev))
               {
                  if(ossim::almostEqual(elev, 0.0)&&theModel.valid())
                  {
                     elev = theModel->getHeightAboveEllipsoid(lat, lon);
                  }
                  else if(theModel.valid())
                  {
                     elev += theModel->getGeoidOffset(lat, lon);
                  }
                  
                  ossimGpt sensorGpt(lat, lon, elev);
                  ossimGpt frameGpt(flat, flon, felev);
                  osg::Matrixd orientationMatrix;
                  
                  ossimLsrSpace lsrSpace(sensorGpt,
                                         angleToNorth);
                  ossimMatrix4x4 lsrMatrix(lsrSpace.lsrToEcefRotMatrix());
                  NEWMAT::Matrix orientation = (ossimMatrix4x4::createRotationXMatrix(obliquityAngle,
                                                                                      OSSIM_LEFT_HANDED));
                  ossimEcefPoint sensorEcefPosition      = sensorGpt;
                  ossimEcefPoint frameCenterEcefPosition = frameGpt;
                  ossimEcefVector zAxis = sensorEcefPosition - frameCenterEcefPosition;
                  ossimEcefVector yCol(orientation[0][1],
                                       orientation[1][1],
                                       orientation[2][1]);
                  ossimEcefVector xAxis = yCol.cross(zAxis);
                  ossimEcefVector yAxis = zAxis.cross(xAxis);
                  xAxis.normalize();
                  yAxis.normalize();

                  coordInfo->theOrientationMatrix(0, 0) = xAxis.x();
                  coordInfo->theOrientationMatrix(0, 1) = xAxis.y();
                  coordInfo->theOrientationMatrix(0, 2) = xAxis.z();
                  coordInfo->theOrientationMatrix(1, 0) = yAxis.x();
                  coordInfo->theOrientationMatrix(1, 1) = yAxis.y();
                  coordInfo->theOrientationMatrix(1, 2) = yAxis.z();
                  coordInfo->theOrientationMatrix(2, 0) = zAxis.x();
                  coordInfo->theOrientationMatrix(2, 1) = zAxis.y();
                  coordInfo->theOrientationMatrix(2, 2) = zAxis.z();

                  coordInfo->theSensorPositionLlh[0] = lat;
                  coordInfo->theSensorPositionLlh[1] = lon;
                  coordInfo->theSensorPositionLlh[2] = elev;

               }
               else
               {
                  coordInfo->theOrientationMatrixValidFlag = false;
               }
               coordInfo->setDescriptionToKlvMetadata();
               if(theCallback.valid())
               {
                  theCallback->coordinatesChanged(klvTable,
                                                  coordInfo);
               }
            }
         }
      }
      if(!frame.valid())
      {
         if(_loopingMode == NO_LOOPING)
         {
            theDoneFlag = true;
         }
         else
         {
//             theMutex.lock();
            rewind();
            if(theCallback.valid())
            {
               theCallback->referenceTimeChanged(0.0);
            }
//             theMutex.unlock();
         }
      }
      else if(theCallback.valid())
      {
//          std::cout << "TIME CHANGE = " << getReferenceTime() << "\n";
         theCallback->referenceTimeChanged(getReferenceTime());
      }
      // now delay based on frame rate
      // we later need to add support for slow play and fast play and
      // seeking.  For now we do constant source frame rate
      //
      double delta = osg::Timer::instance()->delta_s(lastFrameTick, osg::Timer::instance()->tick());
      if((delta < theSecondsToUpdate)&&
			(delta >= 0.0))
      {
         //double idolTime = theSecondsToUpdate-delta;
         OpenThreads::Thread::microSleep(theSecondsToUpdate*1e6);
      }
      
      updateThreadBlock();
   }
}

void ossimPlanetFFMpegImageStream::play()
{
   _status = osg::ImageStream::PLAYING;
   if (!isRunning())
   {
      if(theVideo.valid())
      {
         theDoneFlag = false;
         theVideo->setFirstFrameFlag(false);
         updateThreadBlock();
         start();
      }
   }
   updateThreadBlock();
}

void ossimPlanetFFMpegImageStream::pause()
{
   _status = ImageStream::PAUSED;
   updateThreadBlock();
}

void ossimPlanetFFMpegImageStream::rewind()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theVideo->setFirstFrameFlag(false);
   theVideo->seek(0.0, ossimPredatorVideo::SEEK_ABSOLUTE);
   updateThreadBlock();
}

void ossimPlanetFFMpegImageStream::quit(bool waitForThreadToExit)
{
   theDoneFlag = true;
   if(waitForThreadToExit)
   {
      cancel();
   }
   else
   {
      // make sure we are not blocked
      theBlock->release();
   }
}

double ossimPlanetFFMpegImageStream::getLength() const
{
   return theVideo.valid()?theVideo->duration():0.0;
}

void ossimPlanetFFMpegImageStream::setReferenceTime(double t)
{
   // need to add the seeking based on time offset
   //
   if(theVideo.valid())
   {
      theVideo->seek(t, ossimPredatorVideo::SEEK_ABSOLUTE);
      updateThreadBlock();
   }
}

double ossimPlanetFFMpegImageStream::getReferenceTime()
{
   if(theVideo.valid())
   {
      return theVideo->referenceTime();
   }
   return 0.0;
}

void ossimPlanetFFMpegImageStream::setTimeMultiplier(double)
{
}

double ossimPlanetFFMpegImageStream::getTimeMultiplier()
{
   return 0.0;
}

void ossimPlanetFFMpegImageStream::setVolume(float)
{
}

float ossimPlanetFFMpegImageStream::getVolume()
{
   return 0.0;
}

ossimPlanetPredatorVideoLayerNode::ossimPlanetPredatorVideoLayerNode(ossimPlanetVideoLayer* layer)
   :ossimPlanetVideoLayerNode(layer)
{
   
   theAspect = 1.0;
   theInvAspect = 1.0;
   theGeometryFlag = false;
   theCurrentFrame = new ossimPlanetFFMpegImageStream;
   theCallback = new ossimPlanetPredatorVideoLayerNode::ImageStreamCallback(this);
   theCurrentFrame->setCallback(theCallback.get());
   setUpdateCallback(new ossimPlanetPredatorVideoLayerNodeTraverseCallback);
   setEventCallback(new ossimPlanetPredatorVideoLayerNodeTraverseCallback);
   setCullCallback(new ossimPlanetPredatorVideoLayerNodeTraverseCallback);
   theCameraNode = new osg::CameraNode;
   theSwitchNode = new osg::Switch;
   theIcon = new ossimPlanetIconGeom;
   theCameraNode = new osg::CameraNode;
   theSharedGeode = new osg::Geode;
   theCullFlag = false;
   //    theIcon->setGeometry(osg::Vec3d(0.0,0.0,0.0),
//                         osg::Vec3d(0.0, 0.0, 0.0),
//                         osg::Vec3d(0.0, 0.0, 0.0));
   theSharedGeode->addDrawable(theIcon.get());

//    theGroup = new osg::Group;
   theCameraNode->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
   theCameraNode->setViewMatrix(osg::Matrix::identity());
   theCameraNode->setClearMask(GL_DEPTH_BUFFER_BIT);
   theCameraNode->setRenderOrder(osg::CameraNode::POST_RENDER);
   osg::StateSet* stateset = theCameraNode->getOrCreateStateSet();
   stateset->setMode(GL_LIGHTING,
                     osg::StateAttribute::OFF);
//   stateset->setMode(GL_COLOR_MATERIAL,
//                     osg::StateAttribute::OFF);
   stateset->setMode(GL_DEPTH_TEST,
                     osg::StateAttribute::OFF);
   stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
   stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
   theCameraNode->setStateSet(stateset);
//    addChild(theCameraNode.get());
   the2DMatrixTransform = new osg::MatrixTransform;
   the2DMatrixTransform->addChild(theSharedGeode.get());
   theCameraNode->addChild(the2DMatrixTransform.get());


   // SETUP for billboard option
   //
   theBillboard = new ossimPlanetBillboardIcon;
   theBillboardMatrixTransform = new osg::MatrixTransform;
   theBillboardMatrixTransform->addChild(theBillboard.get());
   stateset = theBillboard->getOrCreateStateSet();
   stateset->setMode(GL_LIGHTING,
                     osg::StateAttribute::OFF);
//   stateset->setMode(GL_COLOR_MATERIAL,
//                     osg::StateAttribute::OFF);
   stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
//    stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
   theBillboard->setStateSet(stateset);
   theSwitchNode->addChild(theCameraNode.get());
   theSwitchNode->addChild(theBillboardMatrixTransform.get());
  
   addChild(theSwitchNode.get());
//   setRenderMode(ossimPlanetVideoLayerNode::RENDER_SCREEN);
   setRenderMode(ossimPlanetVideoLayerNode::RENDER_BILLBOARD);
   
}

ossimPlanetPredatorVideoLayerNode::~ossimPlanetPredatorVideoLayerNode()
{
   
   theCurrentFrame->setCallback(0);
   theCurrentFrame->cancel();
}

bool ossimPlanetPredatorVideoLayerNode::open(const ossimFilename& file)
{
   ossimRefPtr<ossimPredatorVideo> video = new ossimPredatorVideo;
   
   bool result = video->open(file);
   if(!result)
   {
      video = 0;
   }
   else
   {
      setName(file.file().c_str());
   }
   setVideo(video.get());
   return result;
}

void ossimPlanetPredatorVideoLayerNode::setVideo(ossimPredatorVideo* video)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePredatorTraverseMutex);
   theCurrentFrame->cancel();

   theGeometryFlag = false;
   theVideo = video;
   theLookAt = 0;
   if(theVideo.valid())
   {
      theFrameRate = theVideo->videoFrameRate();
      theSecondsToUpdate = 1.0/theFrameRate;
      theCurrentFrame->setVideo(video);
      theIcon->setTexture(theCurrentFrame.get());
      osg::ref_ptr<osg::Vec3Array> coords = dynamic_cast<osg::Vec3Array*>(theIcon->getVertexArray());

      the2DMatrixTransform->setMatrix(osg::Matrixd::scale(osg::Vec3d(theVideo->imageWidth(),
                                                                     theVideo->imageHeight(),
                                                                     1.0))*
                                      osg::Matrixd::translate(osg::Vec3d(theVideo->imageWidth()*.5,
                                                                         theVideo->imageHeight()*.5,
                                                                         0.0)));
      theAspect = (ossim_float64)theVideo->imageWidth()/(ossim_float64)theVideo->imageHeight();
      theInvAspect = 1.0/theAspect;
      theSharedTexture = new osg::Texture2D;
      theSharedTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture2D::LINEAR);
      theSharedTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture2D::LINEAR);
      theSharedTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
      theSharedTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
      theSharedTexture->setWrap(osg::Texture2D::WRAP_R, osg::Texture2D::CLAMP_TO_EDGE);
      theSharedTexture->setDataVariance(osg::Object::DYNAMIC);
      theSharedTexture->setResizeNonPowerOfTwoHint(false);
      theSharedTexture->setImage(theCurrentFrame.get());
      theIcon->setTexture(theSharedTexture.get());
      theViewportChangedFlag = false;

      // set anything needed for the billboard on initialization
      theBillboard->setGeom(theIcon.get());
      theBillboard->setMinPixelSize(64);

      setRedrawFlag(true);
      theCurrentFrame->updateThreadBlock();
      theCurrentFrame->start();
      // force a reset on any icon geometries
      setRenderMode(renderMode());
   }
}

void ossimPlanetPredatorVideoLayerNode::pause()
{
   theCurrentFrame->pause();
   setRedrawFlag(true);
   
}

void ossimPlanetPredatorVideoLayerNode::rewind()
{
   theCurrentFrame->rewind();
   setRedrawFlag(true);
}

void ossimPlanetPredatorVideoLayerNode::play()
{
   theCurrentFrame->play();
   setRedrawFlag(true);
}


ossimPlanetVideoLayerNode::Status ossimPlanetPredatorVideoLayerNode::status()const
{
   switch(theCurrentFrame->getStatus())
   {
      case osg::ImageStream::PLAYING:
      {
         return ossimPlanetVideoLayerNode::STATUS_PLAYING;
      }
      case osg::ImageStream::PAUSED:
      {
         return ossimPlanetVideoLayerNode::STATUS_PAUSED;
      }
      default:
      {
         break;
      }
   }

   return ossimPlanetVideoLayerNode::STATUS_NONE;
}


ossim_float64 ossimPlanetPredatorVideoLayerNode::duration()const
{
   return (ossim_float64)theCurrentFrame->getLength();
}

void ossimPlanetPredatorVideoLayerNode::setReferenceTime(ossim_float64 referenceTime)
{
   theCurrentFrame->setReferenceTime(referenceTime);
   setRedrawFlag(true);
}

ossim_float64 ossimPlanetPredatorVideoLayerNode::referenceTime()const
{
   return theCurrentFrame->getReferenceTime();
}

void ossimPlanetPredatorVideoLayerNode::setRenderMode(ossimPlanetVideoLayerNode::RenderMode mode)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePredatorTraverseMutex);
   if(mode == ossimPlanetVideoLayerNode::RENDER_OVERLAY)
   {
      std::cout << "ossimPlanetPredatorVideoLayerNode::setRenderMode: OVERLAY REDNERING NOT SUPPORTED YET!" << std::endl;
      std::cout << "ossimPlanetPredatorVideoLayerNode::setRenderMode: DEFAULTING TO SCREEN MODE!" << std::endl;
      mode = ossimPlanetVideoLayerNode::RENDER_SCREEN;
   }
   ossimPlanetVideoLayerNode::setRenderMode(mode);
   switch(mode)
   {
      case ossimPlanetVideoLayerNode::RENDER_SCREEN:
      {
         theIcon->setGeometry(osg::Vec3d(-.5, .5, 0.0),
                              osg::Vec3d(1.0, 0.0, 0.0),
                              osg::Vec3d(0.0, -1.0, 0.0));
         break;
      }
      case ossimPlanetVideoLayerNode::RENDER_BILLBOARD:
      {
         // change the geometry based on the render mode;
         theIcon->setGeometry(osg::Vec3d(-.5*theAspect, 0.0, .5),
                              osg::Vec3d(theAspect, 0.0, 0.0),
                              osg::Vec3d(0.0, 0.0, -1.0));
         break;
      }
      default:
      {
         break;
      }
   }
   theSwitchNode->setSingleChildOn((int)mode);
}

void ossimPlanetPredatorVideoLayerNode::traverse(osg::NodeVisitor& nv)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePredatorTraverseMutex);
   bool traverseChildrenFlag = !theCullFlag&&theEnableFlag;
   
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         ossimPlanet* planet = videoLayer()->planet();
         osg::ref_ptr<ossimPlanetGeoRefModel> model = planet->model();
         if(theCurrentFrame.valid())
         {
            if(!theCurrentFrame->model().valid())
            {
               theCurrentFrame->setModel(model.get());
            }
         }
         if(!theEnableFlag)
         {
            break;
         }
         //bool needNewCoordinates = false;
         osg::ref_ptr<ossimPlanetFFMpegImageStream::CoordinateInfo> coordInfo;
         theCoordinateMutex.lock();
         if(theCoordinates.valid())
         {
            coordInfo = theCoordinates->dup();
            theCoordinates = 0;
         }
//          needNewCoordinates = theCoordinatesChangedFlag;
//          theCoordinatesChangedFlag = false;
         if(!theGeometryFlag)
         {
            osg::Matrixd localToWorld;
            theBillboard->setGroundObjectSize(5000*
                                              planet->model()->getInvNormalizationScale());
            
            model->lsrMatrix(osg::Vec3d(0, 0, 0),
                             localToWorld);
            theBillboardMatrixTransform->setMatrix(localToWorld);
            theGeometryFlag = true;
            theCullDistance = 10000*planet->model()->getInvNormalizationScale();
            model->forward(osg::Vec3d(0.0,0.0,0.0), theFrameCenter);
         }
         theCoordinateMutex.unlock();
         if(coordInfo.valid())
         {
            if(description()!=coordInfo->theDescription)
            {
               setDescription(coordInfo->theDescription);
            }
//             std::cout << "Updating coordinates!!" << std::endl;
            if(planet&&
               coordInfo->theFrameCenterLlhValidFlag&&
               coordInfo->theOrientationMatrixValidFlag&&
               coordInfo->theSlantRangeValidFlag)
            {
               
               osg::Matrixd localToWorld;
               ossim_float64 targetInMeters = coordInfo->theTargetWidthInMeters;
               if(targetInMeters < 1.0)
               {
                  targetInMeters = 5000;
               }
               ossim_float64 cullTarget = (coordInfo->theSlantRange*
                                           planet->model()->getInvNormalizationScale());
               theBillboard->setGroundObjectSize(targetInMeters*
                                           planet->model()->getInvNormalizationScale());
               model->lsrMatrix(coordInfo->theFrameCenterLlh,
                                localToWorld);
               osg::Vec3d hpr;
               mkUtils::matrixToHpr(hpr, localToWorld, coordInfo->theOrientationMatrix);

               if(!theLookAt.valid())
               {
                  theLookAt = new ossimPlanetLookAt;
               }
               theLookAt->setAll(coordInfo->theFrameCenterLlh[0], coordInfo->theFrameCenterLlh[1], coordInfo->theFrameCenterLlh[2],
                                 hpr[0], hpr[1], hpr[2],
                                 coordInfo->theSlantRange,
                                 ossimPlanetAltitudeMode_ABSOLUTE);
               model->forward(coordInfo->theFrameCenterLlh, theFrameCenter);
               theBillboardMatrixTransform->setMatrix(localToWorld);
               theCullDistance = cullTarget*5;
            }
         }
         if(theViewport.valid()&&theViewportChangedFlag)
         {
            theCameraNode->setProjectionMatrix(osg::Matrix::ortho2D(theViewport->x(),
                                                                    theViewport->width(),
                                                                    theViewport->y(),
                                                                    theViewport->height()));
            ossim_float64 dx = theViewport->width()*.5;
            ossim_float64 dy = theViewport->height()*.5;
            the2DMatrixTransform->setMatrix(osg::Matrixd::scale(osg::Vec3d(theVideo->imageWidth(),
                                                                           theVideo->imageHeight(),
                                                                           1.0))*
                                            osg::Matrixd::translate(osg::Vec3d(theViewport->x() + dx,
                                                                               theViewport->y() + dy,
                                                                               0.0)));
            theViewportChangedFlag = false;
            setRedrawFlag(true);
         }
         break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
//          if(!theEnableFlag)
//          {
//             break;
//          }
         
         osgUtil::CullVisitor* cullVisitor = dynamic_cast<osgUtil::CullVisitor*>(&nv);
         if(cullVisitor)
         {
            theCullFlag = nv.getDistanceToEyePoint(theFrameCenter, false) > theCullDistance;
            traverseChildrenFlag = theEnableFlag&&!theCullFlag;
            if(!theCullFlag)
            {
               double x = cullVisitor->getViewport()->x();
               double y = (int)cullVisitor->getViewport()->y();
               double w = (int)cullVisitor->getViewport()->width();
               double h = (int)cullVisitor->getViewport()->height();
               if(!theViewport.valid())
               {
                  theViewport = new osg::Viewport(x,y,w,h);
                  theViewportChangedFlag = true;
               }
               else
               {
                  if( !ossim::almostEqual(theViewport->x(), x)||
                      !ossim::almostEqual(theViewport->y(), y)||
                      !ossim::almostEqual(theViewport->width(), w)||
                      !ossim::almostEqual(theViewport->height(), h))
                  {
                     theViewport->setViewport(x,y,w,h);
                     theViewportChangedFlag = true;
                  }
               }
            }
            break;
         }
      }
      case osg::NodeVisitor::EVENT_VISITOR:
      {
         osgGA::EventVisitor* ev = dynamic_cast<osgGA::EventVisitor*>(&nv);
         if(ev)
         {
            const osgGA::EventVisitor::EventList& eventList = ev->getEvents();
            osgGA::EventVisitor::EventList::const_iterator iter = eventList.begin();
            if(eventList.size())
            {
               if(((*iter)->getEventType() == osgGA::GUIEventAdapter::FRAME))
               {
                  if(ev->getActionAdapter())
                  {
                     if(theRedrawFlag)
                     {
                        ev->getActionAdapter()->requestRedraw();
                        theRedrawFlag = false;
                     }
                  }
               }
            }
         }
         break;
      }
      default:
      {
         break;
      }
   }
   
   theCurrentFrame->setEnableFlag(traverseChildrenFlag);
   if(traverseChildrenFlag)
   {
      ossimPlanetVideoLayerNode::traverse(nv);
   }
}

void ossimPlanetPredatorVideoLayerNode::updateIconGeometry()
{
}
