#ifndef ossimPlanetReentrantMutex_HEADER
#define ossimPlanetReentrantMutex_HEADER
#include <OpenThreads/Thread>
#include <mutex>

/**
 * This is out of SVN for the nex 2.8.1 release of OSG.  There was a bug in the lock and unlock
 * I will use this until the new 2.8.1 comes out.
 */
class ossimPlanetReentrantMutex : public std::mutex
{
public:
   
   ossimPlanetReentrantMutex():
   _threadHoldingMutex(0),
   _lockCount(0) {}
   
   virtual ~ossimPlanetReentrantMutex() {}
   
   virtual int lock()
   {
      {
         std::lock_guard<std::mutex> lock(_lockCountMutex);
         if (_threadHoldingMutex==OpenThreads::Thread::CurrentThread() && _lockCount>0)
         {
            ++_lockCount;
            return 0;
         }
      }
      
      int result = Mutex::lock();
      if (result==0)
      {
         std::lock_guard<std::mutex> lock(_lockCountMutex);
         
         _threadHoldingMutex = OpenThreads::Thread::CurrentThread();
         _lockCount = 1;
      }
      return result;
   }
   
   
   virtual int unlock()
   {
      std::lock_guard<std::mutex> lock(_lockCountMutex);
#if 0
      if (_threadHoldingMutex==OpenThreads::Thread::CurrentThread() && _lockCount>0)
      {
         --_lockCount;
         if (_lockCount<=0)
         {
            _threadHoldingMutex = 0;
            return Mutex::unlock();
         }
      }
      else
      {
         osg::notify(osg::NOTICE)<<"Error: ReentrantMutex::unlock() - unlocking from the wrong thread."<<std::endl;
      }
#else
      if (_lockCount>0)
      {
         --_lockCount;
         if (_lockCount<=0)
         {
            _threadHoldingMutex = 0;
            return Mutex::unlock();
         }
      }
#endif    
      return 0;
   }
   
   
   virtual int trylock()
   {
      {
         std::lock_guard<std::mutex> lock(_lockCountMutex);
         if (_threadHoldingMutex==OpenThreads::Thread::CurrentThread() && _lockCount>0)
         {
            ++_lockCount;
            return 0;
         }
      }
      
      int result = Mutex::trylock();
      if (result==0)
      {
         std::lock_guard<std::mutex> lock(_lockCountMutex);
         
         _threadHoldingMutex = OpenThreads::Thread::CurrentThread();
         _lockCount = 1;
      }
      return result;
   }
   
private:
   
   ossimPlanetReentrantMutex(const ossimPlanetReentrantMutex&):std::mutex() {}
   
   ossimPlanetReentrantMutex& operator =(const ossimPlanetReentrantMutex&) { return *(this); }
   
   OpenThreads::Thread* _threadHoldingMutex;
   
   std::mutex  _lockCountMutex;
   unsigned int        _lockCount;
   
};
#endif
