#ifndef ossimPlanetTerrainLayer_HEADER
#define ossimPlanetTerrainLayer_HEADER
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <osg/Object>
#include <osg/Texture>

class OSSIMPLANET_DLL ossimPlanetTerrainLayer : public osg::Object
{
public:
   ossimPlanetTerrainLayer()
   :theDirtyFlag(false),
   theMinLevel(0),
   theMaxLevel(99999),
   theMinFilter(osg::Texture::LINEAR),
   theMagFilter(osg::Texture::LINEAR)
   {
      setThreadSafeRefUnref(true);
   }
   
   /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
   ossimPlanetTerrainLayer(const ossimPlanetTerrainLayer& src,
                           const osg::CopyOp& copyValue=osg::CopyOp::SHALLOW_COPY)
   :osg::Object(src, copyValue),
   theDirtyFlag(src.theDirtyFlag),
   theMinLevel(src.theMinLevel),
   theMaxLevel(src.theMaxLevel),
   theMinFilter(src.theMinFilter),
   theMagFilter(src.theMagFilter)
   {
      setThreadSafeRefUnref(true);
      theMinLevel = src.theMinLevel;
      theMaxLevel = src.theMaxLevel;
   }
   
   META_Object(ossimPlanet, ossimPlanetTerrainLayer);  
   
   virtual void dirty()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theDirtyFlag = true;
   }
   virtual bool isDirty()const
   {      
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theDirtyFlag;
   }
   virtual void setDirtyFlag(bool flag)
   {
      if(flag)
      {
         dirty();
      }
      else
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
         theDirtyFlag = flag;
      }
   }
   
   virtual void setMinMaxLevel(ossim_uint32 minLevel, 
                               ossim_uint32 maxLevel)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theMinLevel = minLevel;
      theMaxLevel = maxLevel;
   }
   ossim_uint32 minLevel()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theMinLevel;
   }
   ossim_uint32 maxLevel()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theMaxLevel;
   }
   osg::Texture::FilterMode minFilter()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theMinFilter;
   }
   osg::Texture::FilterMode magFilter()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theMagFilter;
   }
   OpenThreads::Mutex& propertyMutex(){return thePropertyMutex;}
   virtual void setRefreshFlag(bool flag)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theRefreshFlag = flag;
   }
   virtual bool refreshFlag()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theRefreshFlag;
   }
   
protected:
   mutable OpenThreads::Mutex thePropertyMutex;
   bool theDirtyFlag;
   bool theRefreshFlag;
   ossim_uint32 theMinLevel;
   ossim_uint32 theMaxLevel;
   osg::Texture::FilterMode        theMinFilter;
   osg::Texture::FilterMode        theMagFilter;
};

class OSSIMPLANET_DLL ossimPlanetTerrainImageLayer : public ossimPlanetTerrainLayer
{
public:
   ossimPlanetTerrainImageLayer()
   :theNoMoreDataFlag(false)
   {
   }
   /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
   ossimPlanetTerrainImageLayer(const ossimPlanetTerrainImageLayer& src,
                                const osg::CopyOp& copyValue=osg::CopyOp::SHALLOW_COPY)
   :ossimPlanetTerrainLayer(src, copyValue),
   theNoMoreDataFlag(src.theNoMoreDataFlag)
   {
      if(src.theImage.valid())
      {
         theImage = new ossimPlanetImage(*src.theImage, 
                                         osg::CopyOp::DEEP_COPY_ALL);
      }
  }
   
   
   META_Object(ossimPlanet, ossimPlanetTerrainImageLayer);  
   
   virtual void setImage(ossimPlanetImage* image)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theImage = image;
      if(theImage.valid())
      {
         theImage->dirty();
      }
      theDirtyFlag = true;
   }
   ossimPlanetImage* cloneImage()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      if(theImage.valid())
      {
         return new ossimPlanetImage(*theImage.get());
      }
      return 0;
   }
   ossimPlanetImage* image()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theImage.get();
   }
   const ossimPlanetImage* image()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theImage.get();
   }
   virtual void dirty()
   {
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
         if(theImage.valid())
         {
            theImage->dirty();
         }
      }
      ossimPlanetTerrainLayer::dirty();
   }
   ossim_uint32 modifiedCount()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      if(!theImage) return 0;
      return theImage->getModifiedCount();
   }
   void setNoMoreDataFlag(bool flag)
   {
      theNoMoreDataFlag = flag;
   }
   bool noMoreDataFlag()const
   {
      return theNoMoreDataFlag;
   }
protected:
   bool                           theNoMoreDataFlag;
   osg::ref_ptr<ossimPlanetImage> theImage;
};
#endif
