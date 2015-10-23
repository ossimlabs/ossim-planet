#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <algorithm>
#include <osg/Vec2d>
#include <ossimPlanet/ossimPlanetTextureLayerGroup.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/ossimPlanetIdManager.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimDrect.h>
#include <ossim/base/ossimLine.h>
#include <algorithm>

ossimPlanetTextureLayer::ossimPlanetTextureLayer()
   :osg::Referenced(),
    theName(""),
    theDescription("ossimPlanetTextureLayer"),
    theId(),
    theEnableFlag(true),
    theTransparentColorFlag(false),
    theTransparentColorVector(3),
//     theFilterType("bilinear"),
    theFilterType("bilinear"),
    theDirtyExtentsFlag(true),
    theDirtyStatsFlag(true),
    theStateCode(ossimPlanetTextureLayer_VALID)

{
//    theLookAt = new ossimPlanetLookAt();
//    theLookAt->setRange(7000000);
   theExtents = new ossimPlanetExtents();
   theStats   = new ossimPlanetTextureLayer::Stats();
   theTransparentColorVector[0] = 0;
   theTransparentColorVector[1] = 0;
   theTransparentColorVector[2] = 0;
   
   theBrightness = 0.0;
   theContrast   = 1.0;
   theOpacity    = 1.0;
}

ossimPlanetTextureLayer::ossimPlanetTextureLayer(const ossimPlanetTextureLayer& src)
   :osg::Referenced(),
    theName(src.theName),
    theDescription(src.theDescription),
    theId(src.theId),
    theExtents(src.theExtents->clone()),
    theEnableFlag(src.theEnableFlag),
    theTransparentColorFlag(src.theTransparentColorFlag),
    theTransparentColorVector(src.theTransparentColorVector),
    theFilterType(src.theFilterType),
    theDirtyExtentsFlag(src.theDirtyExtentsFlag),
    theDirtyStatsFlag(src.theDirtyStatsFlag),
    theStateCode(src.theStateCode),
    theBrightness(src.theBrightness),
    theContrast(src.theContrast),
    theOpacity(src.theOpacity)
{
//    std::cout << "ossimPlanetTextureLayer::ossimPlanetTextureLayer ERROR: NEED TO DO PARENT LIST COPY!!!!!" << std::endl;
}

ossimPlanetTextureLayer::~ossimPlanetTextureLayer()
{
   
}

ossimString ossimPlanetTextureLayer::getClassName()const
{
   return "ossimPlanetTextureLayer";
}

void ossimPlanetTextureLayer::dirtyExtents()
{
   // already dirty
   if(theDirtyExtentsFlag) return;
//    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theParentListMutex);
   theDirtyExtentsFlag = true;
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theParentList.size(); ++idx)
   {
      theParentList[idx]->dirtyExtents();
   }
}

void ossimPlanetTextureLayer::setDirtyExtentsFlag(bool flag)
{
   theDirtyExtentsFlag = flag;
}

void ossimPlanetTextureLayer::dirtyStats()
{
//    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theParentListMutex);
   theDirtyStatsFlag = true;

   ossim_uint32 idx = 0;
   for(idx = 0; idx < theParentList.size(); ++idx)
   {
      theParentList[idx]->dirtyStats();
   }
}

void ossimPlanetTextureLayer::addBytesTransferredStat(ossim_uint64 bytesTransferred)const
{
//    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theParentListMutex);
   theStats->setBytesTransferred(theStats->bytesTransferred() + bytesTransferred);
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theParentList.size(); ++idx)
   {
      theParentList[idx]->addBytesTransferredStat(bytesTransferred);
   }
}

void ossimPlanetTextureLayer::clearState(ossimPlanetTextureLayerStateCode stateCode)const
{
   ossim_uint32 temp = (ossim_uint32)theStateCode;

   temp &= (~((ossim_uint32)stateCode));

   theStateCode = (ossimPlanetTextureLayerStateCode)temp;
}

void ossimPlanetTextureLayer::setState(ossimPlanetTextureLayerStateCode stateCode)const
{
   theStateCode = (ossimPlanetTextureLayerStateCode)(theStateCode|stateCode);
   notifyPropertyChanged("stateCode", this);
}
   
ossimPlanetTextureLayerStateCode ossimPlanetTextureLayer::getStateCode()const
{
   return theStateCode;
}

bool ossimPlanetTextureLayer::isStateSet(ossimPlanetTextureLayerStateCode stateCode)const
{
   return (theStateCode & stateCode);
}

const osg::ref_ptr<ossimPlanetExtents>  ossimPlanetTextureLayer::getExtents()const
{
   return theExtents.get();
}

osg::ref_ptr<ossimPlanetExtents>  ossimPlanetTextureLayer::getExtents()
{
   if(theDirtyExtentsFlag)
   {
      updateExtents();
   }

   return theExtents.get();
}



const osg::ref_ptr<ossimPlanetTextureLayer::Stats> ossimPlanetTextureLayer::getStats()const
{
   if(theDirtyStatsFlag)
   {
      updateStats();
      theDirtyStatsFlag = false;
   }
   return theStats;
}

osg::ref_ptr<ossimPlanetTextureLayer::Stats>  ossimPlanetTextureLayer::getStats()
{
   if(theDirtyStatsFlag)
   {
      updateStats();
      theDirtyStatsFlag = false;
   }
   return theStats;
}


void ossimPlanetTextureLayer::setExtents(osg::ref_ptr<ossimPlanetExtents> extents)
{
   theExtents = extents;
   dirtyExtents();
   theDirtyExtentsFlag = false;
}

void ossimPlanetTextureLayer::getDateRange(ossimDate& minDate,
                                           ossimDate& maxDate)const
{
   minDate = theExtents->getMinDate();
   maxDate = theExtents->getMaxDate();
}

const osg::ref_ptr<ossimPlanetLookAt> ossimPlanetTextureLayer::getLookAt()const
{
   return theLookAt.get();
}

void ossimPlanetTextureLayer::setLookAt(osg::ref_ptr<ossimPlanetLookAt> lookAt)
{
   theLookAt = lookAt.get();
}

double ossimPlanetTextureLayer::getApproximateHypotneusLength()const
{
   return ((osg::Vec2d(theExtents->getMaxLat(),
                       theExtents->getMinLon())-
            osg::Vec2d(theExtents->getMinLat(),
                       theExtents->getMaxLon())).length() *
           ossimGpt().metersPerDegree().x);
}

void ossimPlanetTextureLayer::getCenterLatLonLength(double& centerLat,
                                                    double& centerLon,
                                                    double& length)const
{
   length = ((osg::Vec2d(theExtents->getMaxLat(), theExtents->getMinLon())-
              osg::Vec2d(theExtents->getMinLat(), theExtents->getMaxLon())).length() *
             ossimGpt().metersPerDegree().x);
   centerLat = (theExtents->getMinLat()+theExtents->getMaxLat())*.5;
   centerLon = (theExtents->getMinLon()+theExtents->getMaxLon())*.5;
}

ossimPlanetTextureLayerGroup* ossimPlanetTextureLayer::asGroup()
{
   return 0;
}

const ossimPlanetTextureLayerGroup* ossimPlanetTextureLayer::asGroup()const
{
   return 0;
}

ossimPlanetTextureLayerGroup* ossimPlanetTextureLayer::getParent(ossim_uint32 idx)
{
   return parent(idx);
}

const ossimPlanetTextureLayerGroup* ossimPlanetTextureLayer::getParent(ossim_uint32 idx)const
{
   return parent(idx);
}

ossimPlanetTextureLayerGroup* ossimPlanetTextureLayer::parent(ossim_uint32 idx)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);

   if(idx < theParentList.size())
   {
      return theParentList[idx];
   }
   
   return 0;
}

const ossimPlanetTextureLayerGroup* ossimPlanetTextureLayer::parent(ossim_uint32 idx)const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);

   if(idx < theParentList.size())
   {
      return theParentList[idx];
   }
   
   return 0;
}

bool ossimPlanetTextureLayer::hasParent(ossimPlanetTextureLayerGroup* parent)const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   ossim_uint32 idx = 0;

   for(idx = 0; idx < theParentList.size(); ++idx)
   {
      if(theParentList[idx] == parent)
      {
         return true;
      }
   }

   return false;
}

void ossimPlanetTextureLayer::addParent(ossimPlanetTextureLayerGroup* parent)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   ossim_uint32 idx = 0;

   for(idx = 0; idx < theParentList.size(); ++idx)
   {
      if(theParentList[idx] == parent)
      {
         return;
      }
   }

   theParentList.push_back(parent);
}

void ossimPlanetTextureLayer::removeParent(ossimPlanetTextureLayerGroup* parent)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   ossim_uint32 idx = 0;

   for(idx = 0; idx < theParentList.size(); ++idx)
   {
      if(theParentList[idx] == parent)
      {
         theParentList.erase(theParentList.begin()+idx);
      }
   }
}

void ossimPlanetTextureLayer::detachFromParents()
{
   ossim_uint32 idx = 0;

   theMutex.lock();
   ossimPlanetTextureLayerParentList parentList = theParentList;
   theParentList.clear();
   theMutex.unlock();
   for(idx = 0; idx < parentList.size(); ++idx)
   {
      parentList[idx]->removeLayer(this);
   }
}

const ossimString& ossimPlanetTextureLayer::id()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theId;
}

void ossimPlanetTextureLayer::setId(const ossimString& id)
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theId = id;
   }
   notifyPropertyChanged("id", this);
}

void ossimPlanetTextureLayer::setEnableFlag(bool flag)
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theEnableFlag = flag;  
   }
   notifyPropertyChanged("enableFlag", this);
}

bool ossimPlanetTextureLayer::getEnableFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theEnableFlag;
}

bool ossimPlanetTextureLayer::enableFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theEnableFlag;
}

void ossimPlanetTextureLayer::setName(const ossimString& name)
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theName = name;
   }
   notifyPropertyChanged("name", this);
}

const ossimString& ossimPlanetTextureLayer::getName()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theName;
}

const ossimString& ossimPlanetTextureLayer::name()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theName;
}

void ossimPlanetTextureLayer::setDescription(const ossimString& description)
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theDescription = description;
   }
   notifyPropertyChanged("description", this);
}

const ossimString& ossimPlanetTextureLayer::getDescription()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theDescription;
}

const ossimString& ossimPlanetTextureLayer::description()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theDescription;
}

bool ossimPlanetTextureLayer::getTransparentColorFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theTransparentColorFlag;
}

void ossimPlanetTextureLayer::setTransparentColorFlag(bool flag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theTransparentColorFlag = flag;
}

const ossimPlanetTextureLayer::TransparentColorType& ossimPlanetTextureLayer::getTransparentColor()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theTransparentColorVector;
}

void ossimPlanetTextureLayer::setTransparentColor(unsigned int r,
                                                  unsigned int g,
                                                  unsigned int b)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theTransparentColorVector[0] = r;
   theTransparentColorVector[1] = g;
   theTransparentColorVector[2] = b;
}

ossimPlanetImage* ossimPlanetTextureLayer::applyBrightnessContrast(ossimPlanetImage* image, bool duplicateFlag)const
{
  ossimPlanetImage* result = image;
  if(((brightness() != 0.0)||
      (contrast() != 1.0))&&image)
  {
    if(duplicateFlag)
    {
      result = new ossimPlanetImage(*image);
    }
    result->applyBrightnessContrast(brightness(), contrast());
  }

  return result;
}

/*
void ossimPlanetTextureLayer::mergeImage(ossimPlanetImage* result,
                                         const ossimPlanetImage* source,
                                         ossim_float32 sourceOpacity)const
{
   if(!result || !source) return;
   
   unsigned int w    = result->s();
   unsigned int h    = result->t();
   unsigned int area = w*h;
   unsigned int idx  = 0;

   unsigned char* destBuf = result->data();
   const unsigned char* srcBuf  = source->data();

   if(sourceOpacity == 1.0)
   {
     for(idx = 0; idx < area; ++idx)
     {
        if(srcBuf[3] == 255)
        {
           destBuf[0] = srcBuf[0];
           destBuf[1] = srcBuf[1];
           destBuf[2] = srcBuf[2];
           destBuf[3] = 255;
        }
        else if(srcBuf[3] > 0)
        {
           float normalizedValue = (srcBuf[3]/255.0);

           destBuf[0] = static_cast<unsigned char>(destBuf[0]*(1-normalizedValue) + srcBuf[0]*normalizedValue);
           destBuf[1] = static_cast<unsigned char>(destBuf[1]*(1-normalizedValue) + srcBuf[1]*normalizedValue);
           destBuf[2] = static_cast<unsigned char>(destBuf[2]*(1-normalizedValue) + srcBuf[2]*normalizedValue);
           destBuf[3] = 255;
        }
        destBuf += 4;
        srcBuf  += 4;
     }
   }
   else
   {
     for(idx = 0; idx < area; ++idx)
     {
        if(srcBuf[3] ==255)
        {
           destBuf[0] = srcBuf[0]*(1-sourceOpacity) + destBuf[0]*(sourceOpacity);
           destBuf[1] = srcBuf[1]*(1-sourceOpacity) + destBuf[1]*(sourceOpacity);
           destBuf[2] = srcBuf[2]*(1-sourceOpacity) + destBuf[2]*(sourceOpacity);
           destBuf[3] = 255;
        }
        else if(srcBuf[3] > 0)
        {
           float normalizedValue = (srcBuf[3]/255.0)*sourceOpacity;

           destBuf[0] = static_cast<unsigned char>(destBuf[0]*(1-normalizedValue) + srcBuf[0]*normalizedValue);
           destBuf[1] = static_cast<unsigned char>(destBuf[1]*(1-normalizedValue) + srcBuf[1]*normalizedValue);
           destBuf[2] = static_cast<unsigned char>(destBuf[2]*(1-normalizedValue) + srcBuf[2]*normalizedValue);
           destBuf[3] = 255;
        }
        destBuf += 4;
        srcBuf  += 4;
     }
   }
}
*/
void ossimPlanetTextureLayer::mergeImage(ossimPlanetImage* result,
                                         const ossimPlanetImage* source,
                                         ossim_float32 sourceOpacity)const
{
   if(!result || !source) return;
   
   unsigned int w    = result->s();
   unsigned int h    = result->t();
   unsigned int area = w*h;
   unsigned int idx  = 0;
 
   unsigned char* destBuf = result->data();
   const unsigned char* srcBuf  = source->data();
 
   if(sourceOpacity == 1.0)
   {
     for(idx = 0; idx < area; ++idx)
     {
        if(srcBuf[3] == 255)
        {
           destBuf[0] = srcBuf[0];
           destBuf[1] = srcBuf[1];
           destBuf[2] = srcBuf[2];
           destBuf[3] = 255;
        }
        else if(srcBuf[3] > 0)
        {
           float alpha = 255 - (1 - srcBuf[3]/255.0) * (255-destBuf[3]);
           float normalizedValue = srcBuf[3]/alpha;
 
           destBuf[0] = static_cast<unsigned char>(srcBuf[0]*normalizedValue + destBuf[0]*(1-normalizedValue));
           destBuf[1] = static_cast<unsigned char>(srcBuf[1]*normalizedValue + destBuf[1]*(1-normalizedValue));
           destBuf[2] = static_cast<unsigned char>(srcBuf[2]*normalizedValue + destBuf[2]*(1-normalizedValue));
           destBuf[3] = static_cast<unsigned char>(alpha);
        }
        destBuf += 4;
        srcBuf  += 4;
     }
   }
   else if(sourceOpacity > 0.0)
   {
     for(idx = 0; idx < area; ++idx)
     {
        if(srcBuf[3] > 0)
        {
           float alpha = 255 - (1 - srcBuf[3]* sourceOpacity /255.0) * (255-destBuf[3]);
           float normalizedValue = srcBuf[3]/alpha;
 
           destBuf[0] = static_cast<unsigned char>(srcBuf[0]*normalizedValue + destBuf[0]*(1-normalizedValue));
           destBuf[1] = static_cast<unsigned char>(srcBuf[1]*normalizedValue + destBuf[1]*(1-normalizedValue));
           destBuf[2] = static_cast<unsigned char>(srcBuf[2]*normalizedValue + destBuf[2]*(1-normalizedValue));
           destBuf[3] = static_cast<unsigned char>(alpha);
        }
 
        destBuf += 4;
        srcBuf  += 4;
     }
   }
}
 
bool ossimPlanetTextureLayer::insertAlpha(osg::ref_ptr<ossimPlanetImage> texture,
                                          float initialAlphaValue)const
{
   GLint internalFormat = texture->getInternalTextureFormat();
   unsigned int w = texture->s();
   unsigned int h = texture->t();
   unsigned int area = w*h;
   unsigned char* data = texture->data();
   unsigned char alpha = static_cast<unsigned char>(initialAlphaValue*255.0);
   if((internalFormat != GL_RGB)&&
      (internalFormat != GL_RGBA))
   {
      ossimNotify(ossimNotifyLevel_WARN) << "ossimPlanetTextureArchive::insertAlpha: ERROR image not an RGB image format" << std::endl;
      
      return false;
   }
   
   if(internalFormat == GL_RGB)
   {
      unsigned char* newData = new unsigned char[area*4];
      unsigned char* newDataPtr = newData;
      for(unsigned int idx = 0; idx < area;++idx)
      {
         newDataPtr[0] = data[0];
         newDataPtr[1] = data[1];
         newDataPtr[2] = data[2];
         newDataPtr[3] = alpha;
         newDataPtr+=4;
         data+=3;
      }
      texture->setImage(w, h, 1,
                        GL_RGBA,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        newData,
                        osg::Image::USE_NEW_DELETE);
                                
   }
   else // copy over existing alpha since already have alpha
   {
      for(unsigned int idx = 0; idx < area;++idx)
      {
         data[3] = alpha;
         data+=4;
      }
      
   }
   texture->dirty();
   
   return true;
}

bool ossimPlanetTextureLayer::insertAlpha(osg::ref_ptr<ossimPlanetImage> texture)const
{
   if(!theTransparentColorFlag)
   {
      return insertAlpha(texture, 1.0);
   }
   
   GLint internalFormat = texture->getInternalTextureFormat();
   unsigned int w = texture->s();
   unsigned int h = texture->t();
   unsigned int area = w*h;
   unsigned char* data = texture->data();
   if((internalFormat != GL_RGB)&&
      (internalFormat != GL_RGBA))
   {
      ossimNotify(ossimNotifyLevel_WARN) << "ossimPlanetTextureArchive::insertAlpha: ERROR image not an RGB image format" << std::endl;
      
      return false;
   }
   const unsigned char* transparentColor = &theTransparentColorVector.front();
   
   if(internalFormat == GL_RGB)
   {
      unsigned char* newData = new unsigned char[area*4];
      unsigned char* newDataPtr = newData;
      for(unsigned int idx = 0; idx < area;++idx)
      {
         newDataPtr[0] = data[0];
         newDataPtr[1] = data[1];
         newDataPtr[2] = data[2];
         if((newDataPtr[0] == transparentColor[0])&&
            (newDataPtr[1] == transparentColor[1])&&
            (newDataPtr[2] == transparentColor[2]))
         {
            newDataPtr[3] = 0;
         }
         else
         {
            newDataPtr[3] = 255;
         }
         newDataPtr+=4;
         data+=3;
      }
      texture->setImage(w, h, 1,
                        GL_RGBA,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        newData,
                        osg::Image::USE_NEW_DELETE);
   }
   else // copy over existing alpha since already have alpha
   {
      for(unsigned int idx = 0; idx < area;++idx)
      {
         if((data[0] == transparentColor[0])&&
            (data[1] == transparentColor[1])&&
            (data[2] == transparentColor[2]))
         {
            data[3] = 0;
         }
         data+=4;
      }
      
   }
   texture->dirty();
   texture->setPixelStatus();
   
   return true;
}

void ossimPlanetTextureLayer::addTransparent(osg::ref_ptr<ossimPlanetImage> texture)const
{
   if(!theTransparentColorFlag) return;
   const unsigned char* transparentColor = &theTransparentColorVector.front();
   GLint internalFormat = texture->getInternalTextureFormat();
   unsigned int w = texture->s();
   unsigned int h = texture->t();
   unsigned int area = w*h;
   unsigned char* data = texture->data();
   if((internalFormat != GL_RGBA))
   {
      ossimNotify(ossimNotifyLevel_WARN) << "ossimPlanetTextureArchive::addTransparent: ERROR image not an RGBA image format" << std::endl;
      
      return;
   }
   for(unsigned int idx = 0; idx < area;++idx)
   {
      if((data[0] == transparentColor[0])&&
         (data[1] == transparentColor[1])&&
         (data[2] == transparentColor[2]))
      {
         data[3] = 0;
      }
      data+=4;
   }
   texture->dirty();
}



void ossimPlanetTextureLayer::setFilterType(const ossimString& filterType)
{
   theFilterType = filterType;
}

const ossimString& ossimPlanetTextureLayer::getFilterTypeAsString()const
{
   return theFilterType;
}

// void ossimPlanetTextureLayer::addCallback(osg::ref_ptr<ossimPlanetTextureLayer::Callback> callback)
// {
//    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackMutex);
   
//    ossimPlanetTextureLayer::CallbackList::iterator iter = std::find(theCallbackList.begin(), theCallbackList.end(), callback);
//    if(iter == theCallbackList.end())
//    {
//       theCallbackList.push_back(callback);
//    }
// }

// void ossimPlanetTextureLayer::removeCallback(osg::ref_ptr<ossimPlanetTextureLayer::Callback> callback)
// {
//    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackMutex);
//    ossimPlanetTextureLayer::CallbackList::iterator iter = std::find(theCallbackList.begin(), theCallbackList.end(), callback);
//    if(iter != theCallbackList.end())
//    {
//       theCallbackList.erase(iter);
//    }   
// }


void ossimPlanetTextureLayer::notifyRefreshExtent(osg::ref_ptr<ossimPlanetExtents> extent)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx;
   for(idx =0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->refreshExtent(extent);
      }
   }
}

void ossimPlanetTextureLayer::notifyLayerAdded(osg::ref_ptr<ossimPlanetTextureLayer> layer)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx;
   for(idx =0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->layerAdded(layer);
      }
   }
}


void ossimPlanetTextureLayer::notifyLayerRemoved(osg::ref_ptr<ossimPlanetTextureLayer> layer,
                                                 osg::ref_ptr<ossimPlanetTextureLayer> parent)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx;
   for(idx =0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->layerRemoved(layer, parent);
      }
   }   
}


void ossimPlanetTextureLayer::notifyPropertyChanged(const ossimString& name,
                                                    const ossimPlanetTextureLayer* layer)const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx;
   for(idx =0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->propertyChanged(name, layer);
      }
   }   
}

void ossimPlanetTextureLayer::resetLookAt()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   double lat, lon, len;
   getCenterLatLonLength(lat, lon, len);

   theLookAt = new ossimPlanetLookAt;
   
   theLookAt->setLat(lat);
   theLookAt->setLon(lon);
   theLookAt->setRange(len);
}

void ossimPlanetTextureLayer::convertToOsg(ossimImageData* data,
                                           ossimPlanetImage* image)const
{
   if(data->getScalarType() == OSSIM_UINT8)
   {
      ossim_uint32 w = data->getWidth();
      ossim_uint32 h = data->getHeight();
      ossim_uint32 area = w*h;
      ossim_uint32 idx = 0;
      unsigned char* buf = new unsigned char[area*4];
      unsigned char* bufPtr = buf;
      
      unsigned char* b1 = NULL; 
      unsigned char* b2 = NULL; 
      unsigned char* b3 = NULL; 
      unsigned char np1 = 0;
      unsigned char np2 = 0;
      unsigned char np3 = 0;
      
      if(data->getNumberOfBands()< 3)
      {
         b1 = (unsigned char*)data->getBuf(0);
         b2 = (unsigned char*)data->getBuf(0);
         b3 = (unsigned char*)data->getBuf(0);
         np1 = (unsigned char)data->getNullPix(0);
         np2 = (unsigned char)data->getNullPix(0);
         np3 = (unsigned char)data->getNullPix(0);
      }
      else
      {
         b1 = (unsigned char*)data->getBuf(0);
         b2 = (unsigned char*)data->getBuf(1);
         b3 = (unsigned char*)data->getBuf(2);
         np1 = (unsigned char)data->getNullPix(0);
         np2 = (unsigned char)data->getNullPix(1);
         np3 = (unsigned char)data->getNullPix(2);
      }
      
      if(!theTransparentColorFlag)
      {
         for(idx = 0; idx < area; ++idx)
         {
            if((*b1==np1)&&
               (*b2==np2)&&
               (*b3==np3))
            {
               bufPtr[3] = 0;
            }
            else
            {
               bufPtr[3] = 255;
            }
            bufPtr[0] = *b1;
            bufPtr[1] = *b2;
            bufPtr[2] = *b3;
            bufPtr +=4;
            ++b1;
            ++b2;
            ++b3;
         }
      }
      else
      {
         const unsigned char* transparentColor = &theTransparentColorVector.front();
         for(idx = 0; idx < area; ++idx)
         {
            if(((*b1==np1)&&
                (*b2==np2)&&
                (*b3==np3))||
               ((*b1==transparentColor[0])&&
             (*b2==transparentColor[1])&&
                (*b3==transparentColor[2])))
            {
               bufPtr[3] = 0;
            }
            else
            {
               bufPtr[3] = 255;
            }
            bufPtr[0] = *b1;
            bufPtr[1] = *b2;
            bufPtr[2] = *b3;
            bufPtr +=4;
            ++b1;
            ++b2;
            ++b3;
         }
         
      }
      
      image->setImage(w,
                      h,
                      1,
                      GL_RGBA,
                      GL_RGBA,
                      GL_UNSIGNED_BYTE,
                      buf,
                      osg::Image::USE_NEW_DELETE);
//    image->flipVertical();
      image->setPixelStatus();
   }
}

void ossimPlanetTextureLayer::convertToOsg(ossimImageData* data,
                                           ossimPlanetImage* image,
                                           const osg::Vec2d& ulInput,
                                           const osg::Vec2d& /*urInput*/,
                                           const osg::Vec2d& lrInput,
                                           const osg::Vec2d& /*llInput*/,
                                           ossimPlanetGrid::ModelPoints& outputGridPoints,
                                           ossim_uint32 outputWidth,
                                           ossim_uint32 outputHeight)const
{
   ossim_uint32 inputW = data->getWidth();
   ossim_uint32 inputH = data->getHeight();
   ossim_uint32 w = outputWidth;
   ossim_uint32 h = outputHeight;
   ossim_uint32 area = w*h;
   ossim_uint32 idx = 0;
   unsigned char* buf;
   osg::Vec3d latLon;
   unsigned char* b1 = NULL; 
   unsigned char* b2 = NULL; 
   unsigned char* b3 = NULL; 
   unsigned char np1 = 0;
   unsigned char np2 = 0;
   unsigned char np3 = 0;
   bool bufExists = (image->data()!=0);
   
   if(!bufExists)
   {
      buf = new unsigned char[area*4];
      memset(buf, '\0', area*4);
   }
   else
   {
      buf = image->data();
   }
   unsigned char* bufPtr = buf;
   if(data->getNumberOfBands()< 3)
   {
      b1 = (unsigned char*)data->getBuf(0);
      b2 = (unsigned char*)data->getBuf(0);
      b3 = (unsigned char*)data->getBuf(0);
      np1 = (unsigned char)data->getNullPix(0);
      np2 = (unsigned char)data->getNullPix(0);
      np3 = (unsigned char)data->getNullPix(0);
   }
   else
   {
      b1 = (unsigned char*)data->getBuf(0);
      b2 = (unsigned char*)data->getBuf(1);
      b3 = (unsigned char*)data->getBuf(2);
      np1 = (unsigned char)data->getNullPix(0);
      np2 = (unsigned char)data->getNullPix(1);
      np3 = (unsigned char)data->getNullPix(2);
   }
   
   ossim_uint32 y = 0;
   ossim_uint32 x = 0;
   
   osg::Vec2d inputT;
   ossim_uint32 inputIdx = 0;
   osg::Vec2d tempDelta;
   double deltaInputX = std::fabs(lrInput[0] - ulInput[0]);
   double deltaInputY = std::fabs(lrInput[1] - ulInput[1]);
   if(!theTransparentColorFlag)
   {
      for(y = 0; y < h; ++y)
      {
         bufPtr = buf + y*w*4;
         
         for(x = 0; x < w; ++x, bufPtr += 4)
         {
            latLon[0] = outputGridPoints[idx].y();
            latLon[1] = outputGridPoints[idx].x();
            tempDelta[0] = latLon[1] - ulInput[0];
            tempDelta[1] = ulInput[1] - latLon[0];
            tempDelta[0]/=deltaInputX;
            tempDelta[1]/=deltaInputY;
            
            ossim_float32 xf = tempDelta[0]*inputW;
            ossim_float32 yf = tempDelta[1]*inputH;
            ossim_int32 xi = (ossim_int32)(xf);
            ossim_int32 yi = (ossim_int32)(yf);
            if(xi == -1) xi++;
            if(yi == -1) yi++;
            if(xi == inputW) xi--;
            if(yi == inputH) yi--;
            if((xi>=0)&&
               (xi<(int)inputW)&&
               (yi>=0)&&
               (yi<(int)inputH))
            {
               //             if(xi < 0) xi =0;
               //             if(xi >= inputW) xi = inputW-1;
               //             if(yi < 0) yi = 0;
               //             if(yi >= inputH) yi = inputH-1;
               inputIdx = yi*inputW + xi;
               if((b1[inputIdx]==np1)&&
                  (b2[inputIdx]==np2)&&
                  (b3[inputIdx]==np3))
               {
                  bufPtr[3] = 0;
                  bufPtr[0] = b1[inputIdx];
                  bufPtr[1] = b2[inputIdx];
                  bufPtr[2] = b3[inputIdx];
               }
               else
               {
                  
                  bufPtr[3] = 255;
                  ossim_int32 urx = xi+1;
                  ossim_int32 lry = yi+1;
                  
                  if(urx >= (int)inputW) urx = inputW-1;
                  if(lry >= (int)inputH) lry = inputH-1;
                  
                  double xt0 = xf - xi;
                  double yt0 = yf - yi;
                  double xt1 = 1-xt0;
                  double yt1 = 1-yt0;
                  double w00 = xt1*yt1;
                  double w01 = xt0*yt1;
                  double w10 = xt1*yt0;
                  double w11 = xt0*yt0;
                  
                  int uridx = yi*inputW + urx;
                  int lridx = lry*inputW + urx;
                  int llidx = lry*inputW + xi;
                  
                  ossim_float32 wsum = w00 + w01 + w10 + w11;
                  wsum = 1.0/wsum;
                  bufPtr[0] = (unsigned char)((b1[inputIdx]*w00 + b1[uridx]*w01 + b1[llidx]*w10+b1[lridx]*w11)*wsum);
                  bufPtr[1] = (unsigned char)((b2[inputIdx]*w00 + b2[uridx]*w01 + b2[llidx]*w10+b2[lridx]*w11)*wsum);
                  bufPtr[2] = (unsigned char)((b3[inputIdx]*w00 + b3[uridx]*w01 + b3[llidx]*w10+b3[lridx]*w11)*wsum);
                  //                   bufPtr[1] = b2[inputIdx];
                  //                   bufPtr[2] = b3[inputIdx];
                  bufPtr[3] = 255;
                  
               }
            }
            ++idx;
         }
      }
   }
   else
   {
      const unsigned char* transparentColor = &theTransparentColorVector.front();
      for(y = 0; y < h; ++y)
      {
         bufPtr = buf + y*w*4;
         
         for(x = 0; x < w; ++x, bufPtr += 4)
         {
            latLon[0] = outputGridPoints[idx].y();
            latLon[1] = outputGridPoints[idx].x();
            tempDelta[0] = latLon[1] - ulInput[0];
            tempDelta[1] = ulInput[1] - latLon[0];
            tempDelta[0]/=deltaInputX;
            tempDelta[1]/=deltaInputY;
            
            ossim_float32 xf = tempDelta[0]*inputW;
            ossim_float32 yf = tempDelta[1]*inputH;
            ossim_int32 xi = (ossim_int32)(xf);
            ossim_int32 yi = (ossim_int32)(yf);
            if(xi == -1) xi++;
            if(yi == -1) yi++;
            if(xi == inputW) xi--;
            if(yi == inputH) yi--;
            if((xi>=0)&&
               (xi<(int)inputW)&&
               (yi>=0)&&
               (yi<(int)inputH))
            {
               
               //             if(xi < 0) xi =0;
               //             if(xi >= inputW) xi = inputW-1;
               //             if(yi < 0) yi = 0;
               //             if(yi >= inputH) yi = inputH-1;
               inputIdx = yi*inputW + xi;
               
               if(((b1[inputIdx]==np1)&&
                   (b2[inputIdx]==np2)&&
                   (b3[inputIdx]==np3))||
                  ((b1[inputIdx]==transparentColor[0])&&
                   (b2[inputIdx]==transparentColor[1])&&
                   (b3[inputIdx]==transparentColor[2])))
               {
                  bufPtr[3] = 0;
               }
               else
               {
                  bufPtr[3] = 255;
                  ossim_int32 urx = xi+1;
                  ossim_int32 lry = yi+1;
                  
                  if(urx >= (int)inputW) urx = inputW-1;
                  if(lry >= (int)inputH) lry = inputH-1;
                  
                  double xt0 = xf - xi;
                  double yt0 = yf - yi;
                  double xt1 = 1-xt0;
                  double yt1 = 1-yt0;
                  double w00 = xt1*yt1;
                  double w01 = xt0*yt1;
                  double w10 = xt1*yt0;
                  double w11 = xt0*yt0;
                  
                  int uridx = yi*inputW + urx;
                  int lridx = lry*inputW + urx;
                  int llidx = lry*inputW + xi;
                  
                  ossim_float32 wsum = w00 + w01 + w10 + w11;
                  wsum = 1.0/wsum;
                  bufPtr[0] = (unsigned char)((b1[inputIdx]*w00 + b1[uridx]*w01 + b1[llidx]*w10+b1[lridx]*w11)*wsum);
                  bufPtr[1] = (unsigned char)((b2[inputIdx]*w00 + b2[uridx]*w01 + b2[llidx]*w10+b2[lridx]*w11)*wsum);
                  bufPtr[2] = (unsigned char)((b3[inputIdx]*w00 + b3[uridx]*w01 + b3[llidx]*w10+b3[lridx]*w11)*wsum);
                  //                   bufPtr[1] = b2[inputIdx];
                  //                   bufPtr[2] = b3[inputIdx];
                  bufPtr[3] = 255;
               }
            }
            ++idx;
         }
      }
   }
   if(!bufExists)
   {
      image->setImage(w,
                      h,
                      1,
                      GL_RGBA,
                      GL_RGBA,
                      GL_UNSIGNED_BYTE,
                      buf,
                      osg::Image::USE_NEW_DELETE);
   }
   image->setPixelStatus();
}



void ossimPlanetTextureLayer::convertToOsg(ossimImageData* data,
                                           ossimPlanetImage* image,
                                           const osg::Vec2d& ulInput,
                                           const osg::Vec2d& /*urInput*/,
                                           const osg::Vec2d& lrInput,
                                           const osg::Vec2d& /*llInput*/,
                                           std::vector<ossimPlanetGridUtility::GridPoint>& outputGridPoints,
                                           const ossimPlanetGridUtility& utility,
                                           ossim_uint32 outputWidth,
                                           ossim_uint32 outputHeight)const
{
   ossim_uint32 inputW = data->getWidth();
   ossim_uint32 inputH = data->getHeight();
   ossim_uint32 w = outputWidth;
   ossim_uint32 h = outputHeight;
   ossim_uint32 area = w*h;
   ossim_uint32 idx = 0;
   unsigned char* buf;
   osg::Vec3d latLon;
   unsigned char* b1 = NULL; 
   unsigned char* b2 = NULL; 
   unsigned char* b3 = NULL; 
   unsigned char np1 = 0;
   unsigned char np2 = 0;
   unsigned char np3 = 0;
   bool bufExists = (image->data()!=0);

   if(!bufExists)
   {
      buf = new unsigned char[area*4];
      memset(buf, '\0', area*4);
   }
   else
   {
      buf = image->data();
   }
   unsigned char* bufPtr = buf;
   if(data->getNumberOfBands()< 3)
   {
      b1 = (unsigned char*)data->getBuf(0);
      b2 = (unsigned char*)data->getBuf(0);
      b3 = (unsigned char*)data->getBuf(0);
      np1 = (unsigned char)data->getNullPix(0);
      np2 = (unsigned char)data->getNullPix(0);
      np3 = (unsigned char)data->getNullPix(0);
   }
   else
   {
      b1 = (unsigned char*)data->getBuf(0);
      b2 = (unsigned char*)data->getBuf(1);
      b3 = (unsigned char*)data->getBuf(2);
      np1 = (unsigned char)data->getNullPix(0);
      np2 = (unsigned char)data->getNullPix(1);
      np3 = (unsigned char)data->getNullPix(2);
   }
  
   ossim_uint32 y = 0;
   ossim_uint32 x = 0;

   osg::Vec2d inputT;
   ossim_uint32 inputIdx = 0;
   osg::Vec2d tempDelta;
   double deltaInputX = std::fabs(lrInput[0] - ulInput[0]);
   double deltaInputY = std::fabs(lrInput[1] - ulInput[1]);
   if(!theTransparentColorFlag)
   {
      for(y = 0; y < h; ++y)
      {
         bufPtr = buf + y*w*4;
         
         for(x = 0; x < w; ++x, bufPtr += 4)
         {
            utility.getLatLon(latLon, outputGridPoints[idx]);
            tempDelta[0] = latLon[ossimPlanetGridUtility::LON] - ulInput[0];
            tempDelta[1] = ulInput[1] - latLon[ossimPlanetGridUtility::LAT];
            tempDelta[0]/=deltaInputX;
            tempDelta[1]/=deltaInputY;

            ossim_float32 xf = tempDelta[0]*inputW;
            ossim_float32 yf = tempDelta[1]*inputH;
            ossim_int32 xi = (ossim_int32)(xf);
            ossim_int32 yi = (ossim_int32)(yf);

            if((xi>=0)&&
               (xi<(int)inputW)&&
               (yi>=0)&&
               (yi<(int)inputH))
            {
//             if(xi < 0) xi =0;
//             if(xi >= inputW) xi = inputW-1;
//             if(yi < 0) yi = 0;
//             if(yi >= inputH) yi = inputH-1;
               inputIdx = yi*inputW + xi;
               if((b1[inputIdx]==np1)&&
                  (b2[inputIdx]==np2)&&
                  (b3[inputIdx]==np3))
               {
                  bufPtr[3] = 0;
                  bufPtr[0] = b1[inputIdx];
                  bufPtr[1] = b2[inputIdx];
                  bufPtr[2] = b3[inputIdx];
               }
               else
               {
                  
                  bufPtr[3] = 255;
                  ossim_int32 urx = xi+1;
                  ossim_int32 lry = yi+1;
                  
                  if(urx >= (int)inputW) urx = inputW-1;
                  if(lry >= (int)inputH) lry = inputH-1;
                  
                  double xt0 = xf - xi;
                  double yt0 = yf - yi;
                  double xt1 = 1-xt0;
                  double yt1 = 1-yt0;
                  double w00 = xt1*yt1;
                  double w01 = xt0*yt1;
                  double w10 = xt1*yt0;
                  double w11 = xt0*yt0;

                  int uridx = yi*inputW + urx;
                  int lridx = lry*inputW + urx;
                  int llidx = lry*inputW + xi;
                  
                  ossim_float32 wsum = w00 + w01 + w10 + w11;
                  wsum = 1.0/wsum;
                  bufPtr[0] = (unsigned char)((b1[inputIdx]*w00 + b1[uridx]*w01 + b1[llidx]*w10+b1[lridx]*w11)*wsum);
                  bufPtr[1] = (unsigned char)((b2[inputIdx]*w00 + b2[uridx]*w01 + b2[llidx]*w10+b2[lridx]*w11)*wsum);
                  bufPtr[2] = (unsigned char)((b3[inputIdx]*w00 + b3[uridx]*w01 + b3[llidx]*w10+b3[lridx]*w11)*wsum);
//                   bufPtr[1] = b2[inputIdx];
//                   bufPtr[2] = b3[inputIdx];
                  bufPtr[3] = 255;

               }
            }
            ++idx;
         }
      }
   }
   else
   {
      const unsigned char* transparentColor = &theTransparentColorVector.front();
      for(y = 0; y < h; ++y)
      {
         bufPtr = buf + y*w*4;
         
         for(x = 0; x < w; ++x, bufPtr += 4)
         {
            utility.getLatLon(latLon, outputGridPoints[idx]);
            tempDelta[0] = latLon[ossimPlanetGridUtility::LON] - ulInput[0];
            tempDelta[1] = ulInput[1] - latLon[ossimPlanetGridUtility::LAT];
            //osg::Vec2d(latLon[1], latLon[0])-ulInput;
//             tempDelta[0] = tempDelta[0];
//             tempDelta[1] = tempDelta[1];
            tempDelta[0]/=deltaInputX;
            tempDelta[1]/=deltaInputY;

            ossim_float32 xf = tempDelta[0]*inputW;
            ossim_float32 yf = tempDelta[1]*inputH;
            ossim_int32 xi = (ossim_int32)(xf);
            ossim_int32 yi = (ossim_int32)(yf);
            if((xi>=0)&&
               (xi<(int)inputW)&&
               (yi>=0)&&
               (yi<(int)inputH))
            {
               
//             if(xi < 0) xi =0;
//             if(xi >= inputW) xi = inputW-1;
//             if(yi < 0) yi = 0;
//             if(yi >= inputH) yi = inputH-1;
               inputIdx = yi*inputW + xi;
               
               if(((b1[inputIdx]==np1)&&
                   (b2[inputIdx]==np2)&&
                   (b3[inputIdx]==np3))||
                  ((b1[inputIdx]==transparentColor[0])&&
                   (b2[inputIdx]==transparentColor[1])&&
                   (b3[inputIdx]==transparentColor[2])))
               {
                  bufPtr[3] = 0;
               }
               else
               {
                  bufPtr[3] = 255;
                  ossim_int32 urx = xi+1;
                  ossim_int32 lry = yi+1;
                  
                  if(urx >= (int)inputW) urx = inputW-1;
                  if(lry >= (int)inputH) lry = inputH-1;
                  
                  double xt0 = xf - xi;
                  double yt0 = yf - yi;
                  double xt1 = 1-xt0;
                  double yt1 = 1-yt0;
                  double w00 = xt1*yt1;
                  double w01 = xt0*yt1;
                  double w10 = xt1*yt0;
                  double w11 = xt0*yt0;

                  int uridx = yi*inputW + urx;
                  int lridx = lry*inputW + urx;
                  int llidx = lry*inputW + xi;

                  ossim_float32 wsum = w00 + w01 + w10 + w11;
                  wsum = 1.0/wsum;
                  bufPtr[0] = (unsigned char)((b1[inputIdx]*w00 + b1[uridx]*w01 + b1[llidx]*w10+b1[lridx]*w11)*wsum);
                  bufPtr[1] = (unsigned char)((b2[inputIdx]*w00 + b2[uridx]*w01 + b2[llidx]*w10+b2[lridx]*w11)*wsum);
                  bufPtr[2] = (unsigned char)((b3[inputIdx]*w00 + b3[uridx]*w01 + b3[llidx]*w10+b3[lridx]*w11)*wsum);
//                   bufPtr[1] = b2[inputIdx];
//                   bufPtr[2] = b3[inputIdx];
                  bufPtr[3] = 255;
               }
            }
            ++idx;
         }
      }
   }
   if(!bufExists)
   {
      image->setImage(w,
                      h,
                      1,
                      GL_RGBA,
                      GL_RGBA,
                      GL_UNSIGNED_BYTE,
                      buf,
                      osg::Image::USE_NEW_DELETE);
   }
//    image->flipVertical();
   image->setPixelStatus();
}

OpenThreads::Mutex& ossimPlanetTextureLayer::mutex()
{
   return theMutex;
}

void ossimPlanetTextureLayer::getMetadata(ossimRefPtr<ossimXmlNode> /*metadata*/)const
{
   return;
}

ossimRefPtr<ossimXmlNode> ossimPlanetTextureLayer::saveXml(bool /*recurseFlag*/)const
{
   ossimXmlNode* node = new ossimXmlNode();
   node->setTag(getClassName());
   node->addChildNode("name", getName());
   node->addChildNode("description", getDescription());
   node->addChildNode("id", theId);
   node->addChildNode("enableFlag", ossimString::toString(theEnableFlag));

   ossimXmlNode* transparentColorNode = new ossimXmlNode;
   transparentColorNode->setTag("transparentColor");
   transparentColorNode->addChildNode("enableFlag", ossimString::toString(theTransparentColorFlag));
   transparentColorNode->addChildNode("color", (ossimString::toString(theTransparentColorVector[0])+ " " +
                                                ossimString::toString(theTransparentColorVector[1])+ " " +
                                                ossimString::toString(theTransparentColorVector[2])));
   node->addChildNode(transparentColorNode);
   node->addChildNode("filterType", theFilterType);
   if(theExtents.valid())
   {
      node->addChildNode(theExtents->saveXml().get());
   }
   if(theLookAt.valid())
   {
      node->addChildNode(theLookAt->saveXml().get());
   }
   
   return node;
}

bool ossimPlanetTextureLayer::loadXml(ossimRefPtr<ossimXmlNode> node)
{
   if(!node.valid()) return false;
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = node->getChildNodes();
   ossim_uint32 idx = 0;
   ossim_uint32 upper = childNodes.size();
   for(idx = 0; idx < upper; ++idx)
   {
      ossimString tag = childNodes[idx]->getTag();
      if(tag == "name")
      {
         theName = childNodes[idx]->getText();
      }
      else if(tag == "description")
      {
         theDescription = childNodes[idx]->getText();
      }
      else if(tag == "id")
      {
         theId = childNodes[idx]->getText();
      }
      else if(tag == "enableFlag")
      {
         theEnableFlag = childNodes[idx]->getText().toBool();
      }
      else if(tag == "transparentColor")
      {
         ossimString temp;
         
         if(childNodes[idx]->getChildTextValue(temp, "enableFlag"))
         {
            theTransparentColorFlag = temp.toBool();
         }
         if(childNodes[idx]->getChildTextValue(temp, "color"))
         {
            int r,g,b;
            istringstream in(temp);
            in>>r>>g>>b;
            theTransparentColorVector[0] = (unsigned char)r;
            theTransparentColorVector[1] = (unsigned char)g;
            theTransparentColorVector[2] = (unsigned char)b;
         }
      }
      else if(tag == "filterType")
      {
         theFilterType = childNodes[idx]->getText();
      }
      else if(tag == "ossimPlanetLookAt")
      {
         theLookAt = new ossimPlanetLookAt;
         theLookAt->loadXml(childNodes[idx]);
      }
   }

   return true;
}

ossimPlanetTextureLayer* ossimPlanetTextureLayer::findLayerByName(const ossimString& layerName,
                                                                  bool /*recurseFlag*/)
{
   if(layerName == name())
   {
      return this;
   }

   return 0;
}

const ossimPlanetTextureLayer* ossimPlanetTextureLayer::findLayerByName(const ossimString& layerName,
                                                                        bool /*recurseFlag*/)const
{
   if(layerName == name())
   {
      return this;
   }

   return 0;
}

ossimPlanetTextureLayer* ossimPlanetTextureLayer::findLayerByNameAndId(const ossimString& layerName,
                                                                       const ossimString& id)
{
   if((name() == layerName)&&
      (this->id()   == id))
   {
      return this;
   }

   return 0;
}

const ossimPlanetTextureLayer* ossimPlanetTextureLayer::findLayerByNameAndId(const ossimString& layerName,
                                                                             const ossimString& id)const
{
   if((name() == layerName)&&
      (this->id()   == id))
   {
      return this;
   }

   return 0;
}

ossimPlanetTextureLayer* ossimPlanetTextureLayer::findLayerById(const ossimString& id,
                                                                bool /*recurseFlag*/)
{
   if(this->id() == id) return this;
   return 0;
}

const ossimPlanetTextureLayer* ossimPlanetTextureLayer::findLayerById(const ossimString& id,
                                                                      bool /*recurseFlag*/)const
{
   if(this->id() == id) return this;
   return 0;
}
  
void ossimPlanetTextureLayer::remove()
{
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theParentList.size(); ++idx)
   {
      ossimPlanetTextureLayerGroup* p = parent(idx);
      p->removeLayer(this);
   }
}

ossim_float32 ossimPlanetTextureLayer::brightness()const
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
  return theBrightness;
}

ossim_float32 ossimPlanetTextureLayer::contrast()const
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
  return theContrast;
}

ossim_float32 ossimPlanetTextureLayer::opacity()const
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
  return theOpacity;
}

/**
 * Allows one to enable brightness contrast settings.
 */
void ossimPlanetTextureLayer::setBrightnessContrast(ossim_float32 brightnessValue,
                                                    ossim_float32 contrastValue,
                                                    bool notifyFlag)
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
  theBrightness = brightnessValue;
  theContrast   = contrastValue;
  if(notifyFlag)
  {
    notifyRefreshExtent(getExtents().get());
  }
}

void ossimPlanetTextureLayer::setBrightness(ossim_float32 brightnessValue,
                                            bool notifyFlag)
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
  theBrightness = brightnessValue;
  if(notifyFlag)
  {
    notifyRefreshExtent(getExtents().get());
  }
}

void ossimPlanetTextureLayer::setContrast(ossim_float32 contrastValue,
                                          bool notifyFlag)
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
  theContrast   = contrastValue;
  if(notifyFlag)
  {
    notifyRefreshExtent(getExtents().get());
  }
}

/**
 * Allows one to set the opacity.
 *
 * @param opacityValue value of 1.0 is fully opaque and a value of 0 is fully transparent
 */
void ossimPlanetTextureLayer::setOpacity(ossim_float32 opacityValue,
                                         bool notifyFlag)
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
  theOpacity = opacityValue;

  if(notifyFlag)
  {
    notifyRefreshExtent(getExtents().get());
  }
}
