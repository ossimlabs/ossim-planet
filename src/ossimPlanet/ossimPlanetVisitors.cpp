#include <ossimPlanet/ossimPlanetVisitors.h>
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetNode.h>

void ossimPlanetLayerNameIdSearchVisitor::apply(osg::Node& node)
{
   ossimPlanetLayer* layer = dynamic_cast<ossimPlanetLayer*>(&node);
   ossimPlanetNode* layerNode = dynamic_cast<ossimPlanetNode*>(&node);
   
   if(theId.empty() && theName.empty()) return;
   
   if(layer)
   {
      if(!theName.empty()&&!theId.empty())
      {
         if((layer->name() == theName)&&
            (layer->id() == theId))
         {
            theNode = &node;
         }
      }
      else if(!theName.empty())
      {
         if(layer->name() == theName)
         {
            theNode = &node;
         }
         
      }
      else if(!theId.empty())
      {
         if(layer->id() == theId)
         {
            theNode = &node;
         }
      }
   }
   else if(layerNode)
   {
      if(!theName.empty()&&!theId.empty())
      {
         if((layerNode->name() == theName)&&
            (layerNode->id() == theId))
         {
            theNode = &node;
         }
      }
      else if(!theName.empty())
      {
         if(layerNode->name() == theName)
         {
            theNode = &node;
         }
         
      }
      else if(!theId.empty())
      {
         if(layerNode->id() == theId)
         {
            theNode = &node;
         }
      }
   }
   if(theNode.valid()) return;
   traverse(node);
};

ossimPlanetUpdateVisitor::ossimPlanetUpdateVisitor()
:osgUtil::UpdateVisitor(),
theRedrawFlag(false)
{
   
}

void ossimPlanetUpdateVisitor::reset()
{
   theRedrawFlag = false;
}

bool ossimPlanetUpdateVisitor::redrawFlag()const
{
   return theRedrawFlag;
}

void ossimPlanetUpdateVisitor::apply(osg::Node& node)         
{ 
   ossimPlanetNode* n = dynamic_cast<ossimPlanetNode*>(&node);
   if(n)
   {
      if(n->redrawFlag())
      {
         n->setRedrawFlag(false);
         theRedrawFlag = true;
      }
   }
   UpdateVisitor::apply(node);
}

void ossimPlanetUpdateVisitor::apply(osg::Group& node)        
{ 
   ossimPlanetNode* n = dynamic_cast<ossimPlanetNode*>(&node);
   if(n)
   {
      if(n->redrawFlag())
      {
         n->setRedrawFlag(false);
         theRedrawFlag = true;
      }
   }
   UpdateVisitor::apply(node);
}

