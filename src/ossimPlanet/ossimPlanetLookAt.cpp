#include <ossimPlanet/ossimPlanetLookAt.h>

bool ossimPlanetLookAt::loadXml(ossimRefPtr<ossimXmlNode> xmlNode)
{
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
    ossim_uint32 idx;
   ossim_uint32 upper=childNodes.size();

   for(idx = 0; idx < upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();

      if(tag == "latitude")
      {
         theLat = childNodes[idx]->getText().toDouble(); 
      }
      else if(tag == "longitude")
      {
         theLon = childNodes[idx]->getText().toDouble(); 
      }
      else if(tag == "altitude")
      {
         theAltitude = childNodes[idx]->getText().toDouble(); 
      }
      else if(tag == "range")
      {
         theRange = childNodes[idx]->getText().toDouble();
      }
      else if(tag == "roll")
      {
         theRoll = childNodes[idx]->getText().toDouble();
      }
      else if(tag == "tilt")
      {
         thePitch = childNodes[idx]->getText().toDouble();
      }
      else if(tag == "heading")
      {
         theHeading = childNodes[idx]->getText().toDouble();
      }
      else if(tag == "altitudeMode")
      {
         theMode = modeFromString(childNodes[idx]->getText());
      }
   }
   
   return true;
}
