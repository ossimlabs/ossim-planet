#ifndef ossimPlanetImage_HEADER
#define ossimPlanetImage_HEADER
#include "ossimPlanetExport.h"
#include <ossim/base/ossimConstants.h>
#include <ossim/imaging/ossimImageData.h>
#include <osg/Image>
#include <osg/Vec2>
#include <ossim/base/ossimConstants.h>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <ossimPlanet/ossimPlanetTerrainTileId.h>

class OSSIMPLANET_DLL ossimPlanetImage :public osg::Image
{
public:
   enum ossimPlanetImageStateType 
   {
      ossimPlanetImageStateType_NONE = 0,
      ossimPlanetImageStateType_LOADED = 1,
      ossimPlanetImageStateType_NEEDS_LOADING     = 2
   };
   enum ossimPlanetImagePixelStatus
   {
      ossimPlanetImagePixelStatus_EMPTY    = 0,
      ossimPlanetImagePixelStatus_FULL     = 1,
      ossimPlanetImagePixelStatus_PARTIAL  = 2
   };
   virtual osg::Object* cloneType() const
   {
      return new ossimPlanetImage();
   }
   virtual osg::Object* clone(const osg::CopyOp& copyop) const
   {
      return new ossimPlanetImage(*this,copyop);
   }
   virtual const char* className() const
   {
      return "ossimPlanetImage";
   }
   ossimPlanetImage();
   ossimPlanetImage(const osg::Image& image);
   ossimPlanetImage(const ossimPlanetTerrainTileId& id); 
   ossimPlanetImage(const ossimPlanetImage& image,
                    const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
   
   virtual ~ossimPlanetImage();
   void stripPadding();
   int getNumberOfComponents()const; 
   int getHeight()const; 
   int getWidth()const;

   ossimPlanetImageStateType getState()const;
      
   void setId(const ossimPlanetTerrainTileId& id);
   const ossimPlanetTerrainTileId& tileId()const;
   void setState(ossimPlanetImageStateType stateType);
   void setPixelStatus();
   void setPixelStatus(ossimPlanetImagePixelStatus pixelStatus);
   ossimPlanetImagePixelStatus getPixelStatus()const;
   ossimRefPtr<ossimImageData> toOssimImage()const;
   void fromOssimImage(ossimRefPtr<ossimImageData> data,
                       bool reassignNullFlag = true,
                       double nullValue = 0.0);
   void fromOssimImageNoAlpha(ossimRefPtr<ossimImageData> data,
                              bool reassignNullFlag = true,
                              double nullValue = 0.0);
   OpenThreads::Mutex& mutex()const;

   bool hasMinMax()const
   {
      return ((theMinValue.size()!=0)&&
              (theMaxValue.size()!=0));
   }
   void setMinMax(double minValue,
                  double maxValue)
   {
      theMinValue.resize(1);
      theMaxValue.resize(1);

      theMinValue[0] = minValue;
      theMaxValue[0] = maxValue;
   }
   void setMin(ossim_uint32 idx,
               double value)
   {
      if(idx < theMinValue.size())
      {
         theMinValue[idx] = value;
      }
   }
   void setMax(ossim_uint32 idx,
               double value)
   {
      if(idx < theMaxValue.size())
      {
         theMaxValue[idx] = value;
      }
   }
   void setMinMax(const std::vector<double>& minValue,
                  const std::vector<double>& maxValue)
   {
      theMinValue = minValue;
      theMaxValue = maxValue;
   }
   const std::vector<double>& minValue()const
   {
      return theMinValue;
   }
   const std::vector<double>& maxValue()const
   {
      return theMaxValue;
   }
   static osg::Image*       readNewOsgImage(const ossimFilename& src,
                                            bool flipVerticalFlag = true,
                                            bool insertAlphaFlag = true);
   static osg::Image*       readNewOsgImageNoAlpha(const ossimFilename& src,
                                                   bool flipVerticalFlag = true);
   ossimPlanetImage* scaleImagePowerOf2()const;
               
   void copyTo(ossim_uint32 x, ossim_uint32 y, ossimPlanetImage* destination)const;
   
   /**
    * We will only support RGBA or RGB, or single band elevation in 32 bit float
    */
   void copySubImageAndInsertPointsPowerOf2(int x, // starting x 
                                            int y, // starting y
                                            ossim_uint32 lengthx, // length x
                                            ossim_uint32 lengthy, // length y
                                            ossimPlanetImage* source);  
   virtual ossim_uint64 sizeInBytes()const;
   virtual osg::Vec2 deltas(ossim_int32 x,
                            ossim_int32 y)const;
   
   ossim_int32 widthWithoutPadding()const
   {
      return _s-(thePadding*2);
   }
   ossim_int32 heightWithoutPadding()const
   {
      return _t-(thePadding*2);
   }
   ossim_int32 width()const{return _s;}
   ossim_int32 height()const{return _t;}
   
   ossim_int32 padding()const
   {
      return thePadding;
   }
   void setPadding(ossim_int32 value)
   {
      thePadding = value;
   }
   ossim_float32 elevationValue(ossim_int32 x, ossim_int32 y)const
   {
      return reinterpret_cast<const ossim_float32*>(_data)[_s*y+x];
   }
   ossim_float32 elevationValueNoPaddingOffset(ossim_int32 x, ossim_int32 y)const
   {
      return reinterpret_cast<const ossim_float32*>(_data)[_s*(y+thePadding)+(x+thePadding)];
   }
   void applyBrightnessContrast(ossim_float32 brightness, ossim_float32 contrast);

protected:
   template <class T>
   void copySubImageAndInsertPointsPowerOf2(T dummy, // for casting to proper buffer type                                    
                                    int x,
                                    int y, // starting y
                                    ossim_uint32 lengthx, // length x
                                    ossim_uint32 lengthy, // length y
                                    ossimPlanetImage* source); // 
   template <class T>
   ossimPlanetImage* scaleImagePowerOf2(T dummy)const;
   
   template <class T>
   void copyTo(T dummy, ossim_uint32 x, ossim_uint32 y, ossimPlanetImage* destination)const;
   
   mutable ossimPlanetImageStateType theState;
   ossimPlanetTerrainTileId theTileId;
   ossimPlanetImagePixelStatus thePixelStatus;
   mutable ossimPlanetReentrantMutex theMutex;
   std::vector<double> theMinValue;
   std::vector<double> theMaxValue;
   
   ossim_int32 thePadding;
};

#endif
