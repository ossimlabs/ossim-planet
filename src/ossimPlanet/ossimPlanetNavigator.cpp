#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <osg/Quat>
#include <osg/Timer>
#include <osg/io_utils>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/ossimPlanetNavigator.h>
#include <ossimPlanet/ossimPlanetPrimaryBody.h>
#include <ossimPlanet/ossimPlanetInteractionController.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetActionRouter.h>

static inline void adjustHeight(osg::Vec3d& input,
										  ossimPlanetAltitudeMode mode,
										  ossimPlanetGeoRefModel* modelTransform)
{
	switch(mode)
	{
		case ossimPlanetAltitudeMode_CLAMP_TO_GROUND:
		{
			input[2] = modelTransform->getHeightAboveEllipsoid(input[0], input[1]);
			break;
		}
		case ossimPlanetAltitudeMode_RELATIVE_TO_GROUND:
		{
			input[2]+=modelTransform->getHeightAboveEllipsoid(input[0], input[1]);
			break;
		}
		case ossimPlanetAltitudeMode_ABSOLUTE:
		{
			input[2] += modelTransform->getGeoidOffset(input[0], input[2]);
			break;
		}
		default:
		{
			input[2] = modelTransform->getHeightAboveEllipsoid(input[0], input[1]);
			break;
		}
	}
}

ossimPlanetNavigator::ossimPlanetNavigator(osg::ref_ptr<ossimPlanetPrimaryBody> p) :
    pitchOffset_(0.0),
    lat_(0.0),
    lon_(-90.0),
    zoomMin_(-4.2),         // earth about fills the screen here
    fov_(50),
    primary_(p),
    rotating_(false),
    zooming_(false),
    zoominglos_(false),
    looking_(false),
    endLooking_(false),
    gotoing_(false),
    gotoingelev_(false),
    gotoset_(false),
    flying_(false),
    losLookingFlag_(false),
    losXYZValidFlag_(false),
    updateRatePerSecond_(120),
    lastUpdateTime_(osg::Timer::instance()->tick()),
    gotoLookDuration_(4.0),
    xFly_(0.0),
    yFly_(0.0),
    theAnimationMode(ossimPlanetNavigator::NAV_ANIMATION_NONE),
    theAnimationPath(new osg::AnimationPath)
{
   thePlanet = 0;

    // earth is above us, eliminates gimbal lock in looking.
    eyexyz_.set(0.0, 0.0, zoomMin_);        // all the way out
    eyehpr_.set(0.0, pitchOffset_, 0.0);    // looking down (up in pf coords) at earth
    theUseTimedUpdateFlag = false;
   setRedrawFlag(true);
}

ossimPlanetNavigator::~ossimPlanetNavigator()
{
}

osg::Matrixd ossimPlanetNavigator::orientationLsrMatrix(double lat, double lon, double hgt, double h, double p, double r) const
{
   osg::ref_ptr<ossimPlanetGeoRefModel> model = landModel();
   if(!model.valid()) return osg::Matrixd();
   osg::Matrixd output;
   model->lsrMatrix(osg::Vec3d(lat, lon, hgt),
                           output);
   output(3,0) = 0.0;
   output(3,1) = 0.0;
   output(3,2) = 0.0;


   NEWMAT::Matrix orien = ossimMatrix4x4::createRotationZMatrix(h, OSSIM_RIGHT_HANDED)*
                          ossimMatrix4x4::createRotationXMatrix(p-pitchOffset(), OSSIM_LEFT_HANDED)*
                          ossimMatrix4x4::createRotationYMatrix(r, OSSIM_LEFT_HANDED);
   osg::Matrixd tempM(orien[0][0], orien[1][0], orien[2][0], 0.0,
                      orien[0][1], orien[1][1], orien[2][1], 0.0,
                      orien[0][2], orien[1][2], orien[2][2], 0.0,
                      0.0, 0.0, 0.0, 1.0);

   return tempM*output;
}

void ossimPlanetNavigator::setLatLonHeight(double lat, double lon, double height)
{
    lat_ = lat;
    lon_ = lon;
    eyexyz_.z() = ossim::clamp(-(1.0 + (height/primary_->radius(lat_))), zoomMin_, zoomMax_);
}
void ossimPlanetNavigator::setHpr(double h, double p, double r)
{
   eyehpr_.set(h,p,r);
}

double ossimPlanetNavigator::metersToCenter() const
{
    return -eyexyz_.z()*primary_->radius(lat());
}

void ossimPlanetNavigator::update()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   osg::ref_ptr<ossimPlanetGeoRefModel> model = landModel();
   if(!model.valid()) return;
    // cache initial values, we need them at the end to update idol
    //double oldLat(lat()), oldLon(lon()), oldElev(elev());
    //bool oldGotoing(gotoing_);

    // get current logical valuators
    ossimPlanetInteractionController* iac = ossimPlanetInteractionController::instance();
    float inputLat = iac->interactionValuatorValue("LAT") - baseInputLat_;
    float inputLon = iac->interactionValuatorValue("LON") - baseInputLon_;
    float inputZoom = iac->interactionValuatorValue("ZOOM") - baseInputZoom_;
    float inputYaw = iac->interactionValuatorValue("YAW") - baseInputYaw_;
    float inputPitch = iac->interactionValuatorValue("PITCH") - baseInputPitch_;
	bool canUpdateNonTimedUpdates = true;
	if(theUseTimedUpdateFlag)
	{
		double delta = osg::Timer::instance()->delta_s(lastUpdateTime_, osg::Timer::instance()->tick());
		if(delta < 0.0)
		{
			// bad time tick so reset
			lastUpdateTime_ = osg::Timer::instance()->tick();
			return;
		}
		canUpdateNonTimedUpdates = (delta > (1.0/updateRatePerSecond_));
	}
#if 1
    if (theAnimationMode == NAV_ANIMATION_PLAYBACK)
    {

        if (!rotating_ && !zooming_ && !losLookingFlag_ && !zoominglos_ && !flying_)
        {
            double t = osg::Timer::instance()->delta_s( theAnimationStartTime, osg::Timer::instance()->tick());
            if (t >= 0.0)
            {
                osg::Matrixd m;
                if (theAnimationPath->getMatrix(t, m))
                {
                    setViewParameters(m);
                    return;
                }
                else
                {
                    theAnimationMode = NAV_ANIMATION_NONE;
                }
            }
			  else
			  {
				  theAnimationStartTime = osg::Timer::instance()->tick();// need to reset the timer since went negative
			  }
        }
        else
        {
            theAnimationMode = NAV_ANIMATION_NONE;
        }
    }
#endif

    if (canUpdateNonTimedUpdates)
    {
        lastUpdateTime_ = osg::Timer::instance()->tick();
    }
    if (zoominglos_ && losXYZValidFlag_ && model.valid())
    {

        osg::Matrixd eyeLsrMatrix = viewMatrix();
        osg::Vec3d eye(eyeLsrMatrix(3,0), eyeLsrMatrix(3,1), eyeLsrMatrix(3,2));
        osg::Vec3d posXYZ;
        osg::Vec3d posLlh;
        osg::Vec3d tempHpr;
        osg::Vec3d deltaV = eye-losXYZ_;  //losXYZ-eye;
        double distanceCurrent = deltaV.length();
        double minDistance = 64.0/model->getNormalizationScale();
        double distance = distanceCurrent + distanceCurrent*0.1*inputZoom;

        if (distance < minDistance)
        {
            distance = minDistance;
        }
        deltaV.normalize();

        posXYZ = losXYZ_ + deltaV*distance;

        model->inverse(posXYZ, posLlh);

        setLatLonHeight(posLlh[0], posLlh[1], posLlh[2]);//*model->getNormalizationScale());
            osg::Matrixd localLsr;
        model->lsrMatrix(posLlh, localLsr);

        mkUtils::matrixToHpr(tempHpr, localLsr, eyeLsrMatrix);
        eyehpr_ = tempHpr;
        eyehpr_[2] = 0.0;
        eyehpr_[0] = ossim::clamp(eyehpr_[0] , -180.0, 180.0);
        eyehpr_[1] = ossim::clamp(eyehpr_[1] , 0.0, 180.0);
    }
    if (losLookingFlag_ && losXYZValidFlag_ && model.valid())
    {
        if (canUpdateNonTimedUpdates)
        {

            osg::Matrixd eyeLsrMatrix = viewMatrix();
            osg::Vec3d eye(eyeLsrMatrix(3,0), eyeLsrMatrix(3,1), eyeLsrMatrix(3,2));
            double distance = (losXYZ_ - eye).length();
            eyeLsrMatrix(3,0) = 0.0;
            eyeLsrMatrix(3,1) = 0.0;
            eyeLsrMatrix(3,2) = 0.0;

            double rotationZAmount = inputYaw*(10*M_PI/180.0);
            double rotationPitchAmount = inputPitch*(10*M_PI/180.0);
            // osg::Vec3d xn = osg::Vec3d(1.0, 0.0, 0.0);
            osg::Vec3d zn = losXYZ_;
            zn.normalize();
            osg::Matrixd r = osg::Matrix::rotate(rotationZAmount, zn);
            eyeLsrMatrix = eyeLsrMatrix*r;

            eyeLsrMatrix = eyeLsrMatrix*osg::Matrix::rotate(rotationPitchAmount, osg::Vec3d(eyeLsrMatrix(0,0),
                                                                                            eyeLsrMatrix(0,1),
                                                                                            eyeLsrMatrix(0,2)));
            osg::Vec3d newPoint = losXYZ_ + osg::Vec3d(eyeLsrMatrix(2, 0), eyeLsrMatrix(2, 1), eyeLsrMatrix(2, 2))*distance;
            osg::Vec3d eyeLlh;
            model->inverse(newPoint, eyeLlh);
            setLatLonHeight(eyeLlh[0],
                            eyeLlh[1],
                            eyeLlh[2]);//*model->getNormalizationScale());

                osg::Matrixd tempView = viewMatrix();
            osg::Matrixd localLsr;
            model->lsrMatrix(eyeLlh, localLsr);
            osg::Vec3d tempHpr;
            mkUtils::matrixToHpr(tempHpr, localLsr, eyeLsrMatrix);

            eyehpr_[0] = ossim::clamp(tempHpr[0] , -180.0, 180.0);
            eyehpr_[1] = ossim::clamp(tempHpr[1] , 0.0, 90.0);
            eyehpr_[2] = 0.0;
            // eyehpr_[2] = mkUtils::clamp(eyehpr_[2] , -180.0, 180.0);
       }
    }
    if (flying_)
    {
        if (ossim::almostEqual(xFly_, 0.0) &&  ossim::almostEqual(yFly_, 0.0))
        {
            flying_ = false;
        }
        else if (canUpdateNonTimedUpdates)
        {
            float x = osg::DegreesToRadians(eyehpr_[0]);
            float s = sin(x);
            float c = cos(x);

            updateLatLon(xFly_*c + yFly_*s, -xFly_*s + yFly_*c);
            // updateLatLon(-xFly_*c + yFly_*s, xFly_*s - yFly_*c);
        }
    }
    if (rotating_ && canUpdateNonTimedUpdates)
    {
        float x = osg::DegreesToRadians((eyehpr_[0]));// - 90.0));
        float s = sin(x);
        float c = cos(x);
        updateLatLon(inputLon*c + inputLat*s, -inputLon*s + inputLat*c);
        //  updateLatLon(xMouse*c - yMouse*s, xMouse*s + yMouse*c);
        //  updateLatLon(xMouse, yMouse);
        gotoing_ = false;
        gotoset_ = false;
        gotoingelev_ = false;
    }

    updateZoomParameters();

    if (zooming_ && canUpdateNonTimedUpdates) {
        double magic = 0.1;
        eyexyz_.z() = ossim::clamp(eyexyz_.z() + zoomScaleInput(inputZoom)*magic, zoomMin_, zoomMax_);
        // manual zoom neutralizes the zoom of any gotoing that we might be doing right now
        targetStartLookZ_ = eyexyz_.z();
        targetMidpointLookZ_ = eyexyz_.z();
        targetLookZ_ = eyexyz_.z();
    }
    if (looking_ && canUpdateNonTimedUpdates)
    {
        double magic = 5.0;
        eyehpr_[0] = ossim::wrap(eyehpr_[0] - inputYaw*fabsf(inputYaw)*magic, -180.0, 180.0);
        eyehpr_[1] = ossim::clamp(eyehpr_[1] + inputPitch*fabsf(inputPitch)*magic, 0.0, 180.0);
    }

    if (gotoing_)
    {
        gotoingelev_ = false;
        double delta = (osg::Timer::instance()->delta_s(gotoStartTime_, osg::Timer::instance()->tick())) / gotoLookDuration_;
        if((delta < 1.0)&&(delta >= 0.0))
        {
            lon_ = ossim::lerp(delta, targetStartLon_, targetLon_);
            lat_ = ossim::lerp(delta, targetStartLat_, targetLat_);
            eyexyz_.z() = mkUtils::quaderp(delta, targetStartLookZ_, targetMidpointLookZ_, targetLookZ_);
            eyehpr_[0] = ossim::lerp(delta, targetStartLookH_, targetLookH_);
            //           eyehpr_[1] = mkUtils::lerp(delta, targetStartLookP_, targetLookP_);
            eyehpr_[1] = mkUtils::quaderp(delta, targetStartLookP_, 0.0, targetLookP_);
            eyehpr_[2] = ossim::lerp(delta, targetStartLookR_, targetLookR_);
        }
        else
        {
            lon_ = targetLon_;
            lat_ = targetLat_;
            eyexyz_.z() = targetLookZ_;
            eyehpr_[0] = targetLookH_;
            eyehpr_[1] = targetLookP_;
            eyehpr_[2] = targetLookR_;
            gotoing_    = false;
        }
    }
#if 1
    if (theAnimationMode == NAV_ANIMATION_RECORDING)
    {
        osg::Matrixd m = viewMatrix();
        osg::Vec3d eye(m(3,0), m(3,1), m(3,2));
        osg::Vec3d orient = orientation();
        if (theAnimationPath->empty())
        {
            osg::Quat quat = m.getRotate();
            osg::AnimationPath::ControlPoint cp(eye, quat);

            theLastAnimationParameter.quat = quat;
            theLastAnimationParameter.orientation = orient;
            theLastAnimationParameter.eye = eye;
            theAnimationPath->insert(0.0, cp);

        }
        else if (!ossim::almostEqual(eye[0], theLastAnimationParameter.eye[0]) ||
                 !ossim::almostEqual(eye[1], theLastAnimationParameter.eye[1]) ||
                 !ossim::almostEqual(eye[2], theLastAnimationParameter.eye[2]) ||
                 !ossim::almostEqual(orient[0], theLastAnimationParameter.orientation[0]) ||
                 !ossim::almostEqual(orient[1], theLastAnimationParameter.orientation[1]) ||
                 !ossim::almostEqual(orient[2], theLastAnimationParameter.orientation[2]))
        {
            theAnimationPath->insert(osg::Timer::instance()->delta_s(theAnimationStartTime, osg::Timer::instance()->tick()),
                                     osg::AnimationPath::ControlPoint(theLastAnimationParameter.eye,
                                                                      theLastAnimationParameter.quat));

            osg::Quat quat = m.getRotate();
            osg::AnimationPath::ControlPoint cp(eye, quat);

            theLastAnimationParameter.quat = quat;
            theLastAnimationParameter.orientation = orient;
            theLastAnimationParameter.eye = eye;
            theAnimationPath->insert(osg::Timer::instance()->delta_s(theAnimationStartTime, osg::Timer::instance()->tick()), cp);
        }
    }
#endif
#if 0
    // if our lat/lon/elev has changed, send it to the IDOL Browser Agent.
    // note that "idolfederation" is a totally ad hoc name.
    // XXX there should be a flag that determines if we should output this or not?
    if (!oldGotoing && !gotoing_ && (oldLat != lat() || oldLon != lon() || oldElev != elev())) {
        std::ostringstream out;
        out.precision(16);
        out << ":idolbridge gotolatlonelev " << lat() << " " << lon() << " " << elev();
        ossimPlanetAction(out.str()).allExecute();
    }
#endif
}

osg::Vec3d ossimPlanetNavigator::centerOfInterest() const
{
    if (!gotoing_)
        return osg::Vec3d(lat_, lon_, elev());
    else
        return osg::Vec3d(targetLat_, targetLon_, -targetLookZ_*primary_->equatorialRadius());
}

void ossimPlanetNavigator::updateZoomParameters()
{
    double r = primary_->radius(lat_)/primary_->equatorialRadius();
    zoomMin_ = -4.2;         // earth about fills the screen here
    zoomMax_ = 0.0; //  we will need to let current intersection of Line of site handle this if we could
//     zoomMax_ = -1.000001*r;  // close enough
    zoomScaleBaseline_ = r;

    eyexyz_.z() = ossim::clamp(eyexyz_.z(), zoomMin_, zoomMax_);
}

void ossimPlanetNavigator::setViewParameters(const osg::Matrixd& m)
{
   osg::ref_ptr<ossimPlanetGeoRefModel> model=landModel();
   if (model.valid())
   {
      osg::Matrixd tempM;
      osg::Vec3d translation;
      osg::Vec3d scale;
      osg::Quat rotation;
      osg::Quat s;
      osg::Vec3d latLonHeight;
      osg::Vec3d hpr;
      // deomcpose some of the parts we need
      //
      m.decompose(translation, rotation, scale, s);
      model->inverse(translation, latLonHeight);
      model->lsrMatrix(latLonHeight, tempM);
      mkUtils::matrixToHpr(eyehpr_, tempM, m);
      //std::cout << "HPR mkutils = " << eyehpr_ << std::endl;
      setLatLonHeight(latLonHeight[0],
                      latLonHeight[1],
                      latLonHeight[2]);
   }
}

void ossimPlanetNavigator::extractCameraParameters(ossimRefPtr<ossimXmlNode> node,
                                                   double& lat,
                                                   double& lon,
                                                   double& alt,
                                                   double& h,
                                                   double& p,
                                                   double& r)const
{
	osg::ref_ptr<ossimPlanetGeoRefModel> model = landModel();
   ossimString vref = node->getAttributeValue("vref");
   ossimString value;
   lat = lat_;
   lon = lon_;
   alt = elev();
   h = eyehpr_[0];
   p = eyehpr_[1];
   r = eyehpr_[2];

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

void ossimPlanetNavigator::extractLookAtParameters(ossimRefPtr<ossimXmlNode> node,
                                                   ossimPlanetLookAt& look)const
{
	osg::ref_ptr<ossimPlanetGeoRefModel> model = landModel();
   double lat=0.0;
   double lon=0.0;
   double altitude=0.0;
   double h=0.0;
   double p=0.0;
   double r=0.0;
   double range=0.0;
   ossimString value;
   ossimString vref = node->getAttributeValue("vref");
   // solve current view as a lookat
   solveLookAt(lat, lon, altitude, h, p, r, range);

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
      altitude = value.toDouble();
      if(node->getChildTextValue(value, "altitudeMode"))
      {
         if(value.contains("relative"))
         {
            altitude += model->getHeightAboveEllipsoid(lat, lon);
         }
         else if(value.contains("clamp"))
         {
            altitude = model->getHeightAboveEllipsoid(lat, lon);
         }
         else if(value.contains("absolute"))
         {
            if(vref!="wgs84")
            {
               altitude += model->getGeoidOffset(lat, lon);
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
   if(node->getChildTextValue(value, "range"))
   {
      range = value.toDouble();
   }
   look.setAll(lat, lon, altitude,
               h, p, r,
               range, ossimPlanetAltitudeMode_ABSOLUTE);
}

void ossimPlanetNavigator::xmlExecute(const ossimPlanetXmlAction& a)
{
	osg::ref_ptr<ossimPlanetGeoRefModel> model = landModel();
   ossimString command = a.command();
   const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();
	if(command == "Set")
	{
		ossim_uint32 idx = 0;
		for(;idx < children.size();++idx)
		{
			if(children[idx]->getTag() == "Camera")
			{
            double altitude = 0.0;

            // get the altitude vertical frame of reference: msl or wgs84
				extractCameraParameters(children[idx],
                                    lat_,
                                    lon_,
                                    altitude,
                                    eyehpr_[0],
                                    eyehpr_[1],
                                    eyehpr_[2]);

            targetLon_ = lon_;
            targetLat_ = lat_;
            eyexyz_.z() = ossim::clamp(-altitude/primary_->radius(targetLat_) - 1.0, zoomMin_, zoomMax_);
            targetLookZ_      = eyexyz_.z();
            targetStartLookZ_ = eyexyz_.z();
            targetLookH_      = eyehpr_[0];
            targetLookP_      = eyehpr_[1];
            targetLookR_      = eyehpr_[2];
				setRedrawFlag(true);
			}
			else if(children[idx]->getTag() == "LookAt")
			{
            osg::Vec3d llh;
            osg::Vec3d hpr;
            ossimPlanetLookAt look;
            extractLookAtParameters(children[idx],
                                    look);
            solveEyeGivenLocalSpaceAndRange(llh, hpr, osg::Vec3d(look.lat(), look.lon(), look.altitude()), osg::Vec3d(look.heading(),look.pitch(),look.roll()), look.range());
            lon_ = llh[1];
            targetLon_ = llh[1];
            lat_ = llh[0];
            targetLat_ = llh[0];
            eyexyz_.z() = ossim::clamp(-llh[2]/primary_->radius(targetLat_) - 1.0, zoomMin_, zoomMax_);
            targetLookZ_      = eyexyz_.z();
            targetStartLookZ_ = eyexyz_.z();
            eyehpr_[0]   = hpr[0];
            targetLookH_ = eyehpr_[0];
            eyehpr_[1]   = hpr[1];
            targetLookP_ = eyehpr_[1];
            eyehpr_[2]   = hpr[2];
            targetLookR_ = eyehpr_[2];
				setRedrawFlag(true);
			}
		}
	}
	else if(command == "Get")
	{
	}
	else if(command == "FlyTo")
	{
		ossim_uint32 idx = 0;
		for(;idx < children.size();++idx)
		{
			if(children[idx]->getTag() == "Camera")
			{
            double lat=0.0,lon=0.0;
            double altitude = 0.0;
            double h=0.0,p=0.0,r=0.0;
            // get the altitude vertical frame of reference: msl or wgs84
            extractCameraParameters(children[idx],
                                    lat,
                                    lon,
                                    altitude,
                                    h,
                                    p,
                                    r);
            gotoLatLonElevHpr("", lat, lon, altitude,
                              h,
                              p,
                              r);

            setRedrawFlag(true);
         }
			else if(children[idx]->getTag() == "LookAt")
			{
            ossimPlanetLookAt look;
            extractLookAtParameters(children[idx],
                                    look);
            gotoLookAt(look);
         }
      }
	}
}

void ossimPlanetNavigator::destinationCommandExecute(const ossimPlanetDestinationCommandAction& a)
{
   osg::ref_ptr<ossimPlanetGeoRefModel> model = landModel();
   if(!model.valid()) return;
   ossimString command = a.command();
	if ("rotatestart" == command)
	{
	   setRedrawFlag(true);
		rotating_ = true;
		ossimPlanetInteractionController* iac = ossimPlanetInteractionController::instance();
		if(!losLookingFlag_) // only upate if not been initialize by someone else
		{
			baseInputLon_ = iac->interactionValuatorValue("LON");
			baseInputLat_ = iac->interactionValuatorValue("LAT");
		}
	}
	else if ("recordanimation" == command)
	{
	   setRedrawFlag(true);
		startRecording();
	}
	else if ("playanimation" == command)
	{
	   setRedrawFlag(true);
		playRecording();
	}
	else if ("stopanimation" == command)
	{
	   setRedrawFlag(true);
		stopRecording();
	}
	else if ("saveanimation" == command)
	{
		if (a.argCount() == 1)
		{
			std::ofstream outFile(a.arg(1).c_str());
			std::ostringstream outString;
			outFile << "<animationPath>";
			theAnimationPath->write(outString);
			ossimString tempString = ossimString(outString.str().c_str()).substitute("\n", " ", true);
			outFile << tempString << "</animationPath>";
		}
		else
		{
			a.printError("bad argument count");
		}
	}
	else if ("rotatestop" == command)
	{
	   setRedrawFlag(true);
		rotating_ = false;
	}
	else if ("losrotatestart" == command)
	{
		ossimPlanetInteractionController* iac = ossimPlanetInteractionController::instance();
		//         if(!rotating_)
		//         {
		//            baseInputLon_ = iac->interactionValuatorValue("LON");
		//            baseInputLat_ = iac->interactionValuatorValue("LAT");
		//         }
		if(!looking_)
		{
			baseInputYaw_ = iac->interactionValuatorValue("YAW");
			baseInputPitch_ = iac->interactionValuatorValue("PITCH");
		}
		losLookingFlag_ = true;
		losXYZValidFlag_ = false;
	}
	else if ("losrotatestop" == command)
	{
	   setRedrawFlag(true);
		losLookingFlag_ = false;
		losXYZValidFlag_ = false;
	}
	else if ("loszoomstart" == command)
	{
	   setRedrawFlag(true);
		ossimPlanetInteractionController* iac = ossimPlanetInteractionController::instance();
		if(!zooming_)
		{
			baseInputZoom_ = iac->interactionValuatorValue("ZOOM");
		}
		losXYZValidFlag_ = false;
		zoominglos_ = true;
	}
	else if ("loszoomstop" == command)
	{
	   setRedrawFlag(true);
		zoominglos_ = false;
		losXYZValidFlag_ = false;
	}
	else if ("zoomstart" == command)
	{
	   setRedrawFlag(true);
		ossimPlanetInteractionController* iac = ossimPlanetInteractionController::instance();
		if(!zoominglos_)
		{
			baseInputZoom_ = iac->interactionValuatorValue("ZOOM");
		}
		losXYZValidFlag_ = false;
		zooming_ = true;
	}
	else if ("zoomstop" == command)
	{
	   setRedrawFlag(true);
		zooming_ = false;
	}
	else if ("lookstart" == command)
	{
	   setRedrawFlag(true);
		looking_ = true;
		endLooking_ = false;
		ossimPlanetInteractionController* iac = ossimPlanetInteractionController::instance();
		if(!losLookingFlag_)
		{
			baseInputYaw_ = iac->interactionValuatorValue("YAW");
			baseInputPitch_ = iac->interactionValuatorValue("PITCH");
		}
	}
	else if ("los" == command)
	{
		if (a.argCount() == 3)
		{
			if (a.arg(1) == "nan" || a.arg(2) == "nan" || a.arg(3) == "nan")
			{
				losXYZValidFlag_ = false;
			}
		}
		else if (a.argCount() == 0)
		{
			losXYZValidFlag_ = false;
		}
		else
		{
			a.printError("bad argument count");
		}
	}

	else if ("lookstop" == command)
	{
	   setRedrawFlag(true);
		looking_ = false;
		endLooking_ = true;
		endLookingStartTime_ =         osg::Timer::instance()->tick();
	}
	else if ("flystart" == command)
	{
	   setRedrawFlag(true);
		if (a.argCount() == 2)
		{
			if (!flying_)
			{
				flying_ = true;
				xFly_ = mkUtils::asDouble(a.arg(1));
				yFly_ = mkUtils::asDouble(a.arg(2));
			}
			else
			{
				xFly_ += mkUtils::asDouble(a.arg(1));
				yFly_ += mkUtils::asDouble(a.arg(2));
			}
		}
		else
		{
			a.printError("bad argument count");
		}
	}
	else if ("gotolookat" == command)
	{
	   setRedrawFlag(true);
		if (a.argCount() == 7)
		{
			ossimPlanetLookAt info;

			info.setAll(mkUtils::asDouble(a.arg(1)),// lat
							mkUtils::asDouble(a.arg(2)),// lon
							mkUtils::asDouble(a.arg(3)),// altitude
							mkUtils::asDouble(a.arg(4)),// heading
							mkUtils::asDouble(a.arg(5)),// pitch
							mkUtils::asDouble(a.arg(6)),// roll
							mkUtils::asDouble(a.arg(7)) // range
							);
			gotoLookAt(info);
		}
		else if (a.argCount() == 8)
		{
			ossimPlanetLookAt info;

			info.setAll(mkUtils::asDouble(a.arg(1)),// lat
							mkUtils::asDouble(a.arg(2)),// lon
							mkUtils::asDouble(a.arg(3)),// altitude
							mkUtils::asDouble(a.arg(4)),// heading
							mkUtils::asDouble(a.arg(5)),// pitch
							mkUtils::asDouble(a.arg(6)),// roll
							mkUtils::asDouble(a.arg(7)),// range
							ossimPlanetLookAt::modeFromString(a.arg(8))); // mode
			gotoLookAt(info);
		}
		else
		{
			a.printError("bad argument count, the syntax is gotolookat <lat> <lon> <altitude> <heading> <pitch> <roll> <range> <altitudeMode>");
		}
	}
	else if ("gotolatlon" == command)
	{
	   setRedrawFlag(true);
		if (a.argCount() == 2)
			gotoLatLonElevHpr("", mkUtils::asDouble(a.arg(1)), mkUtils::asDouble(a.arg(2)), elev(),
									eyehpr_[0],
									eyehpr_[1],
									eyehpr_[2]);
		else if (a.argCount() == 3)
			gotoLatLonElevHpr(a.arg(1), mkUtils::asDouble(a.arg(2)), mkUtils::asDouble(a.arg(3)), elev(),
									eyehpr_[0],
									eyehpr_[1],
									eyehpr_[2]);
		else
			a.printError("bad argument count");

	}
	else if ("zoomtolatlon" == command)
	{
	   setRedrawFlag(true);
		if (a.argCount() == 2)
		{
			gotoing_ = false;
			lat_ = mkUtils::asDouble(a.arg(1));
			lon_ = mkUtils::asDouble(a.arg(2));
			targetLat_ = lat_;
			targetLon_ = lon_;
			targetLookH_ = eyehpr_[0];
			targetLookP_ = eyehpr_[1];
			targetLookR_ = eyehpr_[2];
			targetLookZ_ = eyexyz_.z();
		}
		else if (a.argCount() == 3)
		{
			gotoing_ = false;
			lat_ = mkUtils::asDouble(a.arg(2));
			lon_ = mkUtils::asDouble(a.arg(3));
			targetLat_ = lat_;
			targetLon_ = lon_;
			targetLookH_ = eyehpr_[0];
			targetLookP_ = eyehpr_[1];
			targetLookR_ = eyehpr_[2];
			targetLookZ_ = eyexyz_.z();

		}
		else
		{
			a.printError("bad argument count");
		}
	}
	else if ("gotolatlonelev" == command)
	{
	   setRedrawFlag(true);
		if (a.argCount() == 3)
		{
			gotoLatLonElevHpr("", mkUtils::asDouble(a.arg(1)), mkUtils::asDouble(a.arg(2)), mkUtils::asDouble(a.arg(3)),
									eyehpr_[0],
									eyehpr_[1],
									eyehpr_[2]);
		}
		else if (a.argCount() == 4)
			gotoLatLonElevHpr(a.arg(1), mkUtils::asDouble(a.arg(2)),
									mkUtils::asDouble(a.arg(3)), mkUtils::asDouble(a.arg(4)),
									eyehpr_[0],
									eyehpr_[1],
									eyehpr_[2]);
		else
			a.printError("bad argument count");

	}
	else if ("setlatlonelevhpr" == command)
	{
	   setRedrawFlag(true);
		if (a.argCount() == 6)
		{
			gotoing_  = false;
			gotoset_  = false;
			rotating_ = false;
			zooming_  = false;
			zoominglos_ = false;
			gotoingelev_=false;
			//              gotoing_ = false;
			//              gotoset_ = true;

			lat_ = mkUtils::asDouble(a.arg(1));
			lon_ = mkUtils::asDouble(a.arg(2));
			eyexyz_.z() = ossim::clamp(-mkUtils::asDouble(a.arg(3))/primary_->radius(targetLat_) - 1.0, zoomMin_, zoomMax_);
			eyehpr_[0] =mkUtils::asDouble(a.arg(4));
			eyehpr_[1] =mkUtils::asDouble(a.arg(5));
			eyehpr_[2] =mkUtils::asDouble(a.arg(6));

			targetLookZ_      = eyexyz_.z();
			targetLookH_      = eyehpr_[0];
			targetLookP_      = eyehpr_[1];
			targetLookR_      = eyehpr_[2];
			targetStartLookZ_ = eyexyz_.z();
			setRedrawFlag(true);
			//std::cout << "ossimPlanetNavigator: setlatlonelevhpr\n";
		}
		else
			a.printError("bad argument count: should be setlatlonelevhpr <lat> <lon> <elev in meters> <heading> <pitch> <roll>");
	}
	//     else if ("gotolatlonelevh" == command)
	//     {
	//        if (a.argCount() == 4)
	//        {
	//           gotoLatLonElevHpr("", mkUtils::asDouble(a.arg(1)), mkUtils::asDouble(a.arg(2)), mkUtils::asDouble(a.arg(3)),
	//                             mkUtils::asDouble(a.arg(4)),
	//                             eyehpr_[1],
	//                             eyehpr_[2]);
	//        }
	//        else if (a.argCount() == 5)
	//           gotoLatLonElevHpr(a.arg(1), mkUtils::asDouble(a.arg(2)),
	//                             mkUtils::asDouble(a.arg(3)), mkUtils::asDouble(a.arg(4)),
	//                             mkUtils::asDouble(a.arg(5)),
	//                             eyehpr_[1],
	//                             eyehpr_[2]);
	//        else
	//           a.printError("bad argument count");

	//     }
	else if ("gotolatlonelevhpr" == command)
	{
	   setRedrawFlag(true);
		if (a.argCount() == 6)
		{
			gotoLatLonElevHpr("", mkUtils::asDouble(a.arg(1)), mkUtils::asDouble(a.arg(2)), mkUtils::asDouble(a.arg(3)),
									mkUtils::asDouble(a.arg(4)),
									mkUtils::asDouble(a.arg(5)),
									mkUtils::asDouble(a.arg(6)));
		}
		else if (a.argCount() == 7)
			gotoLatLonElevHpr(a.arg(1), mkUtils::asDouble(a.arg(2)),
									mkUtils::asDouble(a.arg(3)), mkUtils::asDouble(a.arg(4)),
									mkUtils::asDouble(a.arg(5)),
									mkUtils::asDouble(a.arg(6)),
									mkUtils::asDouble(a.arg(7)));
		else
			a.printError("bad argument count");

	}
	else if ("gotolatlonnadir" == command)
	{
		if (a.argCount() == 2)
		{
			gotoLatLonElevHpr("", mkUtils::asDouble(a.arg(1)), mkUtils::asDouble(a.arg(2)), elev(),
									0.0, pitchOffset_, 0.0);
		}
		else if (a.argCount() == 3)
			gotoLatLonElevHpr(a.arg(1), mkUtils::asDouble(a.arg(2)),
									mkUtils::asDouble(a.arg(3)), elev(),
									0.0, pitchOffset_, 0.0);
		else
			a.printError("bad argument count");

	}
	else if ("gotolatlonelevnadir" == command)
	{
	   setRedrawFlag(true);
		if (a.argCount() == 3)
		{
			gotoLatLonElevHpr("", mkUtils::asDouble(a.arg(1)), mkUtils::asDouble(a.arg(2)), mkUtils::asDouble(a.arg(3)),
									0.0, pitchOffset_, 0.0);
		}
		else if (a.argCount() == 4)
			gotoLatLonElevHpr(a.arg(1), mkUtils::asDouble(a.arg(2)),
									mkUtils::asDouble(a.arg(3)), mkUtils::asDouble(a.arg(4)),
									0.0, pitchOffset_, 0.0);
		else
			a.printError("bad argument count");

	}
	else if ("printlatlonelev" == command)
	{
		cout << "lat = " << lat_ << "  lon = " << lon_ << "  elev = " << elev() << 'm' << endl;

	} else if ("printlookcoordinates" == command) {
		cout << "Look z: " << eyexyz_.z() << "    hpr: " << eyehpr_[0] << ' ' << eyehpr_[1] << ' ' << eyehpr_[2] << endl;

	} else if ("gotoxyz" == command) {
		eyexyz_.set(mkUtils::asDouble(a.arg(1)), mkUtils::asDouble(a.arg(2)), mkUtils::asDouble(a.arg(3)));
		eyehpr_.set(0.0, pitchOffset_, 0.0);

	} else if ("resetzoomlook" == command) {
		eyexyz_.set(0.0, 0.0, zoomMin_);
		eyehpr_.set(0.0, pitchOffset_, 0.0);
		setRedrawFlag(true);

	} else if ("rotatenorth" == command){
	   setRedrawFlag(true);
		if(losXYZValidFlag_&&model.valid())
		{
			osg::Vec3d llh;
			osg::Vec3d newLlh;
			osg::Vec3d xyz;
			osg::Vec3d hpr;
			osg::Vec3d newHpr;
			double range;
			osg::Vec3d tempHpr;

			// first get the look at orientation
			// rotated north
			model->inverse(losXYZ_, llh);
			solveLookAt(llh[0], llh[1], llh[2],
							hpr[0], hpr[1], hpr[2],
							range);


			solveEyeGivenLocalSpaceAndRange(newLlh, newHpr, llh, osg::Vec3d(0.0, hpr[1], 0.0), range);

			setLatLonHeight(newLlh[0], newLlh[1], newLlh[2]);
			eyehpr_ = newHpr;
			eyehpr_[2] = 0.0;
			eyehpr_[0] = ossim::clamp(eyehpr_[0] , -180.0, 180.0);
			eyehpr_[1] = ossim::clamp(eyehpr_[1] , 0.0, 180.0);
		}

	} else if ("rotatenorthup" == command){
	   setRedrawFlag(true);
		if(losXYZValidFlag_&&model.valid())
		{
			osg::Vec3d llh;
			osg::Vec3d newLlh;
			osg::Vec3d xyz;
			osg::Vec3d hpr;
			osg::Vec3d newHpr;
			double range;
			osg::Vec3d tempHpr;

			// first get the look at orientation
			// rotated north
			model->inverse(losXYZ_, llh);
			solveLookAt(llh[0], llh[1], llh[2],
							hpr[0], hpr[1], hpr[2],
							range);


			solveEyeGivenLocalSpaceAndRange(newLlh, newHpr, llh, osg::Vec3d(0.0, pitchOffset_, 0.0), range);

			setLatLonHeight(newLlh[0], newLlh[1], newLlh[2]);
			eyehpr_ = newHpr;
			eyehpr_[2] = 0.0;
			eyehpr_[0] = ossim::clamp(eyehpr_[0] , -180.0, 180.0);
			eyehpr_[1] = ossim::clamp(eyehpr_[1] , 0.0, 180.0);
      }

	} else if ("setgotolookduration" == command) {
		if ((a.argCount() == 1)  &&  (mkUtils::asDouble(a.arg(1)) > 0.0)) {
			gotoLookDuration_ = mkUtils::asDouble(a.arg(1));
		} else {
			a.printError("bad argument, need one float > 0");
		}

	}
	else if ("reset" == command)
	{
		eyexyz_.set(0.0, 0.0, zoomMin_);
		eyehpr_.set(0.0, pitchOffset_, 0.0);
		lat_ = 0.0;
		lon_ = -90.0;
		rotating_ = false;
		zooming_ = false;
		looking_ = false;
		endLooking_ = false;
		gotoing_ = false;
		gotoset_ = false;
		gotoingelev_ = false;
		flying_ = false;
		losLookingFlag_ = false;
		theAnimationMode = ossimPlanetNavigator::NAV_ANIMATION_NONE;
		setRedrawFlag(true);
   }
	else if ("stop" == command)
	{
	   setRedrawFlag(true);
		rotating_   = false;
		zooming_    = false;
		looking_    = false;
		endLooking_ = false;
		gotoing_    = false;
		gotoset_    = false;
		flying_     = false;
		losLookingFlag_ = false;
		theAnimationMode = ossimPlanetNavigator::NAV_ANIMATION_NONE;
	}
	else
	{
		a.printError("bad command");
	}
}


void ossimPlanetNavigator::execute(const ossimPlanetAction &action)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   const ossimPlanetDestinationCommandAction* a = action.toDestinationCommandAction();
   const ossimPlanetXmlAction* xml = action.toXmlAction();
   // for now only support :destination command <args> style actions;
   if(a)
	{
		destinationCommandExecute(*a);
	}
	else if(xml)
	{
		xmlExecute(*xml);
	}
}

// protected

void ossimPlanetNavigator::updateLatLon(float x, float y)
{

   lon_ = ossim::wrap(lon_ - zoomScaleInput(x), primary_->minLon(), primary_->maxLon());
   lat_ -= zoomScaleInput(y);
   if (lat_ > 90.0)
   {
      lat_ = ossim::clamp(90.0 - (lat_ - 90.0), -90.0, 90.0);
      lon_ = ossim::wrap(lon_ + 180.0, primary_->minLon(), primary_->maxLon());
      eyehpr_[0] = ossim::wrap(eyehpr_[0] + 180.0, -180.0, 180.0);
   }
   else if (lat_ < -90.0)
   {
      lat_ = ossim::clamp(-lat_ - 180.0, -90.0, 90.0);
      lon_ = ossim::wrap(lon_ + 180.0, primary_->minLon(), primary_->maxLon());
      eyehpr_[0] = ossim::wrap(eyehpr_[0] - 180.0, -180.0, 180.0);
   }
}

double ossimPlanetNavigator::zoomScaleInput(float input) const
{
   return ossim::sgn(input)*input*input*(std::abs(-eyexyz_.z() - zoomScaleBaseline_));
}

void ossimPlanetNavigator::gotoLookAt(const ossimPlanetLookAt& lookInfo, bool animateFlag)
{
   osg::ref_ptr<ossimPlanetGeoRefModel> model = landModel();
   if(!model.valid()) return;
   osg::Vec3d posLlh = osg::Vec3d(lookInfo.lat(), lookInfo.lon(), lookInfo.altitude());
   osg::Matrixd eyeLsrMatrix = viewMatrix(posLlh[0], posLlh[1], posLlh[2], lookInfo.heading(), lookInfo.pitch(), lookInfo.roll());
   
   // now let's extrude to get the eye location then match so the eye location and lookat are good.
   osg::Vec3d eye(eyeLsrMatrix(3,0), eyeLsrMatrix(3,1), eyeLsrMatrix(3,2));
   osg::Vec3d zAxis(eyeLsrMatrix(2,0), eyeLsrMatrix(2,1), eyeLsrMatrix(2,2));
   eye = eye + zAxis*(lookInfo.range()/model->getNormalizationScale());
   
   osg::Vec3d eyeLlh;
   model->inverse(eye, eyeLlh);
   
   // now solve the lsr matrix
   osg::Matrixd localLsr;
   osg::Vec3d tempHpr;
   model->lsrMatrix(eyeLlh, localLsr);
   mkUtils::matrixToHpr(tempHpr, localLsr, eyeLsrMatrix);
   
   gotoLatLonElevHpr("",
                     eyeLlh[0], eyeLlh[1], eyeLlh[2],
                     tempHpr[0], tempHpr[1], tempHpr[2], 
                     animateFlag);
}

void ossimPlanetNavigator::gotoLatLonElevHpr(const std::string& /*placeName*/,
                                             double latitude,
                                             double longitude,
                                             double elevation,
                                             double heading,
                                             double pitch,
                                             double roll,
                                             bool animateFlag)
{
   
   if(!animateFlag)
   {
      setLatLonHeight(latitude, longitude, elevation);
      setHpr(heading, pitch, roll);
   }
   else
   {
      // This routine is excessively long, should fix that.
      // Also it helps if you have the paper at PJM's desk to understand the math.
      
      if (!ossim::isnan(latitude) && !ossim::isnan(longitude) && !ossim::isnan(elevation) &&
          !ossim::isnan(heading) && !ossim::isnan(pitch) && !ossim::isnan(roll))
      {
         // update() uses the following variables and targetMidpointLookZ_ to move us to the target
         gotoing_ = true;
         losLookingFlag_ = false;
         gotoStartTime_ = osg::Timer::instance()->tick();   //SimClock::currentFrameTime();
         targetStartLat_ = lat_;
         targetStartLon_ = lon_;
         targetStartLookH_ = eyehpr_[0];
         targetStartLookP_ = eyehpr_[1];
         targetStartLookR_ = eyehpr_[2];
         targetLat_ = latitude;
         targetLon_ = longitude;
         targetLookH_ = heading;
         targetLookP_ = pitch;
         targetLookR_ = roll;
         targetLookZ_ = ossim::clamp(-elevation/primary_->radius(lat_) - 1.0, zoomMin_, zoomMax_);
         targetStartLookZ_ = eyexyz_.z();
         
         // the rest of this computes targetMidpointLookZ_ for all cases,
         // so we get a nice zoom every time.
         
         // compute some stuff
         double theta = 0.5*fov_;
         double cosTheta = cos(osg::DegreesToRadians(theta));
         float oldPhi = -1.0f; // init to "see whole globe";  angle from center of screen to edge of screen, angle vertex at globe center
         double d = -eyexyz_.z();
         double d2 = d*d;
         double h = d*cosTheta - sqrt(d2*cosTheta*cosTheta - d2 + 1.0);
         if (!ossim::isnan(h))
         {
            double l = d - 1.0;
            double c = sqrt(l*l + h*h - 2.0*l*h*cosTheta);
            oldPhi = 2.0*osg::RadiansToDegrees(atan(c / (2.0*sqrt(1.0 - 0.25*c*c))));
         }
         float angleDelta = max(fabs(targetLat_ - lat_), fabs(targetLon_ - lon_));   // cheesy
         
         // use that stuff to discern and handle the three cases
         if (oldPhi <= 0.0f || angleDelta < oldPhi)
         {
            // we can already see the target, so don't zoom anywhere.
            targetMidpointLookZ_ = targetStartLookZ_;
            
         }
         else if (angleDelta < 90.0f)
         {
            // target on near side of earth but we're zoomed in too close to see it,
            // so zoom out to see target
            double phi = angleDelta + oldPhi;
            double cosPhi = cos(osg::DegreesToRadians(phi));
            double alpha = 180.0 - theta - phi;
            double cosAlpha = cos(osg::DegreesToRadians(alpha));
            double cos2Alpha = cosAlpha*cosAlpha;
            double cos2AlphaRecip = 1.0/cos2Alpha;
            
            // solve the quadratic to get distance
            pair<double, double> distance = mkUtils::quadraticRoots((cosPhi*cosPhi)/cos2Alpha - 1.0, 2.0*cosPhi - 2.0*cosPhi*cos2AlphaRecip, cos2AlphaRecip - 1.0);
            if (!ossim::isnan(distance.first))
            {
               targetMidpointLookZ_ = -max(distance.first, distance.second);  // use higher elevation
            }
            else
            {
               targetMidpointLookZ_ = zoomMin_; // i don't think this case can happen, but if so take just us all the way out.
            }
            
         }
         else
         {
            // target on far side of earth, so zoom out to where earth just fills screen
            targetMidpointLookZ_ = -(-1.0/(cosTheta*cosTheta - 1.0) + 1.0);
         }
      }
      else
      {
         targetMidpointLookZ_ = (targetStartLookZ_ + targetLookZ_)*.5;
      }
      
      targetMidpointLookZ_ = ossim::clamp(targetMidpointLookZ_, zoomMin_, zoomMax_);
   }
}

osg::Matrixd ossimPlanetNavigator::viewMatrix(double lat, double lon, double hgt,
                                              double h, double p, double r)const
{
   osg::ref_ptr<ossimPlanetGeoRefModel> model = landModel();
   if(model.valid())
   {
      osg::Vec3d xyz;
      model->forward(osg::Vec3d(lat, lon, hgt),///model->getNormalizationScale()),
                            xyz);
      return orientationLsrMatrix(lat, lon, hgt, h, p, r)*osg::Matrixd::translate(xyz[0], xyz[1], xyz[2]);
   }
   return osg::Matrixd();
}

void ossimPlanetNavigator::solveLookAt(double losLat, double losLon, double losHeight,
                                       double& heading, double& pitch, double& roll, double& range)const
{
   osg::ref_ptr<ossimPlanetGeoRefModel> model = landModel();
   if(!model.valid()) return;
   osg::Vec3d eye;
   osg::Matrixd eyeLsrMatrix(viewMatrix());
   osg::Matrixd losLsrMatrix(orientationLsrMatrix(losLat, losLon, losHeight,
                                                  0.0, 0.0, 0.0));
   eye[0] = eyeLsrMatrix(3,0);
   eye[1] = eyeLsrMatrix(3,1);
   eye[2] = eyeLsrMatrix(3,2);
   eyeLsrMatrix(3,0) = 0.0;
   eyeLsrMatrix(3,1) = 0.0;
   eyeLsrMatrix(3,2) = 0.0;

   osg::Vec3d tempHpr;
   mkUtils::matrixToHpr(tempHpr, losLsrMatrix, eyeLsrMatrix);
   osg::Vec3d eyeXyz;
   osg::Vec3d losXyz;
   model->forward(osg::Vec3d(losLat, losLon, losHeight),
                         losXyz);
   range = (losXyz-eye).length()*model->getNormalizationScale();
   heading = tempHpr[0];
   pitch   = tempHpr[1];
   roll    = tempHpr[2];
}
void ossimPlanetNavigator::solveEyeGivenLocalSpaceAndRange(osg::Vec3d& llh, // out lat lon height
                                                           osg::Vec3d& hpr,  // output orientation
                                                           const osg::Vec3d& localLlh,
                                                           const osg::Vec3d& localHpr,
                                                           double rangeInMeters)const
{
   osg::ref_ptr<ossimPlanetGeoRefModel> model = landModel();
   osg::Matrixd m = viewMatrix(localLlh[0], localLlh[1], localLlh[2], localHpr[0], localHpr[1], localHpr[2]);
   osg::Vec3d eye(m(3,0), m(3,1), m(3,2));
   osg::Vec3d zAxis(m(2,0), m(2,1), m(2,2));
   zAxis.normalize();

   // now get the new eye position
   // and solve the orientation
   osg::Vec3d xyz = eye + zAxis*(rangeInMeters/model->getNormalizationScale());
   model->inverse(xyz, llh);
   osg::Matrixd newM = viewMatrix(llh[0], llh[1], llh[2], 0.0, 0.0, 0.0);
   mkUtils::matrixToHpr(hpr, newM, m);
}

void ossimPlanetNavigator::playRecording()
{
   if(!theAnimationPath->getTimeControlPointMap().empty())
   {
      if(theAnimationMode == NAV_ANIMATION_RECORDING)
      {
         theAnimationPath->insert(osg::Timer::instance()->delta_s(theAnimationStartTime, osg::Timer::instance()->tick()),
                                  osg::AnimationPath::ControlPoint(theLastAnimationParameter.eye,
                                                                   theLastAnimationParameter.quat));

      }
      theAnimationMode  = NAV_ANIMATION_PLAYBACK;
      theAnimationStartTime = osg::Timer::instance()->tick();
      rotating_ = false;
      zooming_ = false;
      losLookingFlag_ = false;
      zoominglos_ = false;
      flying_ = false;
   }
   else
   {
      theAnimationMode  = NAV_ANIMATION_NONE;
   }
}

void ossimPlanetNavigator::startRecording()
{
   theAnimationMode = NAV_ANIMATION_RECORDING;
   theAnimationPath->getTimeControlPointMap().clear();
   theAnimationStartTime = osg::Timer::instance()->tick();
}

void ossimPlanetNavigator::stopRecording()
{
   if(theAnimationMode == NAV_ANIMATION_RECORDING)
   {
      theAnimationPath->insert(osg::Timer::instance()->delta_s(theAnimationStartTime, osg::Timer::instance()->tick()),
                               osg::AnimationPath::ControlPoint(theLastAnimationParameter.eye,
                                                                theLastAnimationParameter.quat));

   }
   theAnimationMode  = NAV_ANIMATION_NONE;
}

void ossimPlanetNavigator::saveRecording(std::ostream& out)
{
   if(!theAnimationPath->getTimeControlPointMap().empty())
   {
      theAnimationPath->write(out);
   }
}

bool ossimPlanetNavigator::loadRecording(std::istream& in)
{
   theAnimationPath->getTimeControlPointMap().clear();
   theAnimationPath->read(in);
   return !theAnimationPath->getTimeControlPointMap().empty();
}

void ossimPlanetNavigator::setUseTimedUpdateFlag(bool flag)
{
   theUseTimedUpdateFlag = flag;
}

osg::ref_ptr<ossimPlanetGeoRefModel> ossimPlanetNavigator::landModel()
{
	if(thePlanet)
	{
		return thePlanet->model();
	}
	return 0;
}
const osg::ref_ptr<ossimPlanetGeoRefModel> ossimPlanetNavigator::landModel()const
{
	if(thePlanet)
	{
		return thePlanet->model();
	}
	return 0;
}
