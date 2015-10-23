#ifndef ossimPlanetBoundingBox_HEADER
#define ossimPlanetBoundingBox_HEADER
#include <osg/Referenced>
#include <osg/Polytope>
#include <osg/CullSettings>
#include <vector>
#include <ossim/base/ossimCommon.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/mkUtils.h>

class OSSIMPLANET_DLL ossimPlanetBoundingBox : public osg::Referenced
{
public:
   ossimPlanetBoundingBox()
   :osg::Referenced()
   {
      theCorners[0] = osg::Vec3d(0,0,0);
      theCorners[1] = osg::Vec3d(0,0,0);
      theCorners[2] = osg::Vec3d(0,0,0);
      theCorners[3] = osg::Vec3d(0,0,0);
      theCorners[4] = osg::Vec3d(0,0,0);
      theCorners[5] = osg::Vec3d(0,0,0);
      theCorners[6] = osg::Vec3d(0,0,0);
      theCorners[7] = osg::Vec3d(0,0,0);

      theCenter = osg::Vec3d(0,0,0);
      theRadius = 0.0;
   }
   ossimPlanetBoundingBox(const osg::Vec3d &p0,
                          const osg::Vec3d &p1,
                          const osg::Vec3d &p2,
                          const osg::Vec3d &p3,
                          const osg::Vec3d &p4,
                          const osg::Vec3d &p5,
                          const osg::Vec3d &p6,
                          const osg::Vec3d &p7)
   :osg::Referenced()
   {
      theCorners[0] = p0;
      theCorners[1] = p1;
      theCorners[2] = p2;
      theCorners[3] = p3;
      theCorners[4] = p4;
      theCorners[5] = p5;
      theCorners[6] = p6;
      theCorners[7] = p7;

      theCenter = (p0+p1+p2+p3+p4+p5+p6+p7)*(1.0/8.0);
      computeRadius();
   }
   ossimPlanetBoundingBox(const ossimPlanetBoundingBox& src)
   :osg::Referenced()
   {
      theCorners[0] = src.theCorners[0];
      theCorners[1] = src.theCorners[1];
      theCorners[2] = src.theCorners[2];
      theCorners[3] = src.theCorners[3];
      theCorners[4] = src.theCorners[4];
      theCorners[5] = src.theCorners[5];
      theCorners[6] = src.theCorners[6];
      theCorners[7] = src.theCorners[7];
      theCenter     = src.theCenter;
      theRadius      = src.theRadius;
   }
   // note the distance is a 2d vector.  the distance[0]
   // is the distance to extrude the points in the direction
   // of the normal and the distance[1] is the distance along
   // the reflected direction.
   //
   void extrude(const osg::Vec3d &p0,
                const osg::Vec3d &p1,
                const osg::Vec3d &p2,
                const osg::Vec3d &p3,
                const osg::Vec3d &outwardNormal,
                const osg::Vec2d &distance)
   {
      theCorners[0] = p0 + outwardNormal*distance[1];
      theCorners[1] = p1 + outwardNormal*distance[1];
      theCorners[2] = p2 + outwardNormal*distance[1];
      theCorners[3] = p3 + outwardNormal*distance[1];

      theCorners[4] = p0 + outwardNormal*distance[0];
      theCorners[5] = p1 + outwardNormal*distance[0];
      theCorners[6] = p2 + outwardNormal*distance[0];
      theCorners[7] = p3 + outwardNormal*distance[0];
      theCenter = (theCorners[0]+
                   theCorners[1]+
                   theCorners[2]+
                   theCorners[3]+
                   theCorners[4]+
                   theCorners[5]+
                   theCorners[6]+
                   theCorners[7])*(1.0/8.0);
      computeRadius();
   }
   void extrude(const osg::Vec3d& center,
                const osg::Vec3d& up,
                const osg::Vec3d& right,
                const osg::Vec3d& normal,
                const osg::Vec2d& distanceUp,
                const osg::Vec2d& distanceRight,
                const osg::Vec2d& distanceNormal)
   {
      // along negative normal
      //ul, ur, lr, ll
      theCorners[0] = center+((up*distanceUp[0])+
                              (right*distanceRight[1]) +
                              (normal*distanceNormal[1]));
      theCorners[1] = center+((up*distanceUp[0])+
                              (right*distanceRight[0]) +
                              (normal*distanceNormal[1]));
      theCorners[2] = center+((up*distanceUp[1])+
                              (right*distanceRight[0]) +
                              (normal*distanceNormal[1]));
      
      theCorners[3] = center+((up*distanceUp[1])+
                              (right*distanceRight[1]) +
                              (normal*distanceNormal[1]));
      // along positive normal
      //ul, ur, lr, ll
      theCorners[4] = center+((up*distanceUp[0])+
                              (right*distanceRight[1]) +
                              (normal*distanceNormal[0]));
      theCorners[5] = center+((up*distanceUp[0])+
                              (right*distanceRight[0]) +
                              (normal*distanceNormal[0]));
      theCorners[6] = center+((up*distanceUp[1])+
                              (right*distanceRight[0]) +
                              (normal*distanceNormal[0]));
      
      theCorners[7] = center+((up*distanceUp[1])+
                              (right*distanceRight[1]) +
                              (normal*distanceNormal[0]));
      theCenter = (theCorners[0]+
                   theCorners[1]+
                   theCorners[2]+
                   theCorners[3]+
                   theCorners[4]+
                   theCorners[5]+
                   theCorners[6]+
                   theCorners[7])*(1.0/8.0);
      computeRadius();
   }

   
   osg::Vec3d& operator[](unsigned int idx)
   {
      return theCorners[idx];
   }
   const osg::Vec3d& operator[](unsigned int idx)const
   {
      return theCorners[idx];
   }
   bool intersects(const osg::Polytope& frustum)const;
   bool isInFront(const osg::Vec3d& eye,
                  const osg::Vec3d& direction)const;
   
   
   void transform(const osg::Matrix& m)
   {
      theCorners[0] = theCorners[0]*m,
      theCorners[1] = theCorners[1]*m,
      theCorners[2] = theCorners[2]*m,
      theCorners[3] = theCorners[3]*m,
      theCorners[4] = theCorners[4]*m,
      theCorners[5] = theCorners[5]*m,
      theCorners[6] = theCorners[6]*m,
      theCorners[7] = theCorners[7]*m,
      theCenter = (theCorners[0]+
                   theCorners[1]+
                   theCorners[2]+
                   theCorners[3]+
                   theCorners[4]+
                   theCorners[5]+
                   theCorners[6]+
                   theCorners[7])*(1.0/8.0);
      theTopCenter = (theCorners[0]+
                      theCorners[1]+
                      theCorners[2]+
                      theCorners[3])*.25;
      theBottomCenter = (theCorners[4]+
                         theCorners[5]+
                         theCorners[6]+
                         theCorners[7])*.25;
      computeRadius();
      
   }
   void setCenter(const osg::Vec3d& center)
   {
      theCenter = center;
   }
   const osg::Vec3d& center()const
   {
      return theCenter;
   }
   double radius()const
   {
      return theRadius;
   }
   double groundRadius()const
   {
      return theGroundRadius;
   }
protected:
   void computeRadius()
   {
      theRadius = ossim::max((theCorners[0]-theCenter).length(),
                           ossim::max((theCorners[1]-theCenter).length(),
                                    ossim::max((theCorners[2]-theCenter).length(),
                                             ossim::max((theCorners[3]-theCenter).length(),
                                                      ossim::max((theCorners[4]-theCenter).length(),
                                                               ossim::max((theCorners[5]-theCenter).length(),
                                                                        ossim::max((theCorners[6]-theCenter).length(),
                                                                                 (theCorners[7]-theCenter).length())))))));
      // this is a crude ground radius estimate that just takes the top
      // Quad and Bottom quad and see who has the largest radius and takes the Max of that
      theGroundRadius = ossim::max((theCorners[0]-theTopCenter).length(),
                           ossim::max((theCorners[1]-theTopCenter).length(),
                                    ossim::max((theCorners[2]-theTopCenter).length(),
                                             ossim::max((theCorners[3]-theTopCenter).length(),
                                                      ossim::max((theCorners[4]-theBottomCenter).length(),
                                                               ossim::max((theCorners[5]-theBottomCenter).length(),
                                                                        ossim::max((theCorners[6]-theBottomCenter).length(),
                                                                                 (theCorners[7]-theBottomCenter).length())))))));
   }
   osg::Vec3d theCorners[8];
   osg::Vec3d theCenter;
   osg::Vec3d theTopCenter;
   osg::Vec3d theBottomCenter;
   double theRadius;
   double theGroundRadius;
};

#endif
