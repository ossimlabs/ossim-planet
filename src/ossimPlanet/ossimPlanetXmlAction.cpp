#include <ossimPlanet/ossimPlanetXmlAction.h>
#include <ossim/base/ossimXmlNode.h>
#include <ossim/base/ossimXmlDocument.h>
#include <sstream>

ossimPlanetXmlAction::ossimPlanetXmlAction(const ossimString& code, 
                                           const ossimString& originatingFederate)
:ossimPlanetAction(originatingFederate)
{
   setSourceCode(code);
}

ossimPlanetXmlAction::ossimPlanetXmlAction(ossimRefPtr<ossimXmlNode> code, 
                                           const ossimString& originatingFederate)
:ossimPlanetAction(originatingFederate)
{
	setXmlNode(code);
}

void ossimPlanetXmlAction::setTarget(const ossimString& value)
{
	ossimPlanetAction::setTarget(value);
	if(theXmlNode.valid())
	{
		if(theXmlNode->getAttributeValue("target") != value.c_str())
		{
			// add the attribute if not present already else change it
			//
			theXmlNode->setAttribute("target", value, true);
			std::ostringstream out;
			out << *theXmlNode;
			theSourceCode = out.str();
		}
	}
}

void ossimPlanetXmlAction::setCommand(const ossimString& value)
{
	ossimPlanetAction::setCommand(value);
	
	// Update the tag value if the command changes.
	if(theXmlNode.valid() &&(theXmlNode->getTag()!=value.c_str()))
	{
		theXmlNode->setTag(value);
		std::ostringstream out;
		out << *theXmlNode;
		theSourceCode = out.str();
	}
}

bool ossimPlanetXmlAction::setSourceCode(const ossimString& code)
{
   ossim_uint32 bytesRead = 0;
   ossimRefPtr<ossimXmlNode> node = 0;
   if(code[(size_t)0] == '<')
   {
      std::istringstream in(code);
      if(code[(size_t)1] == '?')
      {
         ossimRefPtr<ossimXmlDocument> doc = new ossimXmlDocument;
         if(doc->read(in))
         {
            node = doc->getRoot();
         }
      }
      else
      {
         node = new ossimXmlNode;
         if(!node->read(in))
         {
            node = 0;
         }
      }
      bytesRead = in.tellg();
   }
   // now setup the sourcecode that was read in
   //
   if(bytesRead > 0)
   {
      theSourceCode = ossimString(code.begin(),
                                  code.begin() + bytesRead);
   }
   
   setXmlNode(node.get());
   return node.valid();
}

void ossimPlanetXmlAction::setXmlNode(ossimRefPtr<ossimXmlNode> node)
{
   theXmlNode = node;
   if(theXmlNode.valid())
   {
      ossimString value;
      setCommand(theXmlNode->getTag());
      if(theXmlNode->getAttributeValue(value, "target"))
      {
         setTarget(value);
      }
      else
      {
         setTarget("");
      }
      if(theXmlNode->getAttributeValue(value, "origin"))
      {
         setOrigin(value);
      }
      else
      {
         setOrigin("");
      }
      std::ostringstream out;
      out << *theXmlNode;
      theSourceCode = out.str();
   }
   else
   {
      setTarget("");
      setOrigin("");
      theSourceCode = "";
   }
}

ossimString ossimPlanetXmlAction::id()const
{
	if(theXmlNode.valid())
	{
		return theXmlNode->getAttributeValue("id");
	}
	
	return "";
}

ossimString ossimPlanetXmlAction::name()const
{
	if(theXmlNode.valid())
	{
		return theXmlNode->getAttributeValue("name");
	}
	
	return "";
}

osg::ref_ptr<ossimPlanetXmlAction> ossimPlanetXmlAction::duplicateChildAndMaintainAction(ossim_uint32 childIdx)const
{
	osg::ref_ptr<ossimPlanetXmlAction> result;
	if(theXmlNode.valid())
	{
		const ossimXmlNode::ChildListType& children = theXmlNode->getChildNodes();
		if(childIdx < children.size())
		{
			ossimXmlNode::AttributeListType attributes;
			theXmlNode->duplicateAttributes(attributes);
			ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
			node->setAttributes(attributes);
			node->addChildNode((ossimXmlNode*)children[childIdx]->dup());
			node->setTag(theXmlNode->getTag());
			result =  new ossimPlanetXmlAction(node, origin());
			result->setTarget(target());
		}
	}	
	
	return result;
}

osg::ref_ptr<ossimPlanetXmlAction> ossimPlanetXmlAction::duplicateChildPropertiesAndMaintainAction(ossim_uint32 childIdx)const
{
	osg::ref_ptr<ossimPlanetXmlAction> result;
	if(theXmlNode.valid())
	{
		const ossimXmlNode::ChildListType& children = theXmlNode->getChildNodes();
		
		if(childIdx < children.size())
		{
			ossimString value;
			ossimXmlNode::ChildListType dupChildren;
			ossimXmlNode::AttributeListType attributes;
			theXmlNode->duplicateAttributes(attributes);
			ossimRefPtr<ossimXmlNode> propertyNode = new ossimXmlNode;
			children[childIdx]->duplicateChildren(dupChildren);
			propertyNode->setAttributes(attributes);
			propertyNode->setChildren(dupChildren);
			propertyNode->setTag(command());

			result = new ossimPlanetXmlAction(propertyNode,
														 origin());
		}	
	}
	
	return result;
}

bool ossimPlanetXmlAction::hasChildren()const
{
	bool result = false;
	
	if(theXmlNode.valid())
	{
		return theXmlNode->getChildNodes().size();
	}
	
	return false;
}

void ossimPlanetXmlAction::print(std::ostream& out)const
{
   if(theXmlNode.valid())
   {
      out << *theXmlNode;
   }
}

void ossimPlanetXmlAction::read(std::istream& in)
{
   ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
   if(node->read(in))
   {
      setXmlNode(theXmlNode.get());
   }
   else
   {
      setTarget(":");
      setCommand("#syntaxerror");
   }
}

