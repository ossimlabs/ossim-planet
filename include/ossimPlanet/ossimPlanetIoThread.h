#ifndef ossimPlanetIoThread_HEADER
#define ossimPlanetIoThread_HEADER
#include <OpenThreads/Thread>
#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetIoMessageHandler.h>
#include <osg/ref_ptr>
#include <osg/Referenced>
#include <ossimPlanet/ossimPlanetIo.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetXmlAction.h>
#include <queue>

class OSSIMPLANET_DLL ossimPlanetIoThread : public OpenThreads::Thread,
   public osg::Referenced,
   public ossimPlanetActionReceiver
{
public:
   typedef std::vector<osg::ref_ptr<ossimPlanetIoMessageHandler> > MessageHandlerListType;
   ossimPlanetIoThread();
   void addIo(osg::ref_ptr<ossimPlanetIo> io,
              bool autoStartFlag = true);
   virtual void execute(const ossimPlanetAction &a);
   osg::ref_ptr<ossimPlanetIo> findIo(const ossimString& searchString);
   const osg::ref_ptr<ossimPlanetIo> findIo(const ossimString& searchString)const;
   /**
    * broadcast to all outgoing sockets the passed in message
    */ 
   void sendMessage(osg::ref_ptr<ossimPlanetMessage> message, bool forceSendFlag=false);
   
   /**
    * push message onto the named object
    */ 
   bool sendMessage(const ossimString& searchName,
                    osg::ref_ptr<ossimPlanetMessage> message,
                    bool forceSendFlag=false);
   osg::ref_ptr<ossimPlanetIo> removeIoGivenSearchString(const ossimString& searchString);
   osg::ref_ptr<ossimPlanetIo> ioGivenSearchName(const ossimString& sarchString);
   bool removeIo(osg::ref_ptr<ossimPlanetIo> io);
   virtual void run();
   int cancel();
   bool addMessageHandler(osg::ref_ptr<ossimPlanetIoMessageHandler> handler);
   bool removeMessageHandler(osg::ref_ptr<ossimPlanetIoMessageHandler> handler);
   bool removeMessageHandler(const ossimString& handlerName);
   ossim_uint32 ioCount()const;
   void clearIo();
   bool startedFlag()const;
   void setStartedFlag(bool flag);
   bool doneFlag()const;
   void setDoneFlag(bool flag);
   virtual void setPauseFlag(bool flag, bool waitTilPaused=true);
   bool pauseFlag()const;
   bool startCalledFlag()const;
   void setStartCalledFlag(bool flag);
   virtual void start();
protected:
   void handleMessage(osg::ref_ptr<ossimPlanetMessage> msg);
   void delayedExecute(const ossimPlanetAction &a);
   void delayedXmlExecute(const ossimPlanetXmlAction &a);
   
   mutable ossimPlanetReentrantMutex thePropertyMutex;
   mutable ossimPlanetReentrantMutex theIoListMutex;
   std::vector<osg::ref_ptr<ossimPlanetIo> > theIoList;
   mutable ossimPlanetReentrantMutex theMessageHandlerListMutex;
   ossimPlanetIoThread::MessageHandlerListType theMessageHandlerList;
   bool theDoneFlag;
   bool theStartedFlag;
   
   bool theStartCalledFlag;
   bool thePauseFlag;
   mutable ossimPlanetReentrantMutex theDelayedExecutionMutex;
   std::queue<osg::ref_ptr<ossimPlanetAction> > theDelayedExecution;
   
   ossimPlanetReentrantMutex theLoopMutex;
};

#endif
