#ifndef ossimPlanetKmlScreenOverlayNode_HEADER
#define ossimPlanetKmlScreenOverlayNode_HEADER
#include <ossimPlanet/ossimPlanetKml.h>
#include <ossimPlanet/ossimPlanetKmlLayerNode.h>
#include <osg/Group>
#include <osg/CameraNode>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/ossimPlanetIconGeom.h>

class ossimPlanetKmlScreenOverlayNode : public ossimPlanetKmlLayerNode
{
public:
   ossimPlanetKmlScreenOverlayNode(ossimPlanetKmlLayer* layer = 0,
                                   ossimPlanetKmlObject* obj  = 0);
   virtual void traverse(osg::NodeVisitor& nv);
   virtual bool init();

protected:
   void update();
   void computeOverlayXY(osg::Vec3d& position);
   void computeScreenXY(osg::Vec3d& position);
   void computeSize(osg::Vec3d& size);
   
   osg::ref_ptr<osg::CameraNode> theCameraNode;
   osg::ref_ptr<osg::Group> theGroup;
   osg::ref_ptr<osg::Viewport> theViewport;
   osg::ref_ptr<osg::Geode> theIconGeode; 
   osg::ref_ptr<ossimPlanetIconGeom> theIconGeom; 
   bool theViewportChangedFlag;
   bool theNeedsUpdateFlag;

   osg::Vec2d theOverlayOrigin;
   ossimPlanetKmlUnits theOverlayXUnits;
   ossimPlanetKmlUnits theOverlayYUnits;
   osg::Vec2d theScreenOrigin;
   ossimPlanetKmlUnits theScreenXUnits;
   ossimPlanetKmlUnits theScreenYUnits;
   
   osg::Vec2d theRotationOrigin;
   ossimPlanetKmlUnits theRotationXUnits;
   ossimPlanetKmlUnits theRotationYUnits;
   osg::Vec2d theSize;
   ossimPlanetKmlUnits theSizeXUnits;
   ossimPlanetKmlUnits theSizeYUnits;   
   float theRotation;
};


#endif
