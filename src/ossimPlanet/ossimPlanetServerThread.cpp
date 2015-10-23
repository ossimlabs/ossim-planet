#include <ossimPlanet/ossimPlanetServerThread.h>
#include <OpenThreads/ScopedLock>
#include <iostream>
#include <osg/Timer>

ossimPlanetServerThread::ossimPlanetServerThread(ossim_uint32 maxQueueSize)
   :thePollingRatePerSecond(60),
    theMaxQueueSize(maxQueueSize),
    theDoneFlag(false),
    theStartedFlag(false),
    theQueueMessagesFlag(false)
{
}

ossimPlanetServerThread::~ossimPlanetServerThread()
{
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theChannelList.size();++idx)
   {
      if(theChannelList[idx].valid())
      {
         theChannelList[idx]->close();
         theChannelList[idx] = 0;
      }
   }
   theChannelList.clear();
}

void ossimPlanetServerThread::run()
{
   if(theStartedFlag) return;
   theStartedFlag = true;
   theDoneFlag    = false;
   while(!theDoneFlag)
   {
      std::vector<char> buf(SG_IO_MAX_MSG_SIZE*2);
      ossim_uint32 idx = 0;
      std::string msg;
      bool needToSleep = true;
      theChannelListMutex.lock();
      for(idx = 0; idx < theChannelList.size(); ++idx)
      {
         int bytesRead = theChannelList[idx]->readline(&buf.front(), buf.size()>>1);
         if(bytesRead > 0)
         {
            needToSleep = false;
            ossimString s(buf.begin(),
                          buf.begin()+bytesRead);
            s = s.trim();
            if(s!="")
            {
               // let's first try to see if the message could be handled immediately
               OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageHandlerMutex);
               ossim_uint32 idx = 0;
               bool handledFlag = false;
               for(idx = 0; ((idx < theMessageHandlerList.size())&&!handledFlag); ++idx)
               {
                  handledFlag = theMessageHandlerList[idx]->handleMessage(s, theChannelList[idx].get());
               }

               // if it was not handled then just add it to the message queue.  Someone may be monitoring
               // the thread and do not have a message handler callback.
               //
               if(!handledFlag&&theQueueMessagesFlag)
               {
                  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageQueueMutex);
                  theMessageQueue.push(s);
                  if(theMessageQueue.size() > theMaxQueueSize)
                  {
                     theMessageQueue.pop();
                  }
               }
            }
         }
      }
      theChannelListMutex.unlock();

      if(needToSleep)
      {
         unsigned int hz = (unsigned int)((1.0/thePollingRatePerSecond)/1.0e-6);
         microSleep(hz);
      }
   }
   theStartedFlag = false;
}

int ossimPlanetServerThread::cancel()
{
   theDoneFlag = true;
   return OpenThreads::Thread::cancel();
}

void ossimPlanetServerThread::setQueueMessagesFlag(bool flag)
{
   theQueueMessagesFlag = flag;
}

bool ossimPlanetServerThread::getQueueMessagesFlag()const
{
   return theQueueMessagesFlag;
}

osg::ref_ptr<SGSocket> ossimPlanetServerThread::addServer(const ossimString& host,
                                                          const ossimString& port,
                                                          const ossimString& portType,
                                                          char delimiter)
{
   osg::ref_ptr<SGSocket> result = new SGSocket(host, port, portType);
   if(result->open(SG_IO_IN))
   {
      result->setReadlineDelimiter(delimiter);
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChannelListMutex);
      theChannelList.push_back(result);
   }
   else
   {
      result = 0;
   }
   if(result.valid())
   {
      if(theChannelList.size()&&!theStartedFlag)
      {
         start();
      }
   }
   return result;
}

osg::ref_ptr<SGSocket> ossimPlanetServerThread::removeServer(ossim_uint32 idx)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChannelListMutex);
   osg::ref_ptr<SGSocket> result;
   if(idx < theChannelList.size())
   {
      if(theChannelList[idx].valid())
      {
         theChannelList[idx]->close();
         result = theChannelList[idx];
         theChannelList.erase(theChannelList.begin()+idx);
      }
   }
   if((theChannelList.size()==0)&&(theStartedFlag))
   {
      cancel();
   }

   return result;
}

osg::ref_ptr<SGSocket> ossimPlanetServerThread::removeServer(const ossimString& host,
                                                             const ossimString& port)
{
   osg::ref_ptr<SGSocket> result;
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChannelListMutex);
      ossim_uint32 idx = 0;
      for(idx = 0; idx < theChannelList.size(); ++idx)
      {
         if((theChannelList[idx]->get_hostname() == host.string())&&
            (theChannelList[idx]->get_port_str() == port.string()))
         {
            theChannelList[idx]->close();
            result = theChannelList[idx];
            theChannelList.erase(theChannelList.begin()+idx);

            break;
         }
      }
   }
   if((theChannelList.size()==0)&&(theStartedFlag))
   {
      cancel();
   }
   
   return result;
}

bool ossimPlanetServerThread::setServer(ossim_uint32 idx,
                                        const ossimString& host,
                                        const ossimString& port,
                                        const ossimString& portType)
{
   bool result = false;
   
   if(idx < theChannelList.size())
   {
      if(theChannelList[idx].valid())
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChannelListMutex);
//          delete theChannelList[idx];
//          theChannelList[idx] = new SGSocket(host, port, portType);
         theChannelList[idx]->setSocket(host, port, portType);
         result = theChannelList[idx]->open(SG_IO_IN);
      }
   }
   if(result)
   {
      if(theChannelList.size()&&!theStartedFlag)
      {
         start();
      }
   }
   return result;
}

ossimString ossimPlanetServerThread::getPort(ossim_uint32 idx)const
{
   ossimString result;
   
   if(idx < theChannelList.size())
   {
      if(theChannelList[idx].valid())
      {
         result = theChannelList[idx]->get_port_str();
      }      
   }

   return result;
}

ossimString ossimPlanetServerThread::getServer(ossim_uint32 idx)const
{
   ossimString result;
   
   if(idx < theChannelList.size())
   {
      if(theChannelList[idx].valid())
      {
         result = theChannelList[idx]->get_hostname();
      }      
   }

   return result;
}

ossimString ossimPlanetServerThread::getPortType(ossim_uint32 idx)const
{
   ossimString result;
   
   if(idx < theChannelList.size())
   {
      if(theChannelList[idx].valid())
      {
         result = theChannelList[idx]->get_port_style();
      }      
   }

   return result;
}

ossim_uint32 ossimPlanetServerThread::getNumberOfServers()const
{
   return theChannelList.size();
}

bool ossimPlanetServerThread::nextMessage(ossimString& msg)
{
   bool result = false;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageQueueMutex);
   if(!theMessageQueue.empty())
   {
      msg = theMessageQueue.front();
      theMessageQueue.pop();
      result = true;
   }

   return result;
}

void ossimPlanetServerThread::addMessageHandler(osg::ref_ptr<ossimPlanetServerMessageHandler> messageHandler)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageHandlerMutex);
   int idx = findMessageHandler(messageHandler.get());

   if(idx < 0)
   {
      theMessageHandlerList.push_back(messageHandler.get());
   }
}

void ossimPlanetServerThread::removeMessageHandler(osg::ref_ptr<ossimPlanetServerMessageHandler> messageHandler)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageHandlerMutex);
   int idx = findMessageHandler(messageHandler.get());
   if(idx >= 0)
   {
      theMessageHandlerList.erase(theMessageHandlerList.begin() + idx);
   }
}

ossim_uint32 ossimPlanetServerThread::getNumberOfMessageHandlers()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageHandlerMutex);
   return theMessageHandlerList.size();
}

int ossimPlanetServerThread::findMessageHandler(ossimPlanetServerMessageHandler* handler)const
{

   ossim_uint32 idx = 0;
   for(idx = 0; idx < theMessageHandlerList.size(); ++idx)
   {
      if(handler == theMessageHandlerList[idx].get())
      {
         return static_cast<int>(idx);
      }
   }

   return -1;
}
