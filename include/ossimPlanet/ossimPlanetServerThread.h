#ifndef ossimPlanetServerThread_HEADER
#define ossimPlanetServerThread_HEADER
#include <queue>
#include "ossimPlanetExport.h"
#include <ossimPlanet/sg_socket.h>
#include <ossimPlanet/ossimPlanetServerMessageHandler.h>
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <OpenThreads/Thread>
#include<ossimPlanet/ossimPlanetReentrantMutex.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimString.h>


class OSSIMPLANET_DLL ossimPlanetServerThread : public osg::Referenced,
                                                public OpenThreads::Thread
{
public:
   ossimPlanetServerThread(ossim_uint32 maxQueueSize = 2048);
   virtual ~ossimPlanetServerThread();
   virtual void run();
   virtual int cancel();

   void setQueueMessagesFlag(bool flag);
   bool getQueueMessagesFlag()const;
   /**
    * Adds a port listener.
    *
    * @param host  The host ip address.  Usually will be localhost or empty string
    * @param port  The port number.
    * @param portType The port type can be udp or tcp listener
    */ 
   osg::ref_ptr<SGSocket> addServer(const ossimString& host,
                                    const ossimString& port,
                                    const ossimString& portType,
                                    char delimiter='\n');

   osg::ref_ptr<SGSocket> removeServer(ossim_uint32 idx);
   osg::ref_ptr<SGSocket> removeServer(const ossimString& host,
                                       const ossimString& port);
   
   ossim_uint32 getNumberOfServers()const;

   bool setServer(ossim_uint32 idx,
                  const ossimString& host,
                  const ossimString& port,
                  const ossimString& portType);
                  
   ossimString getPort(ossim_uint32 idx)const;
   ossimString getServer(ossim_uint32 idx)const;
   ossimString getPortType(ossim_uint32 idx)const;

   bool nextMessage(ossimString& msg);

   void addMessageHandler(osg::ref_ptr<ossimPlanetServerMessageHandler> messageHandler);
   void removeMessageHandler(osg::ref_ptr<ossimPlanetServerMessageHandler> messageHandler);
   ossim_uint32 getNumberOfMessageHandlers()const;
   
protected:
   int findMessageHandler(ossimPlanetServerMessageHandler* handler)const;
   
   mutable ossimPlanetReentrantMutex theMessageQueueMutex;
   mutable ossimPlanetReentrantMutex theChannelListMutex;
   mutable ossimPlanetReentrantMutex theMessageHandlerMutex;
   ossim_uint32               thePollingRatePerSecond;
   ossim_uint32               theMaxQueueSize;
   std::queue<ossimString>    theMessageQueue;
   std::vector<osg::ref_ptr<SGSocket> >    theChannelList;
   bool                       theDoneFlag;
   bool                       theStartedFlag;
   bool                       theQueueMessagesFlag;
   std::vector<osg::ref_ptr<ossimPlanetServerMessageHandler> > theMessageHandlerList;
};

#endif
