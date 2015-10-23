#ifndef ossimPlanetNavigator_HEADER
#define ossimPlanetNavigator_HEADER

#include <assert.h>
#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetConstants.h>
#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetPrimaryBody.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetLookAt.h>
#include <osg/Timer>
#include <osg/AnimationPath>
#include <osg/ref_ptr>
#include <osg/Referenced>
#include <osg/Matrixd>
//#include <osgGA/MatrixManipulator>
#include <osgGA/CameraManipulator>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <ossimPlanet/ossimPlanetDestinationCommandAction.h>
#include <ossimPlanet/ossimPlanetXmlAction.h>

class ossimPlanet;
class OSSIMPLANET_DLL ossimPlanetNavigator : public osg::Referenced//,
	                                         //public ossimPlanetActionReceiver
{
public:
    ossimPlanetNavigator(osg::ref_ptr<ossimPlanetPrimaryBody> p);
        // assert(p);
    ~ossimPlanetNavigator();

    double lat() const
        // latitude of eye in degrees
        { return lat_; };
    
    double lon() const
        // longitude of eye in degrees
        { return lon_; };

    double elev() const
        // elevation of eye in meters
        { return zoom2Elevation(eyexyz_.z()); }

    const osg::Vec3d& orientation() const
        // eyepoint orientation in euler angles: heading (yaw), pitch, roll
        { return eyehpr_; };

    double metersToCenter() const;
        // distance from eye to center of primary

   void setLatLonHeight(double lat, double lon, double height);
   void setHpr(double h, double p, double r);
        // set the lat, lon, elevation above planet

    virtual osg::Vec3d centerOfInterest() const;
        // lat/lon/elev of center of interest in degrees/degrees/meters for CacheEngine purposes
    
    virtual void update();
        // update look/lat/lon/elev/navigation-quasimodes based on current navigation-quasimodes

    void updateZoomParameters();
        // update info needed to clamp/manage the eyepoint zoomin

    const osg::ref_ptr<ossimPlanetPrimaryBody> primary() const
        // the primary we are orbiting
        { return primary_; }
    
    // ActionReceiver features

    virtual void execute(const ossimPlanetAction &a);
        // execute action


    // uncommented mystery routines under here

    osg::Matrixd orientationLsrMatrix() const
        { return orientationLsrMatrix(lat_, lon_, elev(), eyehpr_[0], eyehpr_[1], eyehpr_[2]); }
    
    osg::Matrixd orientationLsrMatrix(double lat, double lon, double hgt, double h, double p, double r) const;

    osg::Matrixd viewMatrix() const
        { return viewMatrix(lat_, lon_, elev(), eyehpr_[0], eyehpr_[1], eyehpr_[2]); }
    
    osg::Matrixd viewMatrix(double lat, double lon, double hgt, double h, double p, double r) const;    

    void setViewParameters(const osg::Matrixd& m);
    
    void setPlanet(ossimPlanet* planet)
   { thePlanet = planet; }
    
    bool canSetLineOfSite() const
    { return ((!zoominglos_ && !losLookingFlag_)||
              (!losXYZValidFlag_||rotating_||looking_||flying_)); }
        
    double pitchOffset() const
        { return pitchOffset_; }

    void setLosXYZ(const osg::Vec3d& losXYZ)
        {
            losXYZ_ = losXYZ;
            losXYZValidFlag_ = !ossim::isnan(losXYZ[0]) && !ossim::isnan(losXYZ[1]) && !ossim::isnan(losXYZ[2]);
        }
    
    void setLosXYZValidFlag(bool flag)
        { losXYZValidFlag_ = flag; }
        
    bool gotZoomingLosFlag() const
        { return zoominglos_; }
        
    bool getLosLookingFlag() const
        { return losLookingFlag_; }
        
    void setLosLookingFlag(bool flag)
        { losLookingFlag_ = flag; }
    
    enum NavigatorAnimationMode { NAV_ANIMATION_NONE = 0, NAV_ANIMATION_RECORDING = 1, NAV_ANIMATION_PLAYBACK  = 2 };

    void solveLookAt(double losLat, double losLon, double losHeight,
                     double& heading, double& pitch, double& roll, double& range)const;
    void solveEyeGivenLocalSpaceAndRange(osg::Vec3d& llh, // out lat lon height
                                         osg::Vec3d& hpr,  // output orientation
                                         const osg::Vec3d& localPt,
                                         const osg::Vec3d& localHpr,
                                         double rangeInMeters)const;
   void playRecording();
   void startRecording();
   void stopRecording();
   void saveRecording(std::ostream& out);
   bool loadRecording(std::istream& in);

   void setUseTimedUpdateFlag(bool flag);

   bool needsContinuousUpdate()const
   {
      return ((theAnimationMode==NAV_ANIMATION_PLAYBACK)||
              rotating_||
              zooming_||
              zoominglos_||
              looking_||
              gotoing_||
              gotoingelev_||
              gotoset_||
              flying_||
              losLookingFlag_);
   }
   void setRedrawFlag(bool flag)
   {
      theRedrawFlag = flag;
   }
   bool redrawFlag()const
   {
      return theRedrawFlag;
   }
   osg::ref_ptr<ossimPlanetGeoRefModel> landModel();
   const osg::ref_ptr<ossimPlanetGeoRefModel> landModel()const;
	void gotoLatLonElevHpr(const string& placeName, 
                          double latitude, 
                          double longitude, 
                          double elevation, 
                          double h, 
                          double p, 
                          double r,
                          bool animateFlag=true);
	void gotoLookAt(const ossimPlanetLookAt& lookInfo,bool animateFlag=true);
protected:
	void xmlExecute(const ossimPlanetXmlAction& action);
	void destinationCommandExecute(const ossimPlanetDestinationCommandAction& action);
	
	void updateLatLon(float x, float y);
	
	
	// update lon_ and lat_ based on user input x,y in [-1,1] and elev()
	void extractCameraParameters(ossimRefPtr<ossimXmlNode> node,
                                double& lat,
                                double& lon,
                                double& alt,
                                double& h,
                                double& p,
                                double& r)const;
	void extractLookAtParameters(ossimRefPtr<ossimXmlNode> node,
                                ossimPlanetLookAt& look)const;
	inline double zoom2Elevation(double zoomParameter) const
	{ return (-zoomParameter - 1.0)*primary_->radius(lat()); }

    double pitchOffset_;

    double lat_;
    double lon_;
        // current latitude and longitude of eye in degrees

    osg::Vec3d eyexyz_;
        // eyepoint position (z for zooming, xy unused)

    osg::Vec3d eyehpr_; 
        // eyepoint orientation (heading/pitch/roll)

    double zoomMin_;
    double zoomMax_;
        // min and max distances to zoom in and out

    double fov_;
        // GraphicsSystem::instance()->camera()->fov()

    ossimPlanet* thePlanet;

    osg::ref_ptr<ossimPlanetPrimaryBody> primary_;
        // the primary we are observing (ie, the celestial body we orbit, ie probably the earth)

    bool rotating_;
    bool zooming_;
    bool zoominglos_;
    bool looking_;
    bool endLooking_;
    bool gotoing_;
    bool gotoingelev_;
    bool gotoset_;
    bool flying_;
    bool losLookingFlag_;
        // current user navigation quasimodes

    double targetLat_;
    double targetLon_;
    double targetLookZ_;
    double targetStartLookZ_;
    double targetStartLat_;
    double targetStartLon_;
    double targetStartLookH_;
    double targetStartLookP_;
    double targetStartLookR_;
    double targetMidpointLookZ_;
    double targetLookH_;
    double targetLookP_;
    double targetLookR_;
    
    osg::Vec3d losXYZ_;
    bool losXYZValidFlag_;

    double updateRatePerSecond_;
    osg::Timer_t lastUpdateTime_;
    
    osg::Timer_t gotoStartTime_;
        // data for going to a target location

    double gotoLookDuration_;
        // duration in seconds of movement in a gotolatlon and a look view snapback

    osg::Timer_t endLookingStartTime_;
        // time when user stopped looking around

    double zoomScaleBaseline_;
        // pfUnits elevation height of maximum user input sensitivity 

    double zoomScaleInput(float input) const;
        // scale user input based on current zoom level and zoomScaleBaseline_

    double baseInputLon_, baseInputLat_, baseInputZoom_, baseInputYaw_, baseInputPitch_;
        // initial values for device input, so we can use relative device movement

    double xFly_, yFly_;
        // values specified by last flying action (which may be still active; flying_ will tell you)
        
    struct LastAnimationParameter
    {
       osg::Quat  quat;
       osg::Vec3d orientation;
       osg::Vec3d eye;
    };
    NavigatorAnimationMode theAnimationMode;
    osg::ref_ptr<osg::AnimationPath> theAnimationPath;
    LastAnimationParameter theLastAnimationParameter;
    osg::Timer_t theAnimationStartTime;

    mutable ossimPlanetReentrantMutex theMutex;
    bool theUseTimedUpdateFlag;
    bool theRedrawFlag;
};
#endif
