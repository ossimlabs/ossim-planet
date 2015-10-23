#include <ossimPlanet/ossimPlanetIoRoutableMessageHandler.h>
#include <ossimPlanet/ossimPlanetAction.h>
#include <ossimPlanet/ossimPlanetDestinationCommandAction.h>

bool ossimPlanetIoRoutableMessageHandler::handleMessage(osg::ref_ptr<ossimPlanetMessage> message)
{
   bool result = false;
   if(!enableFlag()) return result;
   if(!message->data().empty())
   {
      if(*(message->data().begin()) == ':')
      {
         result = true;
         ossimPlanetDestinationCommandAction(std::string(message->data().begin(),
                                                         message->data().end())).execute();
      }
   }
   
   return result;
}
