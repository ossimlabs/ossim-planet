#ifndef ossimPlanetExtents_HEADER
#define ossimPlanetExtents_HEADER
#include <osg/Referenced>
#include <ossim/base/ossimDate.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimXmlNode.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/mkUtils.h>

class OSSIMPLANET_DLL ossimPlanetExtents : public osg::Referenced
{
public:
   ossimPlanetExtents()
      :osg::Referenced(),
      theMinLon(-180.0),
      theMaxLon(180.0),
      theMinLat(-90.0),
      theMaxLat(90.0),
      theMinScale(0.0),
      theMaxScale(1.0/DBL_EPSILON),
      theMinHeight(-(1.0/DBL_EPSILON)+1.0),
      theMaxHeight(1.0/DBL_EPSILON),
      theMinDate(1, 1, 1),
      theMaxDate(12, 31, 9999)
   {
   }
   ossimPlanetExtents(const ossimPlanetExtents& src)
      :osg::Referenced(),
      theMinLon(src.theMinLon),
      theMaxLon(src.theMaxLon),
      theMinLat(src.theMinLat),
      theMaxLat(src.theMaxLat),
      theMinScale(src.theMinScale),
      theMaxScale(src.theMaxScale),
      theMinHeight(src.theMinHeight),
      theMaxHeight(src.theMaxHeight),
      theMinDate(src.theMinDate),
      theMaxDate(src.theMaxDate)
   {
   }
   ossimPlanetExtents* clone()const
   {
      return new ossimPlanetExtents(*this);
   }
   void setMinLatLon(double lat,
                     double lon)
   {
      theMinLat = lat;
      theMinLon = lon;
   }

   void setMaxLatLon(double lat, double lon)
   {
      theMaxLat = lat;
      theMaxLon = lon;
   }
   void setMinMaxLatLon(double minLat, double minLon,
                        double maxLat, double maxLon)
   {
      theMinLat = ossim::min(minLat, maxLat);
      theMaxLat = ossim::max(minLat, maxLat);
      theMinLon = ossim::min(minLon, maxLon);
      theMaxLon = ossim::max(minLon, maxLon);
   }
   void setMinMaxHeight(double minHeight, double maxHeight)
   {
      theMinHeight = ossim::min(minHeight, maxHeight);
      theMaxHeight = ossim::max(minHeight, maxHeight);
   }
   void setMinMaxScale(double minScale,
                       double maxScale)
   {
      theMinScale = ossim::min(minScale, maxScale);
      theMaxScale = ossim::max(minScale, maxScale);
   }
   bool intersectsLatLon(const double& minLat, const double& minLon,
                         const double& maxLat, const double& maxLon)const
   {
      double tempMinLon = ossim::max(theMinLon, minLon);
      double tempMaxLon = ossim::min(theMaxLon, maxLon);
      double tempMinLat = ossim::max(theMinLat, minLat);
      double tempMaxLat = ossim::min(theMaxLat, maxLat);

      return ((tempMinLon <= tempMaxLon)&&(tempMinLat<=tempMaxLat));
      
   }
   bool intersectsLatLon(const ossimPlanetExtents& extents)const
   {
      return intersectsLatLon(extents.theMinLat, extents.theMinLon,
                              extents.theMaxLat, extents.theMaxLon);
   }

   bool intersectsHeight(const double& minHeight, const double& maxHeight)const
   {
      return (ossim::max(minHeight,
                       theMinHeight) <=
              ossim::min(maxHeight,
                       theMaxHeight));
   }
   bool intersectsHeight(const ossimPlanetExtents& extents)const
   {
      return intersectsHeight(extents.theMinHeight,
                              extents.theMaxHeight);
   }
   bool intersectsScale(const double& minScale,
                        const double& maxScale)const
   {
      return (ossim::max(minScale,
                         theMinScale) <=
              ossim::min(maxScale,
                         theMaxScale));      
   }
   
   bool intersectsScale(const ossimPlanetExtents& extents)const
   {
      return intersectsScale(extents.theMinScale,
                             extents.theMaxScale);
   }

   bool intersectsModifiedJulianDate(const double& minDate,
                                     const double& maxDate)const
   {
      return (ossim::max(minDate, theMinDate.getModifiedJulian()) <=
              ossim::min(maxDate, theMaxDate.getModifiedJulian()));
   }
   bool intersectsDate(const ossimPlanetExtents& extents)const
   {
      return intersectsModifiedJulianDate(extents.theMinDate.getModifiedJulian(),
                                          extents.theMaxDate.getModifiedJulian());
   }

   bool equal(osg::ref_ptr<ossimPlanetExtents> extents)const
   {
      if(extents.valid())
      {
         return equal(*extents.get());
      }
      return false;
   }
   bool equal(const ossimPlanetExtents& extents)const
   {
      return (ossim::almostEqual(theMinLon, extents.theMinLon, DBL_EPSILON)&&
              ossim::almostEqual(theMaxLon, extents.theMaxLon, DBL_EPSILON)&&
              ossim::almostEqual(theMinLat, extents.theMinLat, DBL_EPSILON)&&
              ossim::almostEqual(theMaxLat, extents.theMaxLat, DBL_EPSILON)&&
              ossim::almostEqual(theMinScale, extents.theMinScale, DBL_EPSILON)&&
              ossim::almostEqual(theMaxScale, extents.theMaxScale, DBL_EPSILON)&&
              ossim::almostEqual(theMinHeight, extents.theMinHeight, DBL_EPSILON)&&
              ossim::almostEqual(theMaxHeight, extents.theMaxHeight, DBL_EPSILON)&&
              (theMinDate == extents.theMinDate)&&
              (theMaxDate == extents.theMaxDate));
   }
   bool intersects(const ossimPlanetExtents& extents)const
   {
      return (intersectsDate(extents)&&
              intersectsScale(extents)&&
              intersectsHeight(extents)&&
              intersectsLatLon(extents));
   }
   double getMinLat()const
   {
      return theMinLat;
   }
   double getMinLon()const
   {
      return theMinLon;
   }
   double getMaxLat()const
   {
      return theMaxLat;
   }
   double getMaxLon()const
   {
      return theMaxLon;
   }
   double getMinScale()const
   {
      return theMinScale;
   }
   double getMaxScale()const
   {
      return theMaxScale;
   }
   double getMinHeight()const
   {
      return theMinHeight;
   }
   double getMaxHeight()const
   {
      return theMaxHeight;
   }
   const ossimDate& getMinDate()const
   {
      return theMinDate;
   }
   const ossimDate& getMaxDate()const
   {
      return theMaxDate;
   }

   void combineMinMaxLatLon(const double& minLat,
                            const double& minLon,
                            const double& maxLat,
                            const double& maxLon)
   {
      theMinLat = ossim::min(minLat, theMinLat);
      theMinLon = ossim::min(minLon, theMinLon);
      theMaxLat = ossim::max(maxLat, theMaxLat);
      theMaxLon = ossim::max(maxLon, theMaxLon);
   }
   void combineHeights(const double& minHeight,
                       const double& maxHeight)
   {
      theMaxHeight = ossim::max(maxHeight, theMaxHeight);
      theMinHeight = ossim::min(minHeight, theMinHeight);
   }
   void combineScale(const double& minScale,
                     const double& maxScale)
   {
      theMinScale = ossim::min(theMinScale, minScale);
      theMaxScale = ossim::max(theMaxScale, maxScale);
   }
   void combineDate(const ossimDate& minDate,
                    const ossimDate& maxDate)
   {
      if(minDate.getModifiedJulian() < theMinDate.getModifiedJulian())
      {
         theMinDate = minDate;
      }
      if(maxDate.getModifiedJulian() > theMaxDate.getModifiedJulian())
      {
         theMaxDate = maxDate;
      }
      
   }
   void combine(const ossimPlanetExtents* extents)
   {
      if(!extents) return;
      combineMinMaxLatLon(extents->theMinLat,
                          extents->theMinLon,
                          extents->theMaxLat,
                          extents->theMaxLon);
      combineHeights(extents->theMinHeight,
                     extents->theMaxHeight);
      combineScale(extents->theMinScale,
                   extents->theMaxScale);
      combineDate(extents->theMinDate,
                  extents->theMaxDate);
     
   }
   ossimRefPtr<ossimXmlNode> saveXml()const
   {
      ossimRefPtr<ossimXmlNode> result  = new ossimXmlNode();
      result->setTag("ossimPlanetExtents");
      ossimRefPtr<ossimXmlNode> minDate = new ossimXmlNode();
      ossimRefPtr<ossimXmlNode> maxDate = new ossimXmlNode();
      result->addChildNode("minLat", ossimString::toString(theMinLat));
      result->addChildNode("maxLat", ossimString::toString(theMaxLat));
      result->addChildNode("minLon", ossimString::toString(theMinLon));
      result->addChildNode("maxLon", ossimString::toString(theMaxLon));
      result->addChildNode("minScale", ossimString::toString(theMinScale));
      result->addChildNode("maxScale", ossimString::toString(theMaxScale));
      result->addChildNode("minHeight", ossimString::toString(theMinHeight));
      result->addChildNode("maxHeight", ossimString::toString(theMaxHeight));

      minDate->setTag("minDate");
      maxDate->setTag("maxDate");
      minDate->addChildNode(theMinDate.saveXml().get());
      maxDate->addChildNode(theMaxDate.saveXml().get());
      
      result->addChildNode(minDate.get());
      result->addChildNode(maxDate.get());
      
      return result.get();
   }
   bool loadXml(ossimRefPtr<ossimXmlNode> extentsNode)
   {
      if(!extentsNode.valid()) return false;
      bool result = true;

      ossimRefPtr<ossimXmlNode> minLat = extentsNode->findFirstNode("minLat");
      ossimRefPtr<ossimXmlNode> maxLat = extentsNode->findFirstNode("maxLat");
      ossimRefPtr<ossimXmlNode> minLon = extentsNode->findFirstNode("minLon");
      ossimRefPtr<ossimXmlNode> maxLon = extentsNode->findFirstNode("maxLon");
      ossimRefPtr<ossimXmlNode> minScale = extentsNode->findFirstNode("minScale");
      ossimRefPtr<ossimXmlNode> maxScale = extentsNode->findFirstNode("maxScale");
      ossimRefPtr<ossimXmlNode> minHeight = extentsNode->findFirstNode("minHeight");
      ossimRefPtr<ossimXmlNode> maxHeight = extentsNode->findFirstNode("maxHeight");
      ossimRefPtr<ossimXmlNode> minDate = extentsNode->findFirstNode("minDate");
      ossimRefPtr<ossimXmlNode> maxDate = extentsNode->findFirstNode("maxDate");

      if(minLat.valid()&&maxLat.valid()&&
         minLon.valid()&&maxLon.valid()&&
         minScale.valid()&&maxScale.valid()&&
         minHeight.valid()&&maxHeight.valid()&&
         minDate.valid()&&maxDate.valid())
      {
         theMinLat = minLat->getText().toDouble();
         theMaxLat = maxLat->getText().toDouble();
         theMinLon = minLon->getText().toDouble();
         theMaxLon = maxLon->getText().toDouble();
         theMinScale = minScale->getText().toDouble();
         theMaxScale = maxScale->getText().toDouble();
         theMinHeight = minHeight->getText().toDouble();
         theMaxHeight = maxHeight->getText().toDouble();
         if(!theMinDate.loadXml(minDate.get()))
         {
            result = false;
         }
         if(!theMaxDate.loadXml(maxDate.get()))
         {
            result = false;
         }
      }
      else
      {
         result = false;
      }
      
      return result;
   }
   ossimString toString()const
   {
      ossimString result;
      
      result += "theMinLon: " + ossimString::toString(theMinLon) + "\n";
      result += "theMaxLon: " + ossimString::toString(theMaxLon) + "\n";
      result += "theMinLat: " + ossimString::toString(theMinLat) + "\n";
      result += "theMaxLat: " + ossimString::toString(theMaxLat) + "\n";
      result += "theMinScale: " + ossimString::toString(theMinScale) + "\n";
      result += "theMaxScale: " + ossimString::toString(theMaxScale) + "\n";
      result += "theMinHeight: " + ossimString::toString(theMinHeight) + "\n";
      result += "theMaxHeight: " + ossimString::toString(theMaxHeight);
      return result;
   }
protected:
   ~ossimPlanetExtents()
   {
   }
   double    theMinLon;
   double    theMaxLon;
   double    theMinLat;
   double    theMaxLat;
   double    theMinScale;
   double    theMaxScale;
   double    theMinHeight;
   double    theMaxHeight;
   ossimDate theMinDate;
   ossimDate theMaxDate;
};
#endif
