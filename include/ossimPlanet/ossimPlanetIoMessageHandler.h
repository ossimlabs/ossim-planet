#ifndef ossimPlanetIoMessageHandler_HEADER
#define ossimPlanetIoMessageHandler_HEADER
#include <ossim/base/ossimString.h>
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <vector>
#include <ossimPlanet/ossimPlanetMessage.h>

class ossimPlanetIoMessageHandler : public osg::Referenced
{
public:
   ossimPlanetIoMessageHandler()
      :theName(""),
       theEnableFlag(true)
      {}
   virtual bool handleMessage(osg::ref_ptr<ossimPlanetMessage> message)=0;
   void setEnableFlag(bool flag)
      {
         theEnableFlag = flag;
      }
   bool enableFlag()const
      {
         return theEnableFlag;
      }

   void setName(const ossimString& name)
      {
         theName = name;
      }
   const ossimString& name()const
      {
         return theName;
      }
protected:
   ossimString  theName;
   bool theEnableFlag;
};

#endif

