#ifndef ossimPlanetSceneView_HEADER
#define ossimPlanetSceneView_HEADER
#include<osgUtil/SceneView>
#include <ossimPlanet/ossimPlanetExport.h>
#include <osgUtil/IntersectVisitor>

class OSSIMPLANET_DLL ossimPlanetSceneView : public osgUtil::SceneView
{
public:
   ossimPlanetSceneView();
   virtual bool pickObjects(osgUtil::IntersectVisitor::HitList& hits,
                            osg::Node* startNode,
                            double vx, double vy,
                            double startPointShift);
   
/*    virtual bool intersectScene(osg::Node* startNode, */
/*                                osg::Vec3d& intersection, */
/*                                const osg::Vec3d& startPt, */
/*                                const osg::Vec3d& endPt); */
   virtual bool intersectScene(osg::Node* startNode,
                               osg::Vec3d& intersectionPoint,
                               double vx, double vy,
                               double startPointShift=0.0);
/*    virtual bool computeNadirIntersection(osg::Vec3d& intersectionPoint, */
/*                                          double startPointShift=0.0); */
   virtual bool computeLineOfSiteIntersection(osg::Vec3d& intersectionPoint,
                                              double startPointShift=0.0);
   void getEyePosition(osg::Vec3d& eye)const;
   void getLookDirection(osg::Vec3d& direction)const;
/*    bool makeRay(osg::Vec3d& origin, */
/*                 osg::Vec3d& ray, */
/*                 double vx, double vy); */
   
protected:
};

#endif
