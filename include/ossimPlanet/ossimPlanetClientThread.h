#ifndef ossimPlanetClientThread_HEADER
#define ossimPlanetClientThread_HEADER
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <ossimPlanet/sg_socket.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include "ossimPlanetRefBlock.h"
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/sg_socket.h>
#include <queue>
#include <map>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
class OSSIMPLANET_DLL ossimPlanetClientConnection : public osg::Referenced
{
public:
   ossimPlanetClientConnection(SGSocket* socket,
                               ossim_uint32 queueSize=1024);
   ossimPlanetClientConnection();
   virtual ~ossimPlanetClientConnection();
   
   ossimString getHost()const;
   ossimString getPortString()const;
   ossimString getPortType()const;
   void getConnection(ossimString& host,
                      ossimString& port,
                      ossimString& portType);
   bool setConnection(const ossimString& host,
                      const ossimString& port,
                      const ossimString& portType);
   
   void addMessage(const ossimString& message);
   void sendNextMessage();
   bool hasMessages()const;
   void clearQueue();
   
   SGSocket* getSocket();
   void setSocket(SGSocket* socket);

protected:
   ossimString popMessage();
   mutable ossimPlanetReentrantMutex theMutex;
   mutable ossimPlanetReentrantMutex theMessageQueueMutex;
   SGSocket*                  theSocket;
   ossim_uint32               theMaxQueueSize;
   std::deque<ossimString>    theMessageQueue; 
};

class OSSIMPLANET_DLL ossimPlanetClientThread : public osg::Referenced,
                                                public OpenThreads::Thread
{

public:
   
   ossimPlanetClientThread();
   virtual ~ossimPlanetClientThread();
   virtual void run();
   virtual int cancel();

   /**
    * creates a conneciton and returns the id
    */ 
   osg::ref_ptr<ossimPlanetClientConnection> newConnection(const ossimString& host,
                                                           const ossimString& port,
                                                           const ossimString& portType);
   bool setConnection(ossim_uint32 idx,
                      const ossimString& host,
                      const ossimString& port,
                      const ossimString& portType);
   
   void removeConnection(osg::ref_ptr<ossimPlanetClientConnection> connection);
   osg::ref_ptr<ossimPlanetClientConnection> removeConnection(ossim_uint32 idx);
   const osg::ref_ptr<ossimPlanetClientConnection> getConnection(ossim_uint32 idx)const;

   void sendMessage(ossim_uint32 idx,
                    const ossimString& message);
   void broadcastMessage(const ossimString& message);
   ossim_uint32 getNumberOfConnections()const;

   
   void updateClientThreadBlock();
protected:
   void protectedSendMessage(ossim_uint32 idx,
                             const ossimString& message);
   void protectedUpdateClientThreadBlock();
   
   typedef std::vector<osg::ref_ptr<ossimPlanetClientConnection> > ossimPlanetClientThreadConnectionList;
   mutable ossimPlanetReentrantMutex            theConnectionListMutex;
   osg::ref_ptr<ossimPlanetRefBlock>  theBlock;
   bool                                  theStartedFlag;
   bool                                  theDoneFlag;
   ossimPlanetClientThreadConnectionList theClientConnectionList;
};
#endif
