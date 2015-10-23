#include <ossimPlanet/ossimPlanetManipulator.h>
#include <ossimPlanet/ossimPlanetUtility.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetLand.h>
#include <osg/Quat>
#include <osg/Notify>
#include <osgUtil/IntersectVisitor>
#include <ossim/base/ossimEcefPoint.h>
#include <ossim/base/ossimGpt.h>
#include <osgUtil/IntersectVisitor>
#include <ossimPlanet/ossimPlanetInteractionController.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossimPlanet/ossimPlanetActionRouter.h>
#include <osg/CoordinateSystemNode>
#include <OpenThreads/ScopedLock>
#include <ossimPlanet/ossimPlanetDestinationCommandAction.h>
#include <osg/io_utils>


#include <iostream>

using namespace osg;
using namespace osgGA;

ossimPlanetManipulator::ossimPlanetManipulator() :
    theNavigator(new ossimPlanetNavigator(new ossimPlanetPrimaryBody("earth_wgs84", 6378137.0, 6356752.3142, 86400, 5.9742e24, -180, 180))),
    theNode(NULL),
    theEventHandlingFlag(true),
    theUseFrameEventForUpdateFlag(false),
    theAutoCalculateIntersectionFlag(true),
    thePlanet(0),
    theFusionDistance(1.0)
{
//   theViewMatrixBuilder = new ossimPlanetViewMatrixBuilder();

theViewMatrixBuilder = new ossimPlanetViewMatrixBuilder(new ossimPlanetEllipsoidModel());   

   theFromNodeCallback  = new FromNodeCallback(theViewMatrixBuilder.get());
   theToNodeCallback    = new ToNodeCallback(theViewMatrixBuilder.get());

   theNavigator->setUseTimedUpdateFlag(true);
   initializeDefaultBindings(":navigator");
}

void ossimPlanetManipulator::initializeDefaultBindings(const ossimString& pathName)
{
   setPathnameAndRegister(pathName.c_str());
   
   // define our valuators
   ossimPlanetInteractionController* iac = ossimPlanetInteractionController::instance();
   iac->defineInteractionValuator("LAT", -1.0f, 1.0f);
   iac->defineInteractionValuator("LON", -1.0f, 1.0f);
   iac->defineInteractionValuator("ZOOM", -1.0f, 1.0f);
   iac->defineInteractionValuator("PITCH", -1.0f, 1.0f);
   iac->defineInteractionValuator("YAW", -1.0f, 1.0f);
   
   // initialize bindings
   // ossimFilename path = ossimEnvironmentUtility::instance()->getUserOssimSupportDir();
   // path = path.dirCat("planet");
   // path = path.dirCat("binds.act");
//   if(path.exists())
//   {
//      ossimPlanetActionRouter::instance()->executeFile(path);
//   }
//   else
   {
      ossimPlanetDestinationCommandAction(":iac tie x_mouse LON YAW").execute();
      ossimPlanetDestinationCommandAction(":iac tie y_mouse LAT ZOOM PITCH").execute();
      ossimPlanetDestinationCommandAction(":iac bind left_mousedown         {"+pathName+" rotatestart}").execute();
      ossimPlanetDestinationCommandAction(":iac bind left_mouseup           {"+pathName+" rotatestop}").execute();
      ossimPlanetDestinationCommandAction(":iac bind middle_mousedown       {"+pathName+" losrotatestart}").execute();
      ossimPlanetDestinationCommandAction(":iac bind middle_mouseup         {"+pathName+" losrotatestop}").execute();
      ossimPlanetDestinationCommandAction(":iac bind right_mousedown        {"+pathName+" loszoomstart}").execute();
      ossimPlanetDestinationCommandAction(":iac bind right_mouseup          {"+pathName+" loszoomstop}").execute();
      ossimPlanetDestinationCommandAction(":iac bind shift_middle_mousedown {"+pathName+" zoomstart}").execute();
      ossimPlanetDestinationCommandAction(":iac bind shift_middle_mouseup   {"+pathName+" zoomstop}").execute();
      ossimPlanetDestinationCommandAction(":iac bind shift_right_mousedown  {"+pathName+" lookstart}").execute();
      ossimPlanetDestinationCommandAction(":iac bind shift_right_mouseup    {"+pathName+" lookstop}").execute();
      ossimPlanetDestinationCommandAction(":iac bind right_key              {"+pathName+" flystart -.015625 0.0}").execute();
      ossimPlanetDestinationCommandAction(":iac bind left_key               {"+pathName+" flystart .015625 0.0}").execute();
      ossimPlanetDestinationCommandAction(":iac bind up_key                 {"+pathName+" flystart 0.0 -.015625}").execute();
      ossimPlanetDestinationCommandAction(":iac bind down_key               {"+pathName+" flystart 0.0 .015625}").execute();
      ossimPlanetDestinationCommandAction(":iac bind u_key                  {"+pathName+" rotatenorth}").execute();
      ossimPlanetDestinationCommandAction(":iac bind U_key                  {"+pathName+" rotatenorthup}").execute();
      ossimPlanetDestinationCommandAction(":iac bind p_key                  {"+pathName+" printlatlonelev}").execute();
      ossimPlanetDestinationCommandAction(":iac bind P_key                  {"+pathName+" printlookcoordinates}").execute();
      ossimPlanetDestinationCommandAction(":iac bind z_key                  {"+pathName+" recordanimation}").execute();
      ossimPlanetDestinationCommandAction(":iac bind Z_key                  {"+pathName+" playanimation}").execute();
      ossimPlanetDestinationCommandAction(":iac bind space_key              {"+pathName+" reset}").execute();
      ossimPlanetDestinationCommandAction(":iac bind return_key             {"+pathName+" stop}").execute();
      ossimPlanetDestinationCommandAction(":iac bind A_key                  {: manualaction}").execute();
   }
}



void ossimPlanetManipulator::extractLookFromParameters(ossimXmlNode* node,
                                                   double& lat,
                                                   double& lon,
                                                   double& alt,
                                                   double& h,
                                                   double& p,
                                                   double& r)
{
   osg::ref_ptr<ossimPlanetGeoRefModel> model = thePlanet->model();



   ossimString vref = node->getAttributeValue("vref");
   ossimString value;

	
   

   if(node->getChildTextValue(value, "longitude"))
   {
      lon = value.toDouble();
   }

   if(node->getChildTextValue(value, "latitude"))
   {
      lat = value.toDouble();
   }

   if(node->getChildTextValue(value, "altitude"))
   {
      alt = value.toDouble();
      if(node->getChildTextValue(value, "altitudeMode"))
      {
         if(value.contains("relative"))
         {
            alt += model->getHeightAboveEllipsoid(lat, lon);
         }

         else if(value.contains("clamp"))
         {
            alt = model->getHeightAboveEllipsoid(lat, lon);
         }
         else if(value.contains("absolute"))
         {
            if(vref!="wgs84")
            {
               alt += model->getGeoidOffset(lat, lon);
            }
         }
      }
   }

   if(node->getChildTextValue(value, "heading"))
   {
      h = value.toDouble();
   }
   if(node->getChildTextValue(value, "pitch"))
   {
      p = value.toDouble();
   }
   if(node->getChildTextValue(value, "roll"))
   {
      r = value.toDouble();
   }
}


void ossimPlanetManipulator::execute(const ossimPlanetAction& a)
{

   //std::cout << "xxxx\n";
   double lat_ = 0.0, lon_ = 0.0, altitude = 0.0 , heading = 0.0, pitch = 0.0, roll = 0.0;
   ossimString command = a.command();
   bool aircraft = false;
   const ossimPlanetXmlAction* xmlAction = a.toXmlAction();
   ossimXmlNode* lookFromNode = 0;

   if(xmlAction)
   {
      const ossimXmlNode::ChildListType& children = xmlAction->xmlNode()->getChildNodes();
      if(command == "Set")
      {
         	ossim_uint32 idx = 0;
            for(;idx < children.size();++idx)
            {
            	if(children[idx]->getTag() == "LookFrom")
               {
         			ossimRefPtr<ossimXmlNode> node = children[idx];
         			lookFromNode = (ossimXmlNode *)node->getChildNodes()[0].get();
         			break;
         		}
         	}
       }
   }
	if(lookFromNode)
	{
      extractLookFromParameters(lookFromNode,lat_,lon_,altitude,heading,pitch,roll);

      viewMatrixBuilder()->setLookFrom(osg::Vec3d(lat_, lon_, altitude), osg::Vec3d(heading,pitch, roll), 1);
      theNavigator->setRedrawFlag(true);

   	aircraft = true;

	}
if(aircraft == false)
{
	if(theNavigator.valid())
	{

		theNavigator->execute(a);
	}
}
}
ossimPlanetManipulator::~ossimPlanetManipulator()
{
}


void ossimPlanetManipulator::setNode(osg::Node* node)
{
   theNode = node;

   if(theNode.valid())
   {
      thePlanet = ossimPlanet::findPlanet(theNode.get());//finder.thePlanet;
      if(theNavigator.valid())
      {
         if(thePlanet)
         {
            theViewMatrixBuilder->setGeoRefModel(thePlanet->model().get());
         }
         theNavigator->setPlanet(thePlanet);
      }
   }
}


const osg::Node* ossimPlanetManipulator::getNode() const
{
    return theNode.get();
}


osg::Node* ossimPlanetManipulator::getNode()
{
    return theNode.get();
}


void ossimPlanetManipulator::getLatLonHgtHPR(double& lat, double& lon, double& hgt,
                                             double& heading, double& pitch, double& roll)const
{
//    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   lat     = theNavigator->lat();
   lon     = theNavigator->lon();
   hgt     = theNavigator->elev();
   heading = theNavigator->orientation()[0];
   pitch   = theNavigator->orientation()[1];
   roll    = theNavigator->orientation()[2];
}

void ossimPlanetManipulator::solveLookAt(double losLat, double losLon, double losHeight,
                                         double& heading, double& pitch, double& roll, double& range)const

{
   theNavigator->solveLookAt(losLat, losLon, losHeight, heading, pitch, roll, range);
}

void ossimPlanetManipulator::playRecording()
{
   theNavigator->playRecording();
}

void ossimPlanetManipulator::startRecording()
{
   theNavigator->startRecording();
}

void ossimPlanetManipulator::stopRecording()
{
   theNavigator->stopRecording();
}

void ossimPlanetManipulator::saveRecording(std::ostream& out)
{
   theNavigator->saveRecording(out);
}

bool ossimPlanetManipulator::loadRecording(std::istream& in)
{
   return theNavigator->loadRecording(in);
}

void ossimPlanetManipulator::setEventHandlingFlag(bool flag)
{
   theEventHandlingFlag = flag;
}

void ossimPlanetManipulator::setUseFrameEventForUpdateFlag(bool flag)
{
   theUseFrameEventForUpdateFlag = flag;
   if(flag)
   {
      theNavigator->setUseTimedUpdateFlag(false);
   }
   else
   {
      theNavigator->setUseTimedUpdateFlag(true);
   }
}

void ossimPlanetManipulator::setAutoCalculateIntersectionFlag(bool flag)
{
   theAutoCalculateIntersectionFlag = flag;
}

void ossimPlanetManipulator::setLosXYZ(const osg::Vec3d& losXYZ)
{
   if(theNavigator->canSetLineOfSite())
   {
      theNavigator->setLosXYZ(losXYZ);
   }
}

void ossimPlanetManipulator::init(const GUIEventAdapter& ,GUIActionAdapter&)
{
}

void ossimPlanetManipulator::home(const GUIEventAdapter& ,GUIActionAdapter& us)
{
   ossimPlanetDestinationCommandAction(":navigator reset").execute();
   us.requestRedraw();
}

bool ossimPlanetManipulator::handle(const GUIEventAdapter& ea,GUIActionAdapter& us)
{
  updateViewMatrixNodes();
    // XXX most of this belongs in ossimPlanetQT.
   if(!theEventHandlingFlag)
   {
      return false;
   }
//    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   ossimPlanetInteractionController* iac = ossimPlanetInteractionController::instance();

   ossimString modKeyString;

   if(ea.getModKeyMask() & GUIEventAdapter::MODKEY_SHIFT)
   {
      modKeyString += "shift_";
   }
   if(ea.getModKeyMask() & GUIEventAdapter::MODKEY_CTRL)
   {
      modKeyString += "ctrl_";
   }
   if(ea.getModKeyMask() & GUIEventAdapter::MODKEY_ALT)
   {
      modKeyString += "alt_";
   }
   if(ea.getModKeyMask() & GUIEventAdapter::MODKEY_META)
   {
      modKeyString += "meta_";
   }
   switch(ea.getEventType())
   {
      case GUIEventAdapter::FRAME:
      {
         osg::Vec3d pt;
         bool validLos = calculateLineOfSiteLatLonHeight(pt);

         if(validLos)//&&theNavigator->canSetLineOfSite()&&theNavigator->landModel().valid())
         {
            osg::Vec3d ptXYZ;
            theNavigator->landModel()->forward(pt, ptXYZ);
            theNavigator->setLosXYZ(ptXYZ);

         }

         if(theUseFrameEventForUpdateFlag)
         {
            if(theNavigator->needsContinuousUpdate()||theNavigator->redrawFlag())
            {
               theNavigator->update();
            }
         }

         break;
      }
      case GUIEventAdapter::PUSH:
      {
         switch(ea.getButton())
         {
            case GUIEventAdapter::LEFT_MOUSE_BUTTON:
            {
               iac->updateInteractionValuators("x_mouse", ea.getXnormalized());
               iac->updateInteractionValuators("y_mouse", ea.getYnormalized());
               iac->executeBoundAction((modKeyString + "left_mousedown").c_str());
               break;
            }
            case GUIEventAdapter::MIDDLE_MOUSE_BUTTON:
            {
               iac->updateInteractionValuators("x_mouse", ea.getXnormalized());
               iac->updateInteractionValuators("y_mouse", ea.getYnormalized());
               iac->executeBoundAction((modKeyString + "middle_mousedown").c_str());
               break;
            }
            case GUIEventAdapter::RIGHT_MOUSE_BUTTON:
            {
               iac->updateInteractionValuators("x_mouse", ea.getXnormalized());
               iac->updateInteractionValuators("y_mouse", ea.getYnormalized());
               iac->executeBoundAction((modKeyString + "right_mousedown").c_str());
               break;
            }
         }
         break;
      }
      case GUIEventAdapter::DOUBLECLICK:
      {
         switch(ea.getButton())
         {
            case GUIEventAdapter::LEFT_MOUSE_BUTTON:
            {
               break;
            }
            case GUIEventAdapter::MIDDLE_MOUSE_BUTTON:
            {
               break;
            }
            case GUIEventAdapter::RIGHT_MOUSE_BUTTON:
            {
               break;
            }
         }
      }
      case GUIEventAdapter::RELEASE:
      {
         switch(ea.getButton())
         {
            case GUIEventAdapter::LEFT_MOUSE_BUTTON:
            {
               iac->updateInteractionValuators("x_mouse", ea.getXnormalized());
               iac->updateInteractionValuators("y_mouse", ea.getYnormalized());
               iac->executeBoundAction((modKeyString + "left_mouseup").c_str());
               break;
            }
            case GUIEventAdapter::MIDDLE_MOUSE_BUTTON:
            {
               iac->updateInteractionValuators("x_mouse", ea.getXnormalized());
               iac->updateInteractionValuators("y_mouse", ea.getYnormalized());
               iac->executeBoundAction((modKeyString + "middle_mouseup").c_str());
              break;
            }
            case GUIEventAdapter::RIGHT_MOUSE_BUTTON:
            {
                iac->updateInteractionValuators("x_mouse", ea.getXnormalized());
                iac->updateInteractionValuators("y_mouse", ea.getYnormalized());
                iac->executeBoundAction((modKeyString + "right_mouseup").c_str());
               break;
            }
         }
         break;
      }
      case GUIEventAdapter::SCROLL:
      {
         switch(ea.getScrollingMotion())
         {
            case GUIEventAdapter::SCROLL_RIGHT:
            {
                ossimString s ="scroll_right";
                s = modKeyString + s;
                iac->executeBoundAction(s);
               break;
            }

            case GUIEventAdapter::SCROLL_LEFT:
            {
               ossimString s ="scroll_left";
               s = modKeyString + s;
               iac->executeBoundAction(s);
               break;
            }
            case GUIEventAdapter::SCROLL_UP:
            {
               break;
            }
            case GUIEventAdapter::SCROLL_DOWN:
            {
               break;
            }
            default:
            {
               break;
            }
         }
         break;
      }
//      case GUIEventAdapter::MOVE:
      case GUIEventAdapter::DRAG:
      {
         iac->updateInteractionValuators("x_mouse", ea.getXnormalized());
         iac->updateInteractionValuators("y_mouse", ea.getYnormalized());
         break;
      }
      case GUIEventAdapter::KEYDOWN:
      {
         ossimString s;
         switch(ea.getKey())
         {
            case osgGA::GUIEventAdapter::KEY_Up:
            {
               us.requestRedraw();
               s += "up";
               break;
            }
            case osgGA::GUIEventAdapter::KEY_Down:
            {
               us.requestRedraw();

               s += "down";
               break;
            }
            case osgGA::GUIEventAdapter::KEY_Left:
            {
               us.requestRedraw();
               s += "left";
               break;
            }
            case osgGA::GUIEventAdapter::KEY_Right:
            {
               us.requestRedraw();
               s += "right";
               break;
            }
            case osgGA::GUIEventAdapter::KEY_Return:
            {
               s += "return";
               us.requestRedraw();
               break;
            }
            case ' ':
            {
               s+= "space";
               us.requestRedraw();
               break;
            }
            default:
            {
               s += (char)ea.getKey();
               break;
            }
         }
         if(s != "")
         {
            s+="_key";
            modKeyString = modKeyString.substitute("shift_",
                                                   "");
            s = modKeyString + s;
            iac->executeBoundAction(s);
         }
         break;
      }
      default:
      {
         break;
      }
   }
   if(theNavigator->needsContinuousUpdate())
   {
      us.requestRedraw();
   }
   if(theNavigator->redrawFlag())
   {
      us.requestRedraw();
      theNavigator->setRedrawFlag(false);
   }
   if(!theUseFrameEventForUpdateFlag)
   {
      if(theNavigator->needsContinuousUpdate()||theNavigator->redrawFlag())
      {
         theNavigator->update();
      }
   }

   return false;
}

void ossimPlanetManipulator::getUsage(osg::ApplicationUsage& /*usage*/) const
{
}

void ossimPlanetManipulator::setLockToNode(osg::Node* node)
{
   theLockToNode = node;
}

void ossimPlanetManipulator::updateViewMatrixNodes()
{
  if(theViewMatrixBuilder->fromNode()!=theLockFromNode.get())
  {
     ossimPlanetViewMatrixBuilder::Visitor nv;
     if(theLockFromNode.valid())
     {
        theLockFromNode->accept(nv);
        if(nv.theLsrSpaceTransform.valid())
        {
           nv.theLsrSpaceTransform->removeCallback(theFromNodeCallback.get());
        }
     }
     nv.reset();
     // setup from node
     //
     theLockFromNode = theViewMatrixBuilder->fromNode();
     if(theLockFromNode.valid())
     {
        theLockFromNode->accept(nv);
        if(nv.theLsrSpaceTransform.valid())
        {
           nv.theLsrSpaceTransform->addCallback(theFromNodeCallback.get());
        }
        else
        {
           theLockFromNode = 0;
        }
     }
  }
  if(theViewMatrixBuilder->toNode()!=theLockToNode.get())
  {
     ossimPlanetViewMatrixBuilder::Visitor nv;
     if(theLockToNode.valid())
     {
        theLockToNode->accept(nv);
        if(nv.theLsrSpaceTransform.valid())
        {
           nv.theLsrSpaceTransform->removeCallback(theToNodeCallback.get());
        }
     }
     nv.reset();
     // setup from node
     //
     theLockToNode = theViewMatrixBuilder->toNode();
     if(theLockToNode.valid())
     {
        theLockToNode->accept(nv);
        if(nv.theLsrSpaceTransform.valid())
        {
           nv.theLsrSpaceTransform->addCallback(theToNodeCallback.get());
        }
        else
        {
           theLockToNode = 0;
        }
     }
  }
}

bool ossimPlanetManipulator::calculateLineOfSiteLatLonHeight(osg::Vec3d& latLonHeight)
{
   bool hitFound = false;
   if(theNode.valid()&&theNavigator->landModel())
   {
      osg::Matrixd m = getMatrix();

      osg::BoundingSphere bs = theNode->getBound();
      osgUtil::IntersectVisitor iv;

      osg::Vec3d center;
      osg::Vec3d look(- m(2,0),-m(2,1),-m(2,2));
      osg::Vec3d eye(m(3,0),m(3,1),m(3,2));

      center = eye + look*100.0;

      osg::ref_ptr<osg::LineSegment> segLookVector = new osg::LineSegment;
      segLookVector->set(eye, center);
      iv.addLineSegment(segLookVector.get());

      theNode->accept(iv);
      if (iv.hits())
      {
         osgUtil::IntersectVisitor::HitList& hitList = iv.getHitList(segLookVector.get());
         if (!hitList.empty())
         {
            osg::Vec3d intersection = hitList.front().getWorldIntersectPoint();

            theNavigator->landModel()->inverse(intersection, latLonHeight);
            hitFound = true;
	    theFusionDistance = (eye-intersection).length();
         }
      }

   }
   return hitFound;
}

void ossimPlanetManipulator::setByMatrix(const osg::Matrixd& matrix)
{
//     OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
    osg::Matrixd m;
    m.invert(matrix);
    if (!theNavigator->landModel().valid())
        ossimPlanetDestinationCommandAction(":navigator reset").execute();
    else
        theNavigator->setViewParameters(m);
}

osg::Matrixd ossimPlanetManipulator::getMatrix() const
{
  if(theViewMatrixBuilder->isValid())
  {
      theNavigator->setViewParameters(theViewMatrixBuilder->viewMatrix());
//std::cout << "theViewMatrixBuilder->viewMatrix()" << std::endl;
      return theViewMatrixBuilder->viewMatrix();
   }
   else
   {
//std::cout << "theNavigator->viewMatrix()" << std::endl;
      return theNavigator->viewMatrix();
  }
}

osg::Matrixd ossimPlanetManipulator::getInverseMatrix() const
{
   if(theViewMatrixBuilder->isValid())
   {
      return theViewMatrixBuilder->inverseViewMatrix();
   }
   else
   {
      return osg::Matrix::inverse(theNavigator->viewMatrix());
   }
}

osg::Vec3d ossimPlanetManipulator::eyePosition()const
{
   if(theViewMatrixBuilder->isValid())
   {
      osg::Vec3d(0,0,0)*theViewMatrixBuilder->viewMatrix();
   }
//    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   return osg::Vec3d(0,0,0)*theNavigator->viewMatrix();
}

void ossimPlanetManipulator::updateNavigator()
{
    theNavigator->update();
}

void ossimPlanetManipulator::setLatitude(double value)
{
   theNavigator->setLatLonHeight(value, theNavigator->lon(), theNavigator->elev());
}

void ossimPlanetManipulator::setLongitude(double value)
{
   theNavigator->setLatLonHeight(theNavigator->lat(), value, theNavigator->elev());
}

void ossimPlanetManipulator::setAltitude(double value)
{
   theNavigator->setLatLonHeight(theNavigator->lat(), theNavigator->lon(), value);
}

void ossimPlanetManipulator::setLatitudeLongitudeAltitude(double lat, double lon, double alt)
{
   theNavigator->setLatLonHeight(lat, lon, alt);
}

void ossimPlanetManipulator::setPosition(const osg::Vec3d& pos)
{
   theNavigator->setLatLonHeight(pos[0], pos[1], pos[2]);
}

void ossimPlanetManipulator::setHeading(double value)
{
   theNavigator->setHpr(value, theNavigator->orientation()[1], theNavigator->orientation()[2]);
}

void ossimPlanetManipulator::setPitch(double value)
{
   theNavigator->setHpr(theNavigator->orientation()[0], value, theNavigator->orientation()[2]);
}

void ossimPlanetManipulator::setRoll(double value)
{
   theNavigator->setHpr(theNavigator->orientation()[0], theNavigator->orientation()[1], value);
}

void ossimPlanetManipulator::setHeadingPitchRoll(double h, double p, double r)
{
   theNavigator->setHpr(h, p, r);
}

void ossimPlanetManipulator::setOrientation(const osg::Vec3d& orien)
{
   theNavigator->setHpr(orien[0], orien[1], orien[2]);
}

void ossimPlanetManipulator::setPositionAndOrientation(double lat, double lon, double alt,
                                                       double h, double p, double r)
{
   theNavigator->setLatLonHeight(lat, lon, alt);
   theNavigator->setHpr(h, p, r);
}

void ossimPlanetManipulator::setPositionAndOrientation(const osg::Vec3d& pos,
                                                       const osg::Vec3d& orien)
{
   theNavigator->setLatLonHeight(pos[0], pos[1], pos[2]);
   theNavigator->setHpr(orien[0], orien[1], orien[2]);
}
