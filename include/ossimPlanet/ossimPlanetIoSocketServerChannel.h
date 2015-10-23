#ifndef ossimPlanetIoSocketServerChannel_HEADER
#define ossimPlanetIoSocketServerChannel_HEADER
#include <ossimPlanet/netChannel.h>
#include <ossimPlanet/ossimPlanetIo.h>
#include <ossim/base/ossimConstants.h>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <osg/ref_ptr>
#include <queue>
#include <ossimPlanet/ossimPlanetExport.h>
#include <osg/Timer>
#include <iostream>

class OSSIMPLANET_DLL ossimPlanetIoSocketServerChannel : public ossimPlanetIo,
                                                         public netChannel
{
public:
   ossimPlanetIoSocketServerChannel();
   virtual ~ossimPlanetIoSocketServerChannel();
   virtual void searchName(ossimString& searchNameResult)const;
   bool setSocket(const std::string& host,
                  int port,
                  const std::string& ioType);
   virtual void setEnableFlag(bool flag);
   virtual void clearAllBuffers();
   const std::string& host()const;
   const std::string& ioType()const;
   int port()const;
   bool isTcp()const;
   virtual ossim_uint32 read(char* /* buffer */, ossim_uint32 /* bufferSize */,
                             ossimPlanetIo::IoResultType& ioResult)
   {
      ioResult = IO_NO_DATA;
      return 0;
   }
   virtual ossim_uint32 write(const char* /* buffer */, ossim_uint32 /* bufferSize */,
                              ossimPlanetIo::IoResultType& ioResult)
   {
      ioResult = IO_NO_DATA;
      return 0;
   }
   virtual void performIo();
   virtual osg::ref_ptr<ossimPlanetMessage> popMessage();
   int handle()
   {
      return getHandle();
   }
   virtual void closeIo()
   {
      netChannel::close();
   }
   virtual bool openIo()
   {
      closeIo();
      return setSocket(theHost, thePort, theIoType);
   }
   void setAutoReconnectFlag(bool flag);
   bool autoReconnectFlag()const;
   void setAutoReconnectInterval(ossim_uint32 milliseconds);
   ossim_uint32 autoReconnectInterval()const;
   
   // these are the callback handlers and we can create new Sockets with the given handle and add it to the thread.
   
   virtual void handleClose (void)
       {
//          ulSetError(UL_WARNING,"Network: %d: unhandled close",getHandle());
       }
   virtual void handleRead (void); // for udp servers we can read
   virtual void handleWrite (void);
//       {
//          ulSetError(UL_WARNING,"Network: %d: unhandled write",getHandle());
//       }
   virtual void handleAccept (void);// for tcp servers we can accept
    virtual void handleError (int error)
       {
			 std::cout << "ossimPlanetIoSocketServerChannel::handleError() = " << error << std::endl;
//          ulSetError(UL_WARNING,"Network: %d: errno: %s(%d)",getHandle(),strerror(errno),errno);
       }
protected:
   bool protectedFindHandle(int h)const;
   bool protectedSetSocket(const std::string& host,
                           int port,
                           const std::string& ioType);
   std::string theHost;
   int  thePort;
   std::string theIoType;
   bool theIsTcp;
   ossimPlanetReentrantMutex theIoListMutex;
   std::vector<osg::ref_ptr<ossimPlanetIo> > theIoList;
   ossimPlanetReentrantMutex        theInQueueMutex;
   std::queue<osg::ref_ptr<ossimPlanetMessage> >   theInQueue;
   bool                           theAutoReconnectFlag;
   ossim_uint32                   theAutoReconnectInterval; // specified in milliseconds
   osg::Timer_t                   theLastTick;
   ossimPlanetReentrantMutex        theSocketMutex;
};


#endif
