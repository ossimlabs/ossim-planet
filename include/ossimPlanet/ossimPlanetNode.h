#ifndef ossimPlanetNode_HEADER
#define ossimPlanetNode_HEADER
#include <osg/Group>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetLookAt.h>
#include <ossimPlanet/ossimPlanetCallback.h>
#include <ossimPlanet/ossimPlanetAction.h>
#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetVisitors.h>
#include <ossim/base/ossimString.h>
#include <OpenThreads/ReadWriteMutex>

class ossimPlanetNode;
class OSSIMPLANET_DLL ossimPlanetNodeCallback : public ossimPlanetCallback
{
public:
   virtual void propertyChanged(ossimPlanetNode* /*node*/,
                                const ossimString& /*name*/){}
   virtual void destructingNode(ossimPlanetNode* /*node*/){}
   virtual void needsRedraw(ossimPlanetNode* /*node*/){}
   virtual void nodeAdded(osg::Node* /*node*/){}
   virtual void nodeRemoved(osg::Node* /*node*/){}
   
protected:
};


class ossimPlanetLayer;
class OSSIMPLANET_DLL ossimPlanetNode : public osg::Group,
                                        public ossimPlanetCallbackListInterface<ossimPlanetNodeCallback>,
													 public ossimPlanetActionReceiver
{
public:
   typedef std::vector<osg::ref_ptr<ossimPlanetNode> > NodeListType;

   ossimPlanetNode();
   virtual ~ossimPlanetNode();
	static void remove(osg::Node* node);
	virtual void setLayer(ossimPlanetLayer* layer)
	{
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
		theLayer = layer;
	}
	ossimPlanetLayer* layer()
	{
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
		return theLayer;
	}
	const ossimPlanetLayer* layer()const
	{
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
		return theLayer;
	}
   virtual bool addChild( Node *child );
   virtual bool insertChild( unsigned int index, Node *child );
   virtual bool replaceChild( Node *origChild, Node* newChild );
   bool removeChildren(unsigned int pos,unsigned int numChildrenToRemove);
   bool enableFlag()
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      return theEnableFlag;
   }
   virtual void setEnableFlag(bool flag)
   {
      bool changed = flag!=enableFlag();
      {
         std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
         theEnableFlag = flag;
      }
      if(changed) setRedrawFlag(true);
      notifyPropertyChanged(this, "enableFlag");
   }
   bool intersectFlag()const
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      
      return theIntersectableFlag;
   }
   virtual void setIntersectFlag(bool flag)
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      theIntersectableFlag = flag;
   }
   virtual void setRedrawFlag(bool flag);
   bool redrawFlag()const
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodeRedrawPropertyMutex);
      return theRedrawFlag;
   }
   const ossimString& id()const
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      return theId;
   }
   virtual void setId(const ossimString& id)
   {
      {
         std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
         theId = id;
      }
      notifyPropertyChanged(this, "id");
   }
   virtual void setName(const ossimString& value)
   {
      {
         std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
         theName = value;
      }
      notifyPropertyChanged(this, "name");
   }
   const ossimString& name()const
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      return theName;
   }
   void name(ossimString& value)
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      value = theName;
   }
   virtual void setDescription(const ossimString& value)
   {
      {
         std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
         theDescription = value;
      }
      notifyPropertyChanged(this, "description");
   }
   const ossimString& description()const
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      return theDescription;
   }
   void description(ossimString& value)const
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      value = theDescription;
   }
   virtual osg::ref_ptr<ossimPlanetLookAt> lookAt()
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      return theLookAt;
   }
   virtual const osg::ref_ptr<ossimPlanetLookAt> lookAt()const
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      return theLookAt;
   }
   virtual void lookAt(ossimPlanetLookAt& result)const
   {
      std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
      if(theLookAt.valid())
      {
         result = *theLookAt;
      }
   }
   virtual void setLookAt(osg::ref_ptr<ossimPlanetLookAt> value)
   {
      {
         std::lock_guard<std::recursive_mutex> lock(thePlanetNodePropertyMutex);
         theLookAt = value;
      }
      notifyPropertyChanged(this, "LookAt");
   }
   virtual void traverse(osg::NodeVisitor& nv);
   virtual void execute(const ossimPlanetAction& action);
   
   static ossimPlanetNode* findNode(osg::NodePath& currentNodePath);

   
   virtual void notifyPropertyChanged(ossimPlanetNode* node, const ossimString& name);
   virtual void notifyDestructing(ossimPlanetNode* node);
   virtual void notifyNeedsRedraw();
   virtual void notifyAddChild(osg::ref_ptr<osg::Node>    node);
   virtual void notifyRemoveChild(osg::ref_ptr<osg::Node> node);
	
protected:
   virtual void nodeAdded(osg::Node* /*node*/){}
	virtual void nodeRemoved(osg::Node* /*node*/){}
   mutable std::recursive_mutex thePlanetNodeRedrawPropertyMutex;
   mutable std::recursive_mutex thePlanetNodePropertyMutex;
   
   bool        theEnableFlag;
   bool        theIntersectableFlag;
   ossimString theId;
   ossimString theName;
   ossimString theDescription;
   bool        theRedrawFlag;
   osg::ref_ptr<ossimPlanetLookAt> theLookAt;
	ossimPlanetLayer* theLayer;
};

class OSSIMPLANET_DLL ossimPlanetNodeReraiseCallback : public ossimPlanetNodeCallback
{
public:
	ossimPlanetNodeReraiseCallback(ossimPlanetNode* node)
	:theNode(node)
	{
	}
	void setNode(ossimPlanetNode* node)
	{
		theNode = node;
	}
	ossimPlanetNode* node()
	{
		return theNode;
	}
	const ossimPlanetNode* node()const
	{
		return theNode;
	}
   virtual void propertyChanged(ossimPlanetNode* node,
                                const ossimString& name)
	{
		if(theNode)
		{
			theNode->notifyPropertyChanged(node, name);
		}
	}
   virtual void destructingNode(ossimPlanetNode* node)
	{
		if(theNode)
		{
			theNode->notifyDestructing(node);
		}
	}
   virtual void needsRedraw(ossimPlanetNode* /*node*/)
	{
		if(theNode)
		{
			theNode->notifyNeedsRedraw();
		}
	}
   virtual void nodeAdded(osg::Node* node)
	{
		if(theNode)
		{
			theNode->notifyAddChild(node);
		}
	}
   virtual void nodeRemoved(osg::Node* node)
	{
		if(theNode)
		{
			theNode->notifyRemoveChild(node);
		}
	}
protected:
	ossimPlanetNode* theNode;
	osg::ref_ptr<ossimPlanetLookAt> theLookAt;
};


#endif
