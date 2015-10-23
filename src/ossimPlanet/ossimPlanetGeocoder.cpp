#include <ossimPlanet/ossimPlanetGeocoder.h>
#include <sstream>
#include <iostream>
static std::istream& geocoderskipws(std::istream& in)
{
   int c = in.peek();
   while((c == ' ') ||
         (c == '\t') ||
         (c == '\n')||
	 (c == '\r'))
   {
      in.ignore(1);
      c = in.peek();
   }
   
   return in;
}
ossimPlanetGoecoder::ossimPlanetGeocoderLocation::ossimPlanetGeocoderLocation()
{
   theMetaInformation = new ossimXmlNode;
   theMetaInformation->setTag("kml");
   theNameNode = new ossimXmlNode;
   theNameNode->setTag("name");
   theMetaInformation->addChildNode(theNameNode.get());
   thePlacemarkNode = new ossimXmlNode;
   thePlacemarkNode->setTag("Placemark");
   theMetaInformation->addChildNode(thePlacemarkNode.get());
   theAddressNode = new ossimXmlNode;
   theAddressNode->setTag("Address");
   thePointNode = new ossimXmlNode;
   theCoordinatesNode = new ossimXmlNode;
   theCoordinatesNode->setText("0.0, 0.0, 0.0");
   thePointNode->addChildNode(theCoordinatesNode.get());
   
   theMetaInformation->addChildNode(theNameNode.get());
   theMetaInformation->addChildNode(thePlacemarkNode.get());
   theMetaInformation->addChildNode(theAddressNode.get());
   theMetaInformation->addChildNode(thePointNode.get());
}

void ossimPlanetGoecoder::ossimPlanetGeocoderLocation::setLocation(const ossimGpt& location)
{
   theCoordinatesNode->setText(ossimString::toString(location.latd())+ "," +
                               ossimString::toString(location.lond())+ ", 0.0");
                                                     
}

ossimGpt ossimPlanetGoecoder::ossimPlanetGeocoderLocation::getLocation()const
{
   ossimGpt result;
   std::istringstream in(theCoordinatesNode->getText());

   char c;
   double lat, lon;

   in >> lat >> geocoderskipws >> c >> geocoderskipws >>lon;
   result.latd(lat);
   result.lond(lon);

   return result;
}

void ossimPlanetGoecoder::ossimPlanetGeocoderLocation::setName(const ossimString& name)
{
   theNameNode->setText(name);
}

ossimString ossimPlanetGoecoder::ossimPlanetGeocoderLocation::getName()const
{
   return theNameNode->getText();
}

ossimString ossimPlanetGoecoder::ossimPlanetGeocoderLocation::getAddress()const
{
   return theAddressNode->getText();
}

void ossimPlanetGoecoder::ossimPlanetGeocoderLocation::setAddress(const ossimString& address)
{
   theAddressNode->setText(address);
}
