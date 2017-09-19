#ifndef ossimPlanetCallback_HEADER
#define ossimPlanetCallback_HEADER
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <ossimPlanet/ossimPlanetExport.h>
#include <mutex>
#include <vector>
class  OSSIMPLANET_DLL ossimPlanetCallback : public osg::Referenced
{
public:
   ossimPlanetCallback()
   :theEnableFlag(true)
   {}
   void setEnableFlag(bool flag)
   {
      std::lock_guard<std::recursive_mutex> lock(theCallbackMutex);
      theEnableFlag = flag;
   }
   bool enableFlag()const
   {
      std::lock_guard<std::recursive_mutex> lock(theCallbackMutex);
      return theEnableFlag;
   }
protected:
   mutable std::recursive_mutex theCallbackMutex;
   bool theEnableFlag;
};

template <class T>
class ossimPlanetCallbackListInterface
   {
   public:
      typedef std::vector<osg::ref_ptr<T> > CallbackListType;
      ossimPlanetCallbackListInterface()
      :theBlockCallbacksFlag(false)
      {
         
      }
      virtual ~ ossimPlanetCallbackListInterface(){}
      virtual void addCallback(T* callback)
      {
         std::lock_guard<std::recursive_mutex> lock(theCallbackListMutex);
         if(!hasCallbackNoMutex(callback))
         {
            theCallbackList.push_back(callback);
         }
      }
	  virtual void addCallback(osg::ref_ptr<T> callback)
      {
			addCallback(callback.get());
      }
      virtual void removeCallback(T* callback);
	  virtual void removeCallback(osg::ref_ptr<T> callback)
		{
			removeCallback(callback.get());
		}
      virtual void blockCallbacks(bool flag)
      {
         std::lock_guard<std::recursive_mutex> lock(theCallbackListMutex);
         theBlockCallbacksFlag = flag;
      }
      bool hasCallback(const T* callback)const;
   protected:
      bool hasCallbackNoMutex(const T* callback)const;
      
      mutable std::recursive_mutex theCallbackListMutex;
      CallbackListType theCallbackList;
      bool theBlockCallbacksFlag;
      
   };
template <class T>
void ossimPlanetCallbackListInterface<T>::removeCallback(T* callback)
{
   std::lock_guard<std::recursive_mutex> lock(theCallbackListMutex);
   unsigned int idx;
   for(idx = 0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx] == callback)
      {
         theCallbackList.erase(theCallbackList.begin() + idx);
         break;
      }
   }
}

template <class T>
bool ossimPlanetCallbackListInterface<T>::hasCallback(const T* callback)const
{
   std::lock_guard<std::recursive_mutex> lock(theCallbackListMutex);
   return hasCallbackNoMutex(callback);
}

template <class T>
bool ossimPlanetCallbackListInterface<T>::hasCallbackNoMutex(const T* callback)const
{
   unsigned int idx;
   for(idx = 0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx] == callback)
      {
         return true;
      }
   }
   
   return false;
}


#endif
