#include <ossimPlanet/ossimPlanetIoSocket.h>
#include <iostream>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimXmlNode.h>
#include <sstream>
#include <ossimPlanet/ossimPlanetXmlAction.h>
#include <ossimPlanet/ossimPlanetDestinationCommandAction.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimNotify.h>
#include <algorithm>

static ossimTrace traceDebug("ossimPlanetIoSocket:debug");

ossimPlanetIoSocket::ossimPlanetIoSocket()
   :ossimPlanetIo(),
    theSocket(new netSocket()),
    theHost(""),
    thePort(0),
    theIoType(""),
    theStreamingFlag(false),
    theTempBuffer(4096),
    //theAutoCloseOnPeerShutdownFlag(true),
    theAutoReconnectFlag(true),
    theAutoReconnectInterval(5000),
    theMaxBytesToSendPerIo(1024*4),
    theMaxOutgoingBacklogInBytes(1024*1024),
    theTotalBytesToSend(0),
    theFirstReadFlag(true)
{
   setIoDirection(ossimPlanetIoDirection_INOUT);
}

ossimPlanetIoSocket::~ossimPlanetIoSocket()
{
   if(theSocket)
   {
      closeIo();
      delete theSocket;
      theSocket = 0;
   }
}

void ossimPlanetIoSocket::setHandle(int handle)
{
   theSocket->setHandle(handle);
}

void ossimPlanetIoSocket::searchName(ossimString& searchNameResult)const
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

ossim_uint32 ossimPlanetIoSocket::read(char* buffer, ossim_uint32 bufferSize, ossimPlanetIo::IoResultType& ioResult)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theIoMutex);
   ossim_uint32 resultBytesRead = 0;
   if(handle() < 0)
   {
      ioResult = IO_FAIL;
      return resultBytesRead;
   }
	
//   bool useRecvFlag = (theStreamingFlag||(theHost.empty()||(thePort <=0)));
   int bytes = -1;
//	if(useRecvFlag)
	{
		bytes = theSocket->recv(buffer, bufferSize);
	}
//	else
//	{
//		netAddress add(theHost, thePort);
//		bytes = theSocket->recvfrom(buffer, bufferSize, 0, &add);
//	}
   if(bytes>0)
   {
      resultBytesRead = bytes;
      ioResult = IO_SUCCESS;
   }
   else if(bytes < 0) // peer performed shutdown
   {
		if(!theSocket->isNonBlockingError())
		{
			ioResult = IO_FAIL;
		}
		else
		{
			ioResult = IO_NO_DATA;
		}
   }
   else
   {
      ioResult = IO_FAIL;
   }
   
   return resultBytesRead;
}


ossim_uint32 ossimPlanetIoSocket::write(const char* buffer, ossim_uint32 bufferSize, ossimPlanetIo::IoResultType& ioResult)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theIoMutex);
   ossim_uint32 resultBytesWritten = 0;
   if(handle() < 0)
   {
      ioResult = IO_NO_DATA;
      return resultBytesWritten;
   }
//   bool useSendFlag = (theStreamingFlag||theHost.empty()||(thePort <=0));
   int bytes = -1;
	
//	if(useSendFlag)
	{
		bytes = theSocket->send(buffer, bufferSize);
	}
//	else
//	{
//		netAddress add(theHost, thePort);
//		bytes = theSocket->sendto(buffer, bufferSize, 0, &add);
//	}
   if(bytes > 0)
   {
      resultBytesWritten = bytes;
      ioResult = IO_SUCCESS;
   }
   else if(bytes == 0)
	{
      ioResult = IO_NO_DATA;
	}
	else
   {
		if(!theSocket->isNonBlockingError())
		{
			ioResult = IO_FAIL;
		}
		else
		{
			ioResult = IO_NO_DATA;
		}
   }
   
   return resultBytesWritten;
}


bool ossimPlanetIoSocket::setSocket(const ossimString& host,
                                    int port,
                                    const ossimString& ioType)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theSocketMutex);
   return protectedSetSocket(host, port, ioType);
}

bool ossimPlanetIoSocket::protectedSetSocket(const ossimString& host,
                                             int port,
                                             const ossimString& ioType)
{
   bool result = false;
   closeIo();
 	setFinishedFlag(false);
  
   theHost        = host;
   theIoType      = ioType;
   thePort        = port;
	theStreamingFlag = theIoType == "tcp";
	//std::cout << "Trying " << host << " " << port << " " << ioType << std::endl;
   // if I am enabled or not enabled and can't do a reconect in the io thread then
   // go ahead and do a connection now
   //
   if((enableFlag())||
      (!enableFlag()&&!theAutoReconnectFlag))
   {
      if (makeClientSocket())
      {
         pushConnectionHeader();// add connection header if one exists
         result = true;
      }
   }
  
   nonblock();
	
	theFirstReadFlag = true;
   return result;
}

bool ossimPlanetIoSocket::nonblock()
{
	return setBlockingFlag(false);
}

bool ossimPlanetIoSocket::setBlockingFlag(bool flag)
{
   if (theSocket->getHandle() < 0)
   {
      return false;
   }
   theSocket->setBlocking( false );
	
	return true;
}

void ossimPlanetIoSocket::setEnableFlag(bool flag)
{
   bool stateChanged = false;
   // if we are currently going from enabled to disabled then clear the lists after we disable
   // ourselves
   if(!flag&&enableFlag())
   {
      stateChanged = true;
   }
   ossimPlanetIo::setEnableFlag(flag);
   if(stateChanged)
   {
      clearAllBuffers();
   }

}

const ossimString& ossimPlanetIoSocket::host()const
{
   return theHost;
}

int ossimPlanetIoSocket::port()const
{
   return thePort;
}

const ossimString& ossimPlanetIoSocket::ioType()const
{
   return theIoType;
}

void ossimPlanetIoSocket::performIo()
{
   ossimPlanetIo::IoResultType ioResult;
   bool hasOutput = false;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theSocketMutex);

   if(theSocket->getHandle()<0)
   {
      if(theAutoReconnectFlag&&enableFlag())
      {
         if(theLastTick == 0)
         {
           theLastTick = osg::Timer::instance()->tick();
         }
         else if(osg::Timer::instance()->delta_m(theLastTick, osg::Timer::instance()->tick()) > theAutoReconnectInterval)
         {
            theLastTick = 0; // reset the next try
            protectedSetSocket(theHost, thePort, theIoType);  // connect and see if we can perform io on next cycle
            if(theSocket->getHandle() >=0)
            {
               pushConnectionHeader();
            }
         }
			return;
      }
      else if(!theAutoReconnectFlag)
		{
			setFinishedFlag(true);
			return;
		}
		else
      {
        return;
      }
   }
   if(ioDirection()&ossimPlanetIoDirection_OUT&&enableFlag())
   {
      addToOutputBufferIfNeeded();
      OpenThreads::ScopedLock<OpenThreads::Mutex> outBufferLock(theOutBufferMutex);
      if(theOutBuffer.size())
      {
         hasOutput = true;
         // do any sends if we are an output socket
         ossim_uint32 bytesToSend =  ossim::min((ossim_uint32)theOutBuffer.size(),
                                                (ossim_uint32)maxBytesToSendPerIo());
      
         ossim_uint32 bytes = write((char*)(&theOutBuffer.front()), bytesToSend, ioResult);
         if(ioResult == IO_SUCCESS)
         {
            theOutBuffer.erase(theOutBuffer.begin(),
                               theOutBuffer.begin() + bytes);
            
           // theTotalBytesToSend-=bytes;
//            if(theTotalBytesToSend < 0)
//            {
//               theTotalBytesToSend = 0;
//            }
         }
//         if(theOutBuffer.empty())
//         {
//            theTotalBytesToSend = 0;
//         }
      }
   }
   else
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> outBufferLock(theOutBufferMutex);
      OpenThreads::ScopedLock<OpenThreads::Mutex> outQueueLock(theOutQueueMutex);
      theOutBuffer.clear();
      theOutQueue.clear();
   }
   if(ioDirection()&ossimPlanetIoDirection_IN)
   {
   // do any receives if we are an input socket
      ossim_uint32 bytes = read(&theTempBuffer.front(), theTempBuffer.size(), ioResult);
      if(ioResult==IO_SUCCESS)
      {
			theFirstReadFlag = false; // we have performed the first read so set the flag now to false
         if(enableFlag())
         {
				//std::cout << ossimString(theTempBuffer.begin(), theTempBuffer.begin()+bytes) << "\n";
            theInBuffer.insert(theInBuffer.end(), theTempBuffer.begin(), theTempBuffer.begin()+bytes);
         }
      }
      else if(ioResult == IO_FAIL) // peer performed shutdown
      {
         if(traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG) << "ossimPlanetIoSocket::performIo(): read io failed" << std::endl;
         }
			//if(!theFirstReadFlag)
			closeIo();
			if(!theAutoReconnectFlag)
			{
				setFinishedFlag(true);
			}
		}
      bool doneSending = false;
      while(!theInBuffer.empty()&&!doneSending)
      {
         // check if start of old style router or 
         // new style XML token
         //
//         bool hasValidStartToken = ((theInBuffer[(size_t)0] == ':')||
//                                    (theInBuffer[(size_t)0] == '<');
         bool hasValidStartToken = (theInBuffer[(size_t)0] == '<');
         if(!hasValidStartToken)
         {
            // then skip til we find the first valid token
            //
            const char tokens[]={'<'};
            ossimPlanetIo::ByteBufferType::iterator iter = std::find_first_of(theInBuffer.begin(),
                                                                          theInBuffer.end(),
                                                                          tokens,
                                                                          tokens+1);
            if(iter == theInBuffer.end())
            {
               theInBuffer.clear();
            }
            else
            {
               // remove the characters since not a valid start token
               //
               theInBuffer.erase(theInBuffer.begin(),
                                 iter);
            }
         }
         
         if(!theInBuffer.empty())
         {
            {
               ossimString value;
               ossimRefPtr<ossimXmlNode> node = new ossimXmlNode;
               std::istringstream in(ossimString(theInBuffer.begin(),
                                                 theInBuffer.end()));
               if(node->read(in))
               {
						//std::cout<<(*node) << "\n";
                  if(node->getTag() == "Message")
                  {
                     // container for multiple Actions
                     //
                     const vector<ossimRefPtr<ossimXmlNode> >& childNodes = node->getChildNodes();
                     ossim_uint32 idx = 0;
                     for(idx = 0; idx < childNodes.size();++idx)
                     {
                        if(childNodes[idx]->getAttributeValue(value, "target"))
                        {
                           ossimPlanetXmlAction action;
                           action.setXmlNode(childNodes[idx]);
                           action.execute();
                        }
                     }
                  }
                  else if(node->getAttributeValue(value, "target")) // is it a direct message
                  {
                     if(traceDebug())
                     {
                        ossimNotify(ossimNotifyLevel_DEBUG) << "ossimPlanetIoSocket::performIo(): processing message = " << ossimString(theInBuffer.begin(),
                                                                                                                                        theInBuffer.begin()+in.tellg())<<std::endl;
                     }
                     // must be a direct action object
                     //
                     ossimPlanetXmlAction action;
                     action.setXmlNode(node);
                     action.execute();
                  }
                  else // will have to do a custom parse later.
                  {
                     OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theInQueueMutex);
                     
                     theInQueue.push(new ossimPlanetMessage("",std::vector<char>(theInBuffer.begin(),
                                                                              theInBuffer.begin()+in.tellg())));
                  }
                  theInBuffer.erase(theInBuffer.begin(),
                                    theInBuffer.begin()+in.tellg());
               }
               else
               {
                  if(traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_DEBUG) << "ossimPlanetIoSocket::performIo(): Read had a parse error that stopped short of the end of buffer location" << std::endl;
                  }
                  //if(!theFirstReadFlag)
                  // if stopped short then we had a parse error.  Just erase the buffer up to the error and keep trying
                  // if we reached the end the we may need more
                  //
                  if(((ossim_int64)in.tellg() >=0) && ((ossim_int64)in.tellg() < (ossim_int64)theInBuffer.size()))
                  {
//							std::cout << "ERROR=======> " << ossimString(theInBuffer.begin(),
//																						theInBuffer.begin()+in.tellg()) << "\n";
                     theInBuffer.erase(theInBuffer.begin(),
                                       theInBuffer.begin()+in.tellg());

                  }
                  doneSending = true;
               }
            }
         }
      }
   }
   else // only outgoing so let's do a check to see if the peer shutdown
   {
      theInBuffer.clear();
      // let's ignor any incoming bytes and do a check to see if we are still connected to the peer
      
      read(&theTempBuffer.front(), theTempBuffer.size(), ioResult);
      if(ioResult == IO_FAIL) // peer performed shutdown
      {
         closeIo();
         theOutBuffer.clear();
//         theTotalBytesToSend=0;

			if(!theAutoReconnectFlag)
			{
				setFinishedFlag(true);
			}
      }
   }
   if(finishedFlag())
   {
      theInBuffer.clear();
      theOutBuffer.clear();
   }
}

bool ossimPlanetIoSocket::pushMessage(osg::ref_ptr<ossimPlanetMessage> message, bool forcePushFlag)
{
   if(!(ioDirection()&ossimPlanetIoDirection_OUT) || !enableFlag()) return false; 
   bool result = true;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOutQueueMutex);
   OutQueueType::iterator iter = theOutQueue.begin();
   if(!message->id().empty())
   {
      while(iter != theOutQueue.end())
      {
         if((*iter)->id() == message->id())
         {
            OutQueueType::iterator iter2 = theOutQueue.erase(iter);
            theOutQueue.insert(iter2, message);
            std::cout << "REPLACING!!!!!" << std::endl;
            return result;
         }
         ++iter;
      }
   }
   theOutQueue.push_back(message);
   
   return result;
#if 0
   bool needTerminator = message[(size_t)0] == ':'; // check for old : action message format
   if(!enableFlag()) return false;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOutGoingMutex);
   if(forcePushFlag) // we will always support one message no matter the size
   {
      theOutBuffer.push_back(message);
      if(needTerminator)
      {
         theOutBuffer.back().push_back(terminator());
      }
      theTotalBytesToSend += theOutBuffer.back().size();
   }
   else if(theTotalBytesToSend > (int)theMaxOutgoingBacklogInBytes)
   {
      // create some kind of notification for this
      result = false;
   }
   else
   {
      if(theSocket->getHandle() < 0) return false;
      theOutBuffer.push_back(message);
      theOutBuffer.back().push_back(terminator());
      if(needTerminator)
      {
         theOutBuffer.back().push_back(terminator());
      }
      theTotalBytesToSend += theOutBuffer.back().size();
   }
   return result;
#endif
}

osg::ref_ptr<ossimPlanetMessage> ossimPlanetIoSocket::popMessage()
{
   if(!enableFlag()) return 0;
   theInQueueMutex.lock();
   osg::ref_ptr<ossimPlanetMessage> message;
   if(!theInQueue.empty())
   {
      message = theInQueue.front();
      theInQueue.pop();
   }
   theInQueueMutex.unlock();
  
   return message;
}

void ossimPlanetIoSocket::clearAllBuffers()
{
   theInQueueMutex.lock();
   while(!theInQueue.empty())theInQueue.pop();
   theInQueueMutex.unlock();
   theInBuffer.clear();
   theOutBufferMutex.lock();
   theOutBuffer.clear();
//   theTotalBytesToSend = 0;
   theOutBufferMutex.unlock();
   theOutQueueMutex.lock();
   theOutQueue.clear();
   theOutQueueMutex.unlock();
}

//void ossimPlanetIoSocket::setAutoCloseOnPeerShutdownFlag(bool flag)
//{
//   theAutoCloseOnPeerShutdownFlag = flag;
//}

//bool ossimPlanetIoSocket::autoCloseOnPeerShutdownFlag()const
//{
//   return theAutoCloseOnPeerShutdownFlag;
//}

void ossimPlanetIoSocket::setAutoReconnectFlag(bool flag)
{
   theAutoReconnectFlag = flag;
}

bool ossimPlanetIoSocket::autoReconnectFlag()const
{
   return theAutoReconnectFlag;
}

void ossimPlanetIoSocket::setAutoReconnectInterval(ossim_uint32 milliseconds)
{
   theAutoReconnectInterval = milliseconds;
}

ossim_uint32 ossimPlanetIoSocket::autoReconnectInterval()const
{
   return theAutoReconnectInterval;
}

void ossimPlanetIoSocket::setMaxOutputBacklogInBytes(ossim_uint32 bytes)
{
   theMaxOutgoingBacklogInBytes = bytes;
}

ossim_uint32 ossimPlanetIoSocket::maxOutputBacklogInBytes()const
{
   return theMaxOutgoingBacklogInBytes;
}

void ossimPlanetIoSocket::closeIo()
{
   theSocket->close();
}

bool ossimPlanetIoSocket::openIo()
{
   return setSocket(theHost, thePort, theIoType);
}

void ossimPlanetIoSocket::setMaxBytesToSendPerIo(ossim_uint32 byte)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMaxBytesToSendPerIoMutex);

   theMaxBytesToSendPerIo = byte;
}

ossim_uint32 ossimPlanetIoSocket::maxBytesToSendPerIo()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMaxBytesToSendPerIoMutex);
   return theMaxBytesToSendPerIo;
}


bool ossimPlanetIoSocket::makeClientSocket()
{
   if (!theSocket->open( theStreamingFlag ))
   {
      return false;
   }
   
	if(theStreamingFlag)
	{
		int testValue = theSocket->connect( theHost.c_str(), thePort );
		if (testValue < 0)
		{
			theSocket->close();
			return false;
		}
	}
  
   return true;
}

void ossimPlanetIoSocket::addToOutputBufferIfNeeded()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock1(theOutQueueMutex);
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(theOutBufferMutex);
 
   if(theOutBuffer.empty()&&(!theOutQueue.empty()))
   {
      bool needTerminator = theOutQueue.front()->data()[(size_t)0] == ':'; // check for old : action message format
      theOutBuffer.insert(theOutBuffer.end(), theOutQueue.front()->data().begin(), theOutQueue.front()->data().end());
      if(needTerminator)
      {
         theOutBuffer.push_back(terminator());
      }
      theOutQueue.pop_front();
   }
#if 0
   while((theOutBuffer.size() < maxBytesToSendPerIo())&&(!theOutQueue.empty()))
   {
      bool needTerminator = theOutQueue.front()->data()[(size_t)0] == ':'; // check for old : action message format
      theOutBuffer.push_back(theOutQueue.front()->data());
      if(needTerminator)
      {
         theOutBuffer.push_back(terminator());
      }
      //         theTotalBytesToSend += theOutBuffer.size();
   }
#endif
}

