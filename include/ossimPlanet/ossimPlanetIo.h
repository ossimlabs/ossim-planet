#ifndef ossimPlanetIo_HEADER
#define ossimPlanetIo_HEADER
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/ossimPlanetConstants.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetMessage.h>
#include <vector>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <OpenThreads/ScopedLock>

class OSSIMPLANET_DLL ossimPlanetIo : public osg::Referenced
{
public:
   typedef char BufferType;
   typedef std::vector<char> ByteBufferType;
   enum IoResultType
   {
      IO_SUCCESS = 0,
      IO_NO_DATA = 1,
      IO_FAIL    = 2
   };
   ossimPlanetIo(const ossimString& name="",
                 char terminator='\0')
      :theName(name),
      theTerminator(terminator),
      theFinishedFlag(false),
      theEnableFlag(true)
      {
      }
   
   virtual ~ossimPlanetIo()
   {
   }
   
   virtual void performIo()=0;
   
   /**
    * This is a general interface into Io for reading data.
    *
    * @param buffer is a fixed size buffer that we read data and put into it.
    * @param bufferSize Size of the buffer.
    * @result The amount of bytes read into the buffer.
    */
   virtual ossim_uint32 read(char* buffer, ossim_uint32 bufferSize, ossimPlanetIo::IoResultType& ioResult)=0;
   
   /**
    * This is a general interface into Io for writing data.
    *
    * @param buffer is a buffer that we either read data and put into it.
    * @param bufferSize Size of the buffer.
    * @param ioResult Specifies if the IO was succesful or if no data was present or if it failed. On failure derived IO
    *                 can deterimine the action they want to take.
    * @result The amount of bytes writen into the buffer.
    */
   virtual ossim_uint32 write(const char* buffer, ossim_uint32 bufferSize, ossimPlanetIo::IoResultType& ioResult)=0;
   
   virtual bool pushMessage(osg::ref_ptr<ossimPlanetMessage> /*message*/, bool /*forcePushFlag*/)
   {
      return false;
   }
   virtual osg::ref_ptr<ossimPlanetMessage> popMessage()
   {
      return 0;
   }
   virtual void setConnectionHeader(osg::ref_ptr<ossimPlanetMessage> value)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
      theConnectionHeader = value;
   }
   void setFinishedFlag(bool flag)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
      theFinishedFlag = flag;
   }
   bool finishedFlag()const
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
         return theFinishedFlag;
      }
   const ossimString& name()const
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
         return theName;
      }
   void setName(const ossimString& name)
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
         theName = name;
      }
   virtual void searchName(ossimString& searchNameResult)const
      {
         // by default we will return the name.  Derived classes should return
         // a full ID name.  For example, sockets will return <name>:<port>
         searchNameResult = name();
      }
   char terminator()const
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
         return theTerminator;
      }
   void setTerminator(char terminator)
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
         theTerminator = terminator;
      }
   virtual void setEnableFlag(bool flag)
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
         theEnableFlag = flag;
      }
   bool enableFlag()const
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
         return theEnableFlag;
      }
   void setIoDirection(ossimPlanetIoDirection direction)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
      theIoDirection = direction;
   }
   ossimPlanetIoDirection ioDirection()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
      return theIoDirection;
   }
   virtual void pushConnectionHeader() 
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePlanetIoPropertyMutex);
      if(theConnectionHeader.valid())
      {
         if(theConnectionHeader->dataSize() > 0)
         {
            pushMessage(theConnectionHeader, true);
         }
      }
   }
   virtual void closeIo()=0;
   virtual bool openIo()=0;
   
private:
   ossimString                  theName;
   char                         theTerminator;
   bool                         theFinishedFlag;
   bool                         theEnableFlag;
   ossimPlanetIoDirection       theIoDirection;
   osg::ref_ptr<ossimPlanetMessage>      theConnectionHeader;
   mutable ossimPlanetReentrantMutex   thePlanetIoPropertyMutex;
protected:
  mutable ossimPlanetReentrantMutex   theIoMutex;

};


#endif
