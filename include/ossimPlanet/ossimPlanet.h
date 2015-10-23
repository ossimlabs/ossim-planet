#ifndef ossimPlanet_HEADER
#define ossimPlanet_HEADER
#include <osg/MatrixTransform>
#include <vector>
#include "ossimPlanetExport.h"
#include <osg/MatrixTransform>
#include <osgUtil/CullVisitor>
#include <ossimPlanet/ossimPlanetDatabasePager.h>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/ossimPlanetConstants.h>
#include <ossimPlanet/ossimPlanetLand.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetCallback.h>
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetSousaLayer.h>
#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetXmlAction.h>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>

class ossimPlanetLand;
class OSSIMPLANET_DLL ossimPlanet : public osg::MatrixTransform,
                                    public ossimPlanetActionReceiver,
                                    public ossimPlanetCallbackListInterface<ossimPlanetNodeCallback>

{
public:   
   friend class ossimPlanetUpdateCallback;
   class LayerListener : public ossimPlanetNodeCallback
      {
      public:
         LayerListener(ossimPlanet* planet);
         virtual void needsRedraw(ossimPlanetNode* node);
         void setPlanet(ossimPlanet* planet);
      protected:
         ossimPlanet* thePlanet;
      };
   ossimPlanet();
   virtual osg::Object* cloneType() const { return new ossimPlanet(); }
   virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const ossimPlanet *>(obj)!=NULL; }
   virtual const char* className() const { return "ossimPlanet"; } 
   virtual const char* libraryName() const { return "ossimPlanet"; }
   virtual void setupDefaults();
   virtual bool removeChildren(unsigned int pos,unsigned int numChildrenToRemove);
   
   virtual void traverse(osg::NodeVisitor& nv);
   virtual osg::BoundingSphere computeBound() const;
   //   virtual bool computeBound() const;
   osg::ref_ptr<ossimPlanetGeoRefModel> model()
	{
		return theModel;
	}
   const osg::ref_ptr<ossimPlanetGeoRefModel> model()const
	{
		return theModel;
	}
   
   const osg::Vec3d& getEyePositionLatLonHeight()const;
   osg::Vec3d getNadirPoint()const;
   osg::Vec3d getLineOfSitePoint()const;
   osg::Vec3d getNadirLatLonHeightPoint()const;
   osg::Vec3d getLineOfSiteLatLonHeightPoint()const;
   const osg::Vec3d& hpr()const;
   
    void setComputeIntersectionFlag(bool flag);
    bool getComputeIntersectionFlag()const;
   static ossimPlanet* findPlanet(osg::Node* startNode);
   static ossimPlanet* findPlanet(osg::NodePath& currentNodePath);
	void resetAllRedrawFlags();
   void setRedrawFlag(bool flag)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theRedrawFlag = flag;
   }
   bool redrawFlag()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theRedrawFlag;
   }
   /**
    * This executes passed in actions.
    *
    */
   virtual void execute(const ossimPlanetAction& action);
   
   void notifyNeedsRedraw(ossimPlanetNode* node);
   const ossimPlanetLookAt* lookAt()const
   {
      return theLookAt.get();
   }
   void setLookAt(osg::ref_ptr<ossimPlanetLookAt> look)
   {
      theLookAt = look.get();
   }
   const ossimPlanetLookAt* eyePosition()const
   {
      return theEyePosition.get();
   }
   void setEyePosition(osg::ref_ptr<ossimPlanetLookAt> eye)
   {
      theEyePosition = eye.get();
   }
   
protected:
   ~ossimPlanet();
   virtual void xmlExecute(const ossimPlanetXmlAction& xmlAction);
   virtual void childInserted(unsigned int pos);
	virtual void childRemoved(unsigned int /*pos*/, unsigned int /*numChildrenToRemove*/);
   void notifyLayerAdded(ossimPlanetLayer* layer);
   void notifyLayerRemoved(ossimPlanetLayer* layer);
   void computeIntersection(osgUtil::CullVisitor* cullVisitor);
   bool theComputeIntersectionFlag;
   bool theRedrawFlag;
   osg::Vec3d theNadirPoint;
   osg::Vec3d theLineOfSitePoint;
   osg::Vec3d theNadirLatLonHeightPoint;
   osg::Vec3d theLineOfSiteLatLonHeightPoint;
   osg::Vec3d theEyePositionLatLonHeight;
   mutable ossimPlanetReentrantMutex theTraversalMutex;
   mutable ossimPlanetReentrantMutex thePropertyMutex;
   osg::ref_ptr<ossimPlanetGeoRefModel> theModel;
   osg::Vec3d theLsrHeadingPitchRoll;
   osg::ref_ptr<ossimPlanet::LayerListener> theLayerListener;
   
   ossimPlanetReentrantMutex theLayersToAddListMutex;
   // Internal list that is maintained through creation of layers through the
   // recever commands.  It will be synched to the graph on next Update visitation
   //
   std::vector<osg::ref_ptr<ossimPlanetLayer> > theLayersToAddList;
   
   osg::ref_ptr<ossimPlanetLookAt> theLookAt;
   osg::ref_ptr<ossimPlanetLookAt> theEyePosition;

};

#endif
