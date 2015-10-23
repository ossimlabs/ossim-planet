#ifndef ossimPlanetAnnotationLayer_HEADER
#define ossimPlanetAnnotationLayer_HEADER
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetRefBlock.h>
#include <osg/Image>
#include <osg/Texture2D>
#include <osg/ref_ptr>
#include <osgText/Font>
#include <ossimPlanet/ossimPlanetAnnotationLayerNode.h>
#include <ossimPlanet/ossimPlanetOperation.h>
#include <queue>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>

class OSSIMPLANET_DLL ossimPlanetAnnotationLayer : public ossimPlanetLayer
{
public:
	class Stager : public ossimPlanetOperation
		{
		public:
			Stager(ossimPlanetAnnotationLayerNode* node)
			:theNode(node)
			{
				
			}
			virtual void run()
			{
				if(theNode.valid())
				{
					theNode->stage();
				}
			}
		protected:
			osg::ref_ptr<ossimPlanetAnnotationLayerNode> theNode;
		};
	ossimPlanetAnnotationLayer();
	virtual ~ossimPlanetAnnotationLayer();
   virtual osg::Object* cloneType() const { return new ossimPlanetAnnotationLayer(); }
   virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const ossimPlanetAnnotationLayer *>(obj)!=0; }
   virtual const char* className() const { return "ossimPlanetAnnotationLayer"; } 
   virtual const char* libraryName() const { return "ossimPlanet"; }
   virtual void traverse(osg::NodeVisitor& nv);
	virtual void execute(const ossimPlanetAction& a);
	osg::ref_ptr<osg::Texture2D> defaultIconTexture()
	{
		return theDefaultIconTexture;
	}
   osg::ref_ptr<osgText::Font> defaultFont()
	{
		return theDefaultFont.get();
	}
   const osg::ref_ptr<osgText::Font> defaultFont()const
	{
		return theDefaultFont.get();
	}
   void removeByNameAndId(const ossimString& name, const ossimString& id);
   
	osg::ref_ptr<ossimPlanetOperationThreadQueue> stagingThreadQueue()
	{
		return theStagingThreadQueue;
	}
	osg::ref_ptr<ossimPlanetOperationThreadQueue> updateThreadQueue()
	{
		return theUpdateThreadQueue;
	}
protected:
   virtual void nodeAdded(osg::Node* node);
	virtual void nodeRemoved(osg::Node* node);
	virtual void needsRemoving(osg::Node* node);
	osg::ref_ptr<osgText::Font> theDefaultFont;
	osg::ref_ptr<osg::Image> theDefaultIconImage;
	osg::ref_ptr<osg::Texture2D> theDefaultIconTexture;
	mutable ossimPlanetReentrantMutex theGraphMutex;
   
	
	ossimPlanetReentrantMutex theNodesToRemoveListMutex;
	NodeListType theNodesToRemoveList;
	
   osg::ref_ptr<ossimPlanetOperationThreadQueue> theStagingThreadQueue;
   osg::ref_ptr<ossimPlanetOperationThreadQueue> theUpdateThreadQueue;
   
};

#endif
