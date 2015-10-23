#ifndef ossimPlanetKmlLayerNode_HEADER
#define ossimPlanetKmlLayerNode_HEADER
#include <osg/Group>
#include <ossimPlanet/ossimPlanetKml.h>
#include <ossimPlanet/ossimPlanetNode.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <osg/Image>
class ossimPlanetKmlLayer;

class OSSIMPLANET_DLL ossimPlanetKmlLayerNode : public ossimPlanetNode
{
public:
   ossimPlanetKmlLayerNode(ossimPlanetKmlLayer* layer = 0,
                           ossimPlanetKmlObject* obj  = 0);
   void setKmlLayer(ossimPlanetKmlLayer* layer)
   {
      theLayer = layer;
   }
   void setLayer(ossimPlanetKmlLayer* layer);
   void setKmlObject(ossimPlanetKmlObject* obj);
   ossimPlanetKmlLayer* layer();
   
   osg::ref_ptr<ossimPlanetKmlObject> kmlObject()
   {
      return theKmlObject.get();
   }

   virtual void traverse(osg::NodeVisitor& nv);
   
   /**
    * This is a convenient method for interactive GUIs to call to allow a KML node to change its state
    * in the scene graph.
    */ 
   virtual void doNormalStyle(){}

   /**
    * This is a convenient method for interactive GUIs to call to allow the KML node to change its state
    * in the scene graph.
    */ 
   virtual void doHighlightStyle(){}
   
   virtual bool init();
   
protected:
   ossimPlanetKmlLayer*               theLayer;
   osg::ref_ptr<ossimPlanetKmlObject> theKmlObject;
};

#endif
