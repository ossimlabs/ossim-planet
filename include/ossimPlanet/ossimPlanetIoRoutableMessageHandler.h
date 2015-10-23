#ifndef ossimPlanetIoRoutableMessageHandler_HEADER
#define ossimPlanetIoRoutableMessageHandler_HEADER
#include <ossimPlanet/ossimPlanetIoMessageHandler.h>

class ossimPlanetIoRoutableMessageHandler : public ossimPlanetIoMessageHandler
{
public:
   ossimPlanetIoRoutableMessageHandler()
   {
         theName = "Routable Message Handler";
   }
   virtual bool handleMessage(osg::ref_ptr<ossimPlanetMessage> message);
};

#endif
