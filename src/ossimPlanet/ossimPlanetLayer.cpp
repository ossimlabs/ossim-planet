#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetIdManager.h>
#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanet.h>
#include <osgGA/EventVisitor>
#include <iostream>

class ossimPlanetLayerFinder : public osg::NodeVisitor
{
public:
   ossimPlanetLayerFinder()
      :osg::NodeVisitor(NODE_VISITOR,
                        TRAVERSE_ALL_CHILDREN)
      {
         thePlanetLayer = 0;
      }

   virtual void apply(osg::Node& node)
      {
         if(!thePlanetLayer)
         {
            thePlanetLayer = dynamic_cast<ossimPlanetLayer*>(&node);
         }
         else
         {
            return;
         }
         osg::NodeVisitor::apply(node);
      }
   
   ossimPlanetLayer* thePlanetLayer;
};

class ossimPlanetLayerTraverseCallback : public osg::NodeCallback
{
public:
   ossimPlanetLayerTraverseCallback()
      {
      }
   virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
      {
         if(node)
         {
            node->traverse(*nv);
         }
      }
};

ossimPlanetLayer::ossimPlanetLayer()
   :thePlanet(0)
{
   setEventCallback(new ossimPlanetTraverseCallback);
}

ossimPlanetLayer::~ossimPlanetLayer()
{
   thePlanet = 0;
}

void ossimPlanetLayer::traverse(osg::NodeVisitor& nv)
{
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::EVENT_VISITOR:
      {
         if(redrawFlag())
         {
            osgGA::EventVisitor* ev = dynamic_cast<osgGA::EventVisitor*>(&nv);
            if(ev)
            {
               if(ev->getActionAdapter())
               {
                  ev->getActionAdapter()->requestRedraw();
                  setRedrawFlag(false);
               }
            }
         }
         return;
      }
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if(!thePlanet)
         {
            setPlanet(ossimPlanet::findPlanet(this));
         }
         break;
      }
      default:
      {
         
      }
   }
   ossimPlanetNode::traverse(nv);
}

ossimPlanetLayer* ossimPlanetLayer::findLayer(osg::Node* startNode)
{
   osg::Node* rootNode        = startNode;
   osg::Node* rootNonNullNode = startNode;

   while(rootNode)
   {
      rootNonNullNode = rootNode;
      ossimPlanetLayer* castNode = dynamic_cast<ossimPlanetLayer*>(rootNode);
      if(castNode)
      {
         return castNode;
      }
      if(rootNode->getNumParents() > 0)
      {
         rootNode = rootNode->getParent(0);
      }
      else
      {
         rootNode = 0;
      }
   }
   if(rootNonNullNode)
   {
      ossimPlanetLayerFinder finder;
      rootNonNullNode->accept(finder);
      return finder.thePlanetLayer;
   }

   return 0;
}

ossimPlanetLayer* ossimPlanetLayer::findLayer(osg::NodePath& currentNodePath)
{
   if(currentNodePath.empty())
   {
      return 0;
   }
   for(osg::NodePath::reverse_iterator itr = currentNodePath.rbegin();
       itr != currentNodePath.rend();
       ++itr)
   {
      ossimPlanetLayer* layer = dynamic_cast<ossimPlanetLayer*>(*itr);
      if (layer) 
      {
         return layer;
      }
   }
   
   return 0;
}

void ossimPlanetLayer::execute(const ossimPlanetAction& action)
{
   
#if 0
   if(action.command() == "setReceiver")
   {
      if(action.argCount() == 1)
      {
         setPathnameAndRegister(action.arg(1));
      }
   }
   else if(action.command() == "setEnableFlag")
   {
      if(action.argCount() == 1)
      {
         setEnableFlag(ossimString(action.arg(1)).toBool());
      }
   }
   else if(action.command() == "setId")
   {
      if(action.argCount()==1)
      {
         setId(action.arg(1));
      }
   }
   else if(action.command() == "setName")
   {
      if(action.argCount()==1)
      {
         setName(action.arg(1));
      }
   }
   else if(action.command() == "setDescription")
   {
      if(action.argCount()==1)
      {
         setDescription(action.arg(1));
      }
   }
   else if(action.command() == "init")
   {
      if(action.argCount() != 1) return;
      
      ossimString   objectName;
      ossimString   objectArg;
      ossim_uint32 idx = 1;
      ossimPlanetAction nestedAction(":dummy dummy " + action.arg(1) );
      for(idx = 1; idx <= nestedAction.argCount(); ++idx)
      {
         if(mkUtils::extractObjectAndArg(objectName,
                                         objectArg,
                                         nestedAction.arg(idx)))
         {
            if(objectName == "Name")
            {
               setName(objectArg);
            }
            else if(objectName == "Id")
            {
               setId(objectArg);
            }
            else if(objectName == "Description")
            {
               setDescription(objectArg);
            }
            else if(objectName == "ReceiverPath")
            {
               setPathnameAndRegister(objectArg);
            }
            else if(objectName == "Enable")
            {
               setEnableFlag(objectArg.toBool());
            }
         }
      }
   }
#endif
}

