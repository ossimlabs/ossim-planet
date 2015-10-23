#ifndef ossimPlanetSousaLayer_HEADER
#define ossimPlanetSousaLayer_HEADER
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetTextureLayerGroup.h>
#include <ossimPlanet/ossimPlanetAnnotationLayer.h>
#include <ossimPlanet/ossimPlanetXmlAction.h>
#include <OpenThreads/Mutex>
#include <map>
#include <ossimPlanet/ossimPlanetArchive.h>


class ossimPlanet;
class ossimPlanetKmlLayer;
class OSSIMPLANET_DLL ossimPlanetSousaLayer : public ossimPlanetLayer
{
public:
   ossimPlanetSousaLayer();
   virtual ~ossimPlanetSousaLayer();
	
   /**
    * For now this will remove any current connection and then replace with the passed in
    * connection.  All previous entities are cleared from the scene graph.
    */ 
   void setConnection(const ossimString& name,
                      const ossimString& ip,
                      const ossimString& port,
                      const ossimString& portType);
   
   /**
    * add <object definition>
    *
    * for annotation adds see the ossimPlanetAnnotationLayer execute.
    *
    */
   virtual void execute(const ossimPlanetAction &a);
   void closeConnections();
   virtual void traverse(osg::NodeVisitor& nv);

   // added by russc 5/5/08
   void setArchive(osg::ref_ptr<ossimPlanetArchive> archive);
   
	void setIdentity(const ossimString& tempUserName, 
						  const ossimString& tempDomain);
	const ossimString& username()const;
	const ossimString& domain()const;
	
protected:
   class ossimPlanetSousaXmlActionOperation;
   friend class ossimPlanetSousaXmlActionOperation;
   virtual void xmlExecute(const ossimPlanetXmlAction &a);
   virtual void threadedXmlExecute(const ossimPlanetXmlAction &a);
	void removeImage(const ossimString& name, 
						  const ossimString& id);
	void removeAnnotation(const ossimString& name, 
						       const ossimString& id);
	void addImageLayer(const ossimXmlNode* imageObject);
	void addImageToGroup(ossimPlanetTextureLayerGroup* group, const ossimXmlNode* imageObject);
   void setClientSocket(const ossimXmlNode* node);
   void sendViewMessage();
   void sendIdentityMessage();
	
   ossimString          theConnectionName;
   ossimString          theConnectionIp;
   ossimString          theConnectionPort;
   ossimString          theConnectionPortType;
   ossimString          theIoThreadReceiverName;

	ossimString          theUserName;
	ossimString          theDomain;
	
   bool                 theZuiInitializedFlag;
	bool                 theViewChangedFlag;
	double               theViewMessageRate;
	osg::Timer_t         theViewChangeStart;
   osg::Vec3d           theEyePosition;
   osg::Vec3d           theEyeOrientation;
	
	
   osg::ref_ptr<ossimPlanetAnnotationLayer>   theAnnotationLayer;
   osg::ref_ptr<ossimPlanetArchive>           theArchive;
   
	ossimPlanetReentrantMutex theCameraTestDelayMutex;
   ossim_int32 theCameraTestDelay;
   mutable ossimPlanetReentrantMutex theUpdateMutex;
   
   mutable ossimPlanetReentrantMutex theXmlActionThreadQueueMutex;
	
   osg::ref_ptr<ossimPlanetOperationThreadQueue> theXmlActionThreadQueue;
   
	osg::ref_ptr<ossimPlanetNodeReraiseCallback> theNodeCallback;
   
   osg::ref_ptr<ossimPlanetTextureLayerGroup> theSousaImageGroup;
   bool theNeedToAddImageGroupFlag;
};

#endif
