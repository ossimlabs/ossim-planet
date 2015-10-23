#include <ossimPlanet/ossimPlanetKmlLayerNode.h>
#include <ossimPlanet/ossimPlanetImage.h>

ossimPlanetKmlLayerNode::ossimPlanetKmlLayerNode(ossimPlanetKmlLayer* layer,
                                                 ossimPlanetKmlObject* obj)
   :theLayer(layer),
    theKmlObject(obj)
{
   ossimPlanetKmlFeature* feature = dynamic_cast<ossimPlanetKmlFeature*>(obj);
   if(feature)
   {
      setName(feature->name());
      setDescription(feature->description());
      setId(feature->id());
   }
}

void ossimPlanetKmlLayerNode::traverse(osg::NodeVisitor& nv)
{
   if(!enableFlag()) return;

   ossimPlanetNode::traverse(nv);
}

void ossimPlanetKmlLayerNode::setLayer(ossimPlanetKmlLayer* layer)
{
   theLayer = layer;
}

ossimPlanetKmlLayer* ossimPlanetKmlLayerNode::layer()
{
   return theLayer;
}

void ossimPlanetKmlLayerNode::setKmlObject(ossimPlanetKmlObject* obj)
{
   theKmlObject = obj;
   ossimPlanetKmlFeature* feature = dynamic_cast<ossimPlanetKmlFeature*>(obj);
   if(feature)
   {
      setName(feature->name());
      setDescription(feature->description());
      setId(feature->id());
   }
}

bool ossimPlanetKmlLayerNode::init()
{
   return true;
}
