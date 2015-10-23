#ifndef ossimPlanetAnimatedPointModel_HEADER
#define ossimPlanetAnimatedPointModel_HEADER
#include <ossimPlanet/ossimPlanetPointModel.h>
#include <ossimPlanet/ossimPlanetAnimationPath.h>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <OpenThreads/Mutex>

class OSSIMPLANET_DLL ossimPlanetAnimatedPointModel : public ossimPlanetAnnotationLayerNode 
{
public:
   ossimPlanetAnimatedPointModel();
   virtual ~ossimPlanetAnimatedPointModel();
   void execute(const ossimPlanetAction& action);
   void setAnimationPath(ossimPlanetAnimationPath* path);
   void setAnimationPathColor(const osg::Vec4f& value);
   void setAnimationPathLineThickness(ossim_float32 value);
   void setShowPathFlag(bool flag);
   void setShowModelFlag(bool flag);
   void setPointModel(osg::Node* value);
   void setTimeScale(ossim_float64 scale);
   void setTimeOffset(ossim_float64 offset);
   virtual void traverse(osg::NodeVisitor& nv);
	virtual void stage();
   
   osg::Node*          pointModel(){return thePointModel.get();}
   const osg::Node*    pointModel()const{return thePointModel.get();}
   ossimPlanetAnimationPath*       animationPath(){return theAnimationPath.get();}
   const ossimPlanetAnimationPath* animationPath()const{return theAnimationPath.get();}
   
protected:
   void updateCoordinates();
   void updateColor();
   class OSSIMPLANET_DLL PathCallback : public osg::AnimationPathCallback
   {
   public:
      PathCallback();
      PathCallback(const PathCallback& apc,
                   const osg::CopyOp& copyop);
      PathCallback(osg::AnimationPath* ap,
                   double timeOffset=0.0,
                   double timeMultiplier=1.0);
      virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
   };
   OpenThreads::Mutex                     theUpdateCoordinatesMutex;
   OpenThreads::Mutex                     theUpdateColorMutex;
   bool                                   theShowPathFlag;
   bool                                   theShowModelFlag;
   osg::Vec4f                             theAnimationPathColor;
   ossim_float32                          theAnimationPathLineThickness;
   osg::ref_ptr<osg::Node>                thePointModel;
   osg::ref_ptr<ossimPlanetAnimationPath> theAnimationPath;

   // need drawing for the path
   //
   osg::ref_ptr<osg::MatrixTransform>     thePathMatrixTransform;
   osg::ref_ptr<osg::Vec4Array>           thePathColor;
   osg::ref_ptr<osg::Vec3Array>           thePathVertices;
   osg::ref_ptr<osg::LineWidth>           theLineWidth;
   osg::ref_ptr<osg::Geode>               thePathGeode;
   osg::ref_ptr<osg::Geometry>            thePathGeometry;
   
   osg::ref_ptr<PathCallback>             theAnimationPathCallback;
};

#endif
