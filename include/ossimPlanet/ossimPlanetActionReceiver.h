#ifndef ossimPlanetActionReciever_HEADER
#define ossimPlanetActionReciever_HEADER

// abstract superclass for objects that can receive Actions. 

/* #include <assert> */
#include <string>
#include "ossimPlanetAction.h"
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetActionReceiver {
public:
    virtual ~ossimPlanetActionReceiver();
    
    std::string name() const;
	// this receiver's name, the last name in its pathname
	
    const std::string& pathname() const
	// this receiver's full name
	{ return pathname_; }
	
    void setPathname(const std::string& newPath);
	
    void setPathnameAndRegister(const std::string& newPath);
	// call setPathname() and register *this with ActionRouter
	// assert(newPath[0] == ':')
    
    virtual void execute(const ossimPlanetAction& a) = 0;
	// execute the given action
    
protected:
    
    std::string pathname_;
	// this receiver's full name
};



#endif
