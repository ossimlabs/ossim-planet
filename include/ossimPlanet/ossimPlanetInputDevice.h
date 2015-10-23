#ifndef ossimPlanetInputDevice_HEADER
#define ossimPlanetInputDevice_HEADER

#include <ossimPlanet/ossimPlanetExport.h>
#include <osg/Referenced>
// device driver interface for user input devices

class OSSIMPLANET_DLL ossimPlanetInputDevice :public osg::Referenced
{
public:
    ossimPlanetInputDevice() {}
    virtual ~ossimPlanetInputDevice() {}
    
    virtual void processInput() = 0;
    // update interaction valuators and execute actions
    
};


#endif
