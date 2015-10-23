#include <ossimPlanet/ossimPlanetAnnotationLayer.h>
#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanetVisitors.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetNodeRegistry.h>
#include <osgUtil/IntersectVisitor>
#include <algorithm>
#include <ossimPlanet/ossimPlanetXmlAction.h>
#include <OpenThreads/ScopedLock>


ossimPlanetAnnotationLayer::ossimPlanetAnnotationLayer()
{
   theStagingThreadQueue = new ossimPlanetOperationThreadQueue;
   theUpdateThreadQueue = new ossimPlanetOperationThreadQueue;
	theDefaultIconImage = new osg::Image;
	theDefaultIconImage->allocateImage(32, 32,  1, GL_RGBA, GL_UNSIGNED_BYTE);
	memset(theDefaultIconImage->data(), 255, theDefaultIconImage->getImageSizeInBytes());
	theDefaultIconTexture = new osg::Texture2D;
	theDefaultIconTexture->setImage(theDefaultIconImage.get());
	theDefaultIconTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture2D::NEAREST);
	theDefaultIconTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture2D::NEAREST);
	theDefaultIconTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
	theDefaultIconTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
	theDefaultIconTexture->setWrap(osg::Texture2D::WRAP_R, osg::Texture2D::CLAMP_TO_EDGE);
	theDefaultIconTexture->setDataVariance(osg::Object::STATIC);
	theDefaultFont = osgText::readFontFile("fonts/arial.ttf");

	//theDefaultIconTexture->setResizeNonPowerOfTwoHint(true);
}

ossimPlanetAnnotationLayer::~ossimPlanetAnnotationLayer()
{
   theStagingThreadQueue = 0;
}

void ossimPlanetAnnotationLayer::traverse(osg::NodeVisitor& nv)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theGraphMutex);
   switch(nv.getVisitorType())
	{
		case osg::NodeVisitor::UPDATE_VISITOR:
		{
         {
				OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theNodesToRemoveListMutex);
           ossimPlanetNode::NodeListType::iterator iter = theNodesToRemoveList.begin();
            while(iter != theNodesToRemoveList.end())
            {
               ossimPlanetNode::remove((*iter).get());
               ++iter;
            }
            theNodesToRemoveList.clear();
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

void ossimPlanetAnnotationLayer::execute(const ossimPlanetAction& action)
{
   std::string command = action.command();
   const ossimPlanetXmlAction* xmlAction = action.toXmlAction();
   if(command == "Add")
   {
      // we will use the add action to get the child
      if(xmlAction&&xmlAction->xmlNode().valid()) 
      {
         const ossimXmlNode::ChildListType& children = xmlAction->xmlNode()->getChildNodes();
         ossim_uint32 idx = 0;
         for(idx = 0; idx < children.size(); ++idx)
         {
            ossimString tag = children[idx]->getTag();
            ossimString parentId = children[idx]->getAttributeValue("parentId");
            ossimString id = children[idx]->getChildTextValue("id");
            if(!id.empty())
            {
               ossimPlanetLayerNameIdSearchVisitor nv("", id);
               accept(nv);
               if(nv.node().get())
               {
                  // already an id exists with that value so we return and don't add
                  return;
               }
            }
            osg::ref_ptr<ossimPlanetNode> layerNode;
            if(tag != "Group")
            {
               layerNode = ossimPlanetNodeRegistry::instance()->create(tag);
            }
            if(!layerNode.valid()&&(tag == "Group"))
            {
               if(children[idx]->getAttributeValue("groupType")=="feature")
               {
                  layerNode = new ossimPlanetAnnotationGroupNode();
               }
            }
            if(layerNode.valid())
            {
					osg::ref_ptr<ossimPlanetXmlAction> tempAction = xmlAction->duplicateChildAndMaintainAction(idx);
					tempAction->setCommand("Set");
					layerNode->setLayer(this);
               layerNode->execute(*tempAction);
            }
				osg::ref_ptr<ossimPlanetAnnotationLayerNode> annotationNode = dynamic_cast<ossimPlanetAnnotationLayerNode*>(layerNode.get());
            if(annotationNode.valid())
            {
               OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theGraphMutex);
               if(!annotationNode->isStaged())
               {
                  theStagingThreadQueue->add(new ossimPlanetAnnotationLayer::Stager(annotationNode.get()));
               }
               ossimPlanetNode* parentNode=0;
               if(!parentId.empty())
               {
                  ossimPlanetLayerNameIdSearchVisitor nv("", parentId);
                  accept(nv);
                  parentNode = dynamic_cast<ossimPlanetNode*>(nv.node().get());
               }
               
               if(parentNode)
               {
                  parentNode->addChild(annotationNode.get());
               }
               else
               {
                  addChild(annotationNode.get());
               }
					setRedrawFlag(true);					
            }
         }
      }
   }
   else if(command == "Set")
   {
      ossimString actionId = xmlAction->id();
      if(!actionId.empty())
      {
         ossimPlanetLayerNameIdSearchVisitor idSearch;
         idSearch.setId(actionId);
         accept(idSearch);
         ossimPlanetNode* node = dynamic_cast<ossimPlanetNode*>(idSearch.node().get());
         if(node)
         {
            node->execute(action);
         }
      }
      else
      {
         ossim_uint32 idx;
         const ossimXmlNode::ChildListType& children = xmlAction->xmlNode()->getChildNodes();
         for(idx = 0; idx < children.size(); ++idx)
         {
            ossimString id;
            if(children[idx]->getAttributeValue(id, "id"))
            {
               ossimPlanetLayerNameIdSearchVisitor idSearch;
               idSearch.setId(id);
               accept(idSearch);
               ossimPlanetNode* node = dynamic_cast<ossimPlanetNode*>(idSearch.node().get());
               if(node)
               {
                  osg::ref_ptr<ossimPlanetXmlAction> tempAction = xmlAction->duplicateChildAndMaintainAction(idx);
                  tempAction->setCommand("Set");
                  node->execute(*tempAction);
               }
            }
         }
      }
   }
	else if(command == "Get")
	{
	}
	else if(command == "Remove")
	{
		ossimString id; 
		ossimString name;
		// do short form
		// <Remove id="">
		if(!xmlAction->hasChildren())
		{
         ossimString id = xmlAction->id(); 
         ossimString name = xmlAction->name();
			if(!id.empty()||!name.empty())
			{
				removeByNameAndId(name, id);
			}
		}
		else
		{
			const ossimXmlNode::ChildListType& children = xmlAction->xmlNode()->getChildNodes();
			ossim_uint32 idx = 0;
			for(idx = 0; idx < children.size();++idx)
			{
				name = children[idx]->getAttributeValue("name");
				id   = children[idx]->getAttributeValue("id");
            if(name.empty()&&id.empty())
            {
               name = children[idx]->getChildTextValue("name");
               id   = children[idx]->getChildTextValue("id");
            }
				if(!name.empty()||!id.empty())
				{
					removeByNameAndId(name, id);
				}
			}
		}
	}
}

void ossimPlanetAnnotationLayer::removeByNameAndId(const ossimString& name, const ossimString& id)
{
   ossimPlanetLayerNameIdSearchVisitor nv(name, id);
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theGraphMutex);
   accept(nv);
   ossimPlanetNode* node = dynamic_cast<ossimPlanetNode*>(nv.node().get());
   if(node)
   {
		needsRemoving(node);
   }
}

void ossimPlanetAnnotationLayer::nodeAdded(osg::Node* /* node */)
{
}

void ossimPlanetAnnotationLayer::nodeRemoved(osg::Node* /* node */)
{
}

void ossimPlanetAnnotationLayer::needsRemoving(osg::Node* node)
{
	ossimPlanetNode* pNode = dynamic_cast<ossimPlanetNode*>(node);
	if(pNode)
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theNodesToRemoveListMutex);
		// keep it around for next update removal
		theNodesToRemoveList.push_back(pNode);
	}
	setRedrawFlag(true);
}

