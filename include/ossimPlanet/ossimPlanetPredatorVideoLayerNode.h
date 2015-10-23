#ifndef ossimPlanetPredatorVideoLayerNode_HEADER
#define ossimPlanetPredatorVideoLayerNode_HEADER
#include "ossimPlanetVideoLayerNode.h"
#include "ossimPlanetIconGeom.h"
#include "ossimPlanetRefBlock.h"
#include <ossimPredator/ossimPredatorVideo.h>
#include "ossimPlanetBillboardIcon.h"
#include "ossimPlanetGeoRefModel.h"
#include <osg/ImageStream>
#include <osg/Timer>
#include <osg/Viewport>
#include <osg/CameraNode>
#include <osg/MatrixTransform>
#include <osg/Geode>
#include <OpenThreads/Thread>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Block>
#include <osg/Switch>

class OSSIMPLANET_DLL ossimPlanetFFMpegImageStream :  public osg::ImageStream, public OpenThreads::Thread
{
public:
   
   class  OSSIMPLANET_DLL CoordinateInfo : public osg::Referenced
   {
   public:
      CoordinateInfo()
      :theFrameCenterLlhValidFlag(true),
      theSensorPositionLlhValidFlag(true),
      theOrientationMatrixValidFlag(true),
      theTargetWidthInMetersValidFlag(true),
      theTargetWidthInMeters(0.0),
      theSlantRangeValidFlag(true),
      theSlantRange(0.0),
      theReferenceTime(0.0),
      theClock(0.0)
      {
      }
      CoordinateInfo(const CoordinateInfo& src)
         :theFrameCenterLlhValidFlag(src.theFrameCenterLlhValidFlag),
          theFrameCenterLlh(src.theFrameCenterLlh),
          theSensorPositionLlhValidFlag(src.theSensorPositionLlhValidFlag),
          theSensorPositionLlh(src.theSensorPositionLlh),
          theOrientationMatrixValidFlag(src.theOrientationMatrixValidFlag),
          theOrientationMatrix(src.theOrientationMatrix),
          theTargetWidthInMetersValidFlag(src.theTargetWidthInMetersValidFlag),
          theTargetWidthInMeters(src.theTargetWidthInMeters),
          theSlantRangeValidFlag(src.theSlantRangeValidFlag),
          theSlantRange(src.theSlantRange),
          theCornerPoints(src.theCornerPoints),
      theKlvTable(src.theKlvTable.valid()?src.theKlvTable->dup():(ossimPredatorKlvTable*)0),
      theReferenceTime(src.theReferenceTime),
      theClock(src.theClock),
      theDescription(src.theDescription)
      {}
      CoordinateInfo* dup()const
      {
         return new CoordinateInfo(*this);
      }
      void setDescriptionToKlvMetadata();
      
      bool theFrameCenterLlhValidFlag;
      osg::Vec3d theFrameCenterLlh;
      bool theSensorPositionLlhValidFlag;
      osg::Vec3d theSensorPositionLlh;
      bool theOrientationMatrixValidFlag;
      osg::Matrixd theOrientationMatrix;
      bool theTargetWidthInMetersValidFlag;
      ossim_float64 theTargetWidthInMeters;
      bool theSlantRangeValidFlag;
      ossim_float64 theSlantRange;
      std::vector<osg::Vec3d> theCornerPoints;
      ossimRefPtr<ossimPredatorKlvTable> theKlvTable;
      double theReferenceTime;
      double theClock;
      ossimString theDescription; // html or text formatted description field.  
   };
   class OSSIMPLANET_DLL Callback : public osg::Referenced
   {
   public:
      // will add callbacks for when the geometry changed so we can update the drawables position
      //
      virtual void coordinatesChanged(ossimRefPtr<ossimPredatorKlvTable> /* originalTable */,
                                      osg::ref_ptr<CoordinateInfo> /* transformedInfo*/ ){}

      virtual void referenceTimeChanged(ossim_float64 /* reference */){}
   };
   ossimPlanetFFMpegImageStream();
   virtual ~ossimPlanetFFMpegImageStream();
   void setModel(osg::ref_ptr<ossimPlanetGeoRefModel> model)
   {
      theModel = model;
   }
   const osg::ref_ptr<ossimPlanetGeoRefModel> model()const
   {
      return theModel.get();
   }
   void setVideo(ossimRefPtr<ossimPredatorVideo> video);
   virtual int cancel();

   virtual void run();
   virtual void play();
   virtual void pause();
   virtual void rewind();
   virtual void quit(bool /*waitForThreadToExit*/ = true);
   virtual double getLength() const;
        
   virtual void setReferenceTime(double);
   virtual double getReferenceTime();
                
   virtual void setTimeMultiplier(double);
   virtual double getTimeMultiplier();
        
   virtual void setVolume(float);
   virtual float getVolume();
   void updateThreadBlock()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
         
         // setting true will release the block
      theBlock->set((theVideo.valid())&&
                    ((!theVideo->firstFrameFlag()||theVideo->seekingFlag())||
                     (theEnableFlag&&(_status == osg::ImageStream::PLAYING))));
   }
   void setEnableFlag(bool flag)
   {
      theMutex.lock();
      theEnableFlag = flag;
      theMutex.unlock();
      updateThreadBlock();
   }
   void setCallback( osg::ref_ptr<ossimPlanetFFMpegImageStream::Callback> callback)
   {
      theCallback = callback;
   }
   void setNeedFirstFrameFlag(bool flag)
   {
      theMutex.lock();
      theNeedFirstFrameFlag = flag;
      theMutex.unlock();
      updateThreadBlock();
   }
protected:
   ossimPlanetReentrantMutex                 theMutex;
   osg::ref_ptr<ossimPlanetRefBlock> theBlock;
   ossimRefPtr<ossimPredatorVideo>    theVideo;
   ossim_float64                      theFrameRate;
   ossim_float64                      theSecondsToUpdate;
   bool                               theDoneFlag;
   bool                               theEnableFlag;
   bool                               theNeedFirstFrameFlag;
   osg::ref_ptr<ossimPlanetFFMpegImageStream::Callback> theCallback;
   osg::ref_ptr<ossimPlanetGeoRefModel> theModel;
};

class OSSIMPLANET_DLL ossimPlanetPredatorVideoLayerNode : public ossimPlanetVideoLayerNode
{
public:
   class ImageStreamCallback : public ossimPlanetFFMpegImageStream::Callback
   {
   public:
      ImageStreamCallback(ossimPlanetPredatorVideoLayerNode* layer)
         :theLayer(layer)
      {}
      virtual void coordinatesChanged(ossimRefPtr<ossimPredatorKlvTable> /* originalTable */,
                                      osg::ref_ptr<ossimPlanetFFMpegImageStream::CoordinateInfo> transformedInfo)
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theLayer->theCoordinateMutex);
         theLayer->notifyPropertyChanged(theLayer, "coordinates");
         theLayer->theCoordinates = transformedInfo->dup();
         theLayer->setDescription(transformedInfo->theDescription);
      }
      virtual void referenceTimeChanged(ossim_float64 /* reference */)
      {
         theLayer->notifyPropertyChanged(theLayer, "referenceTime");
         theLayer->setRedrawFlag(true);
      }
   protected:
      ossimPlanetPredatorVideoLayerNode* theLayer;
   };
   friend class ImageStreamCallback;
   ossimPlanetPredatorVideoLayerNode(ossimPlanetVideoLayer* layer=0);
   virtual ~ossimPlanetPredatorVideoLayerNode();
   virtual bool open(const ossimFilename& file);
   virtual void setVideo(ossimPredatorVideo* video);
   virtual void traverse(osg::NodeVisitor& nv);

   virtual void pause();
   virtual void rewind();
   virtual void play();
   virtual ossim_float64 duration()const;
   ossimPlanetVideoLayerNode::Status status()const;
   virtual void setReferenceTime(ossim_float64 reference);
   virtual ossim_float64 referenceTime()const;
   virtual void setRenderMode(ossimPlanetVideoLayerNode::RenderMode mode);
	virtual void stage()
	{
		setStagedFlag(true);
	}
	virtual void update()
	{
	}
	
   virtual void setEnableFlag(bool flag)
   {
      if(theCurrentFrame.valid())
      {
         theCurrentFrame->setEnableFlag(flag);
      }
      ossimPlanetVideoLayerNode::setEnableFlag(flag);
   }
  
protected:
   void updateIconGeometry();

   ossimRefPtr<ossimPredatorVideo>            theVideo;
   ossim_float64                               theFrameRate;
   ossim_float64                               theSecondsToUpdate;
   osg::ref_ptr<ossimPlanetFFMpegImageStream>  theCurrentFrame;
   osg::Timer_t                                theCurrentTime;

   osg::ref_ptr<osg::Switch> theSwitchNode;

   ossim_float64 theAspect;
   ossim_float64 theInvAspect;
   bool          theGeometryFlag;
   bool          theCullFlag;
   osg::ref_ptr<ossimPlanetFFMpegImageStream::CoordinateInfo> theCoordinates;
   ossim_float64 theCullDistance;
   osg::Vec3d    theFrameCenter;
   
   /**
    * This is the base drawable for all draw modes
    */
   osg::ref_ptr<osg::Texture2D>      theSharedTexture;
   osg::ref_ptr<ossimPlanetIconGeom> theIcon;
   ossimPlanetReentrantMutex                thePredatorTraverseMutex;

   
   /**
    * Used by the 2-D HUD mode drawing for setting the ortho matrix of the camera
    */ 
   bool theViewportChangedFlag;
   
   osg::ref_ptr<osg::Geode> theSharedGeode;

   /**
   * Used by the 2-D display mode
   */
   osg::ref_ptr<osg::CameraNode> theCameraNode; 
   osg::ref_ptr<osg::Viewport>   theViewport;
   osg::ref_ptr<osg::MatrixTransform> the2DMatrixTransform;

   /**
    * Used by the Billboard display mode.
    */
   osg::ref_ptr<ossimPlanetBillboardIcon> theBillboard;
   osg::ref_ptr<osg::MatrixTransform>     theBillboardMatrixTransform;
   osg::ref_ptr<ossimPlanetPredatorVideoLayerNode::ImageStreamCallback> theCallback;
   ossimPlanetReentrantMutex                     theCoordinateMutex;
};

#endif
