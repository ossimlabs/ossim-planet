#include <ossimPlanet/ossimPlanetIoThread.h>
#include <ossimPlanet/ossimPlanetIoRoutableMessageHandler.h>
#include <ossimPlanet/ossimPlanetIoSocket.h>
#include <ossimPlanet/ossimPlanetIoSocketServerChannel.h>
#include <ossimPlanet/ossimPlanetAction.h>
#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanetDestinationCommandAction.h>
ossimPlanetIoThread::ossimPlanetIoThread()
   :theDoneFlag(false),
    theStartedFlag(false),
    theStartCalledFlag(false),
   thePauseFlag(false)
{
   setPathnameAndRegister(":io");
   //addMessageHandler(new ossimPlanetIoRoutableMessageHandler);
}

void ossimPlanetIoThread::addIo(osg::ref_ptr<ossimPlanetIo> io,
                                bool autoStartFlag)
{
   theIoListMutex.lock();
   theIoList.push_back(io);
   theIoListMutex.unlock();
   if(autoStartFlag&&!startedFlag())
   {
      start();
   }
}

void ossimPlanetIoThread::execute(const ossimPlanetAction &a)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theDelayedExecutionMutex);
   theDelayedExecution.push(a.clone());
   if(!startedFlag())
   {
      start();
   }
}

void ossimPlanetIoThread::sendMessage(osg::ref_ptr<ossimPlanetMessage> message, bool forceSendFlag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theIoListMutex);
   std::vector<osg::ref_ptr<ossimPlanetIo> >::iterator iter = theIoList.begin();
   while(iter != theIoList.end())
   {
      (*iter)->pushMessage(message, forceSendFlag);
      ++iter;
   }
}

bool ossimPlanetIoThread::sendMessage(const ossimString& searchName,
                                      osg::ref_ptr<ossimPlanetMessage> message,
                                      bool forceSendFlag)
{
   bool result = false;
   osg::ref_ptr<ossimPlanetIo> io = findIo(searchName);
   if(io.valid())
   {
      result = true;
      io->pushMessage(message, forceSendFlag);
   }

   return result;
}

osg::ref_ptr<ossimPlanetIo> ossimPlanetIoThread::removeIoGivenSearchString(const ossimString& searchString)
{
   osg::ref_ptr<ossimPlanetIo> result = 0;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theIoListMutex);
   std::vector<osg::ref_ptr<ossimPlanetIo> >::iterator iter = theIoList.begin();
   ossimString tempSearchString;
   while(iter != theIoList.end())
   {
      (*iter)->searchName(tempSearchString);
      if(tempSearchString == searchString)
      {
         result = (*iter);
         theIoList.erase(iter);
         
         return result;
      }
      else
      {
         ++iter;
      }
   }
   
   return result;
}

osg::ref_ptr<ossimPlanetIo> ossimPlanetIoThread::findIo(const ossimString& searchString)
{
   osg::ref_ptr<ossimPlanetIo> result = 0;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theIoListMutex);
   std::vector<osg::ref_ptr<ossimPlanetIo> >::iterator iter = theIoList.begin();
   ossimString tempSearchString;
   while(iter != theIoList.end())
   {
      (*iter)->searchName(tempSearchString);
      if(tempSearchString == searchString)
      {
         return (*iter);
      }
      ++iter;
   }
   
   return result;
}

const osg::ref_ptr<ossimPlanetIo> ossimPlanetIoThread::findIo(const ossimString& searchString)const
{
   const osg::ref_ptr<ossimPlanetIo> result = 0;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theIoListMutex);
   std::vector<osg::ref_ptr<ossimPlanetIo> >::const_iterator iter = theIoList.begin();
   ossimString tempSearchString;
   while(iter != theIoList.end())
   {
      (*iter)->searchName(tempSearchString);
      if(tempSearchString == searchString)
      {
         return (*iter).get();
      }
      ++iter;
   }
   
   return result;
}


bool ossimPlanetIoThread::removeIo(osg::ref_ptr<ossimPlanetIo> io)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theIoListMutex);
   std::vector<osg::ref_ptr<ossimPlanetIo> >::iterator iter = theIoList.begin();
   ossimString tempSearchString;
   while(iter != theIoList.end())
   {
      if((*iter).get() == io.get())
      {
         theIoList.erase(iter);
         return true;
      }
      ++iter;
   }
   return false;
}

void ossimPlanetIoThread::run()
{
   if(startedFlag()) return;
   setStartedFlag(theStartedFlag);
   setDoneFlag(false);
   
   while(!doneFlag())
   {
      {
         
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theLoopMutex);
         if(!pauseFlag())
         {
            theDelayedExecutionMutex.lock();
            while(!theDelayedExecution.empty())
            {
               delayedExecute(*theDelayedExecution.front());
               theDelayedExecution.pop();
            }
            theDelayedExecutionMutex.unlock();
            theIoListMutex.lock();
            std::vector<osg::ref_ptr<ossimPlanetIo> >::iterator iter = theIoList.begin();
            while(iter != theIoList.end())
            {
               osg::ref_ptr<ossimPlanetMessage> msg;
               (*iter)->performIo();
               
               while((msg = (*iter)->popMessage()).valid())
               {
                  handleMessage(msg);
               }
               msg = 0;
               if((*iter)->finishedFlag())
               {
                  iter = theIoList.erase(iter);
               }
               else
               {
                  ++iter;
               }
            }
            theIoListMutex.unlock();
         }
      }
      microSleep(5000);
   }
   setDoneFlag(true);
   setStartedFlag(false);
   setStartCalledFlag(false);
}

int ossimPlanetIoThread::cancel()
{
   setDoneFlag(true);
   return OpenThreads::Thread::cancel();
}

bool ossimPlanetIoThread::addMessageHandler(osg::ref_ptr<ossimPlanetIoMessageHandler> handler)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageHandlerListMutex);
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theMessageHandlerList.size(); ++idx)
   {
      if(handler.get()==theMessageHandlerList[idx].get())
      {
         return false;
      }
   }
   theMessageHandlerList.push_back(handler.get());
   return true;
}

bool ossimPlanetIoThread::removeMessageHandler(osg::ref_ptr<ossimPlanetIoMessageHandler> handler)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageHandlerListMutex);
   ossimPlanetIoThread::MessageHandlerListType::iterator iter = theMessageHandlerList.begin();

   while(iter != theMessageHandlerList.end())
   {
      if((*iter).get() == handler.get())
      {
         theMessageHandlerList.erase(iter);
         return true;
      }
      ++iter;
   }
   
   return false;
}

bool ossimPlanetIoThread::removeMessageHandler(const ossimString& handlerName)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageHandlerListMutex);
   ossimPlanetIoThread::MessageHandlerListType::iterator iter = theMessageHandlerList.begin();

   while(iter != theMessageHandlerList.end())
   {
      if((*iter)->name() == handlerName)
      {
         theMessageHandlerList.erase(iter);
         return true;
      }
   }
   
   return false;
}

ossim_uint32 ossimPlanetIoThread::ioCount()const
{
   return theIoList.size();
}

void ossimPlanetIoThread::clearIo()
{
   theIoListMutex.lock();
   theIoList.clear();
   theIoListMutex.unlock();
}

bool ossimPlanetIoThread::startedFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theStartedFlag;
}

void ossimPlanetIoThread::setStartedFlag(bool flag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theStartedFlag = flag;
}
   
bool ossimPlanetIoThread::doneFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theDoneFlag;
}

void ossimPlanetIoThread::setDoneFlag(bool flag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theDoneFlag = flag;
}

void ossimPlanetIoThread::setPauseFlag(bool flag, bool waitTilPaused)
{
   if(waitTilPaused)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock1(theLoopMutex);
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(thePropertyMutex);
      thePauseFlag = flag;
   }
   else
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      thePauseFlag = flag;
   }
}

bool ossimPlanetIoThread::pauseFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return thePauseFlag;
}

bool ossimPlanetIoThread::startCalledFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theStartCalledFlag;  
}

void ossimPlanetIoThread::setStartCalledFlag(bool flag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theStartCalledFlag = flag;
}

void ossimPlanetIoThread::start()
{
   if(startCalledFlag())
   {
      return;
   }
   setStartCalledFlag(true);
   OpenThreads::Thread::start();
}

void ossimPlanetIoThread::handleMessage(osg::ref_ptr<ossimPlanetMessage> msg)
{
   ossim_uint32 idx = 0;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMessageHandlerListMutex);
   for(idx = 0; idx < theMessageHandlerList.size();++idx)
   {
      if(theMessageHandlerList[idx]->handleMessage(msg))
      {
         return;
      }
   }
}

void ossimPlanetIoThread::delayedXmlExecute(const ossimPlanetXmlAction& a)
{
   ossimString command = a.command();
   if(command == "SendMessage")
   {
      if(a.xmlNode().valid())
      {
         ossimString id = a.xmlNode()->getAttributeValue("id");
         ossimString searchName = a.xmlNode()->getAttributeValue("ioTargetId");
         ossimString forceSend = a.xmlNode()->getAttributeValue("forceSend");
         bool forceSendFlag = forceSend.empty()?false:forceSend.toBool();
         ossim_uint32 idx = 0;
         const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();  
         for(idx = 0;idx<children.size();++idx)
         {
            std::ostringstream out;
            out << *children[idx] << std::endl;
            if(searchName.empty())
            {
               sendMessage(new ossimPlanetMessage(id, ossimString(out.str())), forceSendFlag);
            }
            else
            {
               sendMessage(searchName, new ossimPlanetMessage(id, ossimString(out.str())), forceSendFlag);
            }
         }
      }
   }
   else if(command == "Set")
   {
      if(a.xmlNode().valid())
      {
         ossim_uint32 idx = 0;
         const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();  
         ossimString id = a.xmlNode()->getAttributeValue("id");
         for(idx = 0;idx<children.size();++idx)
         {
            ossimString tag = children[idx]->getTag();
            if(tag == "connectionHeader")
            {
               ossimString headerValue = children[idx]->getText();
               osg::ref_ptr<ossimPlanetIo> io = findIo(id);
               if(io.valid())
               {
                  std::vector<char> value(headerValue.begin(),
                                          headerValue.end());
                  io->setConnectionHeader(new ossimPlanetMessage("", ossimString(headerValue)));
               }
            }
         }
      }
   }
   else if((command == "Remove")||
           (command == "Close"))
   {
      if(a.xmlNode().valid())
      {
         ossimString id = a.xmlNode()->getAttributeValue("id");
         removeIoGivenSearchString(id);
      }
   }
   else if(command == "Open")
   {
      ossim_uint32 idx = 0;
      const ossimXmlNode::ChildListType& children = a.xmlNode()->getChildNodes();  
      for(idx = 0;idx<children.size();++idx)
      {
         ossimString tag = children[idx]->getTag();
         if(tag == "ServerSocket")
         {
            ossimString name       = children[idx]->getChildTextValue("name");
            ossimString ip         = children[idx]->getChildTextValue("ip");
            ossimString port       = children[idx]->getChildTextValue("port");
            ossimString portType   = children[idx]->getChildTextValue("portType");
            ossimString terminator = children[idx]->getChildTextValue("terminator");
            if(name.empty())
            {
               name = ip;
            }
#if 0
            char terminatorChar = '\0';
            if((terminator == "nul")||
               (terminator == "eos"))
            {
               terminatorChar = '\0';
            }
            else if(terminator == "eol")
            {
               terminatorChar = '\n';
            }
#endif
            if(portType.empty())
            {
               portType = "tcp";
            }
            if(!port.empty())
            {
               
            }
            ossimPlanetIoSocketServerChannel* channel = new ossimPlanetIoSocketServerChannel;
            channel->setName(name);
            if(channel->setSocket(ip,
                                  port.toInt32(),
                                  portType))
            {
               addIo(channel);
            }
            else
            {
               delete channel;
               channel = 0;
            }
         }
         else if(tag == "ClientSocket")
         {
            osg::ref_ptr<ossimPlanetIoSocket> socket = 0;
            ossimString name       = children[idx]->getChildTextValue("name");
            ossimString ip         = children[idx]->getChildTextValue("ip");
            ossimString port       = children[idx]->getChildTextValue("port");
            ossimString portType   = children[idx]->getChildTextValue("portType");
            ossimString terminator = children[idx]->getChildTextValue("terminator");
				ossimString autoReconnectFlag = children[idx]->getChildTextValue("autoReconnectFlag");
            char terminatorChar = '\0';
            if((terminator == "nul")||
               (terminator == "eos"))
            {
               terminatorChar = '\0';
            }
            else if(terminator == "eol")
            {
               terminatorChar = '\n';
            }
            if(portType.empty())
            {
               portType = "tcp";
            }
            if(name.empty())
            {
               name = ip;
            }
				if(autoReconnectFlag.empty())
				{
					autoReconnectFlag = "true";
				}
            if(!ip.empty() && !port.empty())
            {
               osg::ref_ptr<ossimPlanetIo> io = findIo(name + ":" + port);
               if(!io.valid())
               {
                  socket = new ossimPlanetIoSocket();
                  socket->setName(name);
                  socket->setTerminator(terminatorChar);
                  socket->setSocket(ip, port.toInt32(), portType);
						socket->setBlockingFlag(false);
						socket->setAutoReconnectFlag(autoReconnectFlag.toBool());
               }
            }
            if(socket.valid())
            {
               addIo(socket.get());
            }
         }
      }
   }
}

void ossimPlanetIoThread::delayedExecute(const ossimPlanetAction &action)
{
   const ossimPlanetXmlAction* xmlAction = action.toXmlAction();
   
	if(xmlAction)
	{
		delayedXmlExecute(*xmlAction);
      return;
	}
   
   const ossimPlanetDestinationCommandAction* a = action.toDestinationCommandAction();
   // for now only support :destination command <args> style actions;
   if(!a) return;
   ossimString command = a->command();
   if(command == "openServerSocket")
   {
      if(a->argCount() == 2) // name and port
      {
         osg::ref_ptr<ossimPlanetIo> io = findIo(a->arg(1) + ":" + a->arg(2));
         if(!io.valid())
         {
            ossimPlanetIoSocketServerChannel* channel = new ossimPlanetIoSocketServerChannel;
            channel->setName(a->arg(1));
            if(channel->setSocket("",
                                  ossimString(a->arg(2)).toUInt32(),
                                  "tcp"))
            {
               addIo(channel);
            }
            else
            {
               delete channel;
            }
         }
      }
      else if(a->argCount() == 3) // <name> <port> <tcp or udp>
      {
         osg::ref_ptr<ossimPlanetIo> io = findIo(a->arg(1) + ":" + a->arg(2));
         if(!io.valid())
         {
            osg::ref_ptr<ossimPlanetIoSocketServerChannel> channel = new ossimPlanetIoSocketServerChannel;
            channel->setName(a->arg(1));
            if(channel->setSocket("",
                                  ossimString(a->arg(2)).toUInt32(),
                                  a->arg(3)))
            {
               addIo(channel.get());
            }
         }
      }
   }
   // Can have the form of:
   //     {ClientSocket {Name <name>} {Port <port>} {Ip <ip>} {Type <type>} {Terminator <eos|eol>} }
   else if(command == "openConnection")
   {
      ossim_uint32 idx = 0;
      ossimString objectName;
      ossimString objectArg;
      for(idx = 1; idx <= a->argCount(); ++idx)
      {
         if(mkUtils::extractObjectAndArg(objectName, objectArg, a->arg(idx)))
         {
            if(objectName == "ClientSocket")
            {
               osg::ref_ptr<ossimPlanetIoSocket> socket = 0;
               ossimString name;
               ossimString port;
               ossimString ip;
               ossimString type = "tcp";
               char terminator = '\0';
               ossimPlanetDestinationCommandAction nestedAction(": ClientSocket " + objectArg);
               ossim_uint32 nestedIdx = 1;
               for(;nestedIdx <= nestedAction.argCount(); ++nestedIdx)
               {
                  if(mkUtils::extractObjectAndArg(objectName, objectArg, nestedAction.arg(nestedIdx)))
                  {
                     if(objectName == "Port")
                     {
                        port = objectArg;
                     }
                     else if(objectName == "Ip")
                     {
                        ip = objectArg;
                     }
                     else if(objectName == "PortType")
                     {
                        type = objectArg;
                     }
                     else if(objectName == "Type")
                     {
                        std::cout << "Obsolete object arg.  Please use PortType instead of Type" << std::endl;
                        type = objectArg;
                     }
                     else if(objectName == "Name")
                     {
                        name = objectArg;
                     }
                     else if(objectName == "Terminator")
                     {
                        if((objectArg == "nul")||
                           (objectArg == "eos"))
                        {
                           terminator = '\0';
                        }
                        else if(objectArg == "eol")
                        {
                           terminator = '\n';
                        }
                     }
                  }
               }// end for nested action

               if(!ip.empty() && !port.empty())
               {
                  osg::ref_ptr<ossimPlanetIo> io = findIo(name + ":" + port);
                  if(!io.valid())
                  {
                     socket = new ossimPlanetIoSocket();
                     socket->setName(name);
                     socket->setTerminator(terminator);
                     if(!socket->setSocket(ip, port.toInt32(), type))
                     {
                        socket = 0;
                     }
                  }
               }
               
               if(socket.valid())
               {
                  addIo(socket.get());
               }
            }// end if ClientSocket
         }
      }
   }
   else if(command == "openClientSocket")
   {
      // This is a bidirectional client socket
      osg::ref_ptr<ossimPlanetIoSocket> socket = 0;
      if(a->argCount() == 3) // default tcp
      {
         osg::ref_ptr<ossimPlanetIo> io = findIo(a->arg(1) + ":" + a->arg(3));
         if(!io.valid())
         {
            socket = new ossimPlanetIoSocket();
            socket->setName(a->arg(1));
            if(!socket->setSocket(a->arg(2),
                                  ossimString(a->arg(3)).toInt32(),
                                  "tcp"))
            {
               socket = 0;
            }
         }
      }
      else if(a->argCount() == 4) // specify the type
      {
         osg::ref_ptr<ossimPlanetIo> io = findIo(a->arg(1) + ":" + a->arg(3));
         if(!io.valid())
         {
            socket = new ossimPlanetIoSocket();
            socket->setName(a->arg(1));
            
            if(!socket->setSocket(a->arg(2),//host
                                  ossimString(a->arg(3)).toInt32(), // port
                                  a->arg(4)))
            {
               socket = 0;
            }
         }
      }
      if(socket.valid())
      {
         addIo(socket.get());
      }
   }
   else if(command == "sendMessage")
   {
      // check for form <search name> <message>
      if(a->argCount() == 2)
      {
         sendMessage(a->arg(1),
                     new ossimPlanetMessage("", a->arg(2)));
      }
      else if(a->argCount() == 1)
      {
         sendMessage(new ossimPlanetMessage("", a->arg(1)));
      }
   }
   else if(command == "removeIo")
   {
      if(a->argCount() == 1)
      {
         removeIoGivenSearchString(a->arg(1));
      }
   }
   else if(command == "changeName")
   {
      if(a->argCount() == 2)
      {
         // need form of <search name> <new name>
         osg::ref_ptr<ossimPlanetIo> io = findIo(a->arg(1));
         if(io.valid())
         {
            io->setName(a->arg(2));
         }
      }
   }
   else if(command == "setTerminator")
   {
      if(a->argCount() == 2)
      {
         osg::ref_ptr<ossimPlanetIo> io = findIo(a->arg(1));
         if(io.valid())
         {
            ossimString terminatorType = a->arg(2);
            
            char terminator;
            if(terminatorType == "eos")
            {
               terminator = '\0';
            }
            else if(terminatorType == "eol")
            {
               terminator = '\n';
            }
            else
            {
               terminator = *(terminatorType.begin());
            }
            io->setTerminator(terminator);
         }
      }
      else if(a->argCount() == 1)
      {
      }
   }   
}
