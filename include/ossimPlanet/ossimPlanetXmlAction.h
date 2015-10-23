#ifndef ossimPlanetXmlAction_HEADER
#define ossimPlanetXmlAction_HEADER
#include <ossimPlanet/ossimPlanetAction.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimXmlNode.h>
#include <osg/ref_ptr>

class OSSIMPLANET_DLL ossimPlanetXmlAction : public ossimPlanetAction
{
public:
   ossimPlanetXmlAction(const ossimString& code = ossimString(), 
                        const ossimString& originatingFederate = defaultOrigin());
	ossimPlanetXmlAction(ossimRefPtr<ossimXmlNode> node,
								const ossimString& originatingFederate = defaultOrigin());
   ossimPlanetXmlAction(const ossimPlanetXmlAction& src)
   :ossimPlanetAction(src),
   theXmlNode(src.theXmlNode.valid()?(ossimXmlNode*)src.theXmlNode->dup():(ossimXmlNode*)0)
   {
      
   }
   virtual ossimPlanetAction* clone()const
   {
      return new ossimPlanetXmlAction(*this);
   }
   virtual ossimPlanetAction* cloneType()const
   {
      return new ossimPlanetXmlAction();
   }
   virtual ossimPlanetXmlAction* toXmlAction()
   {
      return this;
   }
   virtual const ossimPlanetXmlAction* toXmlAction()const
   {
      return this;
   }
   virtual void setTarget(const ossimString& value);
   virtual void setCommand(const ossimString& value);
	
   /**
    * Will allow one to set the source code for the derived actions
    *
    */
	virtual bool setSourceCode(const ossimString& code);
   ossimRefPtr<ossimXmlNode> xmlNode()
   {
      return theXmlNode;
   }
   const ossimRefPtr<ossimXmlNode> xmlNode()const
   {
      return theXmlNode;
   }
   void setXmlNode(ossimRefPtr<ossimXmlNode> code);
	
	/**
	 * Utility method for accessing the attribute id of the Action.
	 */
	ossimString id()const;
	ossimString name()const;
   
   	/**
	 * Utility method to duplicte a child of the action and maintain the same action parameters
	 */
	osg::ref_ptr<ossimPlanetXmlAction> duplicateChildAndMaintainAction(ossim_uint32 childIdx)const;
	
	/**
	 * Utility method to duplicate the child properties and promote them as the children of the Action.
	 * This is typically used when doing a set Action on a node.  The properties are promoted for the set actions
	 *
	 */
	osg::ref_ptr<ossimPlanetXmlAction> duplicateChildPropertiesAndMaintainAction(ossim_uint32 childIdx)const;
	
	bool hasChildren()const;
	virtual void print(std::ostream& out)const;
   virtual void read(std::istream& in);
   
protected:
   ossimRefPtr<ossimXmlNode> theXmlNode;
};

#endif
