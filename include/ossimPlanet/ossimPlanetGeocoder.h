#ifndef ossimPlanetGeocoder_HEADER
#define ossimPlanetGeocoder_HEADER
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimXmlNode.h>
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetGoecoder : public osg::Referenced
{
public:
   class OSSIMPLANET_DLL ossimPlanetGeocoderLocation : public osg::Referenced
   {
   public:
      ossimPlanetGeocoderLocation();

      void setLocation(const ossimGpt& location);
      ossimGpt getLocation()const;
      void setName(const ossimString& name);
      ossimString getName()const;
      void setAddress(const ossimString& address);
      ossimString getAddress()const;
      
   protected:      
      ossimRefPtr<ossimXmlNode> theMetaInformation; // modeled after the GOOGLE KML
      ossimRefPtr<ossimXmlNode> theNameNode;
      ossimRefPtr<ossimXmlNode> thePlacemarkNode;
      ossimRefPtr<ossimXmlNode> theAddressNode;
      ossimRefPtr<ossimXmlNode> thePointNode;
      ossimRefPtr<ossimXmlNode> theCoordinatesNode;
   };
   virtual void getLocationFromAddress(std::vector<osg::ref_ptr<ossimPlanetGeocoderLocation> >& result,
                                       const ossimString& address,
                                       const ossimString& city,
                                       const ossimString& state,
                                       const ossimString& zip,
                                       const ossimString& /*country*/)const
   {
      ossimString location;
      if(!address.trim().empty())
      {
         location += address;
      }

      if(!city.trim().empty())
      {
         if(!location.empty())
         {
            location += ",";
         }
         location += city;
      }

      if(!state.trim().empty())
      {
         if(!location.empty())
         {
            location += ",";
         }
         location += state;
      }

      if(!zip.trim().empty())
      {
         if(!location.empty())
         {
            location += ",";
         }
         location += zip;
      }
      
      getLocationFromAddress(result, location);
      
   }
   virtual void getLocationFromAddress(std::vector<osg::ref_ptr<ossimPlanetGeocoderLocation> >& result,
                                       const ossimString& location)const=0;   
   
};

#endif
