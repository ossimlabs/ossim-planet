#ifndef ossimPlanetDestinationCommandAction_HEADER
#define ossimPlanetDestinationCommandAction_HEADER
#include <ossimPlanet/ossimPlanetAction.h>
#include <ossim/base/ossimString.h>

class OSSIMPLANET_DLL ossimPlanetDestinationCommandAction : public ossimPlanetAction
{
public:
	ossimPlanetDestinationCommandAction(const ossimString& sourcecode = ossimString(), 
                                       const ossimString& originatingFederate = defaultOrigin());
   ossimPlanetDestinationCommandAction(const ossimPlanetDestinationCommandAction& src)
   :ossimPlanetAction(src),
   theTokens(src.theTokens),
	theArgCount(src.theArgCount)
   {
   }
   virtual ossimPlanetDestinationCommandAction* toDestinationCommandAction()
   {
      return this;
   }
   virtual const ossimPlanetDestinationCommandAction* toDestinationCommandAction()const
   {
      return this;
   }
   virtual ossimPlanetAction* clone()const
   {
      return new ossimPlanetDestinationCommandAction(*this);
   }
   virtual ossimPlanetAction* cloneType()const
   {
      return new ossimPlanetDestinationCommandAction();
   }
   ossimString argListSourceCode() const;
	virtual bool setSourceCode(const ossimString& code);
  
   virtual void setTarget(const ossimString& targetPath)
   {
      ossimPlanetAction::setTarget(targetPath);
      if(theTokens.size())
      {
         theTokens[0] = targetPath.string();
      }
   }
   
   virtual void setCommand(const ossimString& value)
   {
      ossimPlanetAction::setCommand(value);
      if(theTokens.size()>1)
      {
         theTokens[1] = value.string();
      }
   }
	unsigned int argCount() const
	{
		return theArgCount;
	}
	
	const std::string& arg(unsigned int i) const
	{
		assert(i >= 1 && i <= argCount());
		return theTokens[i+1];
	}
   void setTargetCommandArg(const ossimString& target, 
                            const ossimString& command, 
                            const ossimString& arg)
   {
      theArgCount = 1;
      theTokens.resize(3);
      theTokens[0] = target.string();
      theTokens[1] = command.string();
      theTokens[2] = arg.string();
   }
	ossimString argListSource() const;
	// source code of the argument list
	virtual void print(std::ostream& out)const;
   virtual void read(std::istream& in);
   
protected:
	std::vector<std::string> theTokens;
	unsigned int theArgCount;
	
	static const char theWhitespace[];
	// characters considered to be whitespace
};
#endif
