#ifndef ossimPlanetLookAt_HEADER
#define ossimPlanetLookAt_HEADER
#include <osg/Referenced>
#include <ossim/base/ossimString.h>
#include <sstream>
#include <ossimPlanet/ossimPlanetConstants.h>
#include <iostream>
#include <iomanip>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimXmlNode.h>

class OSSIMPLANET_DLL ossimPlanetLookAt : public osg::Referenced
{
public:
   ossimPlanetLookAt()
      :osg::Referenced(),
      theLat(0.0),
      theLon(0.0),
      theAltitude(0.0),
      theHeading(0.0),
      thePitch(0.0),
      theRoll(0.0),
      theRange(0.0),
      theMode(ossimPlanetAltitudeMode_CLAMP_TO_GROUND)
   {
   }
   ossimPlanetLookAt(const ossimPlanetLookAt& src)
      :osg::Referenced(),
      theLat(src.theLat),
      theLon(src.theLon),
      theAltitude(src.theAltitude),
      theHeading(src.theHeading),
      thePitch(src.thePitch),
      theRoll(src.theRoll),
      theRange(src.theRange),
      theMode(src.theMode)
   {
   }
	ossimPlanetLookAt(double lat, double lon, double altitude,
							double heading, double pitch, double roll,
							double range, ossimPlanetAltitudeMode mode = ossimPlanetAltitudeMode_CLAMP_TO_GROUND)
	{
		setAll(lat, lon, altitude, heading, pitch, roll, range, mode);
	}
   virtual ossimPlanetLookAt* clone()const
   {
      return new ossimPlanetLookAt(*this);
   }
   
   void setAll(double lat, double lon, double altitude,
               double heading, double pitch, double roll,
               double range, ossimPlanetAltitudeMode mode = ossimPlanetAltitudeMode_CLAMP_TO_GROUND)
   {
      theLat      = lat;
      theLon      = lon;
      theAltitude = altitude;
      theHeading  = heading;
      thePitch    = pitch;
      theRoll     = roll;
      theRange    = range;
      theMode     = mode;
   }
   void setLat(double value)
   {
      theLat = value;
   }
   double lat()const
   {
      return theLat;
   }
   void setLon(double value)
   {
      theLon = value;
   }
   double lon()const
   {
      return theLon;
   }
   void setAltitude(double value)
   {
      theAltitude = value;
   }
   double altitude()const
   {
      return theAltitude;
   }
   void setHeading(double value)
   {
      theHeading = value;
   }
   double heading()const
   {
      return theHeading;
   }
   void setPitch(double value)
   {
      thePitch = value;
   }
   double pitch()const
   {
      return thePitch;
   }
   void setRoll(double value)
   {
      theRoll = value;
   }
   double roll()const
   {
      return theRoll;
   }
   void setRange(double value)
   {
      theRange = value;
   }
   double range()const
   {
      return theRange;
   }
   void setAltitudeMode(ossimPlanetAltitudeMode mode)
   {
      theMode = mode;
   }
   ossimPlanetAltitudeMode altitudeMode()const
   {
      return theMode;
   }
   ossimString toNavigationString()const
   {
      std::ostringstream out;

      out << std::setprecision(15)
          << ":navigator gotolookat "
          << theLat << " "
          << theLon << " "
          << theAltitude << " "
          << theHeading << " "
          << thePitch   << " "
          << theRoll    << " "
          << theRange   << " "
          << modeToString(theMode);
      
      return ossimString(out.str().c_str());
   }
   ossimString toKml()const
   {
      std::ostringstream out;
      out << std::setprecision(15) << "<LookAt>"
          << "<latitude>"     << theLat << "</latitude>"
          << "<longitude>"    << theLon << "</longitude>"
          << "<altitude>"     << theAltitude << "</altitude>"
          << "<range>"        << theRange << "</range>"
          << "<tilt>"         << thePitch << "</tilt>"
          << "<heading>"      << theHeading << "</heading>"
          << "<altitudeMode>" << modeToString(theMode) << "</altitudeMode>"
          << "</LookAt>";
      return ossimString(out.str().c_str());
   }
   ossimRefPtr<ossimXmlNode> saveXml()const
   {
      ossimXmlNode* node = new ossimXmlNode();
      
      node->setTag("ossimPlanetLookAt");
      node->addChildNode("latitude", ossimString::toString(theLat));
      node->addChildNode("longitude", ossimString::toString(theLon));
      node->addChildNode("altitude", ossimString::toString(theAltitude));
      node->addChildNode("range", ossimString::toString(theRange));
      node->addChildNode("roll", ossimString::toString(theRoll));
      node->addChildNode("tilt", ossimString::toString(thePitch));
      node->addChildNode("heading", ossimString::toString(theHeading));
      node->addChildNode("altitudeMode", modeToString(theMode));

      return node;
   }
   
   bool loadXml(ossimRefPtr<ossimXmlNode> xmlNode);
   
   static ossimString modeToString(ossimPlanetAltitudeMode mode)
   {
      ossimString altitudeMode = "clampToGround";
      
      switch(mode)
      {
         case ossimPlanetAltitudeMode_RELATIVE_TO_GROUND:
         {
            altitudeMode = "relativeToGround";
            break;
         }
         case ossimPlanetAltitudeMode_ABSOLUTE:
         {
            altitudeMode = "absolute";
            break;
         }
         default:
         {
            break;
         }
      }
      return altitudeMode;
   }
   static ossimPlanetAltitudeMode modeFromString(const ossimString& mode)
   {
      ossimPlanetAltitudeMode result = ossimPlanetAltitudeMode_CLAMP_TO_GROUND;
      ossimString testString = mode;
      testString = testString.downcase();
      if(testString == "relativeToGround")
      {
         result = ossimPlanetAltitudeMode_RELATIVE_TO_GROUND;
      }
      else if(testString == "absolute")
      {
         result = ossimPlanetAltitudeMode_ABSOLUTE;
      }

      return result;
   }

   
protected:
   double theLat;
   double theLon;
   double theAltitude;
   double theHeading;
   double thePitch;
   double theRoll;
   double theRange;
   ossimPlanetAltitudeMode theMode;
};

#endif
