#ifndef ossimPlanetAnnotationLayerNode_HEADER
#define ossimPlanetAnnotationLayerNode_HEADER
#include <ossimPlanet/ossimPlanetNode.h>
#include <ossim/base/ossimNotify.h>
#include <osg/ref_ptr>
#include <osg/Geometry>
#include <osg/Vec3d>
#include <osg/ClusterCullingCallback>
#include <osg/Matrixd>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/Timer>
#include <ossimPlanet/ossimPlanetFadeText.h>
#include <ossimPlanet/ossimPlanetOperation.h>
#include <osg/Vec4d>
enum ossimPlanetAnnotationColorMode
{
   ossimPlanetAnnotationColorMode_NORMAL = 0,
   ossimPlanetAnnotationColorMode_RANDOM = 1
};

class OSSIMPLANET_DLL ossimPlanetAnnotationExpireTime : public osg::Referenced
{
public:
   ossimPlanetAnnotationExpireTime(){}
   virtual void initTimeStamp()=0;
   virtual bool hasExpired()const=0;
};

class OSSIMPLANET_DLL ossimPlanetAnnotationColorStyle : public osg::Referenced
{
public:
   ossimPlanetAnnotationColorStyle()
   :theColor(1.0,1.0,1.0,1.0),
   theColorMode(ossimPlanetAnnotationColorMode_NORMAL)
   {
   }
   void setColor(const osg::Vec4d& color)
   {
      theColor = color;
   }
   const osg::Vec4d& color()const
   {
      return theColor;
   }
   void color(osg::Vec4d& value)const
   {
      value = theColor;
   }
   void setColorMode(ossimPlanetAnnotationColorMode mode)
   {
      theColorMode = mode;
   }
   ossimPlanetAnnotationColorMode colorMode()const
   {
      return theColorMode;
   }
protected:
   osg::Vec4d theColor;
   ossimPlanetAnnotationColorMode theColorMode;
};

class OSSIMPLANET_DLL ossimPlanetAnnotationLabelStyle : public ossimPlanetAnnotationColorStyle
{
public:
   ossimPlanetAnnotationLabelStyle()
   :theScale(1.0)
   {
   }
   void setScale(ossim_float64 scale)
   {
      theScale = scale;
   }
   ossim_float64 scale()const
   {
      return theScale;
   }
protected:
   ossim_float64 theScale;
};

class OSSIMPLANET_DLL ossimPlanetAnnotationExpireDuration : public ossimPlanetAnnotationExpireTime
{
public:
   ossimPlanetAnnotationExpireDuration(double duration=0.0)
   :theDuration(duration),
   theUnit(ossimPlanetTimeUnit_SECONDS),
   theInitialStamp(osg::Timer::instance()->tick())
   {
   }
   double duration()const
   {
      return theDuration;
   }
   void setDuration(double value)
   {
      theDuration = value;
   }
   ossimPlanetTimeUnit unit()const
   {
      return theUnit;
   }
   virtual void initTimeStamp()
   {
      theInitialStamp = osg::Timer::instance()->tick();
   }
   virtual bool hasExpired()const
   {
      return osg::Timer::instance()->delta_s(theInitialStamp, osg::Timer::instance()->tick())>=theDuration;
   }
protected:
   double theDuration;
   ossimPlanetTimeUnit theUnit;
   osg::Timer_t theInitialStamp;
};
class ossimPlanetAnnotationGroupNode;
class OSSIMPLANET_DLL ossimPlanetAnnotationLayerNode : public ossimPlanetNode
{
public:
	enum DirtyBit
	{
		NOT_DIRTY = 0,
		COORDINATE_DIRTY = 1,
		COLOR_DIRTY      = 2,
		LABEL_DIRTY      = 4,
		ICON_DIRTY       = 8,
		MATRIX_DIRTY     = 16,
      GENERIC_DIRTY    = 32,
		ALL_DIRTY        = (COORDINATE_DIRTY|COLOR_DIRTY|ICON_DIRTY|LABEL_DIRTY|MATRIX_DIRTY|GENERIC_DIRTY)
	};
	ossimPlanetAnnotationLayerNode();
	
	/**
	 * Xml formatted custom data.
	 */
	void setExtendedData(const ossimString& value);
   virtual void execute(const ossimPlanetAction& action);
	virtual void update(){};
	virtual void stage()
   {		
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theStagedFlag = true;
   }
	virtual bool isStaged()const
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
		return theStagedFlag;
	}
   virtual void traverse(osg::NodeVisitor& nv);
   
	void setStagedFlag(bool flag)
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
		theStagedFlag = flag;
	}
   virtual ossimPlanetAnnotationGroupNode* asAnnotationGroup()
   {
      return 0;
   }
   virtual const ossimPlanetAnnotationGroupNode* asAnnotationGroup()const
   {
      return 0;
   }
	void setDirtyBits(DirtyBit bit)
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
		theDirtyBit = bit;
	}
	void setDirtyBit(DirtyBit bit)
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
		theDirtyBit |= (ossim_uint32)bit;
	}
	void clearDirtyBit(DirtyBit bit)
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
		
		theDirtyBit &= (~((ossim_uint32)bit));
	}
	bool isDirtyBitSet(DirtyBit bit)const
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
		return (theDirtyBit & bit);
	}
protected:
	mutable ossimPlanetReentrantMutex thePropertyMutex;
	ossim_uint32 theDirtyBit;
	bool theStagedFlag;
   osg::ref_ptr<ossimPlanetAnnotationExpireTime> theExpireTime;
};

class ossimPlanetAnnotationPoint;
class OSSIMPLANET_DLL ossimPlanetAnnotationGeometry : public osg::Referenced
{
public:
	ossimPlanetAnnotationGeometry()
	:theExtrudeFlag(false),
	theAltitudeMode(ossimPlanetAltitudeMode_CLAMP_TO_GROUND)
	{
		
	}
	virtual void setExtrudeFlag(bool flag)
	{
		theExtrudeFlag = flag;
	}
	virtual void setAltitudeMode(ossimPlanetAltitudeMode mode)
	{
		theAltitudeMode = mode;
	}
	ossimPlanetAltitudeMode altitudeMode()const
	{
		return theAltitudeMode;
	}
	virtual ossimPlanetAnnotationPoint* asPoint()
	{
		return 0;
	}
	virtual const ossimPlanetAnnotationPoint* asPoint()const
	{
		return 0;
	}
	virtual void traverse( osg::NodeVisitor& nv)=0;
protected:
	bool                    theExtrudeFlag;
	ossimPlanetAltitudeMode theAltitudeMode;
};

class ossimPlanetAnnotationPoint: public ossimPlanetAnnotationGeometry
{
public:
	ossimPlanetAnnotationPoint(const osg::Vec3d& coordinate=osg::Vec3d(0.0,0.0,0.0))
	:theCoordinate(coordinate)
	{
		
	}
	virtual ossimPlanetAnnotationPoint* asPoint()
	{
		return this;
	}
	virtual const ossimPlanetAnnotationPoint* asPoint()const
	{
		return this;
	}
	void setCoordinate(const osg::Vec3d& coordinate)
	{
		theCoordinate = coordinate;
	}
	const osg::Vec3d& coordinate()const
	{
		return theCoordinate;
	}
	void setModelCoordinate(const osg::Vec3d& modelCoordinate)
	{
		theModelCoordinate = modelCoordinate;
	}
	const osg::Vec3d& modelCoordinate()const
	{
		return theModelCoordinate;
	}
	virtual void traverse( osg::NodeVisitor& nv)
	{
		if(theMatrixTransform.valid())
		{
			theMatrixTransform->accept(nv);
		}
	}
	void setMatrixTransform(osg::ref_ptr<osg::MatrixTransform> m)
	{
		theMatrixTransform = m;
	}
	osg::ref_ptr<osg::MatrixTransform> matrixTransform()
	{
		return theMatrixTransform;
	}
	const osg::ref_ptr<osg::MatrixTransform> matrixTransform()const
	{
		return theMatrixTransform;
	}
	
protected:
	osg::Vec3d theCoordinate;
	osg::Vec3d theModelCoordinate;
	osg::Vec3d theLocalCoordinate;
	osg::ref_ptr<osg::MatrixTransform> theMatrixTransform;
};


class ossimPlanetAnnotationGroupNode : public ossimPlanetAnnotationLayerNode
{
public:
   ossimPlanetAnnotationGroupNode()
   {
      
   }
   virtual ossimPlanetAnnotationGroupNode* asAnnotationGroup()
   {
      return this;
   }
   virtual const ossimPlanetAnnotationGroupNode* asAnnotationGroup()const
   {
      return this;
   }
   virtual bool addChild( Node *child )
   {
      ossimPlanetAnnotationLayerNode* annotationLayerNode = dynamic_cast<ossimPlanetAnnotationLayerNode*>(child);
      if(annotationLayerNode)
      {
         return ossimPlanetAnnotationLayerNode::addChild(annotationLayerNode);
      }
      ossimNotify(ossimNotifyLevel_WARN) << "ossimPlanetAnnotationGroupNode::addChild: Only annotation layer nodes allowed to be added to an Annotation group\n";
      return false;
   }
	virtual void stage()
   {
      
   }
	virtual void update()
   {
      
   }
protected:
   
};
class ossimPlanetAnnotationTextGeode : public osg::Geode
{
public:
	ossimPlanetAnnotationTextGeode(ossimPlanetNode* layerNode,
											 ossimPlanetFadeText* text)
	:theLayerNode(layerNode),
	theText(text)
	{
		if(text)
		{
			addDrawable(text);
		}
		setUpdateCallback(new ossimPlanetTraverseCallback);
		setCullCallback(new ossimPlanetTraverseCallback);
      getOrCreateStateSet()->setMode(GL_BLEND,osg::StateAttribute::ON);
      getOrCreateStateSet()->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
		
	}
	virtual void traverse(osg::NodeVisitor& nv) 
	{
      switch(nv.getVisitorType())
      {
         case osg::NodeVisitor::UPDATE_VISITOR:
         {
            theSavedOpacity = theText->opacity();
            break;
         }
         default:
         {
            break;
         }
      }
		osg::Geode::traverse(nv);
      if(theSavedOpacity!=theText->opacity())
      {
         if(theLayerNode)
         {
            theLayerNode->setRedrawFlag(true);
         }
      }
	}
	
protected:
   ossim_float64 theSavedOpacity;
	ossimPlanetNode* theLayerNode;
	ossimPlanetFadeText* theText;
};

class OSSIMPLANET_DLL ossimPlanetAnnotationPlacemark : public ossimPlanetAnnotationLayerNode
{
public:
	class PlacemarkUpdater;
	friend class PlacemarkUpdater;
	ossimPlanetAnnotationPlacemark();
	ossimPlanetAnnotationPlacemark(const osg::Vec3d& location,
                                  ossimPlanetAltitudeMode altitudeMode,
                                  const ossimString& nameStr = "",
                                  const ossimString& descriptionStr = "");
	virtual void execute(const ossimPlanetAction& action);
	virtual void stage();
	virtual void update();
	virtual void traverse(osg::NodeVisitor& nv);
	virtual void setName(const ossimString& name);
	const osg::ref_ptr<ossimPlanetFadeText> label()const
	{
		return theLabel.get();
	}
	osg::ref_ptr<ossimPlanetFadeText> label()
	{
	  return theLabel.get();
	}
  const osg::ref_ptr<ossimPlanetAnnotationTextGeode> labelGeode()const
  {
    return theLabelGeode.get();
  }
  osg::ref_ptr<ossimPlanetAnnotationTextGeode> labelGeode()
  {
    return theLabelGeode.get();
  }
	const osg::ref_ptr<ossimPlanetAnnotationGeometry> geometry()const
	{
		return theGeometry;
	}
	osg::ref_ptr<osg::ClusterCullingCallback> clusterCull()
	{
		return theClusterCull;
	}
	const osg::ref_ptr<osg::ClusterCullingCallback> clusterCull()const
	{
		return theClusterCull;
	}
	
	const osg::ref_ptr<ossimPlanetAnnotationLabelStyle> labelStyle()const
	{
		return theLabelStyle;
	}
	osg::ref_ptr<ossimPlanetAnnotationGeometry> geometry()
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theGeometryMutex);
		return theGeometry;
	}
	void setGeometry(osg::ref_ptr<ossimPlanetAnnotationGeometry> geom)
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theGeometryMutex);
		theGeometry = geom;
	}
	virtual void setEnableFlag(bool flag)
	{
		if(enableFlag()!=flag)
		{
			if(theLabel.valid())
			{
				theLabel->setOpacity(0.0);
			}
		}
		ossimPlanetAnnotationLayerNode::setEnableFlag(flag);
	}
protected:
   mutable ossimPlanetReentrantMutex theUpdateMutex;
	osg::ref_ptr<osg::ClusterCullingCallback> theClusterCull;
	osg::ref_ptr<ossimPlanetAnnotationTextGeode> theLabelGeode;
	mutable ossimPlanetReentrantMutex theGeometryMutex;
	osg::ref_ptr<ossimPlanetAnnotationGeometry> theGeometry;
   osg::ref_ptr<ossimPlanetFadeText> theLabel;
	
   osg::ref_ptr<ossimPlanetAnnotationLabelStyle> theLabelStyle;
};


#endif
