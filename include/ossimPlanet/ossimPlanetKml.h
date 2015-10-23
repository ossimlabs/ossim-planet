#ifndef ossimPlanetKml_HEADER
#define ossimPlanetKml_HEADER
#include <iostream>
#include <fstream>
#include <map>
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <osg/Vec3d>
#include <ossim/base/ossimXmlNode.h>
#include <ossim/base/ossimHexString.h>
#include <ossim/base/ossimXmlDocument.h>
#include <ossim/base/ossimDpt3d.h>
#include <ossim/base/ossimFilename.h>
#include <ossimPlanet/ossimPlanetLookAt.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <osg/BoundingSphere>

inline OSSIMPLANET_DLL ossimPlanetKmlColorMode ossimPlanetKmlConvertColorMode(const ossimString& value)
{
   if(value == "normal")
   {
      return ossimPlanetKmlColorMode_NORMAL;
   }
   else if(value == "random")
   {
      return ossimPlanetKmlColorMode_RANDOM;      
   }

   return ossimPlanetKmlColorMode_NONE;
}

inline OSSIMPLANET_DLL ossimPlanetKmlUnits ossimPlanetKmlConvertUnits(const ossimString& value)
{
   if(value == "fraction")
   {
      return ossimPlanetKmlUnits_FRACTION;
   }
   else if(value == "pixels")
   {
      return ossimPlanetKmlUnits_PIXELS;      
   }
   else if(value == "insetPixels")
   {
      return ossimPlanetKmlUnits_INSET_PIXELS;
   }
   
   return ossimPlanetKmlUnits_NONE;
}

inline OSSIMPLANET_DLL ossimString ossimPlanetKmlConvertUnits(ossimPlanetKmlUnits value)
{
   switch(value)
   {
      case ossimPlanetKmlUnits_FRACTION:
      {
         return "fraction";
      }
      case ossimPlanetKmlUnits_PIXELS:
      {
         return "pixels";
      }
      case ossimPlanetKmlUnits_INSET_PIXELS:
      {
         return "insetPixels";
      }
      default:
      {
         break;
      }
   }
   
   return "";
}

inline OSSIMPLANET_DLL ossimPlanetAltitudeMode ossimPlanetKmlConvertAltitudeMode(const ossimString& value)
{
   if((value == "clampToGround")||
      (value == "clampedToGround"))
   {
      return ossimPlanetAltitudeMode_CLAMP_TO_GROUND;
   }
   else if(value == "relativeToGround")
   {
      return ossimPlanetAltitudeMode_RELATIVE_TO_GROUND;      
   }
   else if(value == "absolute")
   {
      return ossimPlanetAltitudeMode_ABSOLUTE;
   }
   
   return ossimPlanetAltitudeMode_NONE;
}

inline OSSIMPLANET_DLL ossimString ossimPlanetKmlConvertAltitudeMode(const ossimPlanetAltitudeMode& value)
{
   switch(value)
   {
      case  ossimPlanetAltitudeMode_CLAMP_TO_GROUND:
      {
         return "clampToGround";
      }
      case ossimPlanetAltitudeMode_RELATIVE_TO_GROUND:
      {
         return "relativeToGround";
      }
      case ossimPlanetAltitudeMode_ABSOLUTE:
      {
         return "absolute";
      }
      default:
      {
         break;
      }
   }
   
   return "";
}

inline OSSIMPLANET_DLL ossimPlanetKmlRefreshMode ossimPlanetKmlConvertRefreshMode(const ossimString& value)
{
   ossimString temp(value);
   temp = temp.downcase();
   
   if(temp == "onchange")
   {
      return ossimPlanetKmlRefreshMode_ON_CHANGE;
   }
   else if(temp == "oninterval")
   {
      return ossimPlanetKmlRefreshMode_ON_INTERVAL;
   }
   else if(temp == "onexpire")
   {
      return ossimPlanetKmlRefreshMode_ON_EXPIRE;
   }

   return ossimPlanetKmlRefreshMode_NONE;
}

inline OSSIMPLANET_DLL ossimString ossimPlanetKmlConvertRefreshMode(ossimPlanetKmlViewRefreshMode value)
{
   switch(value)
   {
      case ossimPlanetKmlRefreshMode_ON_CHANGE:
      {
         return "onChange";
      }
      case ossimPlanetKmlRefreshMode_ON_INTERVAL:
      {
         return "onInterval";
      }
      case ossimPlanetKmlRefreshMode_ON_EXPIRE:
      {
         return "onExpire";
      }
      default:
      {
         break;
      }
   }

   return "";
}

inline OSSIMPLANET_DLL ossimPlanetKmlViewRefreshMode ossimPlanetKmlConvertViewRefreshMode(const ossimString& value)
{
   ossimString temp(value);
   temp = temp.downcase();
   
   if(temp == "never")
   {
      return ossimPlanetKmlViewRefreshMode_NEVER;
   }
   else if(temp == "onrequest")
   {
      return ossimPlanetKmlViewRefreshMode_ON_REQUEST;
   }
   else if(temp == "onstop")
   {
      return ossimPlanetKmlViewRefreshMode_ON_STOP;
   }
   else if(temp == "onregion")
   {
      return ossimPlanetKmlViewRefreshMode_ON_REGION;
   }

   return ossimPlanetKmlViewRefreshMode_NONE;
}

inline OSSIMPLANET_DLL ossimString ossimPlanetKmlConvertViewRefreshMode(ossimPlanetKmlViewRefreshMode value)
{
   switch(value)
   {
      case ossimPlanetKmlViewRefreshMode_NEVER:
      {
         return "never";
      }
      case ossimPlanetKmlViewRefreshMode_ON_REQUEST:
      {
         return "onRequest";
      }
      case ossimPlanetKmlViewRefreshMode_ON_STOP:
      {
         return "onStop";
      }
      case ossimPlanetKmlViewRefreshMode_ON_REGION:
      {
         return "onRegion";
      }
      default:
      {
         break;
      }
   }

   return "never";
}

inline OSSIMPLANET_DLL void ossimPlanetKmlColorToRGBA(ossim_uint8& r,
                                                      ossim_uint8& g,
                                                      ossim_uint8& b,
                                                      ossim_uint8& a,
                                                      const ossimString& colorString)
{
   r = 255;
   g = 255;
   b = 255;
   a = 255;
   if(colorString.size() == 8)
   {
      a = ossimHexString(colorString.begin(),
                         colorString.begin()+2).toUchar();
      b = ossimHexString(colorString.begin()+2,
                         colorString.begin()+4).toUchar();
      g = ossimHexString(colorString.begin()+4,
                         colorString.begin()+6).toUchar();
      r = ossimHexString(colorString.begin()+6,
                         colorString.begin()+8).toUchar();
   }
   else if(colorString.size() == 6)
   {
      b = ossimHexString(colorString.begin(),
                         colorString.begin()+2).toUchar();
      g = ossimHexString(colorString.begin()+2,
                         colorString.begin()+4).toUchar();
      r = ossimHexString(colorString.begin()+4,
                         colorString.begin()+6).toUchar();
      a = 255;
   }
}

class ossimPlanetKmlPoint;
class ossimPlanetKmlLineString;
class ossimPlanetKmlLinearRing;
class ossimPlanetKmlPolygon;
class ossimPlanetKmlMultiGeometry;
class ossimPlanetKmlModel;
class ossimPlanetKmlFeature;

/**
 * This is an abstract base class and cannot be used directly in a KML file.
 * It provides the id attribute, which allows unique identification of a KML element,
 * and the targetId attribute, which is used to reference objects that have already
 * been loaded into Planet. The id attribute must be assigned if the <Update> mechanism is to be used.
 */ 
class OSSIMPLANET_DLL ossimPlanetKmlObject : public osg::Referenced
{
public:
   typedef std::vector<osg::ref_ptr<ossimPlanetKmlObject> > ObjectList;
   ossimPlanetKmlObject()
   {
      theId       = "";
      theTargetId = "";
      theParent = 0;
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode)=0;
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const=0;
   void setId(const ossimString& id)
   {
      theId = id;
   }
   const ossimString& id()
   {
      return theId;
   }

   void setTargetId(const ossimString& targetId)
   {
      theTargetId = targetId;
   }
   
   const ossimString& targetId()const
   {
      return theTargetId;
   }
   ossimPlanetKmlObject::ObjectList& getObjectList()
   {
      return theObjectList;
   }
   const ossimPlanetKmlObject::ObjectList& getObjectList()const
   {
      return theObjectList;
   }
   void setParent(ossimPlanetKmlObject* parent)
   {
      theParent = parent;
   }
   ossimPlanetKmlObject* getParent()
   {
      return theParent;
   }
   const ossimPlanetKmlObject* getParent()const
   {
      return theParent;
   }
   static ossimPlanetKmlObject* getRoot(ossimPlanetKmlObject* start);
   static const ossimPlanetKmlObject* getRoot(const ossimPlanetKmlObject* start);
   ossimFilename getCacheLocation(bool sharedLocationFlag = false)const;
   virtual ossimFilename getKmlFile()const;
protected:
   ossimPlanetKmlObject* theParent;
   ossimPlanetKmlObject::ObjectList theObjectList;
   ossimString theId;
   ossimString theTargetId;
};

/**
 * Lod is an abbreviation for Level of Detail. <Lod> describes the size of the projected
 * region on the screen that is required in order for the region to be considered
 * "active." Also specifies the size of the pixel ramp used for fading in
 * (from transparent to opaque) and fading out (from opaque to transparent).
 */ 
class OSSIMPLANET_DLL ossimPlanetKmlLod: public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlLod()
      :theMinLodPixels(0),
      theMaxLodPixels(-1),
      theMinFadeExtent(0),
      theMaxFadeExtent(0)
      {
      }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;


   ossim_int32 minLodPixels()const
   {
      return theMinLodPixels;
   }
   void setMinLodPixels(ossim_int32 minLod)
   {
      theMinLodPixels = minLod;
   }
   ossim_int32 maxLodPixels()const
   {
      return theMaxLodPixels;
   }
   ossim_int32 minFadeExtent()const
   {
      return theMinFadeExtent;
   }
   void setMinFadeExtent(ossim_int32 minFade)
   {
      theMinFadeExtent = minFade;
   }
   ossim_int32 maxFadeExtent()const
   {
      return theMaxFadeExtent;
   }
   void setMaxFadeExtent(ossim_int32 maxFade)
   {
      theMaxFadeExtent = maxFade;
   }
protected:
   void clearFields()
   {
      theMinLodPixels  = 0;
      theMaxLodPixels  = -1;
      theMinFadeExtent = 0;
      theMaxFadeExtent = 0;
   }
   /**
    *<minLodPixels> (default = 0) Measurement in screen pixels
    * that represents the minimum limit of the visibility range for a
    * given Region. Google Earth calculates the size of the Region when
    * projected onto screen space. Then it computes the square root of
    * the Region's area (if, for example, the Region is square and the
    * viewpoint is directly above the Region, and the Region is not tilted,
    * this measurement is equal to the width of the projected Region).
    * If this measurement falls within the limits defined by
    * <minLodPixels> and <maxLodPixels> (and if the <LatLonAltBox> is in view),
    * the Region is active. If this limit is not reached, the associated geometry
    * is considered to be too far from the user's viewpoint to be drawn.
    */ 
   ossim_int32 theMinLodPixels;

   /**
    *
    * <maxLodPixels> (default = -1) Measurement in screen pixels that
    * represents the maximum limit of the visibility range for a given Region.
    * A value of -1, the default, indicates "active to infinite size."
    */
   ossim_int32 theMaxLodPixels;

   /**
    * <minFadeExtent> (default = 0) Distance over which the geometry fades,
    * from fully opaque to fully transparent. This ramp value,
    * expressed in screen pixels, is applied at the minimum end of the LOD (visibility) limits.
    */
   ossim_int32 theMinFadeExtent;

   /**
    * <maxFadeExtent> (default = 0) Distance over which the geometry fades,
    * from fully transparent to fully opaque. This ramp value, expressed in
    * screen pixels, is applied at the maximum end of the LOD (visibility) limits.
    */ 
   ossim_int32 theMaxFadeExtent;
};

/**
 * Scales a model along the x, y, and z axes in the model's coordinate space.
 */ 
class OSSIMPLANET_DLL ossimPlanetKmlScale: public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlScale()
      :ossimPlanetKmlObject(),
      theX(1.0),
      theY(1.0),
      theZ(1.0)
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   void setX(double x)
   {
      theX = x;
   }
   double x()const
   {
      return theX;
   }

   void setY(double y)
   {
      theY = y;
   }
   double y()const
   {
      return theY;
   }

   void setZ(double z)
   {
      theZ = z;
   }
   double z()const
   {
      return theZ;
   }
protected:
   void clearFields()
   {
      theX = 1.0;
      theY = 1.0;
      theZ = 1.0;
   }
   
   double theX;
   double theY;
   double theZ;
};

/**
 * Specifies the exact coordinates of the Model's origin in latitude, longitude,
 * and altitude. Latitude and longitude measurements are standard lat-lon projection
 * with WGS84 datum.
 */ 
class OSSIMPLANET_DLL ossimPlanetKmlLocation: public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlLocation()
      :ossimPlanetKmlObject(),
      theLongitude(0.0),
      theLatitude(0.0),
      theAltitude(0.0)
   {
   }
   ossimPlanetKmlLocation(const ossimPlanetKmlLocation& src)
      :ossimPlanetKmlObject(src),
      theLongitude(src.theLongitude),
      theLatitude(src.theLatitude),
      theAltitude(src.theAltitude)
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   void setLongitude(double lon)
   {
      theLongitude = lon;
   }
   double longitude()const
   {
      return theLongitude;
   }

   void setLatitude(double lat)
   {
      theLatitude = lat;
   }
   double latitude()const
   {
      return theLatitude;
   }

   void setAltitude(double altitude)
   {
      theAltitude = altitude;
   }
   double altitude()const
   {
      return theAltitude;
   }
protected:
    void clearFields()
    {
       theLongitude = 0.0;
       theLatitude  = 0.0;
       theAltitude  = 0.0;
    }
    double theLongitude;
    double theLatitude;
    double theAltitude;
};

class OSSIMPLANET_DLL ossimPlanetKmlOrientation : public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlOrientation()
      :ossimPlanetKmlObject(),
      theHeading(0.0),
      thePitch(0.0),
      theRoll(0.0)
   {
   }
   ossimPlanetKmlOrientation(const ossimPlanetKmlOrientation& src)
      :ossimPlanetKmlObject(src),
      theHeading(src.theHeading),
      thePitch(src.thePitch),
      theRoll(src.theRoll)
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   void setHeading(double h)
   {
      theHeading = h;
   }
   double heading()const
   {
      return theHeading;
   }
   void setPitch(double p)
   {
      thePitch = p;
   }
   double pitch()const
   {
      return thePitch;
   }
   void setRoll(double r)
   {
      theRoll = r;
   }
   double roll()const
   {
      return theRoll;
   }
protected:
   void clearFields()
   {
      theHeading = 0.0;
      thePitch   = 0.0;
      theRoll    = 0.0;
   }
   double theHeading;
   double thePitch;
   double theRoll;
};


/**
 * Specifies where the top, bottom, right, and left sides of a bounding box for the
 * ground overlay are aligned. The <north>, <south>, <east>, and <west> elements are required.
 * <north> (required) Specifies the latitude of the north edge of the bounding box,
 * in decimal degrees from 0 to ±90.
 * <south> (required) Specifies the latitude of the south edge of the bounding box,
 * in decimal degrees from 0 to ±90.
 * <east> (required) Specifies the longitude of the east edge of the bounding box,
 * in decimal degrees from 0 to ±180. (For overlays that overlap the meridian of 180 longitude, values can extend beyond that range.)
 * <west> (required) Specifies the longitude of the west edge of the bounding box,
 * in decimal degrees from 0 to ±180. (For overlays that overlap the meridian of 180 longitude,
 * values can extend beyond that range.)
 * <rotation> (optional) specifies a rotation of the overlay about its center,
 * in degrees. Values can be ±180. The default is 0 (north). Rotations are specified in a clockwise direction.
 */ 
class OSSIMPLANET_DLL ossimPlanetKmlLatLonBox : public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlLatLonBox()
      :ossimPlanetKmlObject()
   {
      clearFields();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   void clearFields()
   {
      theNorth    = 0.0;
      theSouth    = 0.0;
      theEast     = 0.0;
      theWest     = 0.0;
      theRotation = 0.0;
   }
   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      alt = 0.0;
      lat = (theNorth+theSouth)*0.5;
      lon = (theEast+theWest)*0.5;

      return true;
   }
   void setBounds(double north, double south,
                  double east, double west)
   {
      theNorth = north;
      theSouth = south;
      theEast  = east;
      theWest  = west;
   }
   void setNorth(double n)
   {
      theNorth = n;
   }
   double north()const
   {
      return theNorth;
   }
   void setSouth(double s)
   {
      theSouth = s;
   }
   double south()const
   {
      return theSouth;
   }
   void setEast(double e)
   {
      theEast = e;
   }
   double east()const
   {
      return theEast;
   }
   
   void setRotation(double rotation)
   {
      theRotation = rotation;
   }
   double rotation()const
   {
      return theRotation;
   }
protected:
   
   double theNorth;
   double theSouth;
   double theEast;
   double theWest;
   double theRotation;
};


/**
 * <LatLonAltBox>
 *
 * A bounding box that describes an area of interest
 * defined by geographic coordinates and altitudes.
 * Default values and required fields are as follows:
 *
 * Adds the following tags to LatLonBox:
 * 
 * <altitudeMode> (defaults to clampToGround).
 *                Possible values are clampToGround, relativeToGround, and absolute.
 * <minAltitude> Defaults to 0; specified in meters above sea level (and is affected by
 *               the <altitudeMode> specification).
 * <maxAltitude> Defaults to 0; specified in meters above sea level
 *                (and is affected by the <altitudeMode> specification).
 */ 
class OSSIMPLANET_DLL ossimPlanetKmlLatLonAltBox : public ossimPlanetKmlLatLonBox
{
public:
   ossimPlanetKmlLatLonAltBox()
      :ossimPlanetKmlLatLonBox()
   {
      clearFields();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   void clearFields()
   {
      theMinAltitude = 0.0;
      theMaxAltitude = 0.0;
      theAltitudeMode = ossimPlanetAltitudeMode_CLAMP_TO_GROUND;
   }
   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      alt = (theMinAltitude+theMaxAltitude)*0.5;
      lat = (theNorth+theSouth)*0.5;
      lon = (theEast+theWest)*0.5;
      
      return true;
   }
   void setBounds(double north, double south,
                  double east, double west,
                  double minAlt=0.0, double maxAlt=0.0)
   {
      ossimPlanetKmlLatLonBox::setBounds(north, south, east, west);
      theMinAltitude = minAlt;
      theMaxAltitude = maxAlt;
   }
   void setMinAltitude(double minAlt)
   {
      theMinAltitude = minAlt;
   }
   double minAltitude()const
   {
      return theMinAltitude;
   }
   void setMaxAltitude(double maxAlt)
   {
      theMaxAltitude = maxAlt;
   }
   double maxAltitude()const
   {
      return theMaxAltitude;
   }
   ossimPlanetAltitudeMode altitudeMode()const
   {
      return theAltitudeMode;
   }
   
protected:
   double      theMinAltitude;
   double      theMaxAltitude;
   ossimPlanetAltitudeMode theAltitudeMode;
};

class OSSIMPLANET_DLL ossimPlanetKmlLink : public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlLink()
      :ossimPlanetKmlObject()
   {
      clearFields();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   
   void clearFields()
   {
      theHref            = "";
      theRefreshMode     = ossimPlanetKmlRefreshMode_ON_CHANGE;
      theRefreshInterval = 4.0;
      theViewRefreshMode = ossimPlanetKmlViewRefreshMode_NEVER;
      theViewRefreshTime = 4.0;
      theViewBoundScale  = 1.0;
      theViewFormat      = "";
      theHttpQuery       = "";
   }
   void setHref(const ossimString& ref)
   {
      theHref = ref;
   }
   const ossimString href()const
   {
      return theHref;
   }

   void setRefreshMode(ossimPlanetKmlRefreshMode refreshMode)
   {
      theRefreshMode = refreshMode;
   }
   ossimPlanetKmlRefreshMode refreshMode()const
   {
      return theRefreshMode;
   }

   double refreshInterval()const
   {
      return theRefreshInterval;
   }
   void setRefreshInterval(double interval)
   {
      theRefreshInterval = interval; 
   }

   void setViewRefreshMode(ossimPlanetKmlViewRefreshMode mode)
   {
      theViewRefreshMode = mode;
   }
   ossimPlanetKmlViewRefreshMode viewRefreshMode()const
   {
      return theViewRefreshMode;
   }

   void setViewRefreshTime(double time)
   {
      theViewRefreshTime = time;
   }
   double viewRefreshTime()const
   {
      return theViewRefreshTime;
   }

   void setViewBoundScale(double scale)
   {
      theViewBoundScale = scale;
   }
   double viewBoundScale()const
   {
      return theViewBoundScale;
   }

   void setViewFormat(const ossimString& value)
   {
      theViewFormat = value;
   }
   const ossimString& viewFormat()const
   {
      return theViewFormat;
   }
   void setHttpQuery(const ossimString& value)
   {
      theHttpQuery = value;
   }
   const ossimString& httpQuery()const
   {
      return theHttpQuery;
   }
   virtual ossimFilename download(bool forceOverwrite=false,
                                  const ossimFilename& locationOverride=ossimFilename(""))const;
   
protected:
   
   ossimString theHref;
   ossimPlanetKmlRefreshMode theRefreshMode;
   double      theRefreshInterval;
   ossimPlanetKmlViewRefreshMode theViewRefreshMode;
   double      theViewRefreshTime;
   double      theViewBoundScale;
   ossimString theViewFormat;
   ossimString theHttpQuery;
};

class OSSIMPLANET_DLL ossimPlanetKmlIcon : public ossimPlanetKmlLink
{
public:
   ossimPlanetKmlIcon()
      :ossimPlanetKmlLink()
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
 
};

class OSSIMPLANET_DLL ossimPlanetKmlRegion : public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlRegion()
      :ossimPlanetKmlObject()
   {
      
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      lat = 0.0;
      lon = 0.0;
      alt = 0.0;
      if(theBox.valid())
      {
         return theBox->getCenter(lat, lon, alt);
      }
      return false;
   }
   void setBounds(double north, double south,
                  double east, double west,
                  double minAlt=0.0, double maxAlt=0.0)
   {
      if(!theBox.valid())
      {
         theBox = new ossimPlanetKmlLatLonAltBox;
      }
      theBox->setBounds(north, south, east, west, minAlt, maxAlt);
   }
   void setBox(osg::ref_ptr<ossimPlanetKmlLatLonAltBox> box)
   {
      theBox = box.get();
   }
   const osg::ref_ptr<ossimPlanetKmlLatLonAltBox> box()const
   {
      return theBox.get();
   }
   void setLod(osg::ref_ptr<ossimPlanetKmlLod> lod)
   {
      theLod = lod.get();
   }
   const osg::ref_ptr<ossimPlanetKmlLod> lod()const
   {
      return theLod.get();
   }
protected:
   void clearFields()
   {
      theLod = 0;
      theBox = 0;
   }
   osg::ref_ptr<ossimPlanetKmlLod>          theLod;
   osg::ref_ptr<ossimPlanetKmlLatLonAltBox> theBox;
};

class OSSIMPLANET_DLL ossimPlanetKmlLookAt : public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlLookAt()
      :ossimPlanetKmlObject()
   {
      theLookAt = new ossimPlanetLookAt();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   void setAll(double lat, double lon, double altitude,
               double heading,  double pitch, double range,
               const ossimString& mode="clampToGround")
   {
      theLookAt->setAll(lat, lon, altitude, heading, pitch, 0.0, range,
                        ossimPlanetLookAt::modeFromString(mode));
   }


   osg::ref_ptr<ossimPlanetLookAt> lookAt()
   {
      return theLookAt;
   }
   const osg::ref_ptr<ossimPlanetLookAt> lookAt()const
   {
      return theLookAt;
   }
   
   void clearFields()
   {
      setAll(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, "clampToGround");
   }
   double latitude()const
   {
      return theLookAt->lat();
   }
   double longitude()const
   {
      return theLookAt->lon();
   }
   double altitude()const
   {
      return theLookAt->altitude();
   }
   double heading()const
   {
      return theLookAt->heading();
   }
   double pitch()const
   {
      return theLookAt->pitch();
   }
   double roll()const
   {
      return theLookAt->roll();
   }
   double range()const
   {
      return theLookAt->range();
   }
   void setAlititudeMode(ossimPlanetAltitudeMode mode)
   {
      theLookAt->setAltitudeMode(mode);
   }
   ossimPlanetAltitudeMode altitudeMode()const
   {
      return theLookAt->altitudeMode();
   }
protected:
   osg::ref_ptr<ossimPlanetLookAt> theLookAt;
};

class OSSIMPLANET_DLL ossimPlanetKmlTimePrimitive : public ossimPlanetKmlObject
{
public:
/*    static ossimData toDate(const ossimString& formatString); */
};

class OSSIMPLANET_DLL ossimPlanetKmlTimeSpan : public ossimPlanetKmlTimePrimitive
{
public:
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   void clearFields()
   {
      theBegin = "";
      theEnd  = "";
   }
protected:
   ossimString theBegin;
   ossimString theEnd;
};

class OSSIMPLANET_DLL ossimPlanetKmlTimeStamp : public ossimPlanetKmlTimePrimitive
{
public:
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   void clearFields()
   {
      theWhen = "";
   }
protected:
   ossimString theWhen;
};

class OSSIMPLANET_DLL ossimPlanetKmlColorStyle : public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlColorStyle()
      :ossimPlanetKmlObject(),
      theColor("ffffffff"),
      theColorMode(ossimPlanetKmlColorMode_NORMAL)
   {
   }

   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   void clearFields()
   {
      theColor     = "";
      theColorMode = ossimPlanetKmlColorMode_NORMAL;
   }

   const ossimString& color()const
   {
      return theColor;
   }

   ossimPlanetKmlColorMode colorMode()const
   {
      return theColorMode;
   }
   
public:
   ossimString             theColor;
   ossimPlanetKmlColorMode theColorMode;
};

class OSSIMPLANET_DLL ossimPlanetKmlLineStyle : public ossimPlanetKmlColorStyle
{
public:
   ossimPlanetKmlLineStyle()
      :ossimPlanetKmlColorStyle(),
      theWidth(1)
   {
   }

   bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   ossim_float32 width()const
   {
      return theWidth;
   }
   void clearFields()
   {
      theWidth = 1.0;
   }
   
protected:
   ossim_float32 theWidth;
};

class OSSIMPLANET_DLL ossimPlanetKmlPolyStyle : public ossimPlanetKmlColorStyle
{
public:
   ossimPlanetKmlPolyStyle()
      :ossimPlanetKmlColorStyle(),
      theFillFlag(true),
      theOutlineFlag(true)
   {
   }

   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   bool getFillFlag()const
   {
      return theFillFlag;
   }
   bool getOutlineFlag()const
   {
      return theOutlineFlag;
   }
   void clearFields()
   {
      theFillFlag    = true;
      theOutlineFlag = true;
   }
protected:
   bool theFillFlag;
   bool theOutlineFlag;
};


class OSSIMPLANET_DLL ossimPlanetKmlIconStyle : public ossimPlanetKmlColorStyle
{
public:
   ossimPlanetKmlIconStyle()
      :ossimPlanetKmlColorStyle(),
      theScale(1.0),
      theHeading(0.0),
      theXHotspot(.5),
      theYHotspot(.5),
      theXUnits(ossimPlanetKmlUnits_FRACTION),
      theYUnits(ossimPlanetKmlUnits_FRACTION)
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   void clearFields()
   {
      theScale    = 1.0;
      theHeading  = 1.0;
      theIcon     = 0;
      theXHotspot = .5;
      theYHotspot = .5;
      theXUnits   = ossimPlanetKmlUnits_FRACTION;
   }
   osg::ref_ptr<ossimPlanetKmlIcon> icon()
   {
      return theIcon;
   }
   const osg::ref_ptr<ossimPlanetKmlIcon> icon()const
   {
      return theIcon;
   }
   
   double xHotSpot()const
   {
      return theXHotspot;
   }
   double yHotspot()const
   {
      return theYHotspot;
   }
   double heading()const
   {
      return theHeading;
   }
   double scale()const
   {
      return theScale;
   }
   ossimPlanetKmlUnits xUnits()const
   {
      return theXUnits;
   }
   ossimPlanetKmlUnits yUnits()const
   {
      return theYUnits;
   }
   
protected:
   float theScale;
   float theHeading;
   osg::ref_ptr<ossimPlanetKmlIcon> theIcon;
   float theXHotspot;
   float theYHotspot;
   ossimPlanetKmlUnits theXUnits;
   ossimPlanetKmlUnits theYUnits;
};

class OSSIMPLANET_DLL ossimPlanetKmlLabelStyle : public ossimPlanetKmlColorStyle
{
public:
   ossimPlanetKmlLabelStyle()
      :ossimPlanetKmlColorStyle(),
      theScale(1.0)
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   void setScale(float scale)
   {
      theScale = scale;
   }
   float scale()const
   {
      return theScale;
   }
   void clearFields()
   {
      theScale = 1.0;
   }
protected:
   float theScale;
};

class OSSIMPLANET_DLL ossimPlanetKmlBalloonStyle : public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlBalloonStyle()
      :ossimPlanetKmlObject(),
      theBackgroundColor("ffffffff"),
      theTextColor("ff000000"),
      theText("")
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   const ossimString& backgroundColor()const
   {
      return theBackgroundColor;
   }
   const ossimString& textColor()const
   {
      return theTextColor;
   }
   const ossimString& text()const
   {
      return theText;
   }
   void clearFields()
   {
      theBackgroundColor = "ffffffff";
      theTextColor       = "ff000000";
      theText            = "";
   }
protected:
   ossimString theBackgroundColor;
   ossimString theTextColor;
   ossimString theText;
};

class ossimPlanetKmlStyle;
class ossimPlanetKmlStyleMap;
class OSSIMPLANET_DLL ossimPlanetKmlStyleSelector : public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlStyleSelector()
      :ossimPlanetKmlObject()
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode)
   {
      return ossimPlanetKmlObject::parse(xmlNode);
   }
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const
   {
      ossimPlanetKmlObject::write(xmlNode);
   }
   virtual const ossimPlanetKmlStyle* toStyle()const
   {
      return 0;
   }
   virtual ossimPlanetKmlStyle* toStyle()
   {
      return 0;
   }
   virtual const ossimPlanetKmlStyleMap* toStyleMap()const
   {
      return 0;
   }
   virtual ossimPlanetKmlStyleMap* toStyleMap()
   {
      return 0;
   }
};

class OSSIMPLANET_DLL ossimPlanetKmlStyle : public ossimPlanetKmlStyleSelector
{
public:
   ossimPlanetKmlStyle()
      :ossimPlanetKmlStyleSelector()
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   virtual const ossimPlanetKmlStyle* toStyle()const
   {
      return this;
   }
   virtual ossimPlanetKmlStyle* toStyle()
   {
      return this;
   }
   
   void clearFields()
   {
      theIconStyle  = 0;
      theLabelStyle = 0;
      theLineStyle  = 0;
      thePolyStyle  = 0;
   }
   void setIconStyle(osg::ref_ptr<ossimPlanetKmlIconStyle> value)
   {
      theIconStyle = value;
   }
   const osg::ref_ptr<ossimPlanetKmlIconStyle> iconStyle()const
   {
      return theIconStyle.get();
   }
   void setLabelStyle(osg::ref_ptr<ossimPlanetKmlLabelStyle> value)
   {
      theLabelStyle = value;
   }
   const osg::ref_ptr<ossimPlanetKmlLabelStyle> labelStyle()const
   {
      return theLabelStyle.get();
   }
   void setLineStyle(osg::ref_ptr<ossimPlanetKmlLineStyle> value)
   {
      theLineStyle = value;
   }
   const osg::ref_ptr<ossimPlanetKmlLineStyle> lineStyle()const
   {
      return theLineStyle.get();
   }
   void setPolyStyle(osg::ref_ptr<ossimPlanetKmlPolyStyle> value)
   {
      thePolyStyle = value;
   }
   const osg::ref_ptr<ossimPlanetKmlPolyStyle> polyStyle()const
   {
      return thePolyStyle.get();
   }
   void setBalloonStyle(osg::ref_ptr<ossimPlanetKmlBalloonStyle> value)
   {
      theBalloonStyle = value;
   }
   const osg::ref_ptr<ossimPlanetKmlBalloonStyle> balloonStyle()const
   {
      return theBalloonStyle.get();
   }

protected:
   osg::ref_ptr<ossimPlanetKmlIconStyle>  theIconStyle;
   osg::ref_ptr<ossimPlanetKmlLabelStyle> theLabelStyle;
   osg::ref_ptr<ossimPlanetKmlLineStyle>  theLineStyle;
   osg::ref_ptr<ossimPlanetKmlPolyStyle>  thePolyStyle;
   osg::ref_ptr<ossimPlanetKmlBalloonStyle>  theBalloonStyle;
};

class OSSIMPLANET_DLL ossimPlanetKmlStyleMap : public ossimPlanetKmlStyleSelector
{
public:
   typedef std::map<std::string, std::string> MapType;
   
   ossimPlanetKmlStyleMap()
      :ossimPlanetKmlStyleSelector()
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   void clearFields()
   {
      theStyleMap.clear();
   }
   virtual const ossimPlanetKmlStyleMap* toStyleMap()const
   {
      return this;
   }
   virtual ossimPlanetKmlStyleMap* toStyleMap()
   {
      return this;
   }
   void styles(std::vector<ossimString>& styleNames)const
   {
      MapType::const_iterator i = theStyleMap.begin();
      while(i!=theStyleMap.end())
      {
         styleNames.push_back(i->first);
         ++i;
      }
   }
   ossimString findStyle(const ossimString& styleName)const
   {
      MapType::const_iterator i = theStyleMap.find(styleName);
      if(i != theStyleMap.end())
      {
         return i->second;
      }
      return "";
   }
   ossimString normalUrl()const
   {
      return findStyle("normal");
   }
   ossimString highlightUrl()const
   {
      return findStyle("highlight");
   }
protected:
/*    ossimString theNormalUrl; */
/*    ossimString theHighlightUrl; */
   MapType theStyleMap;
};


class OSSIMPLANET_DLL ossimPlanetKmlFeature : public ossimPlanetKmlObject
{
public:
   ossimPlanetKmlFeature()
      :ossimPlanetKmlObject()
   {
      clearFields();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   void clearFields()
   {
      theName           = "";
      theVisibilityFlag = true;
      theOpenFlag       = true;
      theAddress        = "";
      thePhoneNumber    = "";
      theDescription    = "";
      theCDataDescriptionFlag = false;
      theLookAt         = 0;
      theStyleUrl       = "";
      theStyleSelector  = 0;
      theRegion         = 0;
      theExtendedData   = 0;
   }
   void setName(const ossimString& name)
   {
      theName = name;
   }
   const ossimString& name()const
   {
      return theName;
   }
   bool visibilityFlag()const
   {
      return theVisibilityFlag;
   }
   void setVisibilityFlag(bool flag)
   {
      theVisibilityFlag = flag;
   }
   void setOpenFlag(bool flag)
   {
      theOpenFlag = flag;
   }
   bool openFlag()const
   {
      return theOpenFlag;
   }
   void setAddress(const ossimString& address)
   {
      theAddress = address;
   }
   const ossimString& address()const
   {
      return theAddress;
   }
   void setPhoneNumber(const ossimString& phoneNumber)
   {
      thePhoneNumber = phoneNumber;
   }
   const ossimString& phoneNumber()const
   {
      return thePhoneNumber;
   }
   void setDescription(const ossimString& description)
   {
      theDescription = description;
   }
   const ossimString& description()const
   {
      return theDescription;
   }
   void setLookAt(osg::ref_ptr<ossimPlanetKmlLookAt> lookAt)
   {
      theLookAt = lookAt.get();
   }
   const osg::ref_ptr<ossimPlanetKmlLookAt>  lookAt()const
   {
      return theLookAt;
   }
   void setSyleUrl(const ossimString& styleUrl)
   {
      theStyleUrl = styleUrl;
   }
   const ossimString& styleUrl()const
   {
      return theStyleUrl;
   }
   void setRegion(osg::ref_ptr<ossimPlanetKmlRegion> region)
   {
      theRegion = region.get();
   }
   osg::ref_ptr<ossimPlanetKmlRegion> region()const
   {
      return theRegion;
   }
   void setExtendedData(ossimRefPtr<ossimXmlNode> value)
   {
      theExtendedData = value;
   }
   const ossimXmlNode* extendedData()const
   {
      return theExtendedData.get();
   }
   ossimXmlNode* extendedData()
   {
      return theExtendedData.get();
   }

   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      lat = 0.0;
      lon = 0.0;
      alt = 0.0;

      if(theRegion.valid())
      {
         return theRegion->getCenter(lat, lon, alt);
      }
      return false;
   }
   
   const osg::ref_ptr<ossimPlanetKmlStyleSelector> getStyleSelector()const
   {
      return theStyleSelector;
   }
protected:
   ossimString theName;
   bool        theVisibilityFlag;
   bool        theOpenFlag;
   ossimString theAddress;
   // there is an Address details field.  Need to look at it later.

   // End details field

   ossimString thePhoneNumber;
   ossimString theSnippet;
   ossimString theSnippetMaxLines;
   ossimString theDescription;
   bool        theCDataDescriptionFlag;
   osg::ref_ptr<ossimPlanetKmlLookAt> theLookAt;
   osg::ref_ptr<ossimPlanetKmlTimePrimitive> theTimePrimitive;
   ossimString theStyleUrl;
   osg::ref_ptr<ossimPlanetKmlStyleSelector> theStyleSelector;
   osg::ref_ptr<ossimPlanetKmlRegion> theRegion;
   ossimRefPtr<ossimXmlNode> theExtendedData;
};


class OSSIMPLANET_DLL ossimPlanetKmlGeometry : public ossimPlanetKmlObject
{
public:
   typedef std::vector<osg::Vec3d> PointListType;
   
   ossimPlanetKmlGeometry()
      :ossimPlanetKmlObject()
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   virtual bool getCenter(double& lat, double& lon, double& alt)const=0;
   virtual ossimPlanetAltitudeMode altitudeMode()const
   {
      return ossimPlanetAltitudeMode_NONE;
   }

   virtual ossimPlanetKmlPoint* toPoint(){return 0;}
   virtual const ossimPlanetKmlPoint* toPoint()const{return 0;}
   virtual ossimPlanetKmlLineString* toLineString(){return 0;}
   virtual const ossimPlanetKmlLineString* toLineString()const{return 0;}
   virtual ossimPlanetKmlLinearRing* toLinearRing(){return 0;}
   virtual const ossimPlanetKmlLinearRing* toLinearRing()const{return 0;}
   virtual ossimPlanetKmlPolygon* toPolygon(){return 0;}
   virtual const ossimPlanetKmlPolygon* toPolygon()const{return 0;}
   virtual ossimPlanetKmlMultiGeometry* toMultiGeometry(){return 0;}
   virtual const ossimPlanetKmlMultiGeometry* toMultiGeometry()const{return 0;}
   virtual ossimPlanetKmlModel* toModel(){return 0;}
   virtual const ossimPlanetKmlModel* toModel()const{return 0;}

   static void computeCenter(osg::Vec3d& result,
                             const PointListType& pointList);
};

class OSSIMPLANET_DLL ossimPlanetKmlPoint : public ossimPlanetKmlGeometry
{
public:
   ossimPlanetKmlPoint()
      :ossimPlanetKmlGeometry()
   {
      clearFields();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      if(thePointList.size() > 0)
      {
         lat = theCenter[1];
         lon = theCenter[0];
         alt = theCenter[2];
      }

      return (thePointList.size()>0);
   }
   virtual ossimPlanetKmlPoint* toPoint(){return this;}
   virtual const ossimPlanetKmlPoint* toPoint()const{return this;}

   virtual ossimPlanetAltitudeMode altitudeMode()const
   {
      return theAltitudeMode;
   }

   void setExtrudeFlag(bool flag)
   {
      theExtrudeFlag = flag;
   }
   bool extrudeFlag()const
   {
      return theExtrudeFlag;
   }

   void setTesselateFlag(bool flag)
   {
      theTesselateFlag = flag;
   }
   bool tesselateFlag()const
   {
      return theTesselateFlag;
   }

   const ossimPlanetKmlGeometry::PointListType& pointList()const
   {
      return thePointList;
   }
   ossimPlanetKmlGeometry::PointListType& pointList()
   {
      return thePointList;
   }
   
protected:
   void clearFields()
   {
      theExtrudeFlag = false;
      theTesselateFlag = false;
      theAltitudeMode = ossimPlanetAltitudeMode_CLAMP_TO_GROUND;
      thePointList.clear();
      theCenter = osg::Vec3d(0.0,0.0,0.0);
   }
   
   bool                    theExtrudeFlag;
   bool                    theTesselateFlag;
   ossimPlanetAltitudeMode theAltitudeMode;
   ossimPlanetKmlGeometry::PointListType thePointList;
   osg::Vec3d theCenter;
};

class OSSIMPLANET_DLL ossimPlanetKmlLineString : public ossimPlanetKmlGeometry
{
public:
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   virtual ossimPlanetKmlLineString* toLineString(){return this;}
   virtual const ossimPlanetKmlLineString* toLineString()const{return this;}
   virtual ossimPlanetAltitudeMode altitudeMode()const
   {
      return theAltitudeMode;
   }

   void setExtrudeFlag(bool flag)
   {
      theExtrudeFlag = flag;
   }
   bool extrudeFlag()const
   {
      return theExtrudeFlag;
   }

   void setTesselateFlag(bool flag)
   {
      theTesselateFlag = flag;
   }
   bool tesselateFlag()const
   {
      return theTesselateFlag;
   }

   const ossimPlanetKmlGeometry::PointListType& pointList()const
   {
      return thePointList;
   }
   ossimPlanetKmlGeometry::PointListType& pointList()
   {
      return thePointList;
   }
   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      if(thePointList.size() > 0)
      {
         lat = theCenter[1];
         lon = theCenter[0];
         alt = theCenter[2];
      }

      return (thePointList.size()>0);
   }
   
protected:
   void clearFields()
   {
      theExtrudeFlag = false;
      theTesselateFlag = false;
      theAltitudeMode = ossimPlanetAltitudeMode_CLAMP_TO_GROUND;
      thePointList.clear();
      theCenter = osg::Vec3d(0.0,0.0,0.0);
   }
   bool        theExtrudeFlag;
   bool        theTesselateFlag;
   ossimPlanetAltitudeMode theAltitudeMode;
   ossimPlanetKmlGeometry::PointListType thePointList;
   osg::Vec3d theCenter;
};

class OSSIMPLANET_DLL ossimPlanetKmlLinearRing : public ossimPlanetKmlGeometry
{
public:
   ossimPlanetKmlLinearRing()
      :ossimPlanetKmlGeometry()
   {
      clearFields();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   virtual ossimPlanetAltitudeMode altitudeMode()const
   {
      return theAltitudeMode;
   }

   void setExtrudeFlag(bool flag)
   {
      theExtrudeFlag = flag;
   }
   bool extrudeFlag()const
   {
      return theExtrudeFlag;
   }

   void setTesselateFlag(bool flag)
   {
      theTesselateFlag = flag;
   }
   bool tesselateFlag()const
   {
      return theTesselateFlag;
   }
   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      if(thePointList.size() > 0)
      {
         lat = theCenter[1];
         lon = theCenter[0];
         alt = theCenter[2];
      }

      return (thePointList.size()>0);
   }
   virtual ossimPlanetKmlLinearRing* toLinearRing(){return this;}
   virtual const ossimPlanetKmlLinearRing* toLinearRing()const{return this;}
   const ossimPlanetKmlGeometry::PointListType& pointList()const
   {
      return thePointList;
   }
   ossimPlanetKmlGeometry::PointListType& pointList()
   {
      return thePointList;
   }

protected:
   void clearFields()
   {
      theExtrudeFlag = false;
      theTesselateFlag = false;
      theAltitudeMode = ossimPlanetAltitudeMode_CLAMP_TO_GROUND;
      thePointList.clear();
      theCenter = osg::Vec3d(0.0,0.0,0.0);
   }
   bool        theExtrudeFlag;
   bool        theTesselateFlag;
   ossimPlanetAltitudeMode theAltitudeMode;
   ossimPlanetKmlGeometry::PointListType thePointList;
   osg::Vec3d theCenter;
};

class OSSIMPLANET_DLL ossimPlanetKmlPolygon : public ossimPlanetKmlGeometry
{
public:
   typedef std::vector<osg::ref_ptr<ossimPlanetKmlLinearRing> > InnerBoundaryList;
   
   ossimPlanetKmlPolygon()
      :ossimPlanetKmlGeometry()
   {
      clearFields();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   virtual ossimPlanetAltitudeMode altitudeMode()const
   {
      return theAltitudeMode;
   }

   void setExtrudeFlag(bool flag)
   {
      theExtrudeFlag = flag;
   }
   bool extrudeFlag()const
   {
      return theExtrudeFlag;
   }

   void setTesselateFlag(bool flag)
   {
      theTesselateFlag = flag;
   }
   bool tesselateFlag()const
   {
      return theTesselateFlag;
   }
   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      if(theOuterBoundary.valid())
      {
         return theOuterBoundary->getCenter(lat, lon, alt);
      }

      return false;
   }
   virtual ossimPlanetKmlPolygon* toPolygon(){return this;}
   virtual const ossimPlanetKmlPolygon* toPolygon()const{return this;}


   osg::ref_ptr<ossimPlanetKmlLinearRing> outerBoundary()
   {
      return theOuterBoundary;
   }
   const osg::ref_ptr<ossimPlanetKmlLinearRing> outerBoundary()const
   {
      return theOuterBoundary;
   }

   ossimPlanetKmlPolygon::InnerBoundaryList& innerBoundaryList()
   {
      return theInnerBoundaryList;
   }

   const ossimPlanetKmlPolygon::InnerBoundaryList& innerBoundaryList()const
   {
      return theInnerBoundaryList;
   }
   
protected:
   void clearFields()
   {
      theExtrudeFlag = false;
      theTesselateFlag = false;
      theAltitudeMode = ossimPlanetAltitudeMode_CLAMP_TO_GROUND;
      theOuterBoundary = 0;
      theInnerBoundaryList.clear();
      theCenter = osg::Vec3d(0.0,0.0,0.0);
   }
   bool        theExtrudeFlag;
   bool        theTesselateFlag;
   ossimPlanetAltitudeMode theAltitudeMode;
   osg::ref_ptr<ossimPlanetKmlLinearRing>   theOuterBoundary;
   ossimPlanetKmlPolygon::InnerBoundaryList  theInnerBoundaryList;
   osg::Vec3d theCenter;
};

class OSSIMPLANET_DLL ossimPlanetKmlMultiGeometry : public ossimPlanetKmlGeometry
{
public:
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      if(theGeometryList.size()>0)
      {
         theGeometryList[0]->getCenter(lat, lon, alt);
      }
/*       if(theGeometryList.size() > 0) */
/*       { */
/*          double tempLat=0.0, tempLon=0.0, tempAlt=0.0; */
/*          theGeometryList[0]->getCenter(tempLat, tempLon, tempAlt); */
/*          ossim_uint32 idx = 1; */
/*          for(;idx < theGeometryList.size(); ++idx) */
/*          { */
/*             if(theGeometryList[idx]->getCenter(lat, lon, alt)) */
/*             { */
/*                tempLat += lat; */
/*                tempLon += lon; */
/*                tempAlt += alt; */
/*             } */
/*          } */
/*          lat = tempLat/(double)theGeometryList.size(); */
/*          lon = tempLon/(double)theGeometryList.size(); */
/*          alt = tempAlt/(double)theGeometryList.size(); */
         
/*       } */
      return (theGeometryList.size() > 0);
   }   
   virtual ossimPlanetKmlMultiGeometry* toMultiGeometry(){return this;}
   virtual const ossimPlanetKmlMultiGeometry* toMultiGeometry()const{return this;}

   std::vector<osg::ref_ptr<ossimPlanetKmlGeometry> >& geomtryList()
   {
      return theGeometryList;
   }
   const std::vector<osg::ref_ptr<ossimPlanetKmlGeometry> >& geomtryList()const
   {
      return theGeometryList;
   }
protected:
   void clearFields()
   {
      theGeometryList.clear();
      theCenter = osg::Vec3d(0.0,0.0,0.0);
   }
   std::vector<osg::ref_ptr<ossimPlanetKmlGeometry> > theGeometryList;
   osg::Vec3d theCenter;
};

class OSSIMPLANET_DLL ossimPlanetKmlModel : public ossimPlanetKmlGeometry
{
public:
   ossimPlanetKmlModel()
   {
      clearFields();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      lat = theLocation->latitude();
      lon = theLocation->longitude();
      alt = theLocation->altitude();

      return true;
   }   
   virtual ossimPlanetKmlModel* toModel(){return this;}
   virtual const ossimPlanetKmlModel* toModel()const{return this;}

   osg::ref_ptr<ossimPlanetKmlLocation> location()
   {
      return theLocation;
   }
   const osg::ref_ptr<ossimPlanetKmlLocation> location()const
   {
      return theLocation;
   }
   osg::ref_ptr<ossimPlanetKmlOrientation> orientation()
   {
      return theOrientation;
   }
   const osg::ref_ptr<ossimPlanetKmlOrientation> orientation()const
   {
      return theOrientation;
   }
   osg::ref_ptr<ossimPlanetKmlScale> scale()
   {
      return theScale;
   }
   
   const osg::ref_ptr<ossimPlanetKmlScale> scale()const
   {
      return theScale;
   }
   osg::ref_ptr<ossimPlanetKmlLink> link()
   {
      return theLink;
   }
   const osg::ref_ptr<ossimPlanetKmlLink> link()const
   {
      return theLink;
   }
   virtual ossimPlanetAltitudeMode altitudeMode()const
   {
      return theAltitudeMode;
   }
   void setAltitudeMode(ossimPlanetAltitudeMode mode)
   {
      theAltitudeMode = mode;
   }
   
protected:
   void clearFields()
   {
      theLocation    = new ossimPlanetKmlLocation;
      theOrientation = new ossimPlanetKmlOrientation;
      theScale       = new ossimPlanetKmlScale;
      theLocation->setParent(this);
      theOrientation->setParent(this);
      theScale->setParent(this);
      theLink = 0;
      theAltitudeMode = ossimPlanetAltitudeMode_CLAMP_TO_GROUND;
   }

   osg::ref_ptr<ossimPlanetKmlLocation>    theLocation;
   osg::ref_ptr<ossimPlanetKmlOrientation> theOrientation;
   osg::ref_ptr<ossimPlanetKmlScale>       theScale; 
   osg::ref_ptr<ossimPlanetKmlLink>        theLink;
   ossimPlanetAltitudeMode                 theAltitudeMode;
 
};

class OSSIMPLANET_DLL ossimPlanetKmlOverlay : public ossimPlanetKmlFeature
{
public:
   ossimPlanetKmlOverlay()
      :ossimPlanetKmlFeature()
   {
      clearFields();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   void setColor(const ossimString& value)
   {
      theColor = value;
   }
   const ossimString& color()const
   {
      return theColor;
   }
   void setDrawOrder(ossim_int32 value)
   {
      theDrawOrder = value;
   }
   ossim_int32 drawOrder()const
   {
      return theDrawOrder;
   }
   void clearFields()
   {
      theColor     = "ffffffff";
      theDrawOrder = 0;
      theIcon      = 0;
   }
   osg::ref_ptr<ossimPlanetKmlIcon> icon()
   {
      return theIcon;
   }
   const osg::ref_ptr<ossimPlanetKmlIcon> icon()const
   {
      return theIcon;
   }
   
protected:
   ossimString theColor;
   ossim_int32 theDrawOrder;
   osg::ref_ptr<ossimPlanetKmlIcon> theIcon;
};

class OSSIMPLANET_DLL ossimPlanetKmlScreenOverlay : public ossimPlanetKmlOverlay
{
public:
   ossimPlanetKmlScreenOverlay()
      :ossimPlanetKmlOverlay()
   {
      clearFields();
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   void setOverlayPosition(double x, double y)
   {
      theOverlayX = x;
      theOverlayY = y;
   }
   void setOverlayUnits(ossimPlanetKmlUnits units)
   {
      theOverlayXUnits = units;
      theOverlayYUnits = units;
   }
   void setOverlayXUnits(ossimPlanetKmlUnits units)
   {
      theOverlayXUnits = units;
   }
   void setOverlayYUnits(ossimPlanetKmlUnits units)
   {
      theOverlayYUnits = units;
   }
   double overlayX()const
   {
      return theOverlayX;
   }
   double overlayY()const
   {
      return theOverlayY;
   }
   ossimPlanetKmlUnits overlayXUnits()const
   {
      return theOverlayXUnits;
   }
   ossimPlanetKmlUnits overlayYUnits()const
   {
      return theOverlayYUnits;
   }
   
   void setScreenPosition(double x, double y)
   {
      theScreenX = x;
      theScreenY = y;
   }
   void setScreenUnits(ossimPlanetKmlUnits units)
   {
      theScreenXUnits = units;
      theScreenYUnits = units;
   }
   void setScreenXUnits(ossimPlanetKmlUnits units)
   {
      theScreenXUnits = units;      
   }
   void setScreenYUnits(ossimPlanetKmlUnits units)
   {
      theScreenYUnits = units;      
   }
   double screenX()const
   {
      return theScreenX;
   }
   double screenY()const
   {
      return theScreenY;
   }
   ossimPlanetKmlUnits screenXUnits()const
   {
      return theScreenXUnits;
   }
   ossimPlanetKmlUnits screenYUnits()const
   {
      return theScreenYUnits;
   }
   void setRotationXY(double xRotate, double yRotate)
   {
      theRotationX = xRotate;
      theRotationY = yRotate;
   }
   void setRotationXYUnits(ossimPlanetKmlUnits units)
   {
      theRotationXUnits = units;
      theRotationYUnits = units;
   }
   void setRotationXUnits(ossimPlanetKmlUnits units)
   {
      theRotationXUnits = units;
   }
   void setRotationYUnits(ossimPlanetKmlUnits units)
   {
      theRotationYUnits = units;
   }
   double rotationX()const
   {
      return theRotationX;
   }
   double rotationY()const
   {
      return theRotationY;
   }
   
   ossimPlanetKmlUnits rotationXUnits()const
   {
      return theRotationXUnits;
   }
   ossimPlanetKmlUnits rotationYUnits()const
   {
      return theRotationYUnits;
   }
   void setSize(double x, double y)
   {
      theSizeX = x;
      theSizeY = y;
   }
   void setSizeX(double x)
   {
      theSizeX = x;
   }
   void setSizeY(double y)
   {
      theSizeY = y;
   }
   void setSizeUnits(ossimPlanetKmlUnits units)
   {
      theSizeXUnits = units;
      theSizeYUnits = units;
   }
   double sizeX()const
   {
      return theSizeX;
   }
   double sizeY()const
   {
      return theSizeY;
   }
   ossimPlanetKmlUnits sizeXUnits()const
   {
      return theSizeXUnits;
   }
   ossimPlanetKmlUnits sizeYUnits()const
   {
      return theSizeYUnits;
   }
   
   void setRotation(float value)
   {
      theRotation = value;
   }
   float rotation()const
   {
      return theRotation;
   }
   void clearFields()
   {
      theOverlayX= 0.0;
      theOverlayY= 0.0;
      theOverlayXUnits = ossimPlanetKmlUnits_FRACTION;
      theOverlayYUnits = ossimPlanetKmlUnits_FRACTION;
      theScreenX= 0.0;
      theScreenY= 0.0;
      theScreenXUnits = ossimPlanetKmlUnits_FRACTION;
      theScreenYUnits = ossimPlanetKmlUnits_FRACTION;
      theRotationX = 0.0;
      theRotationY = 0.0;
      theRotationXUnits = ossimPlanetKmlUnits_FRACTION;
      theRotationYUnits = ossimPlanetKmlUnits_FRACTION;
      theSizeX = 0.0;
      theSizeY = 0.0;
      theSizeXUnits = ossimPlanetKmlUnits_FRACTION;
      theSizeYUnits = ossimPlanetKmlUnits_FRACTION;
      theRotation = 0.0;
   }
                    
protected:
   double theOverlayX;
   double theOverlayY;
   ossimPlanetKmlUnits theOverlayXUnits;
   ossimPlanetKmlUnits theOverlayYUnits;
   double theScreenX;
   double theScreenY;
   ossimPlanetKmlUnits theScreenXUnits;
   ossimPlanetKmlUnits theScreenYUnits;
   double theRotationX;
   double theRotationY;
   ossimPlanetKmlUnits theRotationXUnits;
   ossimPlanetKmlUnits theRotationYUnits;
   double theSizeX;
   double theSizeY;
   ossimPlanetKmlUnits theSizeXUnits;
   ossimPlanetKmlUnits theSizeYUnits;
   float theRotation;
};

class OSSIMPLANET_DLL ossimPlanetKmlGroundOverlay : public ossimPlanetKmlOverlay
{
public:
   ossimPlanetKmlGroundOverlay()
      :ossimPlanetKmlOverlay(),
      theAltitude(0.0),
      theAltitudeMode(ossimPlanetAltitudeMode_CLAMP_TO_GROUND),
      theLatLonBox(0)
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   double altitude()const
   {
      return theAltitude;
   }
   void setAltitude(double value)
   {
      theAltitude = value;
   }
   ossimPlanetAltitudeMode altitudeMode()const
   {
      return theAltitudeMode;
   }
   void setAltitudeMode(ossimPlanetAltitudeMode mode)
   {
      theAltitudeMode = mode;
   }
   const osg::ref_ptr<ossimPlanetKmlLatLonBox> latLonBox()const
   {
      return theLatLonBox;
   }
   void setLatLonBox(osg::ref_ptr<ossimPlanetKmlLatLonBox> value)
   {
      theLatLonBox = value;
   }
   void clearFields()
   {
      theAltitude = 0.0;
      theAltitudeMode = ossimPlanetAltitudeMode_CLAMP_TO_GROUND;
      theLatLonBox = 0;
   }
protected:
   double                                theAltitude;
   ossimPlanetAltitudeMode               theAltitudeMode;
   osg::ref_ptr<ossimPlanetKmlLatLonBox> theLatLonBox;
};

class OSSIMPLANET_DLL ossimPlanetKmlNetworkLink : public ossimPlanetKmlFeature
{
public:
   ossimPlanetKmlNetworkLink()
      :ossimPlanetKmlFeature()
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   const ossimPlanetKmlLink* link()const
   {
      return theLink.get();
   }
   ossimPlanetKmlLink* link()
   {
      return theLink.get();
   }
   void setLink(osg::ref_ptr<ossimPlanetKmlLink> value)
   {
      theLink = value;
   }
   bool refreshVisibilityFlag()const
   {
      return theRefreshVisibilityFlag;
   }
   void setRefreshVisibilityFlag(bool value)
   {
      theRefreshVisibilityFlag = value;
   }
   bool flyToViewFlag()const
   {
      return theFlyToViewFlag;
   }
   void setFlyToViewFlag(bool value)
   {
      theFlyToViewFlag = value;
   }
   void clearFields()
   {
      theLink                  = 0;
      theRefreshVisibilityFlag = false;
      theFlyToViewFlag         = false;
   }
   
protected:
   
   osg::ref_ptr<ossimPlanetKmlLink> theLink;

   bool                             theRefreshVisibilityFlag;
   bool                             theFlyToViewFlag;
};

class OSSIMPLANET_DLL ossimPlanetKmlPlacemark : public ossimPlanetKmlFeature
{
public:
   ossimPlanetKmlPlacemark()
      :ossimPlanetKmlFeature()
   {}
   bool parse(const ossimRefPtr<ossimXmlNode> node);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;

   void clearFields()
   {
      theGeometry = 0;
   }
   osg::ref_ptr<ossimPlanetKmlGeometry> getGeometry()
   {
      return theGeometry;
   }
   const osg::ref_ptr<ossimPlanetKmlGeometry> getGeometry()const
   {
      return theGeometry;
   }
   virtual bool getCenter(double& lat, double& lon, double& alt)const
   {
      bool result = ossimPlanetKmlFeature::getCenter(lat, lon, alt);

      if(!result)
      {
         if(theGeometry.valid())
         {
            result = theGeometry->getCenter(lat, lon, alt);
         }
      }      
      return result;
   }

 void setColor(std::string color)
 {
    thePColor = color;    
 }
 const std::string& getPColor()const
 {
   return thePColor;
 }  
 
protected:
    std::string thePColor;
   osg::ref_ptr<ossimPlanetKmlGeometry> theGeometry;
};

class OSSIMPLANET_DLL ossimPlanetKmlContainer : public ossimPlanetKmlFeature
{
public:
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   
};

class OSSIMPLANET_DLL ossimPlanetKmlFolder : public ossimPlanetKmlContainer
{
public:
   ossimPlanetKmlFolder()
      :ossimPlanetKmlContainer()
   {
   }
   virtual bool parse(const ossimRefPtr<ossimXmlNode> xmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
};

class OSSIMPLANET_DLL ossimPlanetKmlDocument : public ossimPlanetKmlContainer
{
public:
   ossimPlanetKmlDocument();
   bool parse(const ossimRefPtr<ossimXmlNode> node);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
};

class OSSIMPLANET_DLL ossimPlanetKml : public ossimPlanetKmlObject
{
public:
   typedef std::map<std::string, osg::ref_ptr<ossimPlanetKmlObject> > IdMapType;
   ossimPlanetKml();

   virtual bool parse(const ossimFilename& file);
   virtual bool parse(std::istream& in, bool fullDocumentFlag=true);
   virtual bool parse(const ossimRefPtr<ossimXmlDocument> document);

/*    const ossimPlanetKmlObject::ObjectList& getObjectList()const */
/*    { */
/*       return theObjectList; */
/*    } */

   virtual bool parse(const ossimRefPtr<ossimXmlNode> kmlNode);
   virtual void write(ossimRefPtr<ossimXmlNode> xmlNode)const;
   virtual ossimRefPtr<ossimXmlDocument> writeDocument()const;
   bool getAllFeatures(ossimPlanetKmlObject::ObjectList& placemarks);
   void createIdMap()const;
   osg::ref_ptr<ossimPlanetKmlObject> findById(const ossimString& id);
   const osg::ref_ptr<ossimPlanetKmlObject> findById(const ossimString& id)const;

   const ossimFilename& filename()const
   {
      return theFilename;
   }
   virtual bool isCompressed()const
   {
      return false;
   }
   
   ossimFilename getCacheLocation(bool sharedLocationFlag = false)const;
   virtual ossimFilename getKmlFile()const
   {
      return theFilename;
   }
protected:
   ossimFilename                    theFilename;
   mutable ossimFilename            theCacheLocation;
   mutable bool                     theIdMapGeneratedFlag;
   mutable IdMapType theIdMap;
};


class OSSIMPLANET_DLL ossimPlanetKmz : public ossimPlanetKml
{
public:

   ossimPlanetKmz();
   virtual ~ossimPlanetKmz();
   virtual bool parse(std::istream& in);
   virtual bool parse(const ossimFilename& file);
   virtual bool isCompressed()const
   {
      return true;
   }
protected:
   void extractFiles();
   void deleteExtractedFiles();
   void* thePrivateData;  
};


class OSSIMPLANET_DLL ossimPlanetKmlObjectRegistry
{
public:
   static ossimPlanetKmlObjectRegistry* instance();
   ossimPlanetKmlObject* newObject(const ossimXmlNode* tag)const;
   ossimPlanetKmlObject* newObject(const ossimString& tag)const;
   ossimPlanetKmlObject* newGeometry(const ossimString& tag)const;
   ossimPlanetKmlObject* newTimePrimitive(const ossimString& tag)const;
   ossimPlanetKmlObject* newColorStyle(const ossimString& tag)const;
    
protected:
   ossimPlanetKmlObjectRegistry();
   ossimPlanetKmlObjectRegistry(const ossimPlanetKmlObjectRegistry&){}
   
   static ossimPlanetKmlObjectRegistry* theInstance;
   
};


#endif
