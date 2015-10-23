#include <ossimPlanet/ossimPlanetYahooGeocoder.h>
#include <wms/wmsCurlMemoryStream.h>
#include <ossim/base/ossimXmlDocument.h>
#include <ossim/base/ossimXmlNode.h>
#include <sstream>

void ossimPlanetYahooGeocoder::getLocationFromAddress(std::vector<osg::ref_ptr<ossimPlanetGeocoderLocation> >& result,
                                                      const ossimString& location)const
{
   ossimString url = theUrl + "appid=" + theAppId +"&location="+location.substitute(" ", "+", true);
   wmsCurlMemoryStream curl(url);

   if(curl.download())
   {
      ossimXmlDocument xml;
      ossimString buffer = curl.getStream()->getBufferAsString();
      std::istringstream in(buffer);
      if(xml.read(in))
      {
         std::vector<ossimRefPtr<ossimXmlNode> > nodes;
         xml.findNodes("/ResultSet/Result",
                       nodes);
         if(nodes.size())
         {
            osg::ref_ptr<ossimPlanetGeocoderLocation> location = new ossimPlanetGeocoderLocation;
            ossim_uint32 idx = 0;
            for(idx = 0; idx < nodes.size(); ++idx)
            {
               ossimRefPtr<ossimXmlNode> lat     = nodes[idx]->findFirstNode("Latitude");
               ossimRefPtr<ossimXmlNode> lon     = nodes[idx]->findFirstNode("Longitude");
               ossimRefPtr<ossimXmlNode> zip     = nodes[idx]->findFirstNode("Zip");
               ossimRefPtr<ossimXmlNode> city    = nodes[idx]->findFirstNode("City");
               ossimRefPtr<ossimXmlNode> state   = nodes[idx]->findFirstNode("State");
               ossimRefPtr<ossimXmlNode> address = nodes[idx]->findFirstNode("Address");
               ossimRefPtr<ossimXmlNode> country = nodes[idx]->findFirstNode("Country");
               if(lat.valid()&&lon.valid())
               {
                  ossimGpt gpt(lat->getText().toDouble(),
                               lon->getText().toDouble());
                  location->setLocation(gpt);
                  ossimString name;

                  if(address.valid())
                  {
                     name = address->getText().trim();
                  }
                  if(city.valid())
                  {
                     if(!name.empty())
                     {
                        name += ", ";
                     }
                     name += city->getText().trim();
                  }
                  if(state.valid())
                  {
                     if(!name.empty())
                     {
                        name += ", ";
                     }
                     name += state->getText().trim();
                  }
                  if(zip.valid())
                  {
                     if(!name.empty())
                     {
                        name += ", ";
                     }
                     name += zip->getText().trim();
                  }
                  if(country.valid())
                  {
                     if(!name.empty())
                     {
                        name += ", ";
                     }
                     name += country->getText().trim();
                  }
                  location->setName(name);
                  
                  result.push_back(location);
               }
            }
         }
      }
   }
}
