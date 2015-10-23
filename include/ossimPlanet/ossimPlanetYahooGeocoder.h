#ifndef ossimPlanetYahooGeocoder_HEADER
#define ossimPlanetYahooGeocoder_HEADER
#include <ossimPlanet/ossimPlanetGeocoder.h>
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetYahooGeocoder : public ossimPlanetGoecoder
{
public:
   ossimPlanetYahooGeocoder(const ossimString url = "http://api.local.yahoo.com/MapsService/V1/geocode?",
                            const ossimString yahooAppId = "YahooDemo")
      :theUrl(url),
      theAppId(yahooAppId)
   {
   }
      virtual void getLocationFromAddress(std::vector<osg::ref_ptr<ossimPlanetGeocoderLocation> >& result,
                                       const ossimString& location)const;   

protected:
   ossimString theUrl;
   ossimString theAppId;
};

#endif
