#ifndef ossimPlanetAction_HEADER
#define ossimPlanetAction_HEADER

// message to make an ActionReceiver do something
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <ossimPlanet/ossimPlanetExport.h>
#include <osg/Referenced>
#include <ossim/base/ossimString.h>

class ossimPlanetDestinationCommandAction;
class ossimPlanetXmlAction;
class OSSIMPLANET_DLL ossimPlanetAction : public osg::Referenced
{
public:
   ossimPlanetAction(const ossimString& originatingFederate=ossimString());
   ossimPlanetAction(const ossimPlanetAction& action)
	:osg::Referenced(),
        theOrigin(action.theOrigin),
	theTarget(action.theTarget),
   theCommand(action.theCommand),
   theSourceCode(action.theSourceCode)
   {
      
   }
   virtual ~ossimPlanetAction()
   {}
   virtual ossimPlanetDestinationCommandAction* toDestinationCommandAction()
   {
      return 0;
   }
   virtual const ossimPlanetDestinationCommandAction* toDestinationCommandAction()const
   {
      return 0;
   }
   virtual ossimPlanetXmlAction* toXmlAction()
   {
      return 0;
   }
   virtual const ossimPlanetXmlAction* toXmlAction()const
   {
      return 0;
   }
   virtual ossimPlanetAction* clone()const=0;
   virtual ossimPlanetAction* cloneType()const=0; 
   /**
    * Will allow one to set the source code for the derived actions
    *
    */
	virtual bool setSourceCode(const ossimString& code)=0;
	void sourceCode(ossimString& code)const
   {
      code = theSourceCode;
   }
	const ossimString& sourceCode()const
   {
      return theSourceCode;
   }
	void target(ossimString& value)  const
   {
      value = theTarget;
   }
   const ossimString& target()const
   {
      return theTarget;
   }
   virtual void setTarget(const ossimString& value)
   {
      theTarget = value;
   }
   
   void command(ossimString& value) const
   {
      value = theCommand;
   }
   const ossimString& command() const
   {
      return theCommand;
   }
   virtual void setCommand(const ossimString& value)
   {
      theCommand = value;
   }
	const ossimString& origin() const
	{ 
      return theOrigin; 
   }
	
	virtual void setOrigin(const ossimString& originatingFederate)
	{ 
      theOrigin = originatingFederate; 
   }
	
   friend std::ostream& operator<<(std::ostream& s, const ossimPlanetAction& a)
   {
      a.print(s);
      return s;
   }
	
   friend std::istream& operator>>(std::istream& s, ossimPlanetAction& a)
   {
      a.read(s);
      return s;
   }
	virtual void print(std::ostream& out)const = 0;
   virtual void read(std::istream& in) = 0;
   
	void printError(const char* message) const;
	void printError(const ossimString& message) const;
	// print error message containing message followed by action source to
	// stderr. usually called in ActionReceiver::execute() error handling.
	
	/**
	 * post the action to the threaded action queue
	 */ 
	virtual void post() const;
	
	virtual void execute() const;
	// execute yourself
	
	virtual void allExecute() const;
	// tell all machines to execute yourself (distributed sims only)
	
	virtual void tellExecute(const ossimString& destination) const;
	// tell another machine to execute yourself (distributed sims only)
	
//   static ossimPlanetAction createNestedAction(const ossimString& argument,
//                                               bool wrapWithBracketsFlag = true)
//   {
//      return createNestedAction(":dummy", "dummy", argument, wrapWithBracketsFlag);
//   }
//	static ossimPlanetAction createNestedAction(const ossimString& receiverName,
//													        const ossimString& actionName,
//															  const ossimString& argument,
//                                               bool wrapWithBracketsFlag = true)
//	{
//      if(!wrapWithBracketsFlag)
//      {
//         return ossimPlanetAction(receiverName + " " + actionName + " " + argument);
//      }
//		return ossimPlanetAction(receiverName + " " + actionName + " { " + argument + " }");
//	}
	// streaming operators
	
protected:
	// federate this Action originated from (for distributed sims)
	ossimString theOrigin;
	
   ossimString theTarget;
   ossimString theCommand;
   ossimString theSourceCode;
	static const ossimString& defaultOrigin();
	// default origin for newly created Actions
};



#endif

