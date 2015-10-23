#ifndef ossimPlanetIoSocket_HEADER
#define ossimPlanetIoSocket_HEADER
#include <ossimPlanet/ossimPlanetIo.h>
#include <ossimPlanet/netSocket.h>
#include <queue>
#include <deque>
#include <osg/Timer>
#include <ossimPlanet/ossimPlanetExport.h>
#include <OpenThreads/Mutex>

class OSSIMPLANET_DLL ossimPlanetIoSocket : public ossimPlanetIo
{
public:
   typedef std::deque<osg::ref_ptr<ossimPlanetMessage> > OutQueueType;
   ossimPlanetIoSocket();
   virtual ~ossimPlanetIoSocket();

   void setHandle(int handle);
	void setSocketInfo(const ossimString& host,
							 int port,
							 const ossimString& ioType);
   virtual void searchName(ossimString& searchNameResult)const;
   bool setSocket(const ossimString& host,
                  int port,
                  const ossimString& ioType);
   int handle()
   {
      if(theSocket) return theSocket->getHandle();
      return -1;
   }
   virtual ossim_uint32 read(char* buffer, ossim_uint32 bufferSize, ossimPlanetIo::IoResultType& ioResult);
   virtual ossim_uint32 write(const char* buffer, ossim_uint32 bufferSize, ossimPlanetIo::IoResultType& ioResult);
   
   bool nonblock();
	bool setBlockingFlag(bool flag);
   virtual void setEnableFlag(bool flag);
   const ossimString& host()const;
   int port()const;
   const ossimString& ioType()const;
   virtual void performIo();
   virtual bool pushMessage(osg::ref_ptr<ossimPlanetMessage> message, bool forcePushFlag);
   virtual osg::ref_ptr<ossimPlanetMessage> popMessage();
   virtual void clearAllBuffers();
   //void setAutoCloseOnPeerShutdownFlag(bool flag);
   //bool autoCloseOnPeerShutdownFlag()const;
   void setAutoReconnectFlag(bool flag);
   bool autoReconnectFlag()const;
   void setAutoReconnectInterval(ossim_uint32 milliseconds);
   ossim_uint32 autoReconnectInterval()const;
   void setMaxOutputBacklogInBytes(ossim_uint32 bytes);
   ossim_uint32 maxOutputBacklogInBytes()const;
   virtual void closeIo();
   virtual bool openIo();
   
   void setMaxBytesToSendPerIo(ossim_uint32 byte);
   ossim_uint32 maxBytesToSendPerIo()const;
protected:   
   bool makeClientSocket();
   bool protectedSetSocket(const ossimString& host,
                           int port,
                           const ossimString& ioType);
   void addToOutputBufferIfNeeded();
   
   netSocket*                theSocket;
   ossimString               theHost;
   int                       thePort;
   ossimString               theIoType; // can be udp or tcp
	bool                      theStreamingFlag;
   ossimPlanetIo::ByteBufferType theTempBuffer;
   ossimPlanetIo::ByteBufferType theInBuffer;
   
   // The In queue is where incoming messages are stored and will be popped and handled in the thread
   //
   ossimPlanetReentrantMutex      theInQueueMutex;
   std::queue<osg::ref_ptr<ossimPlanetMessage> >   theInQueue;

   // The out queue will hold all outgoing messages
   ossimPlanetReentrantMutex      theOutQueueMutex;
   OutQueueType                     theOutQueue;
   ossimPlanetReentrantMutex      theOutBufferMutex;
   std::vector<char>                theOutBuffer;

   /**
    *  If this flag is true then it will auto shutdown the client socket if a server shutdown is
    *  detected.  So if a client socket is communicating and it gets a bad send or receive that indicates
    *  a server shutdown it will set its finished flag if this flag is true.
    */ 
   //bool                           theAutoCloseOnPeerShutdownFlag;

   /**
    * This specifies if the client socket is disconnected it should try to reconnect
    */ 
   bool                           theAutoReconnectFlag;

   /**
    * if the AutoReconnectFlag is true then it will try to reconnect at the specified
    * interval.  The interval is in Milleseconds.  So if you want a reconnect attempt
    * to happen approximately every second then the value should be set to 1000.  This
    * is the default value.
    */ 
   ossim_uint32                   theAutoReconnectInterval; // specified in milliseconds

   mutable ossimPlanetReentrantMutex     theMaxBytesToSendPerIoMutex;
   ossim_uint32                   theMaxBytesToSendPerIo;

   ossim_uint32                   theMaxOutgoingBacklogInBytes;

   ossim_int32                    theTotalBytesToSend;
   /**
    * Used in the reconnect interval  This is reset to 0 for identification that an initial
    * tick needs to be set.  once set the delta milliseconds is used to determine
    * when to do another reconnect.
    */ 
   osg::Timer_t                   theLastTick;

   ossimPlanetReentrantMutex    theSocketMutex;
	
	bool                           theFirstReadFlag;
   
   /**
    * Bytes to auto send on connection
    */
};

#endif
