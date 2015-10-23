#include <iostream>
#include <osg/Projection>
#include <ossimPlanet/ossimPlanetSceneView.h>
#include <osg/LineSegment>
#include <ossim/base/ossimConstants.h>


ossimPlanetSceneView::ossimPlanetSceneView()
{
}

// bool ossimPlanetSceneView::intersectScene(osg::Vec3d& intersection,
//                                           const osg::Vec3d& startPt,
//                                           const osg::Vec3d& endPt)
// {
//    if(!getSceneData()) return false;
//    osg::ref_ptr<osg::LineSegment> segLookVector = new osg::LineSegment;
   
//    osgUtil::IntersectVisitor iv;
//    osg::Vec3d s = startPt;
//    osg::Vec3d e = endPt;

//    segLookVector->set(s, e);
//    iv.addLineSegment(segLookVector.get());
   
//    getSceneData()->accept(iv);
//    bool hitFound = false;
//    if (iv.hits())
//    {
//       osgUtil::IntersectVisitor::HitList& hitList = iv.getHitList(segLookVector.get());
//       if (!hitList.empty())
//       {
//          intersection = hitList.front().getWorldIntersectPoint();
         
//          hitFound = true;
//       }
//    }

//    return hitFound;
// }

bool ossimPlanetSceneView::pickObjects(osgUtil::IntersectVisitor::HitList& hits,
                                       osg::Node* startNode,
                                       double vx, double vy,
                                       double /*startPointShift*/)
{
   osg::Node* rootNode = 0;
   if(startNode)
   {
      rootNode = startNode;
   }
   else
   {
      rootNode = getSceneData();
   }
   osg::Matrixd proj   = getProjectionMatrix();
   osg::Matrixd view   = getViewMatrix();
   const osg::Viewport* viewport = getViewport();
   osg::Matrixd projToWindow = viewport->computeWindowMatrix();
   osg::Vec3d projPt(vx, vy, 1.0);
   osg::Vec3d windowPt = projPt*projToWindow;
   
   osg::NodePathList parentNodePaths = rootNode->getParentalNodePaths(rootNode);
   for(unsigned int i=0;i<parentNodePaths.size();++i)
   {
      osg::NodePath& nodePath = parentNodePaths[i];
      
      // remove the intersection node from the nodePath as it'll be accounted for
      // in the PickVisitor traversal, so we don't double account for its transform.
      if (!nodePath.empty()) nodePath.pop_back();  
      
      osg::Matrixd modelview(view);
      // modify the view matrix so that it accounts for this nodePath's accumulated transform
      if (!nodePath.empty()) modelview.preMult(computeLocalToWorld(nodePath));
      
      osgUtil::PickVisitor pick(viewport, proj, modelview, windowPt[0], windowPt[1]);
      pick.setTraversalMask(0xffffffff);
      rootNode->accept(pick);
      
      // copy all the hits across to the external hits list
      for(osgUtil::PickVisitor::LineSegmentHitListMap::iterator itr = pick.getSegHitList().begin();
          itr != pick.getSegHitList().end();
          ++itr)
      {
         hits.insert(hits.end(), itr->second.begin(), itr->second.end());
      }
      
   }

   return !hits.empty();
}
 
bool ossimPlanetSceneView::intersectScene(osg::Node* startNode,
                                          osg::Vec3d& intersectionPoint,
                                          double vx, double vy,
                                          double startPointShift)
{
   osgUtil::IntersectVisitor::HitList hits;
   if(!pickObjects(hits, startNode, vx, vy, startPointShift))
   {
      return false;
   }
   intersectionPoint = hits.front().getWorldIntersectPoint();

   return true;
}

// bool ossimPlanetSceneView::computeNadirIntersection(osg::Vec3d& intersectionPoint,
//                                                   double startPointShift)
// {
//    osg::Vec3 eye(0,0,0);
//    osg::Vec3 center(0,0,0);
//    osg::Vec3 up(0,0,0);
//    osg::Vec3d start;
//    getViewMatrixAsLookAt(eye, center, up);
//    osg::Vec3d direction = center-eye;
//    direction.normalize();
//    intersectionPoint = osg::Vec3d(0.0,0.0,0.0);
//    start = eye + direction*startPointShift;
//    return intersectScene(intersectionPoint,
//                          start,
//                          start + direction*(1.0/FLT_EPSILON));
// }

bool ossimPlanetSceneView::computeLineOfSiteIntersection(osg::Vec3d& intersectionPoint,
                                                       double startPointShift)
{
   osg::Viewport* v = getViewport();
   
   if(v)
   {
      return intersectScene(0,
                            intersectionPoint,
                            (double)v->x() + v->width()/2.0,
                            (double)v->y() + v->height()/2.0,
                            startPointShift);
   }

   intersectionPoint = osg::Vec3d(0.0,0.0,0.0);
   
   return false;
}

// bool ossimPlanetSceneView::makeRay(osg::Vec3d& origin,
//                                  osg::Vec3d& ray,
//                                  double vx, double vy)
// {
//    osg::Vec3 eye(0,0,0);
//    osg::Vec3 center(0,0,0);
//    osg::Vec3 up(0,0,0);
//    osg::Vec3 direction;
//    double fov;
//    double aspectRatio;
//    double znear;
//    double zfar;
//    osg::Matrixd pm = getProjectionMatrix();
//    ray[0] = 0;
//    ray[1] = 0;
//    ray[2] = 0;
   
//    if(!pm.getPerspective(fov, aspectRatio, znear, zfar))
//    {
//       return false;
//    }
//    getViewMatrixAsLookAt(eye, center, up, 1000.0);
//    osg::Matrixd iv = getViewMatrix();
   
//    const osg::ref_ptr<osg::Viewport> viewport = getViewport();
//    osg::Matrix wm;
//    wm.invert(viewport->computeWindowMatrix());
//    osg::Vec3d normalizedPoint = osg::Vec3d(vx,vy,0.0)*wm;

//    double angleY = (fov*.5)*normalizedPoint[1];
//    double angleX = (fov*.5)*normalizedPoint[0]*aspectRatio;
// //    double angleX = (fov*.5)*normalizedPoint[0];
// //    double angleY = (fov*.5)*normalizedPoint[1];
//    direction = center - eye;
//    direction.normalize();
   
//    osg::Vec3d crossVec = direction^up;
//    osg::Vec3d newUp = crossVec^direction;
//    osg::Matrixd m = (osg::Matrixd::rotate(-angleX*(M_PI/180.0), newUp)*
//                      osg::Matrixd::rotate(angleY*(M_PI/180.0), crossVec));
//    ray    = direction*m;
//    origin = eye;

//    return true;
// }

void ossimPlanetSceneView::getEyePosition(osg::Vec3d& eye)const
{
   osg::Vec3 eyeTemp(0,0,0);
   osg::Vec3 center(0,0,0);
   osg::Vec3 up(0,0,0);
   
   (const_cast<ossimPlanetSceneView*>(this))->getViewMatrixAsLookAt(eyeTemp, center, up);

   eye[0] = eyeTemp[0];
   eye[1] = eyeTemp[1];
   eye[2] = eyeTemp[2];
}

void ossimPlanetSceneView::getLookDirection(osg::Vec3d& direction)const
{
   osg::Vec3 eyeTemp(0,0,0);
   osg::Vec3 center(0,0,0);
   osg::Vec3 up(0,0,0);
   
   (const_cast<ossimPlanetSceneView*>(this))->getViewMatrixAsLookAt(eyeTemp, center, up);

   direction = (center-eyeTemp);

   direction.normalize();
}
