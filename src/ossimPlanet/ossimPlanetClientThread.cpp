#include <ossimPlanet/ossimPlanetClientThread.h>
#include <iostream>
#include <algorithm>

ossimPlanetClientConnection::ossimPlanetClientConnection(SGSocket* socket,
                                                         ossim_uint32 queueSize)
   :theSocket(socket),
    theMaxQueueSize(queueSize)
{
}

ossimPlanetClientConnection::ossimPlanetClientConnection()
   :theSocket(0),
    theMaxQueueSize(1024)
{
}

ossimPlanetClientConnection::~ossimPlanetClientConnection()
{
   if(theSocket)
   {
      theSocket->close();
      delete theSocket;
      theSocket = 0;
   }
   theMessageQueue.clear();
}


ossimString ossimPlanetClientConnection::getHost()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   ossimString result;
   if(theSocket)
   {
      result = theSocket->get_hostname();
   }

   return result;
}

ossimString ossimPlanetClientConnection::getPortString()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   ossimString result;

   if(theSocket)
   {
      result = theSocket->get_port_str();
   }

   return result;
}

ossimString ossimPlanetClientConnection::getPortType()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   ossimString result;

   if(theSocket)
   {
      result = theSocket->get_port_style();
   }

   return result;
}

void ossimPlanetClientConnection::getConnection(ossimString& host,
                                                ossimString& port,
                                                ossimString& portType)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   
   if(theSocket)
   {
      host     = theSocket->get_hostname();
      port     = theSocket->get_port_str();
      portType =  theSocket->get_port_style();
   }
}

bool ossimPlanetClientConnection::setConnection(const ossimString& host,
                                                const ossimString& port,
                                                const ossimString& portType)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   bool result = false;
   
   if(theSocket)
   {
      theSocket->setSocket(host, port, portType);
      result = theSocket->open(SG_IO_OUT);
   }

   return result;
}

void ossimPlanetClientConnection::addMessage(const ossimString& message)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageQueueMutex);
   if(theSocket)
   {
      theMessageQueue.push_back(message);
      if(theMessageQueue.size() >= theMaxQueueSize)
      {
         theMessageQueue.pop_front();
      }
   }
}

void ossimPlanetClientConnection::sendNextMessage()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock1(theMessageQueueMutex);
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(theMutex);

   if(!theMessageQueue.empty()&&theSocket)
   {
      ossimString message = popMessage();
      theSocket->writestring(message.c_str());
   }
}

bool ossimPlanetClientConnection::hasMessages()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageQueueMutex);

   return !theMessageQueue.empty();
}

void ossimPlanetClientConnection::clearQueue()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageQueueMutex);
   theMessageQueue.clear();
}

ossimString ossimPlanetClientConnection::popMessage()
{
   ossimString result;

   if(!theMessageQueue.empty())
   {
      result = theMessageQueue.front();
      theMessageQueue.pop_front();
   }

   return result;
}
  
SGSocket* ossimPlanetClientConnection::getSocket()
{
   return theSocket;
}

void ossimPlanetClientConnection::setSocket(SGSocket* socket)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   if(theSocket != socket)
   {
      theSocket->close();
      delete theSocket;

      theSocket = socket;
   }
}

ossimPlanetClientThread::ossimPlanetClientThread()
   :theStartedFlag(false),
    theDoneFlag(false)
{
   theBlock = new ossimPlanetRefBlock();
}

ossimPlanetClientThread::~ossimPlanetClientThread()
{
   
}

void ossimPlanetClientThread::run()
{
   if(theStartedFlag) return;

   ossim_uint32 idx = 0;
   theStartedFlag = true;
   theDoneFlag    = false;
   while(!theDoneFlag)
   {
      theBlock->block();
      if(theDoneFlag) return;
      bool hasMessages = false;
      theConnectionListMutex.lock();
      for(idx = 0; idx < theClientConnectionList.size(); ++idx)
      {
         if(theClientConnectionList[idx]->hasMessages())
         {
            theClientConnectionList[idx]->sendNextMessage();
            hasMessages = true;
         }
      }
      if(!hasMessages)
      {
         theBlock->set(false);
      }
      theConnectionListMutex.unlock();
      YieldCurrentThread();
   }
   
   theStartedFlag = false;
}

int ossimPlanetClientThread::cancel()
{
   theDoneFlag = true;
   theBlock->release();
   return OpenThreads::Thread::cancel();
}

osg::ref_ptr<ossimPlanetClientConnection>  ossimPlanetClientThread::newConnection(const ossimString& host,
                                                                                  const ossimString& port,
                                                                                  const ossimString& portType)
{
   SGSocket* socket = new SGSocket(host, port, portType);
   socket->open(SG_IO_OUT);

   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theConnectionListMutex);
   osg::ref_ptr<ossimPlanetClientConnection> connection = new ossimPlanetClientConnection(socket);
   theClientConnectionList.push_back(connection.get());

   return connection.get();
}

bool ossimPlanetClientThread::setConnection(ossim_uint32 idx,
                                            const ossimString& host,
                                            const ossimString& port,
                                            const ossimString& portType)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theConnectionListMutex);
   if(idx < theClientConnectionList.size())
   {
      return theClientConnectionList[idx]->setConnection(host, port, portType);
   }

   return false;
}

void ossimPlanetClientThread::removeConnection(osg::ref_ptr<ossimPlanetClientConnection> connection)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theConnectionListMutex);
   ossimPlanetClientThreadConnectionList::iterator iter = std::find(theClientConnectionList.begin(),
                                                                    theClientConnectionList.end(),
                                                                    connection.get());
   if(iter != theClientConnectionList.end())
   {
      theClientConnectionList.erase(iter);
   }
}

osg::ref_ptr<ossimPlanetClientConnection> ossimPlanetClientThread::removeConnection(ossim_uint32 idx)
{
   osg::ref_ptr<ossimPlanetClientConnection> result;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theConnectionListMutex);
   if(idx < theClientConnectionList.size())
   {
      result = theClientConnectionList[idx];
      theClientConnectionList.erase(theClientConnectionList.begin() + idx);
   }

   return result;
}


const osg::ref_ptr<ossimPlanetClientConnection> ossimPlanetClientThread::getConnection(ossim_uint32 idx)const
{
   osg::ref_ptr<ossimPlanetClientConnection> result;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theConnectionListMutex);
   if(idx < theClientConnectionList.size())
   {
      result = theClientConnectionList[idx];
   }

   return result;
}


void ossimPlanetClientThread::sendMessage(ossim_uint32 idx,
                                          const ossimString& message)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theConnectionListMutex);
   protectedSendMessage(idx, message);
   
   if(theStartedFlag)
   {
      protectedUpdateClientThreadBlock();
   }
   else
   {
      start();
   }
}

void ossimPlanetClientThread::protectedSendMessage(ossim_uint32 idx,
                                                   const ossimString& message)
{
   if(idx < theClientConnectionList.size())
   {
      theClientConnectionList[idx]->addMessage(message);

   }
}

void ossimPlanetClientThread::broadcastMessage(const ossimString& message)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theConnectionListMutex);
   ossim_uint32 idx;
   for(idx = 0; idx < theClientConnectionList.size();++idx)
   {
      protectedSendMessage(idx, message);
   }
   
   if(theStartedFlag)
   {
      protectedUpdateClientThreadBlock();
   }
   else
   {
      start();
   }
}

ossim_uint32 ossimPlanetClientThread::getNumberOfConnections()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theConnectionListMutex);
   return theClientConnectionList.size();
}

void ossimPlanetClientThread::updateClientThreadBlock()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theConnectionListMutex);
   protectedUpdateClientThreadBlock();
}

void ossimPlanetClientThread::protectedUpdateClientThreadBlock()
{
   if(!theStartedFlag) return;
   bool blockFlag = false;

   if(theClientConnectionList.size() > 0)
   {
      ossim_uint32 idx = 0;

      for(idx = 0; idx < theClientConnectionList.size(); ++idx)
      {
         if(theClientConnectionList[idx]->hasMessages())
         {
            blockFlag = true;
            break;
         }
      }
   }
   theBlock->set(blockFlag);   
}
