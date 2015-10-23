#ifndef ossimPlanetUtility_HEADER
#define ossimPlanetUtility_HEADER
#include <osg/CoordinateSystemNode>
#include <osg/Vec3d>
#include "ossimPlanetExport.h"

class OSSIMPLANET_DLL ossimPlanetUtility
{
public:
   static double getMaxDistance()
      {
         return 1.0e10;
      }
   static osg::Vec3d normal(const osg::EllipsoidModel& model,
                            double x, double y, double z);
   static void ellipsoidToXYZ( const osg::EllipsoidModel& model,
                               double latitude, double longitude, double height,
                               double &x, double &y, double &z);
   static void XYZToEllipsoid( const osg::EllipsoidModel& model,
                               double x, double y, double z,
                               double& latitude, double& longitude, double& height);

   static bool intersectsEllipsoid(const osg::EllipsoidModel& model,
                                   osg::Vec3d& intersection,
                                   const osg::Vec3d& start,
                                   const osg::Vec3d& end);
};

#endif
