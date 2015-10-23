#include <ossimPlanet/ossimPlanetVideoLayer.h>
#ifdef OSSIMPLANET_ENABLE_PREDATOR
#include <ossimPlanet/ossimPlanetPredatorVideoLayerNode.h>
#endif
#include <ossimPlanet/ossimPlanet.h>
#include <iostream>

void ossimPlanetVideoLayer::traverse(osg::NodeVisitor& nv)
{
   if(!theEnableFlag) return;
   
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if(!thePlanet)
         {
            thePlanet = ossimPlanet::findPlanet(this);
         }
         break;
      }
      default:
      {
         break;
      }
   }
   
   ossimPlanetLayer::traverse(nv);
}

bool ossimPlanetVideoLayer::add(const ossimFilename& file)
{
#ifdef OSSIMPLANET_ENABLE_PREDATOR
   // later we will stage in a background thread but for now we will just open here for testing
   // until we get the drawing working
   //
   osg::ref_ptr<ossimPlanetPredatorVideoLayerNode> node = new ossimPlanetPredatorVideoLayerNode(this);

   if(node->open(file))
   {
      addChild(node.get());
      return true;
   }
#endif
   return false;
}
