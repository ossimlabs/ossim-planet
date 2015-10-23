#ifndef ossimPlanetTextureLayer_HEADER
#define ossimPlanetTextureLayer_HEADER
#include <osg/Referenced>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimId.h>
#include "ossimPlanetExport.h"
#include "ossimPlanetImage.h"
#include "ossimPlanetGridUtility.h"
#include <ossimPlanet/ossimPlanetId.h>
#include <ossim/base/ossimDate.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossimPlanet/ossimPlanetExtents.h>
#include <ossimPlanet/ossimPlanetConstants.h>
#include <ossimPlanet/ossimPlanetLookAt.h>
#include <ossimPlanet/ossimPlanetCallback.h>
#include <ossim/base/ossimXmlNode.h>
#include <vector>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <ossimPlanet/ossimPlanetGrid.h>

class ossimPlanetTextureLayerGroup;

class ossimPlanetTextureLayer;
class OSSIMPLANET_DLL ossimPlanetTextureLayerCallback : public ossimPlanetCallback
{
public:
   ossimPlanetTextureLayerCallback(){}
   
   virtual void layerAdded(osg::ref_ptr<ossimPlanetTextureLayer> /*layer*/)
   {
   }
   virtual void layerRemoved(osg::ref_ptr<ossimPlanetTextureLayer> /*layer*/,
                             osg::ref_ptr<ossimPlanetTextureLayer> /*parent*/)
   {
   }
   virtual void refreshExtent(osg::ref_ptr<ossimPlanetExtents> /*extent*/)
   {
   }
   virtual void propertyChanged(const ossimString& /*name*/,
                                const ossimPlanetTextureLayer* /*object*/){}
   
};

class ossimPlanetTextureLayerRaiseCallback;

class OSSIMPLANET_DLL ossimPlanetTextureLayer : public osg::Referenced,
   public ossimPlanetCallbackListInterface<ossimPlanetTextureLayerCallback>
{
public:
	friend class ossimPlanetTextureLayerRaiseCallback;
   class OSSIMPLANET_DLL Stats : public osg::Referenced
   {
   public:
      Stats()
         :osg::Referenced(),
         theTotalTextureSize(0),
         theBytesTransferred(0)
      {
      }
      Stats(const Stats& src)
         :osg::Referenced(),
         theTotalTextureSize(src.theTotalTextureSize),
         theBytesTransferred(src.theBytesTransferred)
      {
         
      }
         
      Stats* clone()const
      {
         return new Stats(*this);
      }
      
      const ossim_uint64& totalTextureSize()const
      {
         return theTotalTextureSize;         
      }
      void setTotalTextureSize(const ossim_uint64& size)
      {
         theTotalTextureSize = size;
      }
      
      const ossim_uint64& bytesTransferred()const
      {
         return theBytesTransferred;
      }
      void setBytesTransferred(ossim_uint64 amountTransferred)
      {
         theBytesTransferred = amountTransferred;
      }
   protected:
      
      ossim_uint64 theTotalTextureSize;
      ossim_uint64 theBytesTransferred;
   };
/*    typedef std::vector<osg::ref_ptr<ossimPlanetTextureLayer::Callback> > CallbackList; */
   typedef std::vector<ossimPlanetTextureLayerGroup*> ossimPlanetTextureLayerParentList;
   typedef std::vector<unsigned char> TransparentColorType;
   

   ossimPlanetTextureLayer();
   ossimPlanetTextureLayer(const ossimPlanetTextureLayer& src);
   virtual ~ossimPlanetTextureLayer();
   virtual ossimPlanetTextureLayer* dup()const=0;
   virtual ossimPlanetTextureLayer* dupType()const=0;
   virtual ossimString getClassName()const;
   virtual ossimPlanetTextureLayerStateCode updateExtents()=0;
   
   void addBytesTransferredStat(ossim_uint64 bytesTransferred)const;
   virtual void updateStats()const=0;
   /**
    *  Since theStateCode is a utility / mutable variable and is updated during
    *  constant and non constant access we will make this a const on ll other non mutable
    *  variables.
    */
   void clearState(ossimPlanetTextureLayerStateCode stateCode=ossimPlanetTextureLayer_ALL)const;

   /**
    *  Since theStateCode is a utility / mutable variable and is updated during
    *  constant and non constant access we will make this a const on ll other non mutable
    *  variables.
    */
   void setState(ossimPlanetTextureLayerStateCode stateCode)const;

   ossimPlanetTextureLayerStateCode getStateCode()const;
   bool isStateSet(ossimPlanetTextureLayerStateCode)const;
   
   virtual bool hasTexture(ossim_uint32 width,
                           ossim_uint32 height,
                           const ossimPlanetTerrainTileId& tileId,
                           const ossimPlanetGrid& theGrid)=0;
   
   /**
    * We will supply an optional padding value.  This is a request and if the request occured successfully
    * then the padding variable will be set in the resulting image result.
    *
    * 
    */
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 width,
                                                     ossim_uint32 height,
                                                     const ossimPlanetTerrainTileId& tileId,
                                                     const ossimPlanetGrid& theGrid,
                                                     ossim_int32 padding=0)=0;
   /**
    * Deprecated.  Will be replaced with the new tiling grid interface
    */
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 level,
                                                     ossim_uint64 row,
                                                     ossim_uint64 col,
                                                     const ossimPlanetGridUtility& utility)=0;
   
   void dirtyExtents();
   void dirtyStats();
   void setDirtyExtentsFlag(bool flag);
   const osg::ref_ptr<ossimPlanetTextureLayer::Stats> getStats()const;
   osg::ref_ptr<ossimPlanetTextureLayer::Stats>  getStats();
   const osg::ref_ptr<ossimPlanetExtents> getExtents()const;
   osg::ref_ptr<ossimPlanetExtents> getExtents();
   virtual void setExtents (osg::ref_ptr<ossimPlanetExtents> extents);

   virtual const osg::ref_ptr<ossimPlanetLookAt> getLookAt()const;
   virtual void setLookAt(osg::ref_ptr<ossimPlanetLookAt> lookAt);

   virtual void getDateRange(ossimDate& minDate,
                             ossimDate& maxDate)const;
   /**   
    * Approximate length in meters of the hypotneus.  Just uses the getExtents
    * and ten approximates a meter gsd from the degree bounds.
    */ 
   virtual double getApproximateHypotneusLength()const;

   /**
    * Will return the center lat lon and the approximate hypotneus length in meters.
    */ 
   virtual void getCenterLatLonLength(double& centerLat,
                                      double& centerLon,
                                      double& length)const;
   virtual ossimPlanetTextureLayerGroup* asGroup();
   virtual const ossimPlanetTextureLayerGroup* asGroup()const;
   ossimPlanetTextureLayerGroup* getParent(ossim_uint32 idx);
   const ossimPlanetTextureLayerGroup* getParent(ossim_uint32 idx)const;
   ossimPlanetTextureLayerGroup* parent(ossim_uint32 idx);
   const ossimPlanetTextureLayerGroup* parent(ossim_uint32 idx)const;
   ossim_uint32 getNumberOfParents();
   bool hasParent(ossimPlanetTextureLayerGroup* parent)const;
   void setParent(ossimPlanetTextureLayer* parent);
   const ossimString& id()const;
   void setId(const ossimString& id);
   virtual void setEnableFlag(bool flag);
	bool enableFlag()const;
   bool getEnableFlag()const;

   void setName(const ossimString& name);
   const ossimString& getName()const;
   const ossimString& name()const;

   void setDescription(const ossimString& description);
   const ossimString& getDescription()const;
   const ossimString& description()const;

   void setTransparentColorFlag(bool flag);
   bool getTransparentColorFlag()const;
   const TransparentColorType& getTransparentColor()const;
   void setTransparentColor(unsigned int r,
                            unsigned int g,
                            unsigned int b);
   
   bool insertAlpha(osg::ref_ptr<ossimPlanetImage> texture)const;
   bool insertAlpha(osg::ref_ptr<ossimPlanetImage> texture,
                    float initialAlphaValue)const;
   void addTransparent(osg::ref_ptr<ossimPlanetImage> texture)const;

   void detachFromParents();
   void addParent(ossimPlanetTextureLayerGroup* parent);
   void removeParent(ossimPlanetTextureLayerGroup* parent);
   
   virtual void setFilterType(const ossimString& filterType);
   virtual const ossimString& getFilterTypeAsString()const;
/*    void addCallback(osg::ref_ptr<ossimPlanetTextureLayer::Callback> callback); */
/*    void removeCallback(osg::ref_ptr<ossimPlanetTextureLayer::Callback> callback); */

   OpenThreads::Mutex& mutex();
   
   virtual void getMetadata(ossimRefPtr<ossimXmlNode> metadata)const;
   virtual ossimRefPtr<ossimXmlNode> saveXml(bool recurseFlag=true)const;
   virtual bool loadXml(ossimRefPtr<ossimXmlNode> node);


   // all notification methods are propagated if parent texture layers are listening to child layers.
   //
   void notifyRefreshExtent(osg::ref_ptr<ossimPlanetExtents> extent);
   void notifyLayerAdded(osg::ref_ptr<ossimPlanetTextureLayer> layer);
   void notifyLayerRemoved(osg::ref_ptr<ossimPlanetTextureLayer> layer, 
                           osg::ref_ptr<ossimPlanetTextureLayer> parent);
   void notifyPropertyChanged(const ossimString& name,
                              const ossimPlanetTextureLayer* layer)const;

   virtual void resetLookAt();

   virtual ossimPlanetTextureLayer* findLayerByName(const ossimString& layerName,
                                                    bool recurseFlag=false);
   virtual const ossimPlanetTextureLayer* findLayerByName(const ossimString& layerName,
                                                          bool recurseFlag=false)const;
   
   virtual ossimPlanetTextureLayer* findLayerByNameAndId(const ossimString& layerName,
                                                         const ossimString& layerId);
   virtual const ossimPlanetTextureLayer* findLayerByNameAndId(const ossimString& layerName,
                                                               const ossimString& layerId)const;
   
   virtual ossimPlanetTextureLayer* findLayerById(const ossimString& layerId,
                                                  bool recurseFlag=false);
   virtual const ossimPlanetTextureLayer* findLayerById(const ossimString& layerId,
                                                        bool recurseFlag=false)const;

   void remove();
   

   ossim_float32 brightness()const;
   ossim_float32 contrast()const;
   ossim_float32 opacity()const;

   /**
     * Allows one to enable brightness contrast settings.
     */
    void setBrightnessContrast(ossim_float32 brightnessValue,
                               ossim_float32 contrastValue,
                               bool notifyFlag = true);
    /**
      * Allows one to enable brightness  settings.
      */
     void setBrightness(ossim_float32 brightnessValue, bool notifyFlag=true);
     /**
       * Allows one to enable contrast settings.
       */
     void setContrast(ossim_float32 contrastValue, bool notifyFlag = true);

   /**
    * Allows one to set the opacity.
    *
    * @param opacityValue value of 1.0 is fully opaque and a value of 0 is fully transparent
    */
   void setOpacity(ossim_float32 opacityValue, bool notifyFlag = true);
   
protected:
   ossimPlanetImage* applyBrightnessContrast(ossimPlanetImage* image, bool duplicateFlag=true)const;
   virtual void mergeImage(ossimPlanetImage* result,
                           const ossimPlanetImage* source,
                           ossim_float32 sourceOpacity=1.0)const;
   
   void convertToOsg(ossimImageData* data,
                     ossimPlanetImage* image)const;
   void convertToOsg(ossimImageData* data,
                     ossimPlanetImage* image,
                     const osg::Vec2d& ulInput,
                     const osg::Vec2d& /*urInput*/,
                     const osg::Vec2d& lrInput,
                     const osg::Vec2d& /*llInput*/,
                     ossimPlanetGrid::ModelPoints& outputGridPoints,
                     ossim_uint32 outputWidth,
                     ossim_uint32 outputHeight)const;
   
   void convertToOsg(ossimImageData* data,
                     ossimPlanetImage* image,
                     const osg::Vec2d& ulInput,
                     const osg::Vec2d& urInput,
                     const osg::Vec2d& lrInput,
                     const osg::Vec2d& llInput,
                     std::vector<ossimPlanetGridUtility::GridPoint>& outputGridPoints,
                     const ossimPlanetGridUtility& utility,
                     ossim_uint32 outputWidth,
                     ossim_uint32 outputHeight)const;
   
   ossimString                            theName;
   ossimString                            theDescription;
   ossimString                            theId;
   ossimPlanetTextureLayerParentList      theParentList;
   mutable osg::ref_ptr<ossimPlanetExtents>   theExtents;
   mutable osg::ref_ptr<ossimPlanetLookAt>    theLookAt;
   
   bool                                   theEnableFlag;
   bool                                   theTransparentColorFlag;
   TransparentColorType                   theTransparentColorVector; // [0] = r, [1] = g, [2] = b
   ossimString                            theFilterType;
   mutable ossimPlanetReentrantMutex    thePropertyMutex;
   mutable ossimPlanetReentrantMutex    theMutex;
/*    mutable OpenThreads::Mutex             theCallbackMutex; */
   mutable bool                           theDirtyExtentsFlag;
   mutable bool                           theDirtyStatsFlag;
   mutable osg::ref_ptr<Stats>            theStats;
   mutable ossimPlanetTextureLayerStateCode theStateCode;
   
   ossim_float32                          theBrightness;
   ossim_float32                          theContrast;

   ossim_float32                          theOpacity;
/*    ossimPlanetTextureLayer::CallbackList    theCallbackList; */
   
};

class OSSIMPLANET_DLL ossimPlanetTextureLayerRaiseCallback : public ossimPlanetTextureLayerCallback
{
public:
	ossimPlanetTextureLayerRaiseCallback(ossimPlanetTextureLayer* parentLayer):theLayer(parentLayer){}
	ossimPlanetTextureLayer* layer(){return theLayer;}
	void setLayer(ossimPlanetTextureLayer* l){theLayer = l;}
	
   virtual void layerAdded(osg::ref_ptr<ossimPlanetTextureLayer> layer)
   {
		if(theLayer)
		{
			theLayer->notifyLayerAdded(layer.get());
		}
   }
   virtual void layerRemoved(osg::ref_ptr<ossimPlanetTextureLayer> layer,
                             osg::ref_ptr<ossimPlanetTextureLayer> parent)
   {
		if(theLayer)
		{
			theLayer->notifyLayerRemoved(layer, parent);
		}
   }
   virtual void refreshExtent(osg::ref_ptr<ossimPlanetExtents> extent)
   {
		if(theLayer)
		{
			theLayer->notifyRefreshExtent(extent);
		}
   }
   virtual void propertyChanged(const ossimString& name,
                                const ossimPlanetTextureLayer* object)
	{
		if(theLayer)
		{
			theLayer->notifyPropertyChanged(name, object);
		}
	}
   
protected:
	ossimPlanetTextureLayer* theLayer;
};
#endif
