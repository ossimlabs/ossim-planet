#include <ossimPlanet/ossimPlanetBoundingBox.h>

#include <osg/Vec4>
#include <iostream>

bool ossimPlanetBoundingBox::intersects(const osg::Polytope& frustum)const
{
   const osg::Polytope::PlaneList& planeList = frustum.getPlaneList();
   unsigned int idx = 0;
   unsigned int ptIdx = 0;
   unsigned int outsideCount    = 0;
   unsigned int upperBound = planeList.size();
   double testValue = 0.0;
   for(; idx < upperBound;++idx)
   {
      const osg::Vec4& plane = planeList[idx].asVec4();
      outsideCount = 0;
      for(ptIdx = 0; ptIdx < 8; ++ptIdx)
      {
         testValue =  (((((double)plane[0])*theCorners[ptIdx][0] +
                         ((double)plane[1])*theCorners[ptIdx][1] +
                         ((double)plane[2])*theCorners[ptIdx][2])) + (double)plane[3]);
         if(testValue >-FLT_EPSILON)
         {
            break;
         }
         else
         {
            ++outsideCount;
         }
      }
      if(outsideCount == 8)
      {
         return false;
      }
   }
   
   return true;
}

bool ossimPlanetBoundingBox::isInFront(const osg::Vec3d& eye,
                                       const osg::Vec3d& direction)const
{
   ossim_uint32 idx = 0;
   osg::Vec3d deltaP;
   for(idx = 0; idx < 8; ++idx)
   {
      deltaP = theCorners[idx]-eye;
      if((deltaP*direction) > -FLT_EPSILON)
      {
         return true;
      }
   }

   return false;
}
