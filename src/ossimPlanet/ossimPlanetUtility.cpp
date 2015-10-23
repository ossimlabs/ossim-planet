#include "ossimPlanet/ossimPlanetUtility.h"
#include <math.h>

void ossimPlanetUtility::ellipsoidToXYZ( const osg::EllipsoidModel& model,
                                       double latitude, double longitude, double height,
                                       double &x, double &y, double &z)
{
   model.convertLatLongHeightToXYZ(latitude,
                                   longitude,
                                   height,
                                   x,
                                   y,
                                   z);
   x/=model.getRadiusEquator();
   y/=model.getRadiusEquator();
   z/=model.getRadiusEquator();
}

osg::Vec3d ossimPlanetUtility::normal(const osg::EllipsoidModel& model,
                                    double x, double y, double z)
{
   double bSquared = model.getRadiusPolar()/model.getRadiusEquator();
   bSquared *= bSquared;
   return osg::Vec3d((2.0*x),
                     (2.0*y),
                     (2.0*z)/bSquared);
}

void ossimPlanetUtility::XYZToEllipsoid(const osg::EllipsoidModel& model,
                                      double x, double y, double z,
                                      double& latitude, double& longitude, double& height)
{
   x*=model.getRadiusEquator();
   y*=model.getRadiusEquator();
   z*=model.getRadiusEquator();
   model.convertXYZToLatLongHeight(x,
                                   y,
                                   z,
                                   latitude,
                                   longitude,
                                   height);
}

bool ossimPlanetUtility::intersectsEllipsoid(const osg::EllipsoidModel& model,
                                           osg::Vec3d& intersection,
                                           const osg::Vec3d& start,
                                           const osg::Vec3d& end)
{
   osg::Vec3d startPoint = start;//*model.getRadiusEquator();
   osg::Vec3d endPoint   = end;//*model.getRadiusEquator();
   /*
    *Compute ellipsoid quadratics:
    */
//   double A = model.getRadiusEquator();
//   double B = model.getRadiusPolar();
     // double A = 1.0;
    double B = model.getRadiusPolar()/model.getRadiusEquator();
//   double A_squared = A*A;
   double A_squared = 1.0;
   double B_squared = (B*B);
   osg::Vec3d direction = endPoint-startPoint;
   direction.normalize();
//   startPoint = startPoint + direction*.00001;
   
   //***
   // get the origin and direction of ray:
   //***

   //***
   // Solve the coefficents of the quadratic formula
   //***
   double a = ((direction[0] * direction[0])/A_squared) +
              ((direction[1] * direction[1])/A_squared) +
              ((direction[2] * direction[2])/B_squared);

   double b = 2.0*( ((startPoint.x()*direction.x())/A_squared) +
                    ((startPoint.y()*direction.y())/A_squared) +
                    ((startPoint.z()*direction.z())/B_squared) );

   double c = ((startPoint.x()*startPoint.x())/A_squared) +
              ((startPoint.y()*startPoint.y())/A_squared) +
              ((startPoint.z()*startPoint.z())/B_squared) - 1.0;
   
   //***
   // solve the quadratic
   //***
   double root = b*b - 4*a*c;
   double t;
   if(root < 0.0)
   {
      return false;
   }
   else
   {
      double squareRoot = sqrt(root);
      double t1 = (-b + squareRoot ) / (2.0*a);
      double t2 = (-b - squareRoot ) / (2.0*a);

      //***
      // sort t1 and t2 and take the nearest intersection if they
      // are in front of the ray.
      //***
      if(t2 < t1)
      {
         double temp = t1;
         t1 = t2;
         t2 = temp;
      }     

       if(t1 > 0.0)
          t = t1;
       else
          t = t2;
   }

   //***
   // Now apply solved t to ray to extrapolate correct distance to intersection
   //***
   bool rtnval = false;
   if (t >= 0)
   {
      rtnval = true;
      intersection  = start + direction*t;
//      intersection = intersection*(1.0/model.getRadiusEquator());
   }
      
   return rtnval; 
   
}
