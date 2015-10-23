#include <ossimPlanet/ossimPlanetIoSocketServerChannel.h>
#include <ossimPlanet/ossimPlanetIoSocket.h>
#include <iostream>
ossimPlanetIoSocketServerChannel::ossimPlanetIoSocketServerChannel()
   :ossimPlanetIo(),netChannel(),
    theHost(""),
    thePort(0),
    theIoType("tcp"),
    theIsTcp(true),
    theAutoReconnectFlag(true),
    theAutoReconnectInterval(10000),
    theLastTick(0)
{
   setIoDirection(ossimPlanetIoDirection_IN);
}

ossimPlanetIoSocketServerChannel::~ossimPlanetIoSocketServerChannel()
{
   theIoListMutex.lock();
   theIoList.clear();
   theIoListMutex.unlock();
   clearAllBuffers();
   close();
}

void ossimPlanetIoSocketServerChannel::searchName(ossimString& searchNameResult)const
{
   if(name().empty())
   {
      searchNameResult = host() + ":" + ossimString::toString(thePort);
   }
   else
   {
      searchNameResult = name() + ":" + ossimString::toString(thePort);
   }
}

bool ossimPlanetIoSocketServerChannel::setSocket(const std::string& host,
                                                 int port,
                                                 const std::string& ioType)
{
   
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theSocketMutex);
   return protectedSetSocket(host, port, ioType);
}

bool ossimPlanetIoSocketServerChannel::protectedSetSocket(const std::string& host,
                                                          int port,
                                                          const std::string& ioType)
{
   
   theIoList.clear();

   theHost = host;
   thePort = port;
   theIoType = ioType;
   theIsTcp = theIoType!="udp";
   if(getHandle() > -1)
   {
      close();
   }
	if(open(theIsTcp))
   {
      if( bind("", port)<0)
      {
         close();
         return false;
      }
//       if(!theIsTcp)
      {
         setBlocking( false );
      }
//       else
      if(theIsTcp)
      {
         listen(32); // need to add something that is not hardcoded here
      }
   }
   else
   {
      return false;
   }
   return true;
}

void ossimPlanetIoSocketServerChannel::setEnableFlag(bool flag)
{
   bool clearBuffersFlag = false;
   // if we are currently going from enabled to disabled then clear the lists after we disable
   // ourselves
   if(!flag&&enableFlag())
   {
      clearBuffersFlag = true;
   }
   ossimPlanetIo::setEnableFlag(flag);
}

void ossimPlanetIoSocketServerChannel::clearAllBuffers()
{
   theInQueueMutex.lock();
   while(!theInQueue.empty())theInQueue.pop();
   theInQueueMutex.unlock();
}

const std::string& ossimPlanetIoSocketServerChannel::host()const
{
   return theHost;
}

const std::string& ossimPlanetIoSocketServerChannel::ioType()const
{
   return theIoType;
}

int ossimPlanetIoSocketServerChannel::port()const
{
   return thePort;
}

bool ossimPlanetIoSocketServerChannel::isTcp()const
{
   return theIsTcp;
}

void ossimPlanetIoSocketServerChannel::performIo()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theSocketMutex);
   if(handle() < 0)
   {
      if(theAutoReconnectFlag)
      {
         if(theLastTick == 0)
         {
            theLastTick = osg::Timer::instance()->tick();
            return;
         }
         else 
         {
				double delta = osg::Timer::instance()->delta_m(theLastTick, osg::Timer::instance()->tick());
				
				if(delta < 0.0)
				{
					theLastTick = 0;
				}
				else if(delta > theAutoReconnectInterval)
				{
					theLastTick = 0; // reset the next try
					if(!protectedSetSocket(theHost, thePort, theIoType))
					{
						return;
					}
					else if(handle() < 0)// Quick test again to make sure its valid
					{
						return;
					}
				}
				else // we were not able to update the handle yet so return, still invalid
				{
					return;
				}
			}
		}
		else// we were not able to update the handle yet so return, still invalid
		{
			return;
		}
   }
      
   // poll for additional I/O connections
   poll(10);
   
   // handle any current connections and process the messages.
   OpenThreads::ScopedLock<OpenThreads::Mutex> lockList(theIoListMutex);
   std::vector<osg::ref_ptr<ossimPlanetIo> >::iterator iter = theIoList.begin();
   while(iter!=theIoList.end())
   {
      
      osg::ref_ptr<ossimPlanetMessage> msg;
      // perform IO
      (*iter)->performIo();
      
      // now pop any message for handling
      //
      while((msg=(*iter)->popMessage()).valid())
      {
         theInQueueMutex.lock();
         if(enableFlag())
         {
            theInQueue.push(msg);
         }
         else
         {
            while(!theInQueue.empty())
            {
               theInQueue.pop();
            }
         }
         theInQueueMutex.unlock();
      }
      // check to see if it's been marked as finished and if so remove from the list
      if((*iter)->finishedFlag())
      {
         iter = theIoList.erase(iter);
      }
      else
      {
         ++iter;
      }
   }
}

osg::ref_ptr<ossimPlanetMessage> ossimPlanetIoSocketServerChannel::popMessage()
{
   osg::ref_ptr<ossimPlanetMessage> message;
   if(!enableFlag()) return message;
   theInQueueMutex.lock();
   if(!theInQueue.empty())
   {
      message = theInQueue.front();
      theInQueue.pop();
   }
   theInQueueMutex.unlock();
   
   return message;
}

void ossimPlanetIoSocketServerChannel::setAutoReconnectFlag(bool flag)
{
   theAutoReconnectFlag = flag;
}

bool ossimPlanetIoSocketServerChannel::autoReconnectFlag()const
{
   return theAutoReconnectFlag;
}

void ossimPlanetIoSocketServerChannel::setAutoReconnectInterval(ossim_uint32 milliseconds)
{
   theAutoReconnectInterval = milliseconds;
}

ossim_uint32 ossimPlanetIoSocketServerChannel::autoReconnectInterval()const
{
   return theAutoReconnectInterval;
}


void ossimPlanetIoSocketServerChannel::handleRead (void) // for udp servers we can read
{
   netAddress addr ;
   accept ( &addr ) ;
   int h = getHandle();
   if(h > -1)
   {
      theIoListMutex.lock();
      if(!protectedFindHandle(h))
      {
         osg::ref_ptr<ossimPlanetIoSocket> socket = new ossimPlanetIoSocket();
         socket->setIoDirection(ossimPlanetIoDirection_IN);
			//socket->setAutoCloseOnPeerShutdownFlag(true);
			socket->setAutoReconnectFlag(false);
         socket->setHandle(getHandle());
         if(enableFlag())
         {
            theIoList.push_back(socket.get());
         }
//          std::cout << "List size = " << theIoList.size() << "\n";;
      }
      theIoListMutex.unlock();
   }
}

void ossimPlanetIoSocketServerChannel::handleWrite (void) 
{
#if 0
   netAddress addr ;
   accept ( &addr ) ;
   int h = getHandle();
   if(h > -1)
   {
		osg::ref_ptr<ossimPlanetIoSocket> socket = new ossimPlanetIoSocket();
		socket->setIoDirection((ossimPlanetIoDirection)(ossimPlanetIoDirection_IN));
		socket->setHandle(getHandle());
		socket->closeIo();
   }
#endif
}

void ossimPlanetIoSocketServerChannel::handleAccept (void)// for tcp servers we can accept
{
   netAddress addr ;
   int h = accept ( &addr ) ;
   if(h != -1)
   {
      theIoListMutex.lock();
      if(!protectedFindHandle(h))
      {
//          std::cout << "Addind a io to the handleAccept " << h << "\n";
         osg::ref_ptr<ossimPlanetIoSocket> socket = new ossimPlanetIoSocket();
         socket->setIoDirection(ossimPlanetIoDirection_IN);
         socket->setHandle(h);
			socket->setBlockingFlag(false);
			//socket->setAutoCloseOnPeerShutdownFlag(true);
			socket->setAutoReconnectFlag(false);
         if(enableFlag())
         {
            theIoList.push_back(socket.get());
//             std::cout << "Io list size === " << theIoList.size() << std::endl;
         }
      }
      theIoListMutex.unlock();
   }
}

bool ossimPlanetIoSocketServerChannel::protectedFindHandle(int h)const
{
   std::vector<osg::ref_ptr<ossimPlanetIo> >::const_iterator it = theIoList.begin();

   while(it!=theIoList.end())
   {
      ossimPlanetIoSocket* io = static_cast<ossimPlanetIoSocket*>((*it).get());
      if(io->handle() == h)
      {
         return true;
      }
      ++it;
   }

   return false;
}
