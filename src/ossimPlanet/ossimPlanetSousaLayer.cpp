#include <ossimPlanet/ossimPlanetSousaLayer.h>
#include <ossimPlanet/ossimPlanetAction.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetLand.h>
#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <ossimPlanet/ossimPlanetTextureLayerRegistry.h>
#include <ossimPlanet/ossimPlanetViewer.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimPreferences.h>
#include <osgGA/EventVisitor>
#include <ostream>
#include <ossim/base/ossimNotify.h>

class ossimPlanetSousaLayer::ossimPlanetSousaXmlActionOperation : public ossimPlanetOperation
{
public:
   ossimPlanetSousaXmlActionOperation(ossimPlanetSousaLayer* layer, ossimPlanetXmlAction* action)
   :theLayer(layer),
   theAction(action)
   {
   }
protected:
   virtual void run()
   {
      theLayer->threadedXmlExecute(*theAction);
   }
   ossimPlanetSousaLayer* theLayer;
   osg::ref_ptr<ossimPlanetXmlAction> theAction;
};

ossimPlanetSousaLayer::ossimPlanetSousaLayer()
:theConnectionName(""),
 theConnectionIp(""),
 theConnectionPort(""),
 theConnectionPortType(""),
theIoThreadReceiverName(":io"),
theZuiInitializedFlag(false),
theViewChangedFlag(false),
theViewMessageRate(30), // default to 30 times per second
theCameraTestDelay(0),
theNeedToAddImageGroupFlag(true)
{
   theXmlActionThreadQueue = new ossimPlanetOperationThreadQueue();
   theAnnotationLayer      = new ossimPlanetAnnotationLayer;
	theNodeCallback         = new ossimPlanetNodeReraiseCallback(this);
	theAnnotationLayer->addCallback(theNodeCallback.get());
   ossimPlanetLayer::addChild(theAnnotationLayer.get());
   theSousaImageGroup = new ossimPlanetTextureLayerGroup;
   theSousaImageGroup->setName("OSSIM Socket Image Group");
}

ossimPlanetSousaLayer::~ossimPlanetSousaLayer()
{
   theXmlActionThreadQueue->removeAllOperations();
   theXmlActionThreadQueue->cancelCurrentOperation();
   theXmlActionThreadQueue = 0;
	theAnnotationLayer->removeCallback(theNodeCallback.get());
   theAnnotationLayer = 0;
}


void ossimPlanetSousaLayer::removeImage(const ossimString& name, const ossimString& id)
{
	if(!theSousaImageGroup.valid()) return;
   osg::ref_ptr<ossimPlanetTextureLayer> namedLayer;
	if(!name.empty()&&!id.empty())
	{
      namedLayer = theSousaImageGroup->findLayerByNameAndId(name, id);
	}
	else if(!name.empty())
	{
		namedLayer = theSousaImageGroup->findLayerByName(name);
	}
	else if(!id.empty())
	{
		namedLayer = theSousaImageGroup->findLayerById(id);
	}
	if(namedLayer.valid())
	{
		namedLayer->remove();
	}
}

void ossimPlanetSousaLayer::setIdentity(const ossimString& tempUserName, 
													 const ossimString& tempDomain)
{
	theUserName = tempUserName;
	theDomain   = tempDomain;
	sendIdentityMessage();
}

const ossimString& ossimPlanetSousaLayer::username()const
{
	return theUserName;
}

const ossimString& ossimPlanetSousaLayer::domain()const
{
	return theDomain;
}

void ossimPlanetSousaLayer::xmlExecute(const ossimPlanetXmlAction &a)
{
   if(theXmlActionThreadQueue.valid())
   {
      ossim_uint32 idx = 0;
      std::string command = a.command();
      if(command == "Add")
      {
         if(a.xmlNode().valid())
         {
            const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();  
            for(idx = 0;idx<children.size();++idx)
            {
               osg::ref_ptr<ossimPlanetXmlAction> action                  = a.duplicateChildAndMaintainAction(idx);
               osg::ref_ptr<ossimPlanetSousaXmlActionOperation> operation = new ossimPlanetSousaXmlActionOperation(this, action.get());
               
               ossimString tag = children[idx]->getTag();
               ossimString groupType = children[idx]->getAttributeValue("groupType");
               if((tag == "Image")||
                  (groupType=="groundTexture"))
               {
                  ossimString id = children[idx]->getChildTextValue("id");
                  operation->setId(id);
                  operation->setName("Adding id = " + id);
                  theXmlActionThreadQueue->add(operation.get());
               }
               else if((tag == "Placemark")||
                       (groupType == "feature"))
               {
                  ossimString id = children[idx]->getChildTextValue("id");
                  ossimString name = children[idx]->getChildTextValue("name");
                  operation->setId(id);
                  operation->setName(name);
                  theXmlActionThreadQueue->add(operation.get());
               }
               else if(tag == "Group")
               {
                  ossimString id = children[idx]->getChildTextValue("id");
                  ossimString name = children[idx]->getChildTextValue("name");
                  operation->setId(id);
                  operation->setName(name);
                  ossimString groupType = children[idx]->getAttributeValue("groupType");
                  theXmlActionThreadQueue->add(operation.get());
               }
               else
               {
                  theXmlActionThreadQueue->add(operation.get());
               }
            }
         }
      }
      else if(command == "Remove")
      {
         const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();  
         if(children.empty())
         {
            osg::ref_ptr<ossimPlanetXmlAction> action                  = (ossimPlanetXmlAction*)a.clone();
            osg::ref_ptr<ossimPlanetSousaXmlActionOperation> operation = new ossimPlanetSousaXmlActionOperation(this, action.get());
            ossimString id   = a.xmlNode()->getAttributeValue("id");
            if(!id.empty())
            {
               theXmlActionThreadQueue->removeById(id).valid();
            }
            theXmlActionThreadQueue->add(operation.get());
         }
         else
         {
            for(idx = 0;idx<children.size();++idx)
            {
               osg::ref_ptr<ossimPlanetXmlAction> action                  = a.duplicateChildAndMaintainAction(idx);
               osg::ref_ptr<ossimPlanetSousaXmlActionOperation> operation = new ossimPlanetSousaXmlActionOperation(this, action.get());
               ossimString id   = children[idx]->getAttributeValue("id");
               if(id.empty())
               {
                  id = children[idx]->getChildTextValue("id");
               }
               if(!id.empty())
               {
                  theXmlActionThreadQueue->removeById(id).valid();
               }
               theXmlActionThreadQueue->add(operation.get());
            }
         }
      }
      else if(command == "Set")
      {
         theXmlActionThreadQueue->add(new ossimPlanetSousaXmlActionOperation(this,(ossimPlanetXmlAction*)a.clone()));
      }
   }
}

void ossimPlanetSousaLayer::threadedXmlExecute(const ossimPlanetXmlAction &a)
{
  // OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theXmlActionThreadQueueMutex);

	std::string command = a.command();
	if(command == "Add")
	{
      if(a.xmlNode().valid())
      {
         ossim_uint32 idx = 0;
         const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();  
         for(idx = 0;idx<children.size();++idx)
         {
				ossimString tag = children[idx]->getTag();
				ossimString groupType = children[idx]->getAttributeValue("groupType");
				if(tag == "ClientSocket")
				{
					setClientSocket(children[idx].get());
				}
				else if((tag == "Image")||
						  (groupType=="groundTexture"))
				{
					addImageLayer(children[idx].get());
				}
				else if((tag == "Placemark")||
                    (groupType == "feature"))
				{
               if(theAnnotationLayer.valid())
               {
                  ossimRefPtr<ossimXmlNode> xmlAction = new ossimXmlNode;
						
                  ossimPlanetXmlAction tempAction;
                  xmlAction->setTag("Add");
                  xmlAction->addChildNode((ossimXmlNode*)children[idx]->dup());
                  tempAction.setXmlNode(xmlAction.get());
                  
                  theAnnotationLayer->execute(tempAction);
               }
				}
				else if(tag == "Identity")
				{
					ossimString username = children[idx]->getChildTextValue("username");
					ossimString domain   = children[idx]->getChildTextValue("domain");
					
					setIdentity(username, domain);
				}
				else if(tag == "Group")
				{
               ossimString groupType = children[idx]->getAttributeValue("groupType");
					if(groupType == "groundTexture")
					{
					}
					else if(groupType == "feature")
					{
					}
				}
			}
		}
	}
   else if(command == "Set")
   {
      ossim_uint32 idx = 0;
      ossimString id;
      ossimString name;
      const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();
      if(children.size() > 0)
      {
         for(idx = 0;idx<children.size();++idx)
         {
            ossimString tag = children[idx]->getTag();
				if(tag == "ClientSocket")
				{
					setClientSocket(children[idx].get());
				}
            else if(tag == "Camera")
            {
					OpenThreads::ScopedLock<OpenThreads::Mutex> testDelayLock(theCameraTestDelayMutex);
               // let's set a 2 frame delay just in case the draw thread is already in the process of visiting us. So on the second visit
               // we will do a view chnge test
               //
               theCameraTestDelay = 10;
               ossimRefPtr<ossimXmlNode> xmlActionNode = new ossimXmlNode;
					ossimRefPtr<ossimXmlNode> dupNode = (ossimXmlNode*)(children[idx]->dup());
					ossimPlanetXmlAction tempAction;
					xmlActionNode->setTag("Set");
					xmlActionNode->addAttribute("target", ":navigator");
               xmlActionNode->addChildNode(dupNode.get());
               tempAction.setXmlNode(xmlActionNode.get());
					tempAction.execute();
               setRedrawFlag(true);
               theZuiInitializedFlag = true;
            }
				else if(tag == "Identity")
				{
					ossimString username = children[idx]->getChildTextValue("username");
					ossimString domain   = children[idx]->getChildTextValue("domain");
					
					setIdentity(username, domain);
				}
				else if(tag == "viewMessageRate")
				{
					theViewMessageRate = children[idx]->getText().toDouble();
				}
            else if(tag =="Placemark")
            {
               ossimPlanetXmlAction tempAction;
               ossimRefPtr<ossimXmlNode> xmlAction = new ossimXmlNode;
               xmlAction->setTag("Set");
               xmlAction->addChildNode((ossimXmlNode*)children[idx]->dup());
               tempAction.setXmlNode(xmlAction.get());
               theAnnotationLayer->execute(tempAction);
            }
            else
            {
               id   = a.xmlNode()->getAttributeValue("id");
               if(!id.empty())
               {
                  ossimPlanetXmlAction tempAction;
                  ossimRefPtr<ossimXmlNode> xmlAction = new ossimXmlNode;
                  xmlAction->setTag("Set");
                  xmlAction->addAttribute("id", id);
                  xmlAction->addChildNode((ossimXmlNode*)children[idx]->dup());
                  tempAction.setXmlNode(xmlAction.get());
                  theAnnotationLayer->execute(tempAction);
               }
            }
         }
      }
   }
	else if(command == "Remove")
	{
      if(a.xmlNode().valid())
      {
			ossim_uint32 idx = 0;
			ossimString id;
			ossimString name;
         const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();
			if(children.size() > 0)
			{
				for(idx = 0;idx<children.size();++idx)
				{
					ossimString tag = children[idx]->getTag();
               // check attribute section first for searchables
               name = children[idx]->getAttributeValue("name");
               id   = children[idx]->getAttributeValue("id");
               ossimString groupType   = children[idx]->getAttributeValue("groupType");
               
               if(name.empty()&&id.empty())
               {
                  name = children[idx]->getChildTextValue("name");
                  id   = children[idx]->getChildTextValue("id");
               }
               
					if((tag == "Image")||
						(groupType == "groundTexture"))
					{
						removeImage(name, id);
					}
					else if(tag == "Placemark")
					{
                  if(theAnnotationLayer.valid())
                  {
                     ossimRefPtr<ossimXmlNode> xmlAction = new ossimXmlNode;
                     ossimPlanetXmlAction tempAction;
                     xmlAction->setTag("Remove");
                     xmlAction->addChildNode((ossimXmlNode*)children[idx]->dup());
                     tempAction.setXmlNode(xmlAction.get());
                     theAnnotationLayer->execute(tempAction);
                  }
					}
					else 
					{
						removeImage(name, id);
						// do a generalized find and remove
					}
				}
			}
			else
			{
				name = "";
				id   = "";
				// do generalized remove if the Remove has an id attribute
				//
				// basically allow one to do <Remove id="....."/>
				//
				if(a.xmlNode()->getAttributeValue(id, "id")||
					a.xmlNode()->getAttributeValue(name, "name"))
				{
					removeImage(name, id);
				}
            // wil also send to the Annotation layer to find the ID
            //
            if(theAnnotationLayer.valid())
            {
               theAnnotationLayer->execute(a);
            }
			}
		}
	}
}

void ossimPlanetSousaLayer::execute(const ossimPlanetAction &a)
{
   const ossimPlanetXmlAction* xmlAction = a.toXmlAction();

	if(xmlAction)
	{
		xmlExecute(*xmlAction);
	}
}

void ossimPlanetSousaLayer::closeConnections()
{
   std::ostringstream out;

	if(!theConnectionName.empty())
	{
		out<< "<Remove target=\"" << theIoThreadReceiverName << "\"" << "id=\"" << theConnectionName <<":"<<theConnectionPort << "\"/>";
	}
	else
	{
		out<< "<Remove target=\"" << theIoThreadReceiverName << "\"" << "id=\"" << theConnectionIp <<":"<<theConnectionPort << "\"/>";
	}
   ossimPlanetXmlAction(out.str()).execute();

   theConnectionName = "";
   theConnectionIp = "";
   theConnectionPort = "";
   theConnectionPortType = "";
	if(theAnnotationLayer.valid())
	{
		if(theAnnotationLayer->getNumChildren() > 0)
		{
			theAnnotationLayer->removeChild(0, theAnnotationLayer->getNumChildren());
		}
	}
}

void ossimPlanetSousaLayer::traverse(osg::NodeVisitor& nv)
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theUpdateMutex);
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if(!thePlanet)
         {
            thePlanet = ossimPlanet::findPlanet(this);
         }
         if(thePlanet)
         {
            if(theZuiInitializedFlag)
            {
               if(theViewChangedFlag)
               {
                  sendViewMessage();
               }
            }
         }
         break;
      }
      case osg::NodeVisitor::EVENT_VISITOR:
      {
         osgGA::EventVisitor* ev = dynamic_cast<osgGA::EventVisitor*>(&nv);
         if(ev)
         {
            ossimPlanetViewer* viewer = dynamic_cast<ossimPlanetViewer*>(ev->getActionAdapter());
            if(theNeedToAddImageGroupFlag)
            {
               viewer->addImageTexture(theSousaImageGroup.get());
               theNeedToAddImageGroupFlag = false;
            }
            if(viewer&&viewer->currentCamera())
            {
               osg::Vec3d currentEye(viewer->currentCamera()->lat(),
                                     viewer->currentCamera()->lon(),
                                     viewer->currentCamera()->altitude());
               osg::Vec3d currentHpr(viewer->currentCamera()->heading(),
                                     viewer->currentCamera()->pitch(),
                                     viewer->currentCamera()->roll());
               OpenThreads::ScopedLock<OpenThreads::Mutex> testDelayLock(theCameraTestDelayMutex);
               if(theCameraTestDelay>0)
               {
                  theEyePosition    = currentEye;
                  theEyeOrientation = currentHpr;
                  --theCameraTestDelay;
                  setRedrawFlag(true);
               }
               else if(!ossim::almostEqual(theEyePosition[0], currentEye[0])||
                       !ossim::almostEqual(theEyePosition[1], currentEye[1])||
                       !ossim::almostEqual(theEyePosition[2], currentEye[2])||
                       !ossim::almostEqual(theEyeOrientation[0], currentHpr[0])||
                       !ossim::almostEqual(theEyeOrientation[1], currentHpr[1])||
                       !ossim::almostEqual(theEyeOrientation[2], currentHpr[2]))
               {
                 theEyePosition    = currentEye;
                  theEyeOrientation = currentHpr;
                  if(!theViewChangedFlag)
                  {
                     theViewChangeStart = osg::Timer::instance()->tick();
                  }
                  theViewChangedFlag = true;
               }
               
            }
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

void ossimPlanetSousaLayer::addImageLayer(const ossimXmlNode* imageObject)
{
	ossimString parentId    = imageObject->getAttributeValue("parentId");;

#if 0
	//ossimNotify(ossimNotifyLevel_WARN) << "Testing notifications" << std::endl;
	if(!thePlanet) return;
	osg::ref_ptr<ossimPlanetLand> land = thePlanet->land();
	if(!land.valid())
	{
		return;
	}
	if(!land->referenceLayer().valid()) return;
	osg::ref_ptr<ossimPlanetTextureLayerGroup> layerGroup;
	osg::ref_ptr<ossimPlanetTextureLayer> namedLayer;
	if(!parentId.empty())
	{
		namedLayer = land->referenceLayer()->findLayerById(parentId);
	}
	if(namedLayer.valid())
	{
		layerGroup = namedLayer->asGroup();
	}
	
	if(!layerGroup.valid())
	{
		layerGroup = land->referenceLayer().get();
	}
	if(!layerGroup.valid())
	{
		return;
	}
#endif
	osg::ref_ptr<ossimPlanetTextureLayerGroup> layerGroup = theSousaImageGroup.get();
	osg::ref_ptr<ossimPlanetTextureLayer> namedLayer;
	if(!parentId.empty())
	{
		namedLayer = layerGroup->findLayerById(parentId);
	}
	if(namedLayer.valid())
	{
		layerGroup = namedLayer->asGroup();
	}
	
//	if(!layerGroup.valid())
//	{
//		layerGroup = land->referenceLayer().get();
//	}
	if(!layerGroup.valid())
	{
		return;
	}
	addImageToGroup(theSousaImageGroup.get(), imageObject);
}

void ossimPlanetSousaLayer::addImageToGroup(ossimPlanetTextureLayerGroup* group, const ossimXmlNode* imageObject)
{
	if(!group) return;
	ossimString description = imageObject->getChildTextValue("description");
	ossimString id          = imageObject->getChildTextValue("id");
	ossimString name        = imageObject->getChildTextValue("name");
	ossimFilename filename  = imageObject->getChildTextValue("filename").trim();
	ossimString tag = imageObject->getTag();
	if(tag=="Image")
	{
		// test if the filename for a mapping
		if(theArchive.valid()&&
			theArchive->archiveMappingEnabled() )
		{
			//ossimNotify(ossimNotifyLevel_WARN) << "Archive mapping is valid and enabled" << std::endl;
			
			filename = theArchive->matchPath(filename);
			
			//ossimNotify(ossimNotifyLevel_WARN) << "Archive Filename: " << filename << std::endl;
			//ossimNotify(ossimNotifyLevel_WARN) << "Archive Result: " << result << std::endl;
		}
		//ossimNotify(ossimNotifyLevel_WARN) << "Testing the file: " << filename << std::endl;
		if(!filename.empty()&&filename.exists())
		{
         if(!group->findLayerById(id))
         {
//            ossimNotify(ossimNotifyLevel_WARN) << "The file is not empty" << std::endl;
//            ossimNotify(ossimNotifyLevel_WARN) << "Filename:" << filename << std::endl;
            
            osg::ref_ptr<ossimPlanetTextureLayer> layer = ossimPlanetTextureLayerRegistry::instance()->createLayer(filename);
            if(layer.valid())
            {
               layer->setId(id);
               layer->setDescription(description);
               layer->setName(name);
               group->addTop(layer.get());
            }
         }
         else
         {
//            ossimNotify(ossimNotifyLevel_WARN) << "Failed test and coudn't add image" << std::endl;
         }
      }
      else
      {
//         ossimNotify(ossimNotifyLevel_INFO) << "Couldn't add file:" << filename <<"--"<< std::endl;
      }
	}
	else if(tag == "Group")
	{
		const ossimXmlNode::ChildListType& children = imageObject->getChildNodes();
		ossimPlanetTextureLayerGroup* newGroup = new ossimPlanetTextureLayerGroup;
		newGroup->setId(id);
		newGroup->setDescription(description);
		newGroup->setName(name);
		group->addTop(newGroup);
		ossim_int32 idx = 0;
		ossim_int32 n = (ossim_int32)children.size();
		if(n > 0)
		{
			for(idx = n-1; idx >= 0;--idx)
			{
				addImageToGroup(newGroup, children[idx].get());
			}
		}
	}
}


void ossimPlanetSousaLayer::setConnection(const ossimString& name,
                                          const ossimString& ip,
                                          const ossimString& port,
                                          const ossimString& portType)
{
	closeConnections();
   theConnectionName = name;
   theConnectionIp = ip;
   theConnectionPort = port;
   theConnectionPortType = portType;
   std::ostringstream out;
   out
   << "<Open target=\"" << theIoThreadReceiverName << "\">"
   <<    "<ClientSocket>"
   <<       "<name>" << name << "</name>"
   <<       "<ip>"   << ip   << "</ip>"
   <<       "<port>" << port << "</port>"
   <<       "<portType>" << portType << "</portType>"
   <<    "</ClientSocket>"
   << "</Open>";
   ossimPlanetXmlAction(out.str()).execute();
	sendIdentityMessage();   
}

void ossimPlanetSousaLayer::setClientSocket(const ossimXmlNode* node)
{
   ossimString name, ip, port, portType;
	node->getChildTextValue(name, "name");
	node->getChildTextValue(ip, "ip");
	node->getChildTextValue(port, "port");
	node->getChildTextValue(portType, "portType");
   setConnection(name, ip, port, portType);
}

void ossimPlanetSousaLayer::sendViewMessage()
{
	if(theViewMessageRate <FLT_EPSILON)
	{
		theViewChangedFlag = false;
		return;
	}
	if(theConnectionIp.empty()&&theConnectionPort.empty()) return;
	bool canSend = osg::Timer::instance()->delta_s(theViewChangeStart, osg::Timer::instance()->tick()) > (1.0/theViewMessageRate);
	
   if(canSend)
   {
      std::ostringstream out;
      out
      << "<SendMessage target='" << theIoThreadReceiverName << "'" << "ioTargetId='" << theConnectionName <<":"<<theConnectionPort << "'" << "id='" << "View'>"
      <<   "<Set target=\":idolbridge\">"
      <<     "<Camera vref=\"wgs84\">"
      <<          "<longitude>" << ossimString::toString(theEyePosition[1]) << "</longitude>"
      <<          "<latitude>" << ossimString::toString(theEyePosition[0]) << "</latitude>"
      <<          "<altitude>" << ossimString::toString(theEyePosition[2]) << "</altitude>"
      <<          "<heading>"<< ossimString::toString(theEyeOrientation[0])<<"</heading>"
      <<          "<pitch>"<< ossimString::toString(theEyeOrientation[1])<<"</pitch>"
      <<          "<roll>"<< ossimString::toString(theEyeOrientation[2])<<"</roll>"
      <<          "<altitudeMode>absolute</altitudeMode>"
      <<     "</Camera>"
      <<   "</Set>"
      << "</SendMessage>";
      ossimPlanetXmlAction(out.str()).execute();
		theViewChangedFlag = false;
   }	
	if(theViewChangedFlag)
	{
		setRedrawFlag(true);
	}
}

void ossimPlanetSousaLayer::setArchive(osg::ref_ptr<ossimPlanetArchive> archive)
{
	theArchive = archive;
}

void ossimPlanetSousaLayer::sendIdentityMessage()
{
   if(!theConnectionIp.empty()&&
      !theConnectionPort.empty())
   {
		std::ostringstream out;
      std::ostringstream outIdentityConnectionHeader;
      out
      << "<SendMessage target=\"" << theIoThreadReceiverName << "\"" << "id=\"" << theConnectionName <<":"<<theConnectionPort << "\" forceSend=\"true\">"
      <<   "<Set>"
      <<     "<Identity>"
      <<          "<username>" << theUserName << "</username>";
		
		if(!theDomain.empty())
		{
			out <<          "<domain>" << theDomain<< "</domain>";
		}
      out <<     "</Identity>"
      <<   "</Set>"
      << "</SendMessage>";
      
      outIdentityConnectionHeader
      << "<Set target=\"" << theIoThreadReceiverName << "\"" << "id=\"" << theConnectionName <<":"<<theConnectionPort <<"\">"
      <<   "<connectionHeader><![CDATA[<Set>"
      <<     "<Identity>"
      <<          "<username>" << theUserName << "</username>";
		
		if(!theDomain.empty())
		{
			outIdentityConnectionHeader <<          "<domain>" << theDomain<< "</domain>";
		}
      outIdentityConnectionHeader
      <<     "</Identity>"
      <<   "</Set>]]></connectionHeader>"
      << "</Set>";
      
      ossimPlanetXmlAction(out.str()).execute();
      ossimPlanetXmlAction(outIdentityConnectionHeader.str()).execute();
	}
}
