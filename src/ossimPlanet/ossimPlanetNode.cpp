#include <ossimPlanet/ossimPlanetNode.h>
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanetXmlAction.h>
#include <osgUtil/IntersectVisitor>
#include <osgUtil/IntersectionVisitor>
ossimPlanetNode::ossimPlanetNode()
:theEnableFlag(true),
theIntersectableFlag(true),
theRedrawFlag(false),
theLayer(0)
{
   setCullCallback(new ossimPlanetTraverseCallback);
   setUpdateCallback(new ossimPlanetTraverseCallback);
}

ossimPlanetNode::~ossimPlanetNode()
{
   setUpdateCallback(0);
   setCullCallback(0);
   
   notifyDestructing(this);
   theCallbackList.clear();   
}

void ossimPlanetNode::remove(osg::Node* node)
{
	osg::ref_ptr<osg::Node> savedNode = node;
   ossim_uint32 upperIdx = node->getParents().size();
   if(upperIdx > 0)
   {
      ossim_uint32 idx = 0;
      for(;idx < upperIdx;++idx)
      {
         osg::Group* group = node->getParent(idx)->asGroup();
         if(group)
         {
            group->removeChild(node);
         }
      }
   }
}

void ossimPlanetNode::traverse(osg::NodeVisitor& nv)
{
   if(!theLayer)
   {
      setLayer(ossimPlanetLayer::findLayer(nv.getNodePath()));
   }
   if(!intersectFlag()&&(dynamic_cast<osgUtil::IntersectVisitor*> (&nv) ||
                         dynamic_cast<osgUtil::IntersectionVisitor*> (&nv)))
   {
      return;
   }
   
   if(!enableFlag())
   {
      return;
   }
   osg::Group::traverse(nv);
}

void ossimPlanetNode::execute(const ossimPlanetAction& action)
{
   const ossimPlanetXmlAction* xmlAction = action.toXmlAction();
   if(xmlAction&&xmlAction->xmlNode().valid())
   {
      ossimString command = action.command();
      if(command == "Set")
      {
         ossim_uint32 idx = 0;
         const ossimXmlNode::ChildListType& childNodes = xmlAction->xmlNode()->getChildNodes();
			const ossimXmlNode::ChildListType* properties = 0;
			// at this point we should just have a single object in the first child of the set
			//
			if(childNodes.size() == 1)
			{
				properties = &(childNodes[0]->getChildNodes());
			}
			if(!properties) return;
         for(idx = 0;idx<properties->size();++idx)
         {
            ossimString tag = (*properties)[idx]->getTag();
            if(tag=="name")
            {
               setName((*properties)[idx]->getText());
            }
            else if(tag == "description")
            {
               setDescription((*properties)[idx]->getText());
            }
            else if(tag == "id")
            {
               setId((*properties)[idx]->getText());
            }
            else if(tag == "receiverPath")
            {
               setPathnameAndRegister((*properties)[idx]->getText());
            }
         }
      }
   }
}

bool ossimPlanetNode::addChild( Node *child )
{
   bool result = osg::Group::addChild(child);
   
   if(result)
   {
      ossimPlanetNode* layerNode = dynamic_cast<ossimPlanetNode*>(child);
      if(layerNode) layerNode->setLayer(ossimPlanetLayer::findLayer(child));
      notifyAddChild(child);
      setRedrawFlag(true);
      nodeAdded(child);
   }
   
   return result;
}

bool ossimPlanetNode::insertChild( unsigned int index, Node *child )
{
   bool result = osg::Group::insertChild(index, child);
   
   if(result)
   {
      ossimPlanetNode* layerNode = dynamic_cast<ossimPlanetNode*>(child);
      if(layerNode) layerNode->setLayer(ossimPlanetLayer::findLayer(child));
      notifyAddChild(child);
      setRedrawFlag(true);
      nodeAdded(child);
   }
   return result;
}

bool ossimPlanetNode::replaceChild( Node *origChild, Node* newChild )
{
   osg::ref_ptr<osg::Node> origChildSave = origChild;
   osg::ref_ptr<osg::Node> newChildSave  = newChild;
   bool result = osg::Group::replaceChild(origChild, newChild);
   if(result)
   {
      ossimPlanetNode* layerNode = dynamic_cast<ossimPlanetNode*>(newChild);
      if(layerNode)
      {
         layerNode->setLayer(ossimPlanetLayer::findLayer(this));
      }
      notifyAddChild(newChild);
      notifyRemoveChild(origChild);
      setRedrawFlag(true);
      nodeRemoved(origChildSave.get());
      nodeAdded(newChildSave.get());
   }
   
   return result;
}

bool ossimPlanetNode::removeChildren(unsigned int pos,unsigned int numChildrenToRemove)
{
   if (pos<getNumChildren() && numChildrenToRemove>0)
   {
      unsigned int endOfRemoveRange = pos+numChildrenToRemove;
      if (endOfRemoveRange>getNumChildren())
      {
         endOfRemoveRange=getNumChildren();
      }
      osg::Node* node = 0;
      ossim_uint32 idx = pos;
      for(;idx<endOfRemoveRange;++idx)
      {
         node = getChild(idx);
         if(node)
         {
            notifyRemoveChild(node);
            nodeRemoved(node);
         }
      }
   }
   bool result = osg::Group::removeChildren(pos, numChildrenToRemove);
   if(result)
   {
      setRedrawFlag(true);
   }
   
   return result;
}

void ossimPlanetNode::setRedrawFlag(bool flag)//(bool flag, bool passToParentFlag)
{
   bool changed = false;
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetNodeRedrawPropertyMutex);
      changed = (theRedrawFlag != flag);
      theRedrawFlag = flag;
   }
   if(flag)
   {
      ossimPlanetLayer* l = layer();
      if(l&&(l!=this))
      {
         l->setRedrawFlag(flag);
      }
   }
   if(changed&&flag)
   {
      notifyNeedsRedraw();
   }
      
#if 0
   if(!dynamic_cast<ossimPlanetLayer*>(this))
   {
      if(layer())
      {
         layer()->setRedrawFlag(flag, passToParentFlag);
         return;
      }
   }
   bool changed = false;
   {
      OpenThreads::ScopedWriteLock lock(thePlanetNodeRedrawPropertyMutex);
      changed = (theRedrawFlag != flag);
      theRedrawFlag = flag;
   }
   // notify just one time on Redraw flag set to true
   if(changed)
   {
      if(flag)
      {
         notifyNeedsRedraw();
      }
      if(passToParentFlag)
      {
         //OpenThreads::ScopedReadLock lock(thePlanetNodePropertyMutex);
         ossim_uint32 idx = 0;
         ossim_uint32 idxMax = getNumParents();
         for(idx = 0; idx < idxMax; ++idx)
         {
            ossimPlanetNode* layerNode = dynamic_cast<ossimPlanetNode*>(getParent(idx));
            if(layerNode)
            {
               layerNode->setRedrawFlag(flag, passToParentFlag);
            }
         }
      }
   }
#endif
}


ossimPlanetNode* ossimPlanetNode::findNode(osg::NodePath& currentNodePath)
{
   if(currentNodePath.empty())
   {
      return 0;
   }
   for(osg::NodePath::reverse_iterator itr = currentNodePath.rbegin();
       itr != currentNodePath.rend();
       ++itr)
   {
      ossimPlanetNode* node = dynamic_cast<ossimPlanetNode*>(*itr);
      if (node) 
      {
         return node;
      }
   }
   
   return 0;
}

void ossimPlanetNode::notifyPropertyChanged(ossimPlanetNode* node, const ossimString& name)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx = 0;
   ossim_uint32 upper = theCallbackList.size();
   
   for(idx = 0; idx < upper; ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->propertyChanged(node, name); 
      }
   }
}

void ossimPlanetNode::notifyDestructing(ossimPlanetNode* node)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx = 0;
   ossim_uint32 upper = theCallbackList.size();
   
   for(idx = 0; idx < upper; ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->destructingNode(node); 
      }
   }   
}

void ossimPlanetNode::notifyNeedsRedraw()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx = 0;
   ossim_uint32 upper = theCallbackList.size();
   
   for(idx = 0; idx < upper; ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->needsRedraw(this); 
      }
   }   
}

void ossimPlanetNode::notifyAddChild(osg::ref_ptr<osg::Node> node)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   
   ossim_uint32 idx = 0;
   ossim_uint32 upper = theCallbackList.size();
   
   for(idx = 0; idx < upper; ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->nodeAdded(node.get());
      }
   }
}

void ossimPlanetNode::notifyRemoveChild(osg::ref_ptr<osg::Node> node)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   
   ossim_uint32 idx = 0;
   ossim_uint32 upper = theCallbackList.size();
   
   for(idx = 0; idx < upper; ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->nodeRemoved(node.get());
      }
   }
}
