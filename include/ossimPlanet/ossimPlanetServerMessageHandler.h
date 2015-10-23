#ifndef ossimPlanetServerMessageHandler_HEADER
#define ossimPlanetServerMessageHandler_HEADER
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/sg_socket.h>

class ossimPlanetServerMessageHandler : public osg::Referenced
{
public:
   ossimPlanetServerMessageHandler()
      :theEnableFlag(true)
   {
   }
   virtual bool handleMessage(const ossimString& message,
                              osg::ref_ptr<SGIOChannel> iochannel)=0;
   void setEnableFlag(bool flag)
   {
      theEnableFlag = flag;
   }
   bool getEnableFalg()const
   {
      return theEnableFlag;
   }
   
protected:
      bool theEnableFlag;
};

#endif
