#ifndef ossimPlanetVideoLayerNode_HEADER
#define ossimPlanetVideoLayerNode_HEADER
#include <ossimPlanet/ossimPlanetAnnotationLayerNode.h>
#include <ossim/base/ossimFilename.h>

class ossimPlanetVideoLayer;
/**
 * 
 * I will eventually have this do the drawing and let the derived classes just populate the geometries.
 * For now,  I will put the implementation in ossimPlanetPredatorVideoLayerNode and move implementations out
 * as we finish and test them.
 * 
 */ 
class OSSIMPLANET_DLL ossimPlanetVideoLayerNode : public ossimPlanetAnnotationLayerNode
{
public:
   enum Status
   {
      STATUS_NONE = 0,
      STATUS_PLAYING,
      STATUS_PAUSED
   };
   enum RenderMode
   {
      RENDER_SCREEN   = 0,
      RENDER_BILLBOARD = 1,
      RENDER_OVERLAY  = 2 
   };
   ossimPlanetVideoLayerNode(ossimPlanetVideoLayer* layer):theLayer(layer){}
   virtual ~ossimPlanetVideoLayerNode(){}
   virtual bool open(const ossimFilename& file)=0;
	
   virtual void pause()=0;
   virtual void rewind()=0;
   virtual void play()=0;
   virtual ossim_float64 duration()const=0;
   virtual ossimPlanetVideoLayerNode::Status status()const = 0;
   virtual void setReferenceTime(ossim_float64 reference)=0;
   virtual ossim_float64 referenceTime()const=0;
   virtual ossimPlanetVideoLayerNode::RenderMode renderMode()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theVideoLayerNodeMutex);
      return theRenderMode;
   }
   virtual void setRenderMode(ossimPlanetVideoLayerNode::RenderMode mode)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theVideoLayerNodeMutex);
      theRenderMode = mode;
   }
   
   void setVideoLayer(ossimPlanetVideoLayer* layer)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theVideoLayerNodeMutex);
      theLayer = layer;
   }
   ossimPlanetVideoLayer* videoLayer()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theVideoLayerNodeMutex);
      return theLayer;
   }
   const ossimPlanetVideoLayer* videoLayer()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theVideoLayerNodeMutex);
      return theLayer;
   }
	
protected:
   mutable ossimPlanetReentrantMutex   theVideoLayerNodeMutex;
   ossimPlanetVideoLayer*                theLayer;
   ossimPlanetVideoLayerNode::RenderMode theRenderMode;
};
#endif
