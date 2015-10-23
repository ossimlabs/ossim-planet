#ifndef ossimPlanetLatLonHud_HEADER
#define ossimPlanetLatLonHud_HEADER
#include <ossimPlanet/ossimPlanetLayer.h>
#include <osgText/Text>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/Projection>
#include <osg/CameraNode>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetCompass.h>
class ossimPlanet;
namespace osg
{
   class LineWidth;
}
class OSSIMPLANET_EXPORT ossimPlanetLatLonHud : public ossimPlanetLayer
{
public:
   ossimPlanetLatLonHud();
   virtual void traverse(osg::NodeVisitor& nv);
   osg::Vec4 getCrossHairColor()const;
   osg::Vec4 getTextColor()const;
   void setCrosshairColor(const osg::Vec4& color);
   void setTextColor(const osg::Vec4& color);
   void setFont(const ossimString& fontFile);
   void setLatDisplayString(const ossimString& latDisplayString);
   void setLonDisplayString(const ossimString& lonDisplayString);
   void setCharacterSize(float size);
   void setViewport(osg::ref_ptr<osg::Viewport> viewport);
   void setAutoUpdateFlag(bool flag);
   void setCompassTexture(const ossimFilename& compass);
//   void setCompassTexture(const ossimFilename& ring,
//                          const ossimFilename& interior);
  
   virtual void execute(const ossimPlanetAction& action);
   
protected:
   void initialize();
   void updatePosition();
   osg::ref_ptr<osg::CameraNode> theCameraNode;
/*    osg::ref_ptr<osg::Projection> theProjection; */
   osg::ref_ptr<osgText::Text> thePositionText;
   
   osg::ref_ptr<osgText::Text> theLookText;
   osg::ref_ptr<osgText::Text> theEyeText;
   osg::ref_ptr<osgText::Text> theRangeText;
   
   osg::ref_ptr<osg::Geode> theGeode;
   osg::ref_ptr<osg::Viewport> theViewport;
   osg::ref_ptr<osg::Geometry> theCrosshair;
   osg::Vec3d theLineOfSiteLatLon;
   osg::Vec3d theNadirLatLon;
   double     theRange;
   double     theAltitude;
   osg::Vec4 theTextColor;
   osg::Vec4 theShadowTextColor;
   osg::Vec4 theCrosshairColor;
   osg::ref_ptr<osg::LineWidth> theCrosshairLineWidth;
   bool theAutoUpdateFlag;
   mutable bool theFontChanged;
   float theCharacterSize;
   bool theCharacterSizeDirtyFlag;
   ossimString theFontName;
   osg::ref_ptr<osgText::Font> theFont;

   
   ossimString theLookLabel;
   ossimString theEyeLabel;
   ossimString theRangeLabel;
   ossimString theLookDisplayString;
   ossimString theEyeDisplayString;
   
   ossimString theLatDisplayString;
   ossimString theLonDisplayString;
   
   
   bool theInitializedFlag;
   mutable OpenThreads::ReentrantMutex theMutex;
   bool theViewportChangedFlag;
   osg::ref_ptr<ossimPlanetCompass> theCompass;
};

#endif
