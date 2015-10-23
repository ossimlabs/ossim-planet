#include <ossimPlanet/ossimPlanetKml.h>
#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanetSetup.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <wms/wmsCurlMemoryStream.h>

#include <queue>
#ifdef OSSIMPLANET_HAS_LIBZ
#include <ossimPlanet/unzip.h>
#endif

static std::istream& kmlskipws(std::istream& in)
{
   int c = in.peek();
   while(!in.fail()&&
         ((c == ' ') ||
         (c == '\t') ||
         (c == '\n')||
	 (c == '\r')))
   {
      in.ignore(1);
      c = in.peek();
   }
   
   return in;
}

static void kmlReadCoordinates(ossimPlanetKmlGeometry::PointListType& pointListResult,
                            std::istream& in)
{
   bool pointIsGood = true;
   osg::Vec3d point;
   while(!in.fail())
   {
      in >>point[0]>>kmlskipws;
      in.ignore();
      in >>point[1]>>kmlskipws;
	  
	  // if we get at least 2 values then we are good
      pointIsGood = !in.fail();
      point[2] = 0.0;
      if(in.peek() == ',')
      {
         in.ignore();
         in >> point[2];
      }
      
      in>>kmlskipws;
      
      if(in.peek() == ',')
      {
         in.ignore(); // we will suport separated by commas even though the spec says spaces
      }
      if(pointIsGood)
      {
         pointListResult.push_back(point);
      }
   }
}

static void kmlReadCoordinates(ossimPlanetKmlGeometry::PointListType& pointListResult,
                            const ossimString& inString)
{
   if(inString.empty()) return;
   std::vector<ossimString> points;
   inString.split(points, " \n\t\r");
   ossim_uint32 idx = 0;
   osg::Vec3d point;
   for(idx = 0; idx < points.size(); ++idx)
   {
	   std::vector<ossimString> coordinate;
	   points[idx].split(coordinate, ",");
	   if(coordinate.size() >= 2)
	   {
	      point[0] = coordinate[0].toDouble();
	      point[1] = coordinate[1].toDouble();
		  if(coordinate.size() > 2)
		  {
			point[2] = coordinate[2].toDouble();
		  }
		  else
		  {
			point[2] = 0.0;
		  }
		  pointListResult.push_back(point);
	   }
   }
   //std::istringstream in(inString);
   //kmlReadCoordinates(pointListResult, in);
}

ossimPlanetKmlObjectRegistry* ossimPlanetKmlObjectRegistry::theInstance = 0;
ossimPlanetKmlObjectRegistry::ossimPlanetKmlObjectRegistry()
{
   theInstance = this;
}


ossimPlanetKmlObjectRegistry* ossimPlanetKmlObjectRegistry::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimPlanetKmlObjectRegistry;
   }

   return theInstance;
} 
ossimPlanetKmlObject* ossimPlanetKmlObjectRegistry::newObject(const ossimString& tag)const
{
   if(tag == "Document")
   {
      return new ossimPlanetKmlDocument;
   }
   else if(tag == "Placemark")
   {
      return new ossimPlanetKmlPlacemark;
   }
   else if(tag == "Folder")
   {
      return new ossimPlanetKmlFolder;
   }
   else if(tag == "Icon")
   {
      return new ossimPlanetKmlIcon;
   }
   else if(tag == "LatLonBox")
   {
      return new ossimPlanetKmlLatLonBox;
   }
   else if(tag == "LatLonAltBox")
   {
      return new ossimPlanetKmlLatLonAltBox;
   }
   else if(tag == "GroundOverlay")
   {
      return new ossimPlanetKmlGroundOverlay;
   }
   else if(tag == "ScreenOverlay")
   {
      return new ossimPlanetKmlScreenOverlay;
   }
   else if(tag == "Point")
   {
      return new ossimPlanetKmlPoint;
   }
   else if(tag == "NetworkLink")
   {
      return new ossimPlanetKmlNetworkLink;
   }
   else if(tag == "Orientation")
   {
      return new ossimPlanetKmlOrientation;
   }
   else if(tag == "Location")
   {
      return new ossimPlanetKmlLocation;
   }
   else if(tag == "Scale")
   {
      return new ossimPlanetKmlScale;
   }
   else if(tag == "Lod")
   {
      return new ossimPlanetKmlLod;
   }
   else if(tag == "Region")
   {
      return new ossimPlanetKmlRegion;
   }
   else if(tag == "BalloonStyle")
   {
      return new ossimPlanetKmlBalloonStyle;
   }
   else if(tag == "Style")
   {
      return new ossimPlanetKmlStyle;
   }
   else if(tag == "StyleMap")
   {
      return new ossimPlanetKmlStyleMap;
   }
   ossimPlanetKmlObject* obj = newTimePrimitive(tag);
   if(obj)
   {
      return obj;
   }
   obj = newColorStyle(tag);
   if(obj)
   {
      return obj;
   }
   return newGeometry(tag);
}

ossimPlanetKmlObject* ossimPlanetKmlObjectRegistry::newGeometry(const ossimString& tag)const
{
   if(tag == "Point")
   {
      return new ossimPlanetKmlPoint;
   }
   else if(tag == "LineString")
   {
      return new ossimPlanetKmlLineString;
   }
   else if(tag == "LinearRing")
   {
      return new ossimPlanetKmlLinearRing;
   }
   else if(tag == "Polygon")
   {
      return new ossimPlanetKmlPolygon;
   }
   else if((tag == "MultiGeometry")||
           (tag == "GeometryCollection"))
   {
      return new ossimPlanetKmlMultiGeometry;
   }
   else if(tag == "Model")
   {
      return new ossimPlanetKmlModel;
   }
   return 0;
}

ossimPlanetKmlObject* ossimPlanetKmlObjectRegistry::newColorStyle(const ossimString& tag)const
{
   if(tag == "LineStyle")
   {
      return new ossimPlanetKmlLineStyle;
   }
   else if(tag == "PolyStyle")
   {
      return new ossimPlanetKmlPolyStyle;
   }
   else if(tag == "IconStyle")
   {
      return new ossimPlanetKmlIconStyle;
   }
   else if(tag == "LabelStyle")
   {
      return new ossimPlanetKmlLabelStyle;
   }
   return 0;
}

ossimPlanetKmlObject* ossimPlanetKmlObjectRegistry::newTimePrimitive(const ossimString& tag)const
{
   if(tag == "TimeSpan")
   {
      return new ossimPlanetKmlTimeSpan;
   }
   else if(tag == "TimeStamp")
   {
      return new ossimPlanetKmlTimeStamp;
   }

   return 0;
}

ossimPlanetKmlObject* ossimPlanetKmlObjectRegistry::newObject(const ossimXmlNode* tag)const
{
   if(tag)
   {
      return newObject(tag->getTag());
      
   }

   return 0;
}

bool ossimPlanetKmlObject::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   if(!xmlNode.valid()) return false;

   xmlNode->getAttributeValue(theId, "id");
   xmlNode->getAttributeValue(theTargetId, "targetId");
   
   return true;
}

void ossimPlanetKmlObject::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   if(!xmlNode.valid()) return;
   if(!theId.empty())
   {
      xmlNode->addAttribute("id", theId);
   }
   if(!theTargetId.empty())
   {
      xmlNode->addAttribute("targetId", theTargetId);      
   }
}

ossimPlanetKmlObject* ossimPlanetKmlObject::getRoot(ossimPlanetKmlObject* start)
{
   ossimPlanetKmlObject* root = start;

   if(!root) return 0;
   while(root->getParent()) root = root->getParent();
   return root;
}

ossimFilename ossimPlanetKmlObject::getKmlFile()const
{
   if(getParent()) return getParent()->getKmlFile();

   return "";
}

const ossimPlanetKmlObject* ossimPlanetKmlObject::getRoot(const ossimPlanetKmlObject* start)
{
   const ossimPlanetKmlObject* root = start;

   if(!root) return 0;
   while(root->getParent()) root = root->getParent();
   return root;
}


ossimFilename ossimPlanetKmlObject::getCacheLocation(bool sharedLocationFlag)const
{
   const ossimPlanetKmlObject* kmlObject = getRoot(this);

   const ossimPlanetKml* kml = dynamic_cast<const ossimPlanetKml*>(kmlObject);

   if(kml)
   {
      return kml->getCacheLocation(sharedLocationFlag);
   }

   if(sharedLocationFlag)
   {
      ossimFilename sharedDir = ossimEnvironmentUtility::instance()->getUserOssimSupportDir();
      sharedDir = sharedDir.dirCat("kml");
      if(!sharedDir.exists())
      {
         sharedDir.createDirectory();
      }
      return sharedDir;
   }
   return "";
}

bool ossimPlanetKmlBalloonStyle::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }

   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();

   ossim_uint32 idx;
   ossim_uint32 upper=childNodes.size();

   for(idx = 0; idx < upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();
      if(tag == "bgColor")
      {
         theBackgroundColor = childNodes[idx]->getText();
         theBackgroundColor = theBackgroundColor.trim();
      }
      else if(tag == "textColor")
      {
         theTextColor = childNodes[idx]->getText();
         theTextColor = theTextColor.trim();
      }
      else if(tag == "text")
      {
         theText = childNodes[idx]->getText();
      }
   }

   return true;
}

void ossimPlanetKmlBalloonStyle::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlObject::write(xmlNode);
   
   xmlNode->setTag("BalloonStyle");
   xmlNode->addChildNode("bgColor", theBackgroundColor);
   xmlNode->addChildNode("textColor", theTextColor);
   xmlNode->addChildNode("text", theText);
}

bool ossimPlanetKmlLod::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }
   if(!xmlNode.valid())
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();

   ossim_uint32 idx;
   ossim_uint32 upper=childNodes.size();

   for(idx = 0; idx < upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();

      if(tag == "minLodPixels")
      {
         theMinLodPixels = childNodes[idx]->getText().toInt32();
      }
      else if(tag == "maxLodPixels")
      {
         theMaxLodPixels = childNodes[idx]->getText().toInt32();
      }
      else if(tag == "minFadeExtent")
      {
         theMinFadeExtent = childNodes[idx]->getText().toInt32();
      }
      else if(tag == "maxFadeExtent")
      {
         theMaxFadeExtent = childNodes[idx]->getText().toInt32();
      }
   }

   return true;
}

void ossimPlanetKmlLod::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   xmlNode->setTag("Lod");
   
   xmlNode->addChildNode("minLodPixels", ossimString::toString(theMinLodPixels));
   xmlNode->addChildNode("maxLodPixels", ossimString::toString(theMaxLodPixels));
   xmlNode->addChildNode("minFadeExtent", ossimString::toString(theMinFadeExtent));
   xmlNode->addChildNode("maxFadeExtent", ossimString::toString(theMaxFadeExtent));
   
   ossimPlanetKmlObject::write(xmlNode);   
}

bool ossimPlanetKmlScale::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   
   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }

   if(!xmlNode.valid())
   {
      return false;
   }

   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 upper = childNodes.size();

   for(idx = 0; idx< upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();
      if(tag == "x")
      {
         theX = childNodes[idx]->getText().toDouble();
      }
      else if(tag == "y")
      {
         theY = childNodes[idx]->getText().toDouble();
      }
      else if(tag == "z")
      {
          theZ = childNodes[idx]->getText().toDouble();
      }
   }

   return true;
}

void ossimPlanetKmlScale::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   xmlNode->setTag("Scale");
   
   xmlNode->addChildNode("x", ossimString::toString(theX));
   xmlNode->addChildNode("y", ossimString::toString(theY));
   xmlNode->addChildNode("z", ossimString::toString(theZ));

   ossimPlanetKmlObject::write(xmlNode);
}

bool ossimPlanetKmlLocation::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();

   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }

   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 upper = childNodes.size();

   for(idx = 0; idx< upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();
      if(tag == "longitude")
      {
         theLongitude = childNodes[idx]->getText().toDouble();
      }
      else if(tag == "latitude")
      {
         theLatitude = childNodes[idx]->getText().toDouble();
      }
      else if(tag == "altitude")
      {
         theAltitude = childNodes[idx]->getText().toDouble();
      }
   }

   return true;
}

void ossimPlanetKmlLocation::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   xmlNode->setTag("Location");

   xmlNode->addChildNode("longitude",ossimString::toString(theLongitude));
   xmlNode->addChildNode("latitude", ossimString::toString(theLatitude));
   xmlNode->addChildNode("altitude", ossimString::toString(theAltitude));
   
   ossimPlanetKmlObject::write(xmlNode);
}

bool ossimPlanetKmlOrientation::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();

   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }


   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 upper = childNodes.size();

   for(idx = 0; idx< upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();
      if(tag == "heading")
      {
         theHeading = childNodes[idx]->getText().toDouble();
      }
      else if(tag == "tilt")
      {
         thePitch = childNodes[idx]->getText().toDouble();
      }
      else if(tag == "roll")
      {
          theRoll = childNodes[idx]->getText().toDouble();
      }
   }

   return true;
}

void ossimPlanetKmlOrientation::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   xmlNode->setTag("Orientation");
   xmlNode->addChildNode("heading", ossimString::toString(theHeading));
   xmlNode->addChildNode("tilt", ossimString::toString(thePitch));
   xmlNode->addChildNode("roll", ossimString::toString(theRoll));
}

bool ossimPlanetKmlLatLonBox::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }
   ossimString tempText;
   if(!xmlNode->getChildTextValue(tempText, "north"))
   {
      return false;
   }
   theNorth = tempText.toDouble();
   if(!xmlNode->getChildTextValue(tempText, "south"))
   {
      return false;
   }
   theSouth = tempText.toDouble();
   if(!xmlNode->getChildTextValue(tempText, "east"))
   {
      return false;
   }
   theEast = tempText.toDouble();
   if(!xmlNode->getChildTextValue(tempText, "west"))
   {
      return false;
   }
   theWest = tempText.toDouble();
   
   if(xmlNode->getChildTextValue(tempText, "rotation"))
   {
      theRotation = tempText.toDouble();
   }
   return true;
}
void ossimPlanetKmlLatLonBox::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   xmlNode->setTag("LatLonBox");
   xmlNode->addChildNode("north", ossimString::toString(theNorth));
   xmlNode->addChildNode("south", ossimString::toString(theSouth));
   xmlNode->addChildNode("east",  ossimString::toString(theEast));
   xmlNode->addChildNode("west",  ossimString::toString(theWest));
   xmlNode->addChildNode("rotation", ossimString::toString(theRotation));
}

bool ossimPlanetKmlLatLonAltBox::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlLatLonBox::parse(xmlNode))
   {
      return false;
   }
   ossimString tempText;
   if(xmlNode->getChildTextValue(tempText, "minAltitude"))
   {
      theMinAltitude = tempText.toDouble();
   }
   if(xmlNode->getChildTextValue(tempText, "maxAltitude"))
   {
      theMaxAltitude = tempText.toDouble();
   }
   ossimString altMode;
   xmlNode->getChildTextValue(altMode, "altitudeMode");
   theAltitudeMode = ossimPlanetKmlConvertAltitudeMode(altMode);
   
   return true;
}

void ossimPlanetKmlLatLonAltBox::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlLatLonBox::write(xmlNode);
   xmlNode->setTag("LatLonAltBox");
   xmlNode->addChildNode("minAltitude", ossimString::toString(theMinAltitude));
   xmlNode->addChildNode("maxAltitude", ossimString::toString(theMaxAltitude));
   xmlNode->addChildNode("altitudeMode", ossimPlanetKmlConvertAltitudeMode(theAltitudeMode));
}

bool ossimPlanetKmlColorStyle::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();

   ossim_uint32 idx;
   ossim_uint32 upper = childNodes.size();;
   ossim_uint32 count = 0;
   for(idx = 0; ((idx < upper)&&(count < 2)); ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();
      if(tag == "color")
      {
         theColor = childNodes[idx]->getText();
         ++count;
      }
      else if(tag == "colorMode")
      {
         theColorMode = ossimPlanetKmlConvertColorMode(childNodes[idx]->getText());
         ++count;
      }
   }

   return true;
}

void ossimPlanetKmlColorStyle::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlObject::write(xmlNode);
   xmlNode->setTag("ColorStyle");
   xmlNode->addChildNode("color", theColor);
   xmlNode->addChildNode("colorMode", ossimPlanetKmlConvertColorMode(theColorMode));
}

bool ossimPlanetKmlLineStyle::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   if(!ossimPlanetKmlColorStyle::parse(xmlNode))
   {
      return false;
   }
   ossimString tempString;

   if(xmlNode->getChildTextValue(tempString, "width"))
   {
      theWidth = tempString.toFloat32();
   }

   return true;
}

void ossimPlanetKmlLineStyle::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlColorStyle::write(xmlNode);
   xmlNode->setTag("LineStyle");
   xmlNode->addChildNode("width", ossimString::toString(theWidth));
}

bool ossimPlanetKmlPolyStyle::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   if(!ossimPlanetKmlColorStyle::parse(xmlNode))
   {
      return false;
   }

   ossimString tempString;

   if(xmlNode->getChildTextValue(tempString, "fill"))
   {
      theFillFlag = tempString.toBool();
   }

   if(xmlNode->getChildTextValue(tempString, "outline"))
   {
      theOutlineFlag = tempString.toBool();
   }

   return true;
}

void ossimPlanetKmlPolyStyle::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlColorStyle::write(xmlNode);
   xmlNode->setTag("PolyStyle");
   xmlNode->addChildNode("fill", theFillFlag?"1":"0");   
   xmlNode->addChildNode("outline", theOutlineFlag?"1":"0");   
}

bool ossimPlanetKmlIconStyle::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlColorStyle::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 upper = childNodes.size();
   ossim_uint32 count = 0;
   for(idx = 0; ((idx< upper)&&(count < 4)); ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();

      if(tag == "scale")
      {
         theScale = childNodes[idx]->getText().toDouble();
         ++count;
      }
      else if(tag == "heading")
      {
         theHeading = childNodes[idx]->getText().toDouble();
         ++count;
      }
      else if(tag == "Icon")
      {
         theIcon = new ossimPlanetKmlIcon;
         theIcon->setParent(this);
         if(!theIcon->parse(childNodes[idx]))
         {
            theIcon = 0;
            return false;
         }
         ++count;
      }
      else if(tag == "hotSpot")
      {
         ossimString tempString;
         if(childNodes[idx]->getAttributeValue(tempString, "x"))
         {
            theXHotspot = tempString.toFloat32();
         }
         if(childNodes[idx]->getAttributeValue(tempString, "y"))
         {
            theYHotspot = tempString.toFloat32();
         }
         if(childNodes[idx]->getAttributeValue(tempString, "xunits"))
         {
            theXUnits   = ossimPlanetKmlConvertUnits(tempString);
         }
         if(childNodes[idx]->getAttributeValue(tempString, "yunits"))
         {
            theYUnits   = ossimPlanetKmlConvertUnits(tempString);
         }
         
         ++count;
      }
   }

   return true;
}

void ossimPlanetKmlIconStyle::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlColorStyle::write(xmlNode);
   xmlNode->setTag("IconStyle");
   xmlNode->addChildNode("scale", ossimString::toString(theScale));
   xmlNode->addChildNode("heading", ossimString::toString(theHeading));
   if(theIcon.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theIcon->write(node);
      xmlNode->addChildNode(node.get());
   }
   ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
   node->setTag("hotSpot");
   node->addAttribute("x", ossimString::toString(theXHotspot));
   node->addAttribute("y", ossimString::toString(theYHotspot));
   node->addAttribute("xunits", ossimPlanetKmlConvertUnits(theXUnits));
   node->addAttribute("yunits", ossimPlanetKmlConvertUnits(theYUnits));
}

bool ossimPlanetKmlLabelStyle::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();

   if(!ossimPlanetKmlColorStyle::parse(xmlNode))
   {
      return false;
   }
   ossimString tempText;
   if(xmlNode->getChildTextValue(tempText, "scale"))
   {
      theScale = tempText.toDouble();
   }

   return true;
}

void ossimPlanetKmlLabelStyle::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlColorStyle::write(xmlNode);
   xmlNode->setTag("LabelStyle");
   xmlNode->addChildNode("scale", ossimString::toString(theScale));
}

bool ossimPlanetKmlStyle::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlStyleSelector::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 upper = childNodes.size();
   for(idx = 0; idx < upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();
      if(tag == "LineStyle")
      {
         theLineStyle = new ossimPlanetKmlLineStyle;
         theLineStyle->setParent(this);
         if(!theLineStyle->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(tag == "IconStyle")
      {
         theIconStyle = new ossimPlanetKmlIconStyle;
         theIconStyle->setParent(this);
         if(!theIconStyle->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(tag == "LabelStyle")
      {
         theLabelStyle = new ossimPlanetKmlLabelStyle;
         theLabelStyle->setParent(this);
         if(!theLabelStyle->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(tag == "PolyStyle")
      {
         thePolyStyle = new ossimPlanetKmlPolyStyle;
         thePolyStyle->setParent(this);
         if(!thePolyStyle->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(tag == "BalloonStyle")
      {
         theBalloonStyle = new ossimPlanetKmlBalloonStyle;
         theBalloonStyle->setParent(this);
         if(!theBalloonStyle->parse(childNodes[idx]))
         {
            return false;
         }
      }
   }

   return true;
}

void ossimPlanetKmlStyle::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlStyleSelector::write(xmlNode);
   xmlNode->setTag("Style");
   if(theIconStyle.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theIconStyle->write(node);
      xmlNode->addChildNode(node.get());
   }
   if(theLabelStyle.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theLabelStyle->write(node);
      xmlNode->addChildNode(node.get());
   }
   if(theLineStyle.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theLineStyle->write(node);
      xmlNode->addChildNode(node.get());
   }
   if(thePolyStyle.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      thePolyStyle->write(node);
      xmlNode->addChildNode(node.get());
   }
   if(theBalloonStyle.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theBalloonStyle->write(node);
      xmlNode->addChildNode(node.get());
   }
}

bool ossimPlanetKmlStyleMap::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlStyleSelector::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 upper = childNodes.size();
   for(idx = 0; idx < upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();

      if(tag == "Pair")
      {
         ossimString key;
         ossimString value;
         if( childNodes[idx]->getChildTextValue(key, "key")&&
             childNodes[idx]->getChildTextValue(value, "styleUrl"))
         {
//             if(key == "normal")
//             {
//                theNormalUrl = value;
//             }
//             else if(key == "highlight")
//             {
//                theHighlightUrl = value;
//             }
            theStyleMap.insert(std::make_pair(key.c_str(), value.c_str()));
         }
      }
   }

   return true;
}

void ossimPlanetKmlStyleMap::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlStyleSelector::write(xmlNode);
   xmlNode->setTag("StyleMap");
   MapType::const_iterator i = theStyleMap.begin();
   while(i != theStyleMap.end())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      
      node->setTag("Pair");
      node->addChildNode(ossimString(i->first), ossimString(i->second));
      xmlNode->addChildNode(node.get());
      
      ++i;
   }
}

bool ossimPlanetKmlLink::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }
   ossimString tempText;

   xmlNode->getChildTextValue(theHref, "href");
   xmlNode->getChildTextValue(tempText, "refreshMode");
   theRefreshMode = ossimPlanetKmlConvertRefreshMode(tempText);
   
   xmlNode->getChildTextValue(tempText, "viewRefreshMode");
   theViewRefreshMode = ossimPlanetKmlConvertViewRefreshMode(tempText);
   xmlNode->getChildTextValue(theViewFormat, "viewFormat");
   xmlNode->getChildTextValue(theHttpQuery, "httpQuery");
   if(xmlNode->getChildTextValue(tempText, "refreshInterval"))
   {
      theRefreshInterval = tempText.toDouble();
   }
   if(xmlNode->getChildTextValue(tempText, "viewRefreshTime"))
   {
      theViewRefreshTime = tempText.toDouble();
   }
   if(xmlNode->getChildTextValue(tempText, "viewBoundScale"))
   {
      theViewBoundScale = tempText.toDouble();
   }

   return true;
}

void ossimPlanetKmlLink::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlObject::write(xmlNode);
   xmlNode->setTag("Link");
   xmlNode->addChildNode("href", theHref);
   xmlNode->addChildNode("refreshMode", ossimPlanetKmlConvertRefreshMode(theRefreshMode));
   xmlNode->addChildNode("viewRefreshMode", ossimPlanetKmlConvertViewRefreshMode(theViewRefreshMode));
   xmlNode->addChildNode("viewFormat", theViewFormat);
   xmlNode->addChildNode("httpQuery", theHttpQuery);
   xmlNode->addChildNode("refreshInterval", ossimString::toString(theRefreshInterval));
   xmlNode->addChildNode("viewRefreshTime", ossimString::toString(theViewRefreshTime));
   xmlNode->addChildNode("viewBoundScale", ossimString::toString(theViewBoundScale));
                         
}

ossimFilename ossimPlanetKmlLink::download(bool forceOverwrite,
                                           const ossimFilename& locationOverride)const
{
   ossimFilename fileToLoad = theHref;
   
   if(ossimFilename(fileToLoad).downcase().contains("http"))
   {
      if(locationOverride == "")
      {
         fileToLoad = getCacheLocation(true); // will use the root kml to find the cache location
         if(fileToLoad == "")
         {
            return "";
         }
//          ossimFilename temp(ossimString(wmsUrl(theHref).path()).after("/"));
         ossimFilename temp(wmsUrl(theHref.string()).server());
         temp = ossimFilename(temp.substitute(".", "_", true));
         temp = temp.dirCat(ossimFilename(wmsUrl(theHref.string()).path()));
         fileToLoad = fileToLoad.dirCat(temp);
         ossimFilename path = fileToLoad.path();
         path.createDirectory();
      }
      else
      {
         fileToLoad = locationOverride;
      }
      
      if((!fileToLoad.exists())||
         (forceOverwrite))
      {
         ossimFilename tempFile = ossimFilename(fileToLoad + ".tmp");
         wmsCurlMemoryStream netDownload(theHref);
         
         if(!netDownload.download(tempFile))
         {
            return "";
         }
         tempFile.rename(fileToLoad, true);
         
      }
   }
   else if(!fileToLoad.exists())// check absolute and relative
   {
      ossimFilename file = getKmlFile();
      if(!file.empty())
      {
         ossimFilename testFile = file.path().dirCat(fileToLoad);
         if(testFile.exists())
         {
            fileToLoad = testFile;
         }
      }
   }
   // really need to check other protocols.  Maybe need to check for file:// and convert to
   // local file 
   //

   return fileToLoad;
}

bool ossimPlanetKmlIcon::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   return ossimPlanetKmlLink::parse(xmlNode);
}

void ossimPlanetKmlIcon::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlLink::write(xmlNode);
   xmlNode->setTag("Icon");
}

bool ossimPlanetKmlRegion::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!xmlNode.valid()) return false;
   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 upper = childNodes.size();
   for(idx = 0; idx < upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();

      if(tag == "Lod")
      {
         theLod = new ossimPlanetKmlLod;
         theLod->setParent(this);
         if(!theLod->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(tag == "LatLonAltBox")
      {
         theBox = new ossimPlanetKmlLatLonAltBox;
         theBox->setParent(this);
         if(!theBox->parse(childNodes[idx]))
         {
            return false;
         }
      }
   }

   return theBox.valid();
}


void ossimPlanetKmlRegion::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlObject::write(xmlNode);
   
   xmlNode->setTag("Region");
   if(theLod.valid())
   {
      ossimRefPtr<ossimXmlNode> lodNode = new ossimXmlNode;
      theLod->write(lodNode.get());
      xmlNode->addChildNode(lodNode.get());
   }
   if(theBox.valid())
   {
      ossimRefPtr<ossimXmlNode> boxNode = new ossimXmlNode;
      theBox->write(boxNode.get());
      xmlNode->addChildNode(boxNode.get());
   }
}

bool ossimPlanetKmlLookAt::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }
   ossimString tempText;
   if(!xmlNode->getChildTextValue(tempText, "longitude"))
   {
      return false;
   }
   theLookAt->setLon(tempText.toDouble());
   if(!xmlNode->getChildTextValue(tempText, "latitude"))
   {
      return false;
   }
   theLookAt->setLat(tempText.toDouble());
   if(!xmlNode->getChildTextValue(tempText, "range"))
   {
      return false;
   }
   theLookAt->setRange(tempText.toDouble());

   if(xmlNode->getChildTextValue(tempText, "altitude"))
   {
      theLookAt->setAltitude(tempText.toDouble());
   }

   if(xmlNode->getChildTextValue(tempText, "tilt"))
   {
      theLookAt->setPitch(tempText.toDouble());
      theLookAt->setPitch(mkUtils::clamp(theLookAt->pitch(), (double)0.0, (double)90.0));
   }

   if(xmlNode->getChildTextValue(tempText, "heading"))
   {
      theLookAt->setHeading(tempText.toDouble());
   }
   if(xmlNode->getChildTextValue(tempText, "altitudeMode"))
   {
      theLookAt->setAltitudeMode(ossimPlanetLookAt::modeFromString(tempText));
   }
   
   return true;
}

void ossimPlanetKmlLookAt::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlObject::write(xmlNode);
   xmlNode->setTag("LookAt");

   xmlNode->addChildNode("longitude", ossimString::toString(theLookAt->lon()));
   xmlNode->addChildNode("latitude", ossimString::toString(theLookAt->lat()));
   xmlNode->addChildNode("range", ossimString::toString(theLookAt->range()));
   xmlNode->addChildNode("altitude", ossimString::toString(theLookAt->altitude()));
   xmlNode->addChildNode("tilt", ossimString::toString(theLookAt->pitch()));
   xmlNode->addChildNode("heading", ossimString::toString(theLookAt->heading()));
   xmlNode->addChildNode("altitudeMode", ossimPlanetLookAt::modeToString(theLookAt->altitudeMode()));
}

bool ossimPlanetKmlTimeSpan::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }
   
   xmlNode->getChildTextValue(theBegin, "begin");
   xmlNode->getChildTextValue(theEnd, "end");
  
   return true;
}

void ossimPlanetKmlTimeSpan::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlTimePrimitive::write(xmlNode);
   xmlNode->setTag("TimeSpan");

   if(!theBegin.empty())
   {
      xmlNode->addChildNode("begin", theBegin);
   }
   if(!theEnd.empty())
   {
      xmlNode->addChildNode("end", theEnd);      
   }
}

bool ossimPlanetKmlTimeStamp::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();

   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }
   
   xmlNode->getChildTextValue(theWhen, "when");

   return true;
}

void ossimPlanetKmlTimeStamp::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   xmlNode->setTag("TimeStamp");
   ossimPlanetKmlTimePrimitive::write(xmlNode);
   xmlNode->addChildNode("when", theWhen);
}

bool ossimPlanetKmlFeature::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!xmlNode.valid())
   {
      return false;
   }
   if(!xmlNode.valid()) return false;
   if(!ossimPlanetKmlObject::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 upper = childNodes.size();
   for(idx = 0; idx < upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();
      ossimString tempString;

      if(tag == "name")
      {
         theName = childNodes[idx]->getText();
      }
      else if(tag == "visibility")
      {
         theVisibilityFlag = childNodes[idx]->getText().toBool();
      }
      else if(tag == "open")
      {
         theOpenFlag = childNodes[idx]->getText().toBool();
      }
      else if(tag == "address")
      {
         theAddress = childNodes[idx]->getText().toBool();
      }
      else if(tag == "phoneNumber")
      {
         theAddress = childNodes[idx]->getText().toBool();
      }
      else if(tag == "Region")
      {
         theRegion = new ossimPlanetKmlRegion();
         theRegion->setParent(this);
         if(!theRegion->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(tag == "description")
      {
         theDescription = childNodes[idx]->getText();
         theCDataDescriptionFlag = childNodes[idx]->cdataFlag();
         if(!theCDataDescriptionFlag)
         {
            theDescription = ossim::convertHtmlSpecialCharactersToNormalCharacter(theDescription);
         }
      }
      else if(tag == "LookAt")
      {
         theLookAt = new ossimPlanetKmlLookAt();
         theLookAt->setParent(this);
         if(!theLookAt->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(tag == "styleUrl")
      {
         theStyleUrl = childNodes[idx]->getText();
      }
      else if(tag == "Snippet")
      {
         theSnippet = childNodes[idx]->getText();
         childNodes[idx]->getAttributeValue(theSnippetMaxLines, "maxLines");
      }
      else if(tag == "Metadata")
      {
         theExtendedData = childNodes[idx];
      }
      else if(tag == "ExtendedData")
      {
         theExtendedData = childNodes[idx];
      }
      else if(tag == "Style")
      {
         theStyleSelector = new ossimPlanetKmlStyle;
         theStyleSelector->setParent(this);
         if(!theStyleSelector->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(tag == "StyleMap")
      {
         theStyleSelector = new ossimPlanetKmlStyleMap;
         theStyleSelector->setParent(this);
         if(!theStyleSelector->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else
      {
         ossimPlanetKmlObject* timePrimitive = ossimPlanetKmlObjectRegistry::instance()->newTimePrimitive(tag);
         if(timePrimitive)
         {
            theTimePrimitive = (ossimPlanetKmlTimePrimitive*)timePrimitive;
            theTimePrimitive->setParent(this);
         }
      }
   }
   return true;
}

void ossimPlanetKmlFeature::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlObject::write(xmlNode);

   xmlNode->addChildNode("name", name());
   xmlNode->addChildNode("visibility", visibilityFlag()?"1":"0");
   xmlNode->addChildNode("open", openFlag()?"1":"0");
   xmlNode->addChildNode("address", address());
   xmlNode->addChildNode("phoneNumber", phoneNumber());
   if(theCDataDescriptionFlag)
   {
      ossimRefPtr<ossimXmlNode> xmlTempNode = new ossimXmlNode;
      xmlNode->setTag("description");
      xmlNode->setCDataFlag(true);
      xmlNode->setText(description());
   }
   else
   {
      xmlNode->addChildNode("description", description());
   }
   xmlNode->addChildNode("styleUrl", styleUrl());
   if(theRegion.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theRegion->write(node.get());
      xmlNode->addChildNode(node.get());
   }
   if(theLookAt.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theLookAt->write(node.get());
      xmlNode->addChildNode(node.get());
   }
   if(!theSnippet.empty())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      node->setTag("Snippet");
      if(!theSnippetMaxLines.empty())
      {
         node->addAttribute("maxLines", theSnippetMaxLines);
      }
      xmlNode->addChildNode(node.get());
      
   }
   if(theExtendedData.valid())
   {
      xmlNode->addChildNode((ossimXmlNode*)theExtendedData->dup());
   }
   if(theStyleSelector.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theStyleSelector->write(node.get());
      xmlNode->addChildNode(node.get());      
   }
   if(theTimePrimitive.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theTimePrimitive->write(node.get());
      xmlNode->addChildNode(node.get());      
   }
}

bool ossimPlanetKmlGeometry::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   return ossimPlanetKmlObject::parse(xmlNode);
}

void ossimPlanetKmlGeometry::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlObject::write(xmlNode);
}

void ossimPlanetKmlGeometry::computeCenter(osg::Vec3d& result,
                                           const PointListType& pointList)
{
   ossim_uint32 idx;
   double minLat=999999.0, minLon=999999.0;
   double maxLat=-999999.0, maxLon=-999999.0;
   double minAlt=99999999.0, maxAlt=-99999999.0;

   if(pointList.size())
   {
      for(idx = 0; idx < pointList.size(); ++idx)
      {
         osg::Vec3d pt = pointList[idx];
        if(pt[0]<minLon) minLon = pt[0];
         if(pt[0]>maxLon) maxLon = pt[0];
         if(pt[1]<minLat) minLat = pt[1];
         if(pt[1]>maxLat) maxLat = pt[1];
         if(pt[2]<minAlt) minAlt = pt[2];
         if(pt[2]>maxAlt) maxAlt = pt[2];
      }
      result[0] = (maxLon+minLon)*.5;
      result[1] = (maxLat+minLat)*.5;
      result[2] = (maxAlt+minAlt)*.5;
      if(maxLon-minLon > 180.0) // we will guess it crossed the -180 mark.
      {
         double len1, len2, totalLen;
         len1 = 180-maxLon;
         len2 = 180+minLon;
         totalLen = len1+len2;
         result[0] += maxLon + totalLen*.5;
         if(result[0] > 180) result[0] -= 360.0;
      }
   }
}

bool ossimPlanetKmlPoint::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!xmlNode.valid())
   {
      return false;
   }
   if(!ossimPlanetKmlGeometry::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   bool hasCoordinates = true;
   for(idx = 0; idx < childNodes.size(); ++idx)
   {
      if(childNodes[idx]->getTag() == "extrude")
      {
         theExtrudeFlag = childNodes[idx]->getText().toBool();
      }
      else if(childNodes[idx]->getTag() == "tessellate")
      {
         theTesselateFlag = childNodes[idx]->getText().toBool();
      }
      else if(childNodes[idx]->getTag() == "altitudeMode")
      {
         theAltitudeMode = ossimPlanetKmlConvertAltitudeMode(childNodes[idx]->getText());
      }
      else if(childNodes[idx]->getTag() == "coordinates")
      {
         kmlReadCoordinates(thePointList, childNodes[idx]->getText());
         computeCenter(theCenter, thePointList);
      }
   }
   return hasCoordinates;
}

void ossimPlanetKmlPoint::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlGeometry::write(xmlNode);
   xmlNode->setTag("Point");

   xmlNode->addChildNode("extrude", theExtrudeFlag?"1":"0");
   xmlNode->addChildNode("tessellate", theTesselateFlag?"1":"0");
   xmlNode->addChildNode("altitudeMode", ossimPlanetKmlConvertAltitudeMode(theAltitudeMode));

   if(thePointList.size() > 0)
   {
      ossimString value = ossimString::toString(thePointList[0][0]) + "," +
                          ossimString::toString(thePointList[0][1]) + "," +
                          ossimString::toString(thePointList[0][2]);
         
      xmlNode->addChildNode("coordinates", value);
   }
}

bool ossimPlanetKmlLineString::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!xmlNode.valid())
   {
      return false;
   }
   if(!ossimPlanetKmlGeometry::parse(xmlNode))
   {
      return false;
   }
   ossimString tempString;

   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   bool hasCoordinates = false;
   for(idx = 0; idx < childNodes.size(); ++idx)
   {
      if(childNodes[idx]->getTag() == "extrude")
      {
         theExtrudeFlag = childNodes[idx]->getText().toBool();
      }
      else if(childNodes[idx]->getTag() == "tessellate")
      {
         theTesselateFlag = childNodes[idx]->getText().toBool();
      }
      else if(childNodes[idx]->getTag() == "altitudeMode")
      {
         theAltitudeMode = ossimPlanetKmlConvertAltitudeMode(childNodes[idx]->getText());
      }
      else if(childNodes[idx]->getTag() == "coordinates")
      {
         hasCoordinates = true;
         kmlReadCoordinates(thePointList, childNodes[idx]->getText());
         computeCenter(theCenter, thePointList);
      }
   }
   return hasCoordinates;   
}

void ossimPlanetKmlLineString::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlGeometry::write(xmlNode);
   xmlNode->setTag("LineString");
   xmlNode->addChildNode("extrude", theExtrudeFlag?"1":"0");
   xmlNode->addChildNode("tessellate", theTesselateFlag?"1":"0");
   xmlNode->addChildNode("altitudeMode", ossimPlanetKmlConvertAltitudeMode(theAltitudeMode));
   ossimString value;
   if(thePointList.size() > 0)
   {
      ossim_uint32 idx = 0;
      for(idx = 0; idx < thePointList.size(); ++idx)
      {
         value += (ossimString::toString(thePointList[idx][0])+","+
                   ossimString::toString(thePointList[idx][1])+","+
                   ossimString::toString(thePointList[idx][2])+" ");
      }
   }
   xmlNode->addChildNode("coordinates", value);
}

bool ossimPlanetKmlLinearRing::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!xmlNode.valid())
   {
      return false;
   }
   if(!ossimPlanetKmlGeometry::parse(xmlNode))
   {
      return false;
   }
   ossimString tempString;

   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   bool hasCoordinates = false;
   for(idx = 0; idx < childNodes.size(); ++idx)
   {
      if(childNodes[idx]->getTag() == "extrude")
      {
         theExtrudeFlag = childNodes[idx]->getText().toBool();
      }
      else if(childNodes[idx]->getTag() == "tessellate")
      {
         theTesselateFlag = childNodes[idx]->getText().toBool();
      }
      else if(childNodes[idx]->getTag() == "altitudeMode")
      {
         theAltitudeMode = ossimPlanetKmlConvertAltitudeMode(childNodes[idx]->getText());
      }
      else if(childNodes[idx]->getTag() == "coordinates")
      {
         hasCoordinates = true;
         kmlReadCoordinates(thePointList, childNodes[idx]->getText());
         computeCenter(theCenter, thePointList);
      }
   }
   return hasCoordinates;   
}

void ossimPlanetKmlLinearRing::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlGeometry::write(xmlNode);
   xmlNode->setTag("LinearRing");
   xmlNode->addChildNode("extrude", theExtrudeFlag?"1":"0");
   xmlNode->addChildNode("tessellate", theTesselateFlag?"1":"0");
   xmlNode->addChildNode("altitudeMode", ossimPlanetKmlConvertAltitudeMode(theAltitudeMode));
   ossimString value;
   if(thePointList.size() > 0)
   {
      ossim_uint32 idx = 0;
      for(idx = 0; idx < thePointList.size()-1; ++idx)
      {
         value += (ossimString::toString(thePointList[idx][0])+","+
                   ossimString::toString(thePointList[idx][1])+","+
                   ossimString::toString(thePointList[idx][2])+" ");
      }
   }
   xmlNode->addChildNode("coordinates", value);
}

bool ossimPlanetKmlPolygon::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!xmlNode.valid())
   {
      return false;
   }
   if(!ossimPlanetKmlGeometry::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 bound = childNodes.size();

   for(idx = 0; idx < bound; ++idx)
   {
      if(childNodes[idx]->getTag() == "extrude")
      {
         theExtrudeFlag = childNodes[idx]->getText().toBool();
      }
      else if(childNodes[idx]->getTag() == "tessellate")
      {
         theTesselateFlag = childNodes[idx]->getText().toBool();
      }
      else if(childNodes[idx]->getTag() == "altitudeMode")
      {
         theAltitudeMode = ossimPlanetKmlConvertAltitudeMode(childNodes[idx]->getText());
      }
      else if(childNodes[idx]->getTag() == "outerBoundaryIs")
      {
         theOuterBoundary = new ossimPlanetKmlLinearRing;
         theOuterBoundary->parse(childNodes[idx]->findFirstNode("LinearRing"));
         theOuterBoundary->setParent(this);
      }
      else if(childNodes[idx]->getTag() == "innerBoundaryIs")
      {
         ossimPlanetKmlLinearRing* linearRing = new ossimPlanetKmlLinearRing;
         if(linearRing->parse(childNodes[idx]->findFirstNode("LinearRing")))
         {
            linearRing->setParent(this);
            theInnerBoundaryList.push_back(linearRing);
         }
      }
   }
   return theOuterBoundary.valid();   
}

void ossimPlanetKmlPolygon::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlGeometry::write(xmlNode);

   xmlNode->setTag("Polygon");

   xmlNode->addChildNode("extrude", theExtrudeFlag?"1":"0");
   xmlNode->addChildNode("tessellate", theTesselateFlag?"1":"0");
   xmlNode->addChildNode("altitudeMode", ossimPlanetKmlConvertAltitudeMode(theAltitudeMode));
   if(theOuterBoundary.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      ossimRefPtr<ossimXmlNode> linearRingNode = new ossimXmlNode;
      theOuterBoundary->write(linearRingNode);
      node->addChildNode(linearRingNode.get());
      node->setTag("outerBoundaryIs");
      xmlNode->addChildNode(node.get());
   }
   if(!theInnerBoundaryList.empty())
   {
      ossim_uint32 idx = 0;
      for(idx = 0; idx < theInnerBoundaryList.size(); ++idx)
      {
         ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
         ossimRefPtr<ossimXmlNode> linearRingNode = new ossimXmlNode;
         theInnerBoundaryList[idx]->write(linearRingNode);
         node->addChildNode(linearRingNode.get());
         node->setTag("innerBoundaryIs");
         xmlNode->addChildNode(node.get());
      }
   }
}

bool ossimPlanetKmlMultiGeometry::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!xmlNode.valid())
   {
      return false;
   }
   if(!ossimPlanetKmlGeometry::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 bound = childNodes.size();

   for(idx = 0; idx < bound; ++idx)
   {
      osg::ref_ptr<ossimPlanetKmlGeometry> geom = (ossimPlanetKmlGeometry*)ossimPlanetKmlObjectRegistry::instance()->newGeometry(childNodes[idx]->getTag());
      if(geom.valid())
      {
         if(geom->parse(childNodes[idx]))
         {
            theGeometryList.push_back(geom.get());
            geom->setParent(this);
         }
         else
         {
            return false;
         }
      }
   }
   return true;   
   
}

void ossimPlanetKmlMultiGeometry::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlGeometry::write(xmlNode);

   xmlNode->setTag("MultiGeometry");
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theGeometryList.size(); ++idx)
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theGeometryList[idx]->write(node.get());
      xmlNode->addChildNode(node.get());
   }
}

bool ossimPlanetKmlModel::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!xmlNode.valid())
   {
      return false;
   }
   if(!ossimPlanetKmlGeometry::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 bound = childNodes.size();
   for(idx = 0; idx < bound; ++idx)
   {
      if(childNodes[idx]->getTag() == "Location")
      {         
         if(!theLocation->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(childNodes[idx]->getTag() == "Orientation")
      {
         if(!theOrientation->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(childNodes[idx]->getTag() == "Scale")
      {
         if(!theScale->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(childNodes[idx]->getTag() == "Link")
      {
         theLink = new ossimPlanetKmlLink;
         theLink->setParent(this);
         if(!theLink->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(childNodes[idx]->getTag() == "altitudeMode")
      {
         theAltitudeMode = ossimPlanetKmlConvertAltitudeMode(childNodes[idx]->getText());
      }
   }
   
   return true;   
  
}

void ossimPlanetKmlModel::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlGeometry::write(xmlNode);
   xmlNode->setTag("Model");
   ossimRefPtr<ossimXmlNode> node;
   if(theLocation.valid())
   {
      node = new ossimXmlNode;
      theLocation->write(node.get());
      xmlNode->addChildNode(node.get());
   }
   if(theOrientation.valid())
   {
      node = new ossimXmlNode;
      theOrientation->write(node.get());
      xmlNode->addChildNode(node.get());
   }
   if(theScale.valid())
   {
      node = new ossimXmlNode;
      theScale->write(node);
      xmlNode->addChildNode(node.get());
   }
   if(theLink.valid())
   {
      node = new ossimXmlNode;
      theLink->write(node);
      xmlNode->addChildNode(node.get());
   }
}

bool ossimPlanetKmlOverlay::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   ossimString tempString;
   clearFields();

   xmlNode->getChildTextValue(theColor, "color");
   if(xmlNode->getChildTextValue(tempString, "drawOrder"))
   {
      theDrawOrder = tempString.toInt32();
   }
   const ossimRefPtr<ossimXmlNode> iconNode = xmlNode->findFirstNode("Icon");
   if(iconNode.valid())
   {
      theIcon = new ossimPlanetKmlIcon();
      theIcon->setParent(this);
      if(!theIcon->parse(iconNode))
      {
         return false;
      }
   }
   return ossimPlanetKmlFeature::parse(xmlNode);
}

void ossimPlanetKmlOverlay::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlFeature::write(xmlNode);
   xmlNode->addChildNode("color", theColor);
   xmlNode->addChildNode("drawOrder", ossimString::toString(theDrawOrder));

   if(theIcon.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theIcon->write(node);
      xmlNode->addChildNode(node.get());
   }
}

bool ossimPlanetKmlScreenOverlay::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   if(!ossimPlanetKmlOverlay::parse(xmlNode))
   {
      return false;
   }
   ossimString tempString;
   ossimRefPtr<ossimXmlNode> tempNode = xmlNode->findFirstNode("overlayXY");
   if(tempNode.valid())
   {
      if(tempNode->getAttributeValue(tempString, "x"))
      {
         theOverlayX = tempString.toDouble();
      }
      if(tempNode->getAttributeValue(tempString, "y"))
      {
         theOverlayY = tempString.toDouble();
      }
      if(tempNode->getAttributeValue(tempString, "xunits"))
      {
         theOverlayXUnits = ossimPlanetKmlConvertUnits(tempString);
      }
      if(tempNode->getAttributeValue(tempString, "yunits"))
      {
         theOverlayYUnits = ossimPlanetKmlConvertUnits(tempString);
      }
   }
   tempNode = xmlNode->findFirstNode("screenXY");
   if(tempNode.valid())
   {
      if(tempNode->getAttributeValue(tempString, "x"))
      {
         theScreenX = tempString.toDouble();
      }
      if(tempNode->getAttributeValue(tempString, "y"))
      {
         theScreenY = tempString.toDouble();
      }
      if(tempNode->getAttributeValue(tempString, "xunits"))
      {
         theScreenXUnits = ossimPlanetKmlConvertUnits(tempString);
      }
      if(tempNode->getAttributeValue(tempString, "yunits"))
      {
         theScreenYUnits = ossimPlanetKmlConvertUnits(tempString);
      }
   }
   tempNode = xmlNode->findFirstNode("rotationXY");
   if(tempNode.valid())
   {
      if(tempNode->getAttributeValue(tempString, "x"))
      {
         theRotationX = tempString.toDouble();
      }
      if(tempNode->getAttributeValue(tempString, "y"))
      {
         theRotationY = tempString.toDouble();
      }
      if(tempNode->getAttributeValue(tempString, "xunits"))
      {
         theRotationXUnits = ossimPlanetKmlConvertUnits(tempString);
      }
      if(tempNode->getAttributeValue(tempString, "yunits"))
      {
         theRotationYUnits = ossimPlanetKmlConvertUnits(tempString);
      }
   }
   tempNode = xmlNode->findFirstNode("size");
   if(tempNode.valid())
   {
      if(tempNode->getAttributeValue(tempString, "x"))
      {
         theSizeX = tempString.toDouble();
      }
      if(tempNode->getAttributeValue(tempString, "y"))
      {
         theSizeY = tempString.toDouble();
      }
      if(tempNode->getAttributeValue(tempString, "xunits"))
      {
         theSizeXUnits = ossimPlanetKmlConvertUnits(tempString);
      }
      if(tempNode->getAttributeValue(tempString, "yunits"))
      {
         theSizeYUnits = ossimPlanetKmlConvertUnits(tempString);
      }
   }
   tempNode = xmlNode->findFirstNode("rotation");
   if(tempNode.valid())
   {
      theRotation = tempNode->getText().toDouble();
   }

   return true;
}

void ossimPlanetKmlScreenOverlay::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlOverlay::write(xmlNode);
   xmlNode->setTag("ScreenOverlay");
   ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
   node->setTag("overlayXY");
   node->addAttribute("x", ossimString::toString(theOverlayX));
   node->addAttribute("y", ossimString::toString(theOverlayY));
   node->addAttribute("xunits", ossimPlanetKmlConvertUnits(theOverlayXUnits));
   node->addAttribute("yunits", ossimPlanetKmlConvertUnits(theOverlayYUnits));   
   xmlNode->addChildNode(node.get());
   
   node = new ossimXmlNode;
   node->setTag("screenXY");
   node->addAttribute("x", ossimString::toString(theScreenX));
   node->addAttribute("y", ossimString::toString(theScreenY));
   node->addAttribute("xunits", ossimPlanetKmlConvertUnits(theScreenXUnits));
   node->addAttribute("yunits", ossimPlanetKmlConvertUnits(theScreenYUnits));   
   xmlNode->addChildNode(node.get());
   
   node = new ossimXmlNode;
   node->setTag("rotationXY");
   node->addAttribute("x", ossimString::toString(theRotationX));
   node->addAttribute("y", ossimString::toString(theRotationY));
   node->addAttribute("xunits", ossimPlanetKmlConvertUnits(theRotationXUnits));
   node->addAttribute("yunits", ossimPlanetKmlConvertUnits(theRotationYUnits));   
   xmlNode->addChildNode(node.get());
   
   node = new ossimXmlNode;
   xmlNode->setTag("size");
   node->addAttribute("x", ossimString::toString(theSizeX));
   node->addAttribute("y", ossimString::toString(theSizeY));
   node->addAttribute("xunits", ossimPlanetKmlConvertUnits(theSizeXUnits));
   node->addAttribute("yunits", ossimPlanetKmlConvertUnits(theSizeYUnits));   
   xmlNode->addChildNode(node.get());
   
   xmlNode->addChildNode("rotation", ossimString::toString(theRotation));
}

bool ossimPlanetKmlGroundOverlay::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlOverlay::parse(xmlNode))
   {
      return false;
   }
   ossimRefPtr<ossimXmlNode> latLonBoxNode = xmlNode->findFirstNode("LatLonBox");
   if(!latLonBoxNode.valid())
   {
      return false;
   }
   theLatLonBox = new ossimPlanetKmlLatLonBox();
   theLatLonBox->setParent(this);
   if(!theLatLonBox->parse(latLonBoxNode))
   {
      theLatLonBox = 0;
      return false;
   }
   ossimString tempString;
   if(xmlNode->getChildTextValue(tempString, "altitude"))
   {
      theAltitude = tempString.toDouble();
   }
   xmlNode->getChildTextValue(tempString, "altitudeMode");
   theAltitudeMode = ossimPlanetKmlConvertAltitudeMode(tempString);
   
   return true;
}

void ossimPlanetKmlGroundOverlay::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlOverlay::write(xmlNode);
   xmlNode->setTag("GroundOverlay");
   if(theLatLonBox.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theLatLonBox->write(node.get());
      xmlNode->addChildNode(node.get());
   }
   xmlNode->addChildNode("altitude", ossimString::toString(theAltitude));
   xmlNode->addChildNode("altitudeMode", ossimPlanetKmlConvertAltitudeMode(theAltitudeMode));
}
bool ossimPlanetKmlNetworkLink::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(!ossimPlanetKmlFeature::parse(xmlNode))
   {
      return false;
   }
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   ossim_uint32 idx = 0;
   for(idx = 0; idx < childNodes.size(); ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();
      if(tag == "flyToView")
      {
         theFlyToViewFlag = childNodes[idx]->getText().toBool();
      }
      else if(tag == "refreshVisibility")
      {
         theRefreshVisibilityFlag = childNodes[idx]->getText().toBool();
      }
      else if(tag == "Link")
      {
         theLink = new ossimPlanetKmlLink;
         theLink->setParent(this);
         if(!theLink->parse(childNodes[idx]))
         {
            return false;
         }
      }
      else if(tag == "Url")
      {
         theLink = new ossimPlanetKmlLink;
         theLink->setParent(this);
         if(!theLink->parse(childNodes[idx]))
         {
            return false;
         }         
      }
   }
   
   
   return theLink.valid();
}

void ossimPlanetKmlNetworkLink::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlFeature::write(xmlNode);
   xmlNode->setTag("NetworkLink");
   if(theLink.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theLink->write(node.get());
      xmlNode->addChildNode(node.get());
   }
   xmlNode->addChildNode("refreshVisibility", theRefreshVisibilityFlag?"1":"0");
   xmlNode->addChildNode("flyToView", theFlyToViewFlag?"!":"0");
}


bool ossimPlanetKmlPlacemark::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   clearFields();
   if(xmlNode->getTag() == "Placemark")
   {
      ossim_uint32 foundCount = 0;
      ossim_uint32 idx = 0;
      const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
      for(idx = 0; (idx < childNodes.size())&&(foundCount < 1); ++idx)
      {
        std::string tag = childNodes[idx]->getTag().c_str();
        if(tag == "description")
        {
            theDescription = childNodes[idx]->getText();
            
            /*
            theCDataDescriptionFlag = childNodes[idx]->cdataFlag();
            ossimRefPtr<ossimXmlNode> node2 = new ossimXmlNode;
            std::istringstream in(theDescription);
            if(node2->read(in))
            {
            }
            */
              
            if(theDescription.contains("<color>"))
            {
                ossimString temp = theDescription.after("<color>");
                std::string color = temp.before("</color>").c_str();
                //std::cout << "T:" << color << "\n";
                this->setColor(color);
            }
          }
         theGeometry = (ossimPlanetKmlGeometry*)ossimPlanetKmlObjectRegistry::instance()->newGeometry(childNodes[idx]->getTag());
         if(theGeometry.valid())
         {
            theGeometry->setParent(this);
            if(!theGeometry->parse(childNodes[idx]))
            {
               return false;
            }
            ++foundCount;
         }
      }
      
      return ossimPlanetKmlFeature::parse(xmlNode);
   }

   return false;
}

void ossimPlanetKmlPlacemark::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlFeature::write(xmlNode);
   xmlNode->setTag("Placemark");
   if(theGeometry.valid())
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theGeometry->write(node.get());
      xmlNode->addChildNode(node.get());
   }
}


bool ossimPlanetKmlContainer::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   if(!xmlNode.valid()) return false;
   
   bool result = ossimPlanetKmlFeature::parse(xmlNode);
   if(result)
   {
      const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
      ossim_uint32 idx = 0;
      
      for(idx = 0; (idx < childNodes.size())&&result; ++idx)
      {
         osg::ref_ptr<ossimPlanetKmlObject> obj = ossimPlanetKmlObjectRegistry::instance()->newObject(childNodes[idx].get());
         if(obj.valid())
         {
            obj->setParent(this);
            if(obj->parse(childNodes[idx]))
            {
               theObjectList.push_back(obj.get());
            }
            else
            {
               result = false;
            }
         }
      }
   }

   return result;
}

void ossimPlanetKmlContainer::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlFeature::write(xmlNode);
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theObjectList.size(); ++idx)
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theObjectList[idx]->write(node.get());
      xmlNode->addChildNode(node.get());
   }
}

bool ossimPlanetKmlFolder::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   return ossimPlanetKmlContainer::parse(xmlNode);
}

void ossimPlanetKmlFolder::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlContainer::write(xmlNode);
   xmlNode->setTag("Folder");
}


ossimPlanetKmlDocument::ossimPlanetKmlDocument()
{
}

bool ossimPlanetKmlDocument::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   return ossimPlanetKmlContainer::parse(xmlNode);
}

void ossimPlanetKmlDocument::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossimPlanetKmlContainer::write(xmlNode);
    xmlNode->setTag("Document");
  
}

ossimPlanetKml::ossimPlanetKml()
   :theIdMapGeneratedFlag(false)
{
}

bool ossimPlanetKml::parse(const ossimFilename& file)
{
   theIdMapGeneratedFlag = false;

   std::vector<char> buf(file.fileSize());
   if(!buf.size())
   {
      return false;
   }
   std::ifstream in(file.c_str(), std::ios::binary|std::ios::in);
   in.read(&buf.front(), buf.size());
   
   if(in.gcount())
   {
      std::istringstream inStringStream(std::string(buf.begin(),
                                                    buf.begin()+in.gcount()));
      theFilename = file;
      
      return parse(inStringStream);
   }

   return false;
}

bool ossimPlanetKml::parse(std::istream& in, bool fullDocumentFlag)
{
  bool result = false;
   theIdMapGeneratedFlag = false;
   if(!in.fail())
   {
      if(fullDocumentFlag)
      {
         ossimRefPtr<ossimXmlDocument> document = new ossimXmlDocument;
         
         if(document->read(in))
         {
            result = parse(document.get());
         }
      }
      else
      {
         ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
         if(node->read(in))
         {
            result = parse(node.get());
         }
      }
   }

   return result;
}

bool ossimPlanetKml::parse(const ossimRefPtr<ossimXmlDocument> document)
{
   bool result = document.valid();
   theIdMapGeneratedFlag = false;

   if(result)
   {
      if(document->getRoot().valid())
      {
         result = parse(document->getRoot());
      }
      else
      {
         result = false;
      }
   }
   
   return result;
}

bool ossimPlanetKml::parse(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   bool result = true;
   if(!xmlNode.valid()) return false;
   if(xmlNode->getTag() != "kml") return false;
   
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();
   
   ossim_uint32 idx = 0;
   
   for(idx = 0; (idx < childNodes.size())&&(result); ++idx)
   {
      osg::ref_ptr<ossimPlanetKmlObject> obj = ossimPlanetKmlObjectRegistry::instance()->newObject(childNodes[idx].get());
      if(obj.valid())
      {
         obj->setParent(this);
         result = obj->parse(childNodes[idx]);
         if(result)
         {
            theObjectList.push_back(obj.get());
         }
      }
   }

//    if(result)
//    {
//       ossimRefPtr<ossimXmlDocument> doc = writeDocument();
//       std::cout << *(doc.get()) << std::endl;
//    }
   return result;
}

void ossimPlanetKml::write(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   ossim_uint32 idx = 0;
   xmlNode->setTag("Kml");
   for(idx = 0; idx < theObjectList.size(); ++idx)
   {
      ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
      theObjectList[idx]->write(node.get());
      xmlNode->addChildNode(node.get());
   }
}

ossimRefPtr<ossimXmlDocument> ossimPlanetKml::writeDocument()const
{
   ossimRefPtr<ossimXmlDocument> document = new ossimXmlDocument;
   ossimRefPtr<ossimXmlNode> rootNode = new ossimXmlNode;
   document->initRoot(rootNode);
   write(rootNode);
   
   return document;
}

void ossimPlanetKml::createIdMap()const
{
   theIdMap.clear();
   std::queue<osg::ref_ptr<ossimPlanetKmlObject> > objectSearchQueue;
   ossimPlanetKmlObject::ObjectList::const_iterator iter = theObjectList.begin();
   while(iter != theObjectList.end())
   {
      objectSearchQueue.push(*iter);
      ++iter;
   }
   while(!objectSearchQueue.empty())
   {
      osg::ref_ptr<ossimPlanetKmlObject> topObj = objectSearchQueue.front().get();
      objectSearchQueue.pop();
      if(topObj->id() != "")
      {
         theIdMap.insert(std::make_pair(topObj->id().c_str(), topObj));
      }
      if(topObj->getObjectList().size())
      {
         ossim_uint32 idx = 0;
         ossimPlanetKmlObject::ObjectList& objList = topObj->getObjectList();
         
         for(idx = 0; idx < objList.size(); ++idx)
         {
            objectSearchQueue.push(objList[idx]);
         }
      }
   }
   theIdMapGeneratedFlag = true;
}

osg::ref_ptr<ossimPlanetKmlObject> ossimPlanetKml::findById(const ossimString& id)
{
   if(!theIdMapGeneratedFlag)
   {
      createIdMap();
   }
   IdMapType::iterator iter = theIdMap.find(id);

   if(iter != theIdMap.end())
   {
      return iter->second;
   }

   return 0;
}

const osg::ref_ptr<ossimPlanetKmlObject> ossimPlanetKml::findById(const ossimString& id)const
{
   if(!theIdMapGeneratedFlag)
   {
      createIdMap();
   }

   IdMapType::const_iterator iter = theIdMap.find(id);

   if(iter != theIdMap.end())
   {
      return iter->second.get();
   }

   return 0;
}

ossimFilename ossimPlanetKml::getCacheLocation(bool sharedLocationFlag)const
{
   if(sharedLocationFlag)
   {
      ossimFilename sharedDir = ossimEnvironmentUtility::instance()->getUserOssimSupportDir();
      sharedDir = sharedDir.dirCat("kml");
      if(!sharedDir.exists())
      {
         sharedDir.createDirectory();
      }
      return sharedDir;
   }
   if(theCacheLocation == "")
   {
      theCacheLocation = ossimEnvironmentUtility::instance()->getUserOssimSupportDir();
      theCacheLocation = theCacheLocation.dirCat("kml");
      if(theCacheLocation != "")
      {
         theCacheLocation = theCacheLocation.dirCat(theFilename.fileNoExtension());
      }
      if(!theCacheLocation.exists())
      {
         theCacheLocation.createDirectory();
      }
   }

   return theCacheLocation;
}

bool ossimPlanetKml::getAllFeatures(ossimPlanetKmlObject::ObjectList& features)
{
   std::queue<osg::ref_ptr<ossimPlanetKmlObject> > objectSearchQueue;
   ossimPlanetKmlObject::ObjectList::iterator iter = theObjectList.begin();
   
   while(iter != theObjectList.end())
   {
      objectSearchQueue.push(*iter);
      ++iter;
   }

   while(!objectSearchQueue.empty())
   {
      osg::ref_ptr<ossimPlanetKmlObject> topObj = objectSearchQueue.front().get();
      objectSearchQueue.pop();
      if(dynamic_cast<ossimPlanetKmlPlacemark*>(topObj.get()))
      {
         features.push_back(topObj.get());
      }
      else if(dynamic_cast<ossimPlanetKmlGroundOverlay*>(topObj.get()))
      {
         features.push_back(topObj.get());
      }
      else if(dynamic_cast<ossimPlanetKmlNetworkLink*>(topObj.get()))
      {
         features.push_back(topObj.get());
      }
      if(topObj->getObjectList().size())
      {
         ossim_uint32 idx = 0;
         ossimPlanetKmlObject::ObjectList& objList = topObj->getObjectList();

         for(idx = 0; idx < objList.size(); ++idx)
         {
            objectSearchQueue.push(objList[idx]);
         }
      }
   }

   return (features.size() > 0);
}


#ifdef OSSIMPLANET_HAS_LIBZ
struct ossimPlantKmzPrivate
{
   ossimPlantKmzPrivate()
      {
         theArchive = 0;
      }
   unzFile       theArchive;
   std::vector<ossimFilename> theFileList;
};
static ossimPlantKmzPrivate* ossimPlanetKmzPrivateData(void* pvt)
{
   return (ossimPlantKmzPrivate*)pvt;
}
#endif

ossimPlanetKmz::ossimPlanetKmz()
   :ossimPlanetKml()
{
#ifdef OSSIMPLANET_HAS_LIBZ
   thePrivateData = 0;
#endif
}


ossimPlanetKmz::~ossimPlanetKmz()
{
#ifdef OSSIMPLANET_HAS_LIBZ
   if(thePrivateData)
   {
      ossim_uint32 idx = 0;
      std::vector<ossimFilename>& fileList = ossimPlanetKmzPrivateData(thePrivateData)->theFileList;
      for(idx = 0; idx < fileList.size(); ++idx)
      {
         fileList[idx].remove();
         fileList[idx].path().remove();
      }
      unzClose(ossimPlanetKmzPrivateData(thePrivateData)->theArchive);
      delete ossimPlanetKmzPrivateData(thePrivateData);
      thePrivateData = 0;
   }
#endif

}

bool ossimPlanetKmz::parse(std::istream& /*in*/)
{
   // currently we do not support bridging an istream to libz
   return false;
}

bool ossimPlanetKmz::parse(const ossimFilename& file)
{
#ifdef OSSIMPLANET_HAS_LIBZ
   if(thePrivateData)
   {
      unzClose(ossimPlanetKmzPrivateData(thePrivateData)->theArchive);
      delete ossimPlanetKmzPrivateData(thePrivateData);
      thePrivateData = 0;
   }
   thePrivateData = new ossimPlantKmzPrivate;
   ossimPlanetKmzPrivateData(thePrivateData)->theArchive = unzOpen(file.c_str());
   
   if(!(ossimPlanetKmzPrivateData(thePrivateData))->theArchive)
   {
      delete ossimPlanetKmzPrivateData(thePrivateData);
      thePrivateData = 0;
   }
   if(thePrivateData)
   {
      unz_global_info gi;
      int err;
      unzFile uf = ossimPlanetKmzPrivateData(thePrivateData)->theArchive;
      err = unzGetGlobalInfo (uf, &gi);
      if (err!=UNZ_OK)
      {
         return false;
      }
      theFilename = file;
      ossimFilename cacheLocation = getCacheLocation();
      cacheLocation.createDirectory();
      ossim_uint32 i = 0;
      for (i=0;i<gi.number_entry;i++)
      {
         std::vector<char> tempFileInfo(2048);
         unz_file_info file_info;
         err = unzGetCurrentFileInfo(uf,
                                     &file_info,
                                     (char*)(&tempFileInfo.front()),
                                     tempFileInfo.size(),
                                     NULL,
                                     0,NULL,0);
         if (err!=UNZ_OK)
         {
            //ossimNotify(ossimNotifyLevel_WARN) << "Unable to get the global information to read the directory of the passed in KMZ file\n";
            return false;
         }
         ossimFilename outFile;
         ossimFilename testFile((char*)(&tempFileInfo.front()));
         if(!testFile.empty())
         {
            testFile = cacheLocation.dirCat(testFile.c_str());
            outFile = testFile;
            if(testFile.ext().downcase() == "kml")
            {
               theFilename =  testFile.path().dirCat(theFilename.fileNoExtension()) + ".kml";
               outFile = theFilename;
            }
            if(unzOpenCurrentFile(ossimPlanetKmzPrivateData(thePrivateData)->theArchive) == UNZ_OK)
            {
               ossimFilename dir(outFile.path());
               if(!dir.exists())
               {
                  dir.createDirectory();
               }
               ossimPlanetKmzPrivateData(thePrivateData)->theFileList.push_back(outFile);
                  std::ofstream out(outFile.c_str(),
                                 std::ios::out|std::ios::binary);

               if(!out.fail())
               {
                  std::vector<char> buf(2048);
                  int bytes = 0;
                  while((bytes=unzReadCurrentFile(uf, (char*)(&buf.front()), buf.size()))>0)
                  {
                     out.write((char*)(&buf.front()), bytes);
                  }
                  out.close();
               }
               unzCloseCurrentFile(ossimPlanetKmzPrivateData(thePrivateData)->theArchive);
            }
         }
         if ((i+1)<gi.number_entry)
         {
            unzGoToNextFile(uf);
         }
      }
      if(theFilename.exists())
      {
            ossimRefPtr<ossimXmlDocument> document = new ossimXmlDocument;
            
            if(document->openFile(theFilename))
            {
               return ossimPlanetKml::parse(document);
            }
      }
   }
#if 0
   if(thePrivateData&&!ossimPlanetKmzPrivateData(thePrivateData)->theKmlFile.empty())
   {
      
      unzFile uf = ossimPlanetKmzPrivateData(thePrivateData)->theArchive;
      if(unzLocateFile(uf,
                       ossimPlanetKmzPrivateData(thePrivateData)->theKmlFile.c_str(),
                       true) == UNZ_OK)
      {
         if(unzOpenCurrentFile(ossimPlanetKmzPrivateData(thePrivateData)->theArchive) == UNZ_OK)
         {
            std::stringstream kmlBuf;
            int bytes = 0;
            std::vector<char> buf(2048);
            while((bytes=unzReadCurrentFile(uf, (char*)(&buf.front()), buf.size()))>0)
            {
               kmlBuf.write((char*)(&buf.front()), bytes);
            }
            
            ossimRefPtr<ossimXmlDocument> document = new ossimXmlDocument;
            
            if(document->read(kmlBuf))
            {
               return ossimPlanetKml::parse(document);
            }
         }
      }
   }
#endif
   return false;
//    if(thePrivateData)
//    {
//       if(ossimPlanetKmzPrivateData(thePrivateData)->theDocFile)
//       {
//          zzip_file_close(ossimPlanetKmzPrivateData(thePrivateData)->theDocFile);
//       }
//       zzip_dir_close(ossimPlanetKmzPrivateData(thePrivateData)->theArchive);
//       ossimPlanetKmzPrivateData(thePrivateData)->theArchive = 0;
//       delete ossimPlanetKmzPrivateData(thePrivateData);
//       thePrivateData = 0;
      
//    }
//    theFilename = file;
//    zzip_error_t errorFlag = ZZIP_NO_ERROR;
//    thePrivateData = new ossimPlantKmzPrivate;
//    ossimPlanetKmzPrivateData(thePrivateData)->theArchive = zzip_dir_open(file.c_str(), &errorFlag);
//    if((!ossimPlanetKmzPrivateData(thePrivateData)->theArchive)||(ZZIP_NO_ERROR != ZZIP_NO_ERROR))
//    {
//       if(ossimPlanetKmzPrivateData(thePrivateData)->theArchive)
//       {
//          zzip_dir_close(ossimPlanetKmzPrivateData(thePrivateData)->theArchive);
//       }
      
//       return false;
//    }
//    ossimPlanetKmzPrivateData(thePrivateData)->theDocFile = zzip_file_open(ossimPlanetKmzPrivateData(thePrivateData)->theArchive,
//                                                                           "doc.kml",
//                                                                           0);

//    if(!ossimPlanetKmzPrivateData(thePrivateData)->theDocFile)
//    {
//       return false;
//    }
//    std::stringstream buf;
//    std::vector<char> tempBuf(1024);

//    int count = 0;
//    while( (count = zzip_file_read(ossimPlanetKmzPrivateData(thePrivateData)->theDocFile,
//                                   &tempBuf.front(),
//                                   1024)) != 0)
//    {
//       buf.write(&tempBuf.front(), count);
//    }
//    buf.seekg(0);
//    ossimRefPtr<ossimXmlDocument> document = new ossimXmlDocument;
   
//    if(document->read(buf))
//    {
//       return ossimPlanetKml::parse(document);
//    }

   return false;
#else
      return ossimPlanetKml::parse(file);
#endif
}
