#include <iostream>
#include <ossimPlanet/ossimPlanetImage.h>
#include <OpenThreads/ScopedLock>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimHsiVector.h>
#include <ossim/base/ossimRgbVector.h>
#include <osgDB/ReadFile>
//#define OSGPLANET_ENABLE_ALLOCATION_COUNT
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
static ossim_uint32 imageCount = 0;
#endif

ossimPlanetImage::ossimPlanetImage()
:osg::Image(),
theState(ossimPlanetImageStateType_NONE),
thePixelStatus(ossimPlanetImagePixelStatus_EMPTY),
thePadding(0)
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   ++imageCount;
   std::cout << "ossimPlanetImage2D count = " << imageCount << std::endl;
#endif
}

ossimPlanetImage::ossimPlanetImage(const osg::Image& src)
:osg::Image(src),
theState(ossimPlanetImageStateType_NONE),
thePixelStatus(ossimPlanetImagePixelStatus_EMPTY),
thePadding(0)
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   ++imageCount;
   std::cout << "ossimPlanetImage2D count = " << imageCount << std::endl;
#endif
}

ossimPlanetImage::ossimPlanetImage(const ossimPlanetTerrainTileId& id)
:osg::Image(),
theState(ossimPlanetImageStateType_NONE),
theTileId(id),
thePixelStatus(ossimPlanetImagePixelStatus_EMPTY),
thePadding(0)
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   ++imageCount;
   std::cout << "ossimPlanetImage2D count = " << imageCount << std::endl;
#endif
}

ossimPlanetImage::ossimPlanetImage(const ossimPlanetImage& image, 
const osg::CopyOp& copyop)
:osg::Image(image,copyop),
theState(image.theState),
theTileId(image.theTileId),
thePixelStatus(image.thePixelStatus),
theMinValue(image.theMinValue),
theMaxValue(image.theMaxValue),
thePadding(image.thePadding)
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   ++imageCount;
   std::cout << "ossimPlanetImage2D count = " << imageCount << std::endl;
#endif
}

ossimPlanetImage::~ossimPlanetImage()
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   --imageCount;
   std::cout << "ossimPlanetImage2D count = " << imageCount << std::endl;
#endif
}

void ossimPlanetImage::stripPadding()
{
   if(padding() > 0)
   {
      osg::ref_ptr<ossimPlanetImage> tempImage = new ossimPlanetImage(*this);
      allocateImage(widthWithoutPadding(), 
                    heightWithoutPadding(), 
                    1,
                    getPixelFormat(), 
                    getDataType(), 
                    getPacking());
      setPadding(0);
      tempImage->copyTo(tempImage->padding(), tempImage->padding(), this);
   }
}

int ossimPlanetImage::getNumberOfComponents()const
{
   return computeNumComponents(getPixelFormat());
}

int ossimPlanetImage::getHeight()const
{
   return t();
}

int ossimPlanetImage::getWidth()const
{
   return s();
}

void ossimPlanetImage::setId(const ossimPlanetTerrainTileId& id)
{
   theTileId = id;
}

const ossimPlanetTerrainTileId& ossimPlanetImage::tileId()const
{
   return theTileId;
}

ossimPlanetImage::ossimPlanetImageStateType ossimPlanetImage::getState()const
{
   return theState;
}

void ossimPlanetImage::setState(ossimPlanetImageStateType stateType)
{
   theState = stateType;
}

OpenThreads::Mutex& ossimPlanetImage::mutex()const
{
   return theMutex;
}

void ossimPlanetImage::setPixelStatus()
{
   GLint internalFormat = getInternalTextureFormat();
   switch(internalFormat)
   {
      case GL_RGBA:
      {
         unsigned char* dataPtr = data();
         unsigned int w = s();
         unsigned int h = t();
         unsigned int area = w*h;
         thePixelStatus = ossimPlanetImagePixelStatus_FULL;
         bool hasFull = false;
         bool hasTranslucent = false;
         bool hasTransparent = false;
         if(!dataPtr) return;
         
         for(unsigned int idx = 0; idx < area;++idx)
         {
            if(dataPtr[3] == 0)
            {
               hasTransparent = true;
            }
            if(dataPtr[3] == 255)
            {
               hasFull = true;
            }
            else
            {
               hasTranslucent = true;
            }
            if((hasTranslucent) ||
               (hasFull&&hasTransparent))
            {
               break;
            }
            dataPtr+=4;
         }
         if(hasTranslucent)
         {
            thePixelStatus = ossimPlanetImagePixelStatus_PARTIAL;
         }
         else if(hasTransparent&&hasFull)
         {
            thePixelStatus = ossimPlanetImagePixelStatus_PARTIAL;
         }
         else if(hasTransparent)
         {
            thePixelStatus = ossimPlanetImagePixelStatus_EMPTY;
         }
         else if(hasFull)
         {
            thePixelStatus = ossimPlanetImagePixelStatus_FULL;
         }
         break;
      }
      default:
      {
         thePixelStatus = ossimPlanetImagePixelStatus_FULL;
      }
   }
}

void ossimPlanetImage::setPixelStatus(ossimPlanetImagePixelStatus pixelStatus)
{
   thePixelStatus = pixelStatus;
}

ossimPlanetImage::ossimPlanetImagePixelStatus ossimPlanetImage::getPixelStatus()const
{
   return thePixelStatus;
}

void ossimPlanetImage::applyBrightnessContrast(ossim_float32 brightness, ossim_float32 contrast)
{
  GLint internalFormat = getInternalTextureFormat();
  ossim_uint32 width  = s();
  ossim_uint32 height = t();
  ossim_uint32 inputBands = 4;
  switch(internalFormat)
  {
     case GL_RGBA:
     {
       ossim_float32 normBrightness = brightness/255.0;
       ossim_uint8* inputPtr = (ossim_uint8*)data();
       ossim_uint32 size = width*height;
       ossim_uint32 idx = 0;
       inputBands = 4;
       for(;idx < size; ++idx)
       {
         if(inputPtr[3] > 0.0)
         {
           ossimHsiVector hsi(ossimRgbVector(inputPtr[0], inputPtr[1], inputPtr[2]));

           ossim_float32 i = hsi.getI()*contrast + normBrightness;
           //           i = ossim::clamp(i, OSSIM_DEFAULT_MIN_PIX_NORM_FLOAT, 1.0f);
           i = ossim::clamp(i, 0.0f, 1.0f);
           hsi.setI(i);
           ossimRgbVector newRgb(hsi);
           inputPtr[0] = newRgb.getR();
           inputPtr[1] = newRgb.getG();
           inputPtr[2] = newRgb.getB();
         }
         inputPtr+=inputBands;
       }
       break;
     }
     default:
     {
       break;
     }
  }
}

ossimRefPtr<ossimImageData> ossimPlanetImage::toOssimImage()const
{
   ossimRefPtr<ossimImageData> imageData;
   GLint internalFormat = getInternalTextureFormat();
   ossim_uint32 inputBands = 3;
   if(internalFormat == GL_RGBA)
   {
      inputBands = 4;
   }

   if(!data()) return 0;
   ossim_uint32 width  = s();
   ossim_uint32 height = t();

   switch(internalFormat)
   {
      case GL_RGB:
      case GL_RGBA:
      {
         if((internalFormat == GL_RGB)||
            (internalFormat == GL_RGBA))
         {
            imageData = new ossimImageData(0,
                                           OSSIM_UINT8,
                                           3,
                                           width,
                                           height);
            imageData->initialize();
            const ossim_uint8* inputPtr = (const ossim_uint8*)data();
            ossim_uint8* outputPtr[3];
            outputPtr[0] = (ossim_uint8*)(imageData->getBuf(0));
            outputPtr[1] = (ossim_uint8*)(imageData->getBuf(1));
            outputPtr[2] = (ossim_uint8*)(imageData->getBuf(2));
            ossim_uint32 idx = 0;
            ossim_uint32 size = width*height;
            if(internalFormat != GL_RGBA)
            {
               for(idx =0; idx < size; ++idx)
               {
                  outputPtr[0][idx] = inputPtr[0];
                  outputPtr[1][idx] = inputPtr[1];
                  outputPtr[2][idx] = inputPtr[2];
                  inputPtr+=inputBands;
               }
            }
            else
            {
               for(idx =0; idx < size; ++idx)
               {
                  if(inputPtr[3] == 0)
                  {
                     outputPtr[0][idx] = 0;
                     outputPtr[1][idx] = 0;
                     outputPtr[2][idx] = 0;
                     
                  }
                  else
                  {
                     outputPtr[0][idx] = inputPtr[0];
                     outputPtr[1][idx] = inputPtr[1];
                     outputPtr[2][idx] = inputPtr[2];
                  }
                  inputPtr+=inputBands;
               }
            }
            imageData->validate();
         }
         break;
      }
      case GL_LUMINANCE:
      {
         if(getDataType() == GL_FLOAT)
         {
            imageData = new ossimImageData(0,
                                           OSSIM_FLOAT32,
                                           1,
                                           width,
                                           height);
            imageData->initialize();
            const ossim_float32* inputPtr = (const ossim_float32*)data();
            ossim_float32* outputPtr = (ossim_float32*)imageData->getBuf();
            ossim_uint32 idx = 0;
            ossim_uint32 size = width*height;
            for(idx =0; idx < size; ++idx)
            {
               *outputPtr = *inputPtr;
               ++inputPtr;
               ++outputPtr;
            }
            imageData->setDataObjectStatus(OSSIM_FULL);
         }
         break;
      }
      default:
      {
         break;
      }
   }
   

   return imageData;
}

void ossimPlanetImage::fromOssimImage(ossimRefPtr<ossimImageData> data,
                                      bool reassignNullFlag,
                                      double nullValue)
{
   ossim_uint32 w = 0;
   ossim_uint32 h = 0;
   GLint internalFormat = GL_LUMINANCE;
   GLenum pixelFormat   = GL_LUMINANCE;
   GLenum type          = GL_FLOAT;
   osg::Image::AllocationMode allocMode = osg::Image::USE_NEW_DELETE;
   unsigned char* buf = 0;
   ossim_uint32 validPixelCount = 0;
   ossim_uint32 invalidPixelCount = 0;
   if(data.valid())
   {
      w = data->getWidth();
      h = data->getHeight();
      switch(data->getScalarType())
      {
         case OSSIM_UINT8: // only support 1 to 3 band and will convert to RGBA gl type 
         {
            if(data->getNumberOfBands()>0)
            {
               ossim_uint8 nullPix = (ossim_uint8)data->getNullPix(0);
               ossim_uint32 sizeInBytes = data->getSizePerBandInBytes()*4;
               buf = new unsigned char[sizeInBytes];
               unsigned char* bufPtr = buf;
               
               
               memset(buf, 0, sizeInBytes);

               if(data->getBuf()&&
                  data->getDataObjectStatus() != OSSIM_EMPTY)
               {
                  unsigned char* dataBuf[3];

                  dataBuf[0] = (unsigned char*)data->getBuf(0);
                  if(data->getNumberOfBands() > 1)
                  {
                     dataBuf[1] = (unsigned char*)data->getBuf(1);
                  }
                  else
                  {
                     dataBuf[1] = (unsigned char*)data->getBuf(0);
                  }
                  if(data->getNumberOfBands() > 2)
                  {
                     dataBuf[2] = (unsigned char*)data->getBuf(2);
                  }
                  else
                  {
                     dataBuf[2] = (unsigned char*)data->getBuf(0);
                  }
                  ossim_uint32 area = data->getWidth()*data->getHeight();
                  ossim_uint32 idx = 0;
                  for(;idx < area; ++idx)
                  {
                     if((*dataBuf[0] != nullPix)||
                        (*dataBuf[1] != nullPix)||
                        (*dataBuf[2] != nullPix))
                     {
                        bufPtr[0] = *dataBuf[0];
                        bufPtr[1] = *dataBuf[1];
                        bufPtr[2] = *dataBuf[2];
                        bufPtr[3] = 255;
                        ++validPixelCount;
                     }
                     else
                     {
                       ++invalidPixelCount;
                        bufPtr[3] = 0;
                     }
                     bufPtr+=4;
                     ++dataBuf[0];
                     ++dataBuf[1];
                     ++dataBuf[2];
                  }
                  if(invalidPixelCount > 0)
                  {
                    thePixelStatus = validPixelCount > 0?ossimPlanetImagePixelStatus_PARTIAL:ossimPlanetImagePixelStatus_EMPTY;
                  }
                  else
                  {
                    thePixelStatus = ossimPlanetImagePixelStatus_FULL;
                  }
               }
               else
               {
                 thePixelStatus = ossimPlanetImagePixelStatus_EMPTY;
               }
               internalFormat = GL_RGBA;
               pixelFormat    = GL_RGBA;
               type           = GL_UNSIGNED_BYTE;
            }
            break;
         }
         case OSSIM_FLOAT32: // only support 1 band 
         {
            ossim_float32* bufPtr = new ossim_float32[w*h];
            buf = reinterpret_cast<unsigned char*>(bufPtr);

            memset(buf, 0, w*h*sizeof(ossim_float32));
            
            ossim_float32* dataBuf = (ossim_float32*)data->getBuf();
            ossim_float32 nullPix  = (ossim_float32)data->getNullPix(0);
            ossim_float32 tempNullValue = nullValue;
            
            ossim_uint32 idx = 0;
            ossim_uint32 area = w*h;
            
            if(dataBuf)
            {
               for(;idx < area; ++idx)
               {
                  *bufPtr = *dataBuf;
                  if(reassignNullFlag)
                  {
                     if(nullPix == *dataBuf)
                     {
                        *bufPtr = tempNullValue;
                     }
                  }
                  ++bufPtr;
                  ++dataBuf;
               }
            }
            
            break;
         }
         case OSSIM_SINT16: // only support 1 band
         {
            ossim_float32* bufPtr = new ossim_float32[w*h];
            buf = reinterpret_cast<unsigned char*>(bufPtr);

            memset(buf, 0, w*h*sizeof(ossim_float32));

            ossim_sint16* dataBuf = (ossim_sint16*)data->getBuf();
            ossim_sint16 nullPix  = (ossim_sint16)data->getNullPix(0);
            ossim_float32 tempNullValue = (ossim_float32)nullValue;
            
            ossim_uint32 idx = 0;
            ossim_uint32 area = w*h;
            
            if(dataBuf)
            {
               for(;idx < area; ++idx)
               {
                  *bufPtr = *dataBuf;
                  if(reassignNullFlag)
                  {
                     if(nullPix == *dataBuf)
                     {
                        *bufPtr = tempNullValue;
                     }
                  }
                  ++bufPtr;
                  ++dataBuf;
               }
            }
            break;
         }
         case OSSIM_UINT16: // only support 1 band
         case OSSIM_USHORT11: // only support 1 band
         {
            ossim_float32* bufPtr = new ossim_float32[w*h];
            buf = reinterpret_cast<unsigned char*>(bufPtr);

            memset(buf, 0, w*h*sizeof(ossim_float32));

            ossim_uint16* dataBuf       = (ossim_uint16*)data->getBuf();
            ossim_uint16 nullPix        = (ossim_uint16)data->getNullPix(0);
            ossim_float32 tempNullValue = (ossim_float32)nullValue;
            
            ossim_uint32 idx = 0;
            ossim_uint32 area = w*h;
            
            if(dataBuf)
            {
               for(;idx < area; ++idx)
               {
                  *bufPtr = *dataBuf;
                  if(reassignNullFlag)
                  {
                     if(nullPix == *dataBuf)
                     {
                        *bufPtr = tempNullValue;
                     }
                  }
                  ++bufPtr;
                  ++dataBuf;
               }
            }
            break;
         }
         default:
         {
            // not supported yet
            break;
         }
      }
   }

   if(buf)
   {
      setImage(w, h, 1, internalFormat, pixelFormat, type, buf, allocMode);
   }
}

void ossimPlanetImage::fromOssimImageNoAlpha(ossimRefPtr<ossimImageData> data,
                                             bool reassignNullFlag,
                                             double nullValue)
{
   ossim_uint32 w = 0;
   ossim_uint32 h = 0;
   GLint internalFormat = GL_LUMINANCE;
   GLenum pixelFormat   = GL_LUMINANCE;
   GLenum type          = GL_FLOAT;
   osg::Image::AllocationMode allocMode = osg::Image::USE_NEW_DELETE;
   unsigned char* buf = 0;
   if(data.valid())
   {
      w = data->getWidth();
      h = data->getHeight();
      switch(data->getScalarType())
      {
         case OSSIM_UINT8: // only support 1 to 3 band and will convert to RGBA gl type 
         {
            if(data->getNumberOfBands()>0)
            {
               ossim_uint8 nullPix = (ossim_uint8)data->getNullPix(0);
               ossim_uint32 sizeInBytes = data->getSizePerBandInBytes()*3;
               buf = new unsigned char[sizeInBytes];
               unsigned char* bufPtr = buf;
               
               
               memset(buf, 0, sizeInBytes);

               if(data->getBuf()&&
                  data->getDataObjectStatus() != OSSIM_EMPTY)
               {
                  unsigned char* dataBuf[3];

                  dataBuf[0] = (unsigned char*)data->getBuf(0);
                  if(data->getNumberOfBands() > 1)
                  {
                     dataBuf[1] = (unsigned char*)data->getBuf(1);
                  }
                  else
                  {
                     dataBuf[1] = (unsigned char*)data->getBuf(0);
                  }
                  if(data->getNumberOfBands() > 2)
                  {
                     dataBuf[2] = (unsigned char*)data->getBuf(2);
                  }
                  else
                  {
                     dataBuf[2] = (unsigned char*)data->getBuf(0);
                  }
                  ossim_uint32 area = data->getWidth()*data->getHeight();
                  ossim_uint32 idx = 0;
                  for(;idx < area; ++idx)
                  {
                     if((*dataBuf[0] != nullPix)||
                        (*dataBuf[1] != nullPix)||
                        (*dataBuf[2] != nullPix))
                     {
                        bufPtr[0] = *dataBuf[0];
                        bufPtr[1] = *dataBuf[1];
                        bufPtr[2] = *dataBuf[2];
                     }
                     bufPtr+=3;
                     ++dataBuf[0];
                     ++dataBuf[1];
                     ++dataBuf[2];
                  }
                  internalFormat = GL_RGB;
                  pixelFormat    = GL_RGB;
                  type           = GL_UNSIGNED_BYTE;
               }
            }
            break;
         }
         case OSSIM_FLOAT32: // only support 1 band 
         {
            ossim_float32* bufPtr = new ossim_float32[w*h];
            buf = reinterpret_cast<unsigned char*>(bufPtr);

            memset(buf, 0, w*h*sizeof(ossim_float32));
            
            ossim_float32* dataBuf = (ossim_float32*)data->getBuf();
            ossim_float32 nullPix  = (ossim_float32)data->getNullPix(0);
            ossim_float32 tempNullValue = nullValue;
            
            ossim_uint32 idx = 0;
            ossim_uint32 area = w*h;
            
            if(dataBuf)
            {
               for(;idx < area; ++idx)
               {
                  *bufPtr = *dataBuf;
                  if(reassignNullFlag)
                  {
                     if(nullPix == *dataBuf)
                     {
                        *bufPtr = tempNullValue;
                     }
                  }
                  ++bufPtr;
                  ++dataBuf;
               }
            }
            
            break;
         }
         case OSSIM_SINT16: // only support 1 band
         {
            ossim_float32* bufPtr = new ossim_float32[w*h];
            buf = reinterpret_cast<unsigned char*>(bufPtr);

            memset(buf, 0, w*h*sizeof(ossim_float32));

            ossim_sint16* dataBuf = (ossim_sint16*)data->getBuf();
            ossim_sint16 nullPix  = (ossim_sint16)data->getNullPix(0);
            ossim_float32 tempNullValue = (ossim_float32)nullValue;
            
            ossim_uint32 idx = 0;
            ossim_uint32 area = w*h;
            
            if(dataBuf)
            {
               for(;idx < area; ++idx)
               {
                  *bufPtr = *dataBuf;
                  if(reassignNullFlag)
                  {
                     if(nullPix == *dataBuf)
                     {
                        *bufPtr = tempNullValue;
                     }
                  }
                  ++bufPtr;
                  ++dataBuf;
               }
            }
            break;
         }
         case OSSIM_UINT16: // only support 1 band
         case OSSIM_USHORT11: // only support 1 band
         {
            ossim_float32* bufPtr = new ossim_float32[w*h];
            buf = reinterpret_cast<unsigned char*>(bufPtr);

            memset(buf, 0, w*h*sizeof(ossim_float32));

            ossim_uint16* dataBuf       = (ossim_uint16*)data->getBuf();
            ossim_uint16 nullPix        = (ossim_uint16)data->getNullPix(0);
            ossim_float32 tempNullValue = (ossim_float32)nullValue;
            
            ossim_uint32 idx = 0;
            ossim_uint32 area = w*h;
            
            if(dataBuf)
            {
               for(;idx < area; ++idx)
               {
                  *bufPtr = *dataBuf;
                  if(reassignNullFlag)
                  {
                     if(nullPix == *dataBuf)
                     {
                        *bufPtr = tempNullValue;
                     }
                  }
                  ++bufPtr;
                  ++dataBuf;
               }
            }
            break;
         }
         default:
         {
            // not supported yet
            break;
         }
      }
   }

   if(buf)
   {
      setImage(w, h, 1, internalFormat, pixelFormat, type, buf, allocMode);
   }
}

osg::Image* ossimPlanetImage::readNewOsgImage(const ossimFilename& src,
                                              bool flipVerticalFlag,
                                              bool insertAlphaFlag)
{
   ossimRefPtr<ossimImageHandler> ih = ossimImageHandlerRegistry::instance()->open(ossimFilename(src));

//    if(!ih.valid()) return 0;
//    if(!ih.valid())
//    {
//       return osgDB::readImageFile(src);
//    }
//    else
   if(ih.valid())
   {
      osg::ref_ptr<ossimPlanetImage> planetImage = new ossimPlanetImage;
      ossimIrect rect                  = ih->getBoundingRect() ;
      ossimRefPtr<ossimImageData> tile = ih->getTile(rect);
      if(insertAlphaFlag)
      {
         planetImage->fromOssimImage(tile.get());
      }
      else
      {
         planetImage->fromOssimImageNoAlpha(tile.get());
      }
      if(flipVerticalFlag)
      {
         planetImage->flipVertical();
      }
      return new osg::Image(*(planetImage.get()), osg::CopyOp::DEEP_COPY_ALL);
   }
   
   return 0;
}

ossimPlanetImage* ossimPlanetImage::scaleImagePowerOf2()const
{
   GLint internalFormat = getInternalTextureFormat();
//   ossim_int32 oddW = getWidth()%2;
//   ossim_int32 oddH = getHeight()%2;
   switch(internalFormat)
   {
      case GL_LUMINANCE:
      {
         if(getDataType() == GL_FLOAT)
         {
            return scaleImagePowerOf2((ossim_float32)0.0);
         }
         break;
      }
   }
   
   return 0;
}

void ossimPlanetImage::copySubImageAndInsertPointsPowerOf2(int x, // starting x 
                                                           int y, // starting y
                                                           ossim_uint32 lengthx, // length x
                                                           ossim_uint32 lengthy, // length y
                                                           ossimPlanetImage* source)
{
   if(!source||!source->data()||(_pixelFormat != source->getPixelFormat())||
      (getDataType()!=source->getDataType()))
   {
      return;
   }
   GLint internalFormat = getInternalTextureFormat();

   switch(internalFormat)
   {
      case GL_LUMINANCE:
      {
         if(getDataType() == GL_FLOAT)
         {
            copySubImageAndInsertPointsPowerOf2((ossim_float32)0,
                                                x,
                                                y,
                                                lengthx,
                                                lengthy,
                                                source);
         }
         break;
      }
   }
}

void ossimPlanetImage::copyTo(ossim_uint32 x, ossim_uint32 y, ossimPlanetImage* destination)const
{
   if(!destination||!data()||(_pixelFormat != destination->getPixelFormat())||
      (getDataType()!=destination->getDataType()))
   {
      return;
   }
   GLint internalFormat = getInternalTextureFormat();
   
   switch(internalFormat)
   {
      case GL_LUMINANCE:
      {
         if(getDataType() == GL_FLOAT)
         {
            copyTo((ossim_float32)0,
                 x,
                 y,
                 destination);
         }
         break;
      }
   }
}

template<class T>
void ossimPlanetImage::copyTo(T dummy, ossim_uint32 x, ossim_uint32 y, ossimPlanetImage* destination)const
{
   ossim_uint32 outW = destination->getWidth();
   ossim_uint32 outH = destination->getHeight();
   ossim_uint32 inW = getWidth();
   
   T* destData = (T*)destination->data();
   T* srcData  = (T*)data();
   
   srcData = srcData + inW*y + x;
   
   ossim_uint32 xidx = 0;
   ossim_uint32 yidx = 0;
   for(yidx = 0; yidx < outH; ++yidx)
   {
      for(xidx = 0; xidx < outW; ++xidx)
      {
         destData[xidx] = srcData[xidx];
      }
      destData+=outW;
      srcData += inW;
   }
   destination->setMinMax(minValue(), maxValue());
}

template<class T>
ossimPlanetImage* ossimPlanetImage::scaleImagePowerOf2(T dummy)const
{
   ossimPlanetImage* result = new ossimPlanetImage(tileId());
   result->setPadding(padding());
   result->setMinMax(minValue(), maxValue());
   result->setId(tileId());
   
   //   ossim_uint32 outW = widthWithoutPadding()*2+2*padding() - ((ossim_int32)widthWithoutPadding()%2); 
   //   ossim_uint32 outH = heightWithoutPadding()*2+2*padding()- ((ossim_int32)heightWithoutPadding()%2); 
   ossim_uint32 outW = getWidth()*2 - ((ossim_int32)widthWithoutPadding()%2); 
   ossim_uint32 outH = getHeight()*2- ((ossim_int32)heightWithoutPadding()%2); 
   ossim_uint32 srcW = getWidth(); 
//   ossim_uint32 srcH = getHeight();
   ossim_uint32 inOriginY = 0;//padding();
   ossim_uint32 inOriginX = 0;//padding();
   ossim_uint32 outOriginX = 0;//result->padding();
   ossim_uint32 outOriginY = 0;//result->padding();
   // we will interpolate the padding differently if padding exists
//      ossim_uint32 lengthX = outW - padding()*2; // now setup the bilinear interpolation length for x direction
//      ossim_uint32 lengthY = outH - padding()*2; // now setup the bilinear interpolation length for y direction
   ossim_uint32 lengthX = outW;// - padding(); // now setup the bilinear interpolation length for x direction
   ossim_uint32 lengthY = outH;// - padding(); // now setup the bilinear interpolation length for y direction
   result->allocateImage(outW, outH, 1, getPixelFormat(), getDataType(), getPacking());
   
   T* destData = (T*)result->data();
   T* srcData  = (T*)data();

   ossim_uint32 xidx = 0;
   ossim_uint32 yidx = 0;
   
   ossim_uint32 inputOffsetY = 0;
   ossim_uint32 inputOffset  = 0;
   ossim_uint8  testLocation  = 0;
   ossim_uint32 destOffset = outOriginY*outW + outOriginX;
   // first resample the interior points that aren't padding points
   //
   for(yidx = 0; yidx < lengthY; ++yidx)
   {
      inputOffsetY = ((yidx>>1)+inOriginY)*srcW; // offset for padding along y
      for(xidx = 0; xidx < lengthX; ++xidx)
      {
         inputOffset = inputOffsetY + ((xidx>>1)+inOriginX);
         testLocation = ((yidx&1) << 1) | (xidx&1); // test even odd resampling
         switch(testLocation)
         {
            case 0: // if none are odd then just copy the point over
            {
               destData[destOffset+xidx] = srcData[inputOffset];
               break;
            }
            case 1: // if x is odd the do horizontal edge post
            {
               destData[destOffset+xidx] = (srcData[inputOffset] +
                                            srcData[inputOffset+1])*.5;
               break;
            }
            case 2: // if y is odd then do vertical edge post
            {
               destData[destOffset+xidx] = (srcData[inputOffset] +
                                            srcData[inputOffset+srcW])*.5;
               break;
            }
            case 3: // do center post if both bits are set
            {
               destData[destOffset+xidx] = (srcData[inputOffset] +
                                            srcData[inputOffset+srcW]+
                                            srcData[inputOffset+srcW+1]+
                                            srcData[inputOffset+1]
                                            )*.25;
               break;
            }
            default:
            {
               destData[destOffset+xidx] = srcData[inputOffset];
            }
         }
      }
      destOffset += outW;
   }
   
   result->setMinMax(minValue(), maxValue());
   
#if 0
   // now scale the edges
   //
   destData = (T*)result->data();
   srcData  = (T*)data();
   T* rightDestData = (T*)result->data() + outW-1;
   T* rightSrcData  = (T*)data() + srcW - 1;
   ossim_uint32 idx = 0;
   for(idx = 0; idx < outH; ++idx)
   {
      inputOffset = (idx>>1)*srcW;
      testLocation = (idx&1); // test even odd resampling
      destOffset = idx*outW;
      switch(testLocation)
      {
         case 0: // if none are odd then just copy the point over
         {
            destData[destOffset]      = srcData[inputOffset];
            rightDestData[destOffset] = rightSrcData[inputOffset];
            break;
         }
         case 1: // if x is odd the do vertical edge post
         {
            destData[destOffset] = (srcData[inputOffset] +
                                    srcData[inputOffset+srcW] )*.5;
            rightDestData[destOffset] = (rightSrcData[inputOffset] +
                                         rightSrcData[inputOffset + srcW])*.5;
            break;
         }
      }
   }
   destData = (T*)result->data();
   srcData  = (T*)data();
   T* destDataTop = (T*)result->data() + (outH-1)*outW;
   T* srcDataTop  = (T*)data() + (srcH-1)*srcW;
   for(idx = 0; idx < outW; ++idx)
   {
      inputOffset = (idx>>1);
      testLocation = (idx&1); // test even odd resampling
      switch(testLocation)
      {
         case 0: // if none are odd then just copy the point over
         {
            destData[idx]    = srcData[inputOffset];
            destDataTop[idx] = srcDataTop[inputOffset];
            break;
         }
         case 1: // if x is odd the do horizontal edge post
         {
            destData[idx] = (srcData[inputOffset] +
                             srcData[inputOffset+1])*.5;
            destDataTop[idx] = (srcDataTop[inputOffset] +
                                srcDataTop[inputOffset + 1])*.5;
            break;
         }
      }
   }
#endif
   return result;
}

template<class T>
void ossimPlanetImage::copySubImageAndInsertPointsPowerOf2(T dummy,
                                                   int x, // starting x 
                                                   int y, // starting y
                                                   ossim_uint32 lengthx, // length x
                                                   ossim_uint32 lengthy, // length y
                                                   ossimPlanetImage* source)
{
   ossim_uint32 srcWidth = source->s();
   ossim_uint32 myWidth = s();
   T* myData = (T*)data();
   T* srcData = (T*)source->data();
   
   ossim_float64 minValue = 1.0/FLT_EPSILON;
   ossim_float64 maxValue = -minValue;
//   GLint internalFormat = getInternalTextureFormat();
   
   srcData = srcData + y*srcWidth + x;

   //std::cout << "SRC W = " << source->s() << std::endl
   //<< "SRC H = " << source->t() << std::endl;
   //std::cout << "DEST W = " << s() << std::endl
   //<< "DEST H = " << t() << std::endl;
   // copy the shared points then go back through and fill in
   // the holes with a bilinear estimate
   //
   ossim_uint32 idxY = 0;
   ossim_uint32 idxX = 0;
   ossim_uint32 myHeight = t();
   ossim_uint32 inputOffsetY = 0;
   ossim_uint32 inputOffset = 0;
   ossim_uint8 testLocation=0;
   for(idxY = 0; idxY < myHeight; ++idxY)
   {
      inputOffsetY = (idxY>>1)*srcWidth;
      for(idxX = 0; idxX < myWidth; ++idxX)
      {
         inputOffset = inputOffsetY + (idxX>>1);
         testLocation = ((idxY&1) << 1) | (idxX&1);
         switch(testLocation)
         {
            case 0: // if none are odd then just copy the point over
            {
               *myData = srcData[inputOffset];
               break;
            }
            case 1: // if x is odd the do horizontal edge post
            {
               *myData = (srcData[inputOffset] +
                          srcData[inputOffset+1])*.5;
               break;
            }
            case 2: // if y is odd then do vertical edge post
            {
               *myData = (srcData[inputOffset] +
                          srcData[inputOffset+srcWidth])*.5;
              break;
            }
            case 3: // do center post if both bits are set
            {
               *myData = (srcData[inputOffset] +
                          srcData[inputOffset+srcWidth]+
                          srcData[inputOffset+srcWidth+1]+
                          srcData[inputOffset+1]
                          )*.25;
               break;
            }
            default:
            {
               *myData = srcData[inputOffset];
            }
               
               
         }
         if(*myData > maxValue) maxValue = *myData;
         if(*myData < minValue) minValue = *myData;
         
         ++myData;
      }
   }
   
   theMinValue.resize(1);
   theMaxValue.resize(1);
   theMinValue[0] = minValue;
   theMaxValue[0] = maxValue;
}

osg::Vec2 ossimPlanetImage::deltas(ossim_int32 x,
                                   ossim_int32 y)const
{
   osg::Vec2 delta(0.0,0.0);
   GLint internalFormat = getInternalTextureFormat();
   
   // we only support single band float for delta lookups
   if(internalFormat != GL_LUMINANCE) return delta;
   if(getDataType()!=GL_FLOAT)
   {
      return delta;
   }
   
   if (x==0)
   {
      delta.x() = (elevationValue(x+1,y)-elevationValue(x,y));
   }
   else if (x==getWidth()-1)
   {
      delta.x() = (elevationValue(x,y)-elevationValue(x-1,y));
   }
   else // assume 0<c<_numColumns-1
   {
      delta.x() = 0.5f*(elevationValue(x+1,y)-elevationValue(x-1,y));
   }
   
   if (y==0)
   {
      delta.y() = (elevationValue(x,y+1)-elevationValue(x,y));
   }
   else if (y==getHeight()-1)
   {
      delta.y() = (elevationValue(x,y)-elevationValue(x,y-1));
   }
   else // assume 0<r<_numRows-1
   {
      delta.y() = 0.5f*(elevationValue(x,y+1)-elevationValue(x,y-1));
   }
   
   return delta;
}

ossim_uint64 ossimPlanetImage::sizeInBytes()const
{
   return ((((ossim_uint64)getRowSizeInBytes())*((ossim_uint64)_t)*((ossim_uint64)_r))+
           (ossim_uint64)sizeof(ossimPlanetImage));
}
