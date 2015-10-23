#ifndef ossimPlanetVisitors_HEADER
#define ossimPlanetVisitors_HEADER
#include <ossimPlanet/ossimPlanetExport.h>
#include <osg/Texture>
#include <osg/Texture2D>
#include <osg/StateSet>
#include <osg/Node>
#include <iostream>
#include <osg/NodeVisitor>
#include <ossim/base/ossimString.h>
#include <osgUtil/UpdateVisitor>

class OSSIMPLANET_DLL ossimSetNonPowerOfTwoTextureVisitor : public osg::NodeVisitor
{
public:
   ossimSetNonPowerOfTwoTextureVisitor()
      :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
   {
      theResizePowerOfTwoHint = false;
      theMinFilterMode = osg::Texture2D::LINEAR;
      theMagFilterMode = osg::Texture2D::LINEAR;
      theSWrapMode     = osg::Texture2D::CLAMP_TO_EDGE;
      theTWrapMode     = osg::Texture2D::CLAMP_TO_EDGE;
      theRWrapMode     = osg::Texture2D::CLAMP_TO_EDGE;
      theDataVariance  = osg::Object::STATIC;
   }
   virtual void apply(osg::Node& node)
   {
      osg::StateSet* set = node.getStateSet();
      if(set)
      {
         osg::Texture* texture = dynamic_cast<osg::Texture*>(set->getTextureAttribute(0,
                                                                                      osg::StateAttribute::TEXTURE));
         if(texture)
         {
/*             std::cout << GL_CLAMP <<", " << GL_CLAMP_TO_EDGE <<", " << GL_CLAMP_TO_BORDER_ARB << ", " << GL_REPEAT << "\n"; */
/*             std::cout <<"<" << texture->getWrap(osg::Texture2D::WRAP_S) <<", " */
/*                       <<texture->getWrap(osg::Texture2D::WRAP_T) << ">\n"; */
            texture->setFilter(osg::Texture::MIN_FILTER, theMinFilterMode);
            texture->setFilter(osg::Texture::MAG_FILTER, theMagFilterMode);
            texture->setWrap(osg::Texture2D::WRAP_S, theSWrapMode);
            texture->setWrap(osg::Texture2D::WRAP_T, theTWrapMode);
            texture->setWrap(osg::Texture2D::WRAP_R, theRWrapMode);
            texture->setDataVariance(theDataVariance);
            texture->setResizeNonPowerOfTwoHint(theResizePowerOfTwoHint);
         }
      }
      traverse(node);
   }
protected:
   bool theResizePowerOfTwoHint;
   osg::Texture::FilterMode theMinFilterMode;
   osg::Texture::FilterMode theMagFilterMode;
   osg::Texture::WrapMode   theSWrapMode;
   osg::Texture::WrapMode   theTWrapMode;
   osg::Texture::WrapMode   theRWrapMode;
   osg::Object::DataVariance theDataVariance;
};

class OSSIMPLANET_DLL ossimPlanetLayerNameIdSearchVisitor : public osg::NodeVisitor
{
public:
   ossimPlanetLayerNameIdSearchVisitor()
   :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
   {
   }
   ossimPlanetLayerNameIdSearchVisitor(const ossimString& name, const ossimString& id)
   :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
   theName(name),
   theId(id)
   {
   }
   void setName(const ossimString& value)
   {
      theName = value;
   }
   const ossimString& name()const
   {
      return theName;
   }
   void setId(const ossimString& value)
   {
      theId = value;
   }
   const ossimString& id()const
   {
      return theId;
   }
   osg::ref_ptr<osg::Node> node()
   {
      return theNode.get();
   }
   const osg::ref_ptr<osg::Node> node()const
   {
      return theNode.get();
   }
   virtual void apply(osg::Node& node);
protected:
   ossimString theName;
   ossimString theId;
   osg::ref_ptr<osg::Node> theNode;
};

class OSSIMPLANET_DLL ossimPlanetTraverseCallback : public osg::NodeCallback
{
	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
	{
		if(node)
		{
			node->traverse(*nv);
		}
	}
};


/**
 * Used to determine if a node in the graph needs refreshing.  It will query the refresh flag while its
 * going through the update phase and set an internal flag.  Note, ossimPlanetViewer's updateTraversal
 * method override uses this information to help determine if it's in a static state
 */
class OSSIMPLANET_DLL ossimPlanetUpdateVisitor :  public osgUtil::UpdateVisitor
{
public:
   ossimPlanetUpdateVisitor();
   /**
    * Virtual method override that calls the base implementation but also sets/clears the RedrawFlag
    * for next traversal
    */
   virtual void reset();
   
   /**
    * @return the value of the redraw flag.  True means we need to redraw
    */
   bool redrawFlag()const;
protected:
   virtual void apply(osg::Node& node);       
   virtual void apply(osg::Group& node);      
   bool theRedrawFlag;
};
#endif
