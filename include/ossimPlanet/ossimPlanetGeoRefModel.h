#ifndef ossimPlanetGeoRefModel_HEADER
#define ossimPlanetGeoRefModel_HEADER
#include <osg/Referenced>
#include <osg/Vec3d>
#include <osg/CoordinateSystemNode>
#include <ossim/base/ossimLsrSpace.h>
#include <ossim/base/ossimMatrix4x4.h>
#include <ossim/base/ossimEllipsoid.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossim/base/ossimGeoid.h>
#include <ossim/base/ossimEcefPoint.h>
#include <ossim/base/ossimEcefVector.h>
#include <ossim/base/ossimGeoidManager.h>
#include <ossim/elevation/ossimElevManager.h>

class OSSIMPLANET_DLL ossimPlanetGeoRefModel : public osg::Referenced
{
public:
   ossimPlanetGeoRefModel()
      :theGeoid(ossimGeoidManager::instance()->findGeoidByShortName("geoid1996", false))
   {
   }
   virtual void latLonHeightToXyz(const osg::Vec3d& input,
                                  osg::Vec3d& output)const=0;
   virtual void xyzToLatLonHeight(const osg::Vec3d& input,
                                  osg::Vec3d& output)const=0;
   virtual void latLonHeightMslToXyz(const osg::Vec3d& input,
                                  osg::Vec3d& output)
   {
      osg::Vec3d llh(input);
      mslToEllipsoidal(llh);
      latLonHeightToXyz(llh, output);
   }
   virtual void xyzToLatLonHeightMsl(const osg::Vec3d& input,
                                     osg::Vec3d& output)
   {
      xyzToLatLonHeight(input, output);
      ellipsoidalToMsl(output);
   }
   virtual void forward(const osg::Vec3d& input,
                        osg::Vec3d& output)const
   {
      latLonHeightToXyz(input, output);
   }
   virtual void inverse(const osg::Vec3d& input,
                        osg::Vec3d& output)const
   {
      xyzToLatLonHeight(input, output);
   }
   virtual void normal(const osg::Vec3d& input,
                       osg::Vec3d& output)const
   {
      output = input;
      output.normalize();
   }

   /**
    * This will take a lat lon height and shift the height to the ellipsoid
    */ 
   virtual void mslToEllipsoidal(osg::Vec3d& llh)
   {
      llh[2] += getGeoidOffset(llh[0],llh[1]);
   }
   
   /**
    * This will take a lat lon height and shift the height to the Mean Sea Level.
    * This assumes the height you passed in was Ellipsoidal
    */ 
   virtual void ellipsoidalToMsl(osg::Vec3d& llh)
   {
      llh[2] -= getGeoidOffset(llh[0],llh[1]);
   }
   virtual double getHeightAboveMsl(const double& lat, const double& lon)
   {
      double result = ossimElevManager::instance()->getHeightAboveMSL(ossimGpt(lat, lon));
      if(ossim::isnan(result))
      {
         result = 0.0;
      }
      return result;
   }
   virtual double getGeoidOffset(const double& lat, const double& lon)
   {
      double result = 0.0;
      if(theGeoid.valid())
      {
         result = theGeoid->offsetFromEllipsoid(ossimGpt(lat,lon));
         if(ossim::isnan(result))
         {
            result = 0.0;
         }
      }
      return result;
   }
   virtual double getHeightAboveEllipsoid(const double& lat, const double& lon)
   {
      double result =  ossimElevManager::instance()->getHeightAboveEllipsoid(ossimGpt(lat, lon));
      if(ossim::isnan(result))
      {
         if(theGeoid.valid())
         {
            result = theGeoid->offsetFromEllipsoid(ossimGpt(lat,lon));
            if(ossim::isnan(result))
            {
               result = 0.0;
            }
         }
         else
         {
            result = 0.0;
         }
      }

      return result;
   }
   virtual void lsrMatrix(const osg::Vec3d& /*latLonHeight*/,
                          osg::Matrixd& output,
                          double /*heading*/=0.0,
                          bool /*rotationOnlyFlag*/=false)const
   {
      output = osg::Matrixd();
   }
   virtual void orientationLsrMatrix(osg::Matrixd& result,
                                     const osg::Vec3d& llh,
                                     double h, double p, double r)const
   {
      lsrMatrix(llh, result, 0.0);
      
      NEWMAT::Matrix orien = ossimMatrix4x4::createRotationZMatrix(h, OSSIM_RIGHT_HANDED)*
      ossimMatrix4x4::createRotationXMatrix(p, OSSIM_LEFT_HANDED)*
      ossimMatrix4x4::createRotationYMatrix(r, OSSIM_LEFT_HANDED);
      osg::Matrixd tempM(orien[0][0], orien[1][0], orien[2][0], 0.0,
                         orien[0][1], orien[1][1], orien[2][1], 0.0,
                         orien[0][2], orien[1][2], orien[2][2], 0.0,
                         0.0, 0.0, 0.0, 1.0);
      
      result = tempM*result;
   }
/*    virtual void computeLocalToWorldTransform(const osg::Vec3d& input, */
/*                                              osg::Matrixd& output)const=0; */
   virtual double calculateUnnormalizedLengthXyz(const osg::Vec3d& p1,
                                                 const osg::Vec3d& p2)const
   {
      return calculateUnnormalizedLength((p1-p2).length());
   }
   virtual double calculateUnnormalizedLength(double delta)const
   {
      return getNormalizationScale()*delta;
   }
   virtual double getNormalizationScale()const
   {
      return 1.0;
   }
   virtual double getInvNormalizationScale()const
   {
      return 1.0;
   }
   const ossimRefPtr<ossimGeoid> geoid()const
   {
      return theGeoid;
   }
   void setGeoid(ossimRefPtr<ossimGeoid> geoid)
   {
      theGeoid = geoid.get();
   }
protected:
   ossimRefPtr<ossimGeoid> theGeoid;
   
};


class ossimPlanetEllipsoidModel : public ossimPlanetGeoRefModel
{
public:
   ossimPlanetEllipsoidModel(double radiusEquator = osg::WGS_84_RADIUS_EQUATOR,
                             double radiusPolar   = osg::WGS_84_RADIUS_POLAR)
      :theModel(radiusEquator, radiusPolar)
   {
   }


   virtual void lsrMatrix(const osg::Vec3d& latLonHeight,
                          osg::Matrixd& output,
                          double heading=0.0,
                          bool rotationOnlyFlag=false)const
   {
      osg::Vec3d xyz;
      ossimLsrSpace lsrSpace;
      lsrSpace = ossimLsrSpace(ossimGpt(latLonHeight[0],
                                        latLonHeight[1],
                                        latLonHeight[2]),
                               heading);//,
      if(!rotationOnlyFlag)
      {
         latLonHeightToXyz(latLonHeight, xyz);
      }
      ossimMatrix4x4 lsrMatrix(lsrSpace.lsrToEcefRotMatrix());
      NEWMAT::Matrix compositeMatrix = lsrMatrix.getData();
/*       compositeMatrix = compositeMatrix.t(); */
      output = osg::Matrixd(compositeMatrix[0][0], compositeMatrix[1][0], compositeMatrix[2][0], 0.0,
                            compositeMatrix[0][1], compositeMatrix[1][1], compositeMatrix[2][1], 0.0,
                            compositeMatrix[0][2], compositeMatrix[1][2], compositeMatrix[2][2], 0.0,
                            xyz[0], xyz[1], xyz[2], 1.0);
   }
   virtual void latLonHeightToXyz(const osg::Vec3d& input,
                                  osg::Vec3d& output)const
   {
      theModel.latLonHeightToXYZ(input[0],
                                 input[1],
                                 input[2],
                                 output[0],
                                 output[1],
                                 output[2]);
   }
   virtual void xyzToLatLonHeight(const osg::Vec3d& input,
                        osg::Vec3d& output)const
   {
      theModel.XYZToLatLonHeight(input[0],
                                 input[1],
                                 input[2],
                                 output[0],
                                 output[1],
                                 output[2]);
   }
   virtual void normal(const osg::Vec3d& input,
                       osg::Vec3d& output)const
   {
      ossimEcefPoint location(input[0], input[1], input[2]);
      ossimEcefVector gradient;
      theModel.gradient(location, gradient);
      output[0] = gradient.x();
      output[1] = gradient.y();
      output[2] = gradient.z();
   }
protected:
   ossimEllipsoid theModel;
/*    osg::ref_ptr<osg::EllipsoidModel> theModel; */
};

class ossimPlanetNormalizedEllipsoidModel : public ossimPlanetEllipsoidModel
{
public:
   ossimPlanetNormalizedEllipsoidModel(double radiusEquator = osg::WGS_84_RADIUS_EQUATOR,
                                           double radiusPolar   = osg::WGS_84_RADIUS_POLAR)
      :ossimPlanetEllipsoidModel(radiusEquator, radiusPolar)
   {
      theNormalizationScale    = osg::WGS_84_RADIUS_EQUATOR;
      theInvNormalizationScale = 1.0/osg::WGS_84_RADIUS_EQUATOR;
   }
      
   virtual void latLonHeightToXyz(const osg::Vec3d& input,
                                  osg::Vec3d& output)const
   {
      theModel.latLonHeightToXYZ(input[0],
                                 input[1],
                                 input[2],
                                 output[0],
                                 output[1],
                                 output[2]);
      output[0]*=theInvNormalizationScale;
      output[1]*=theInvNormalizationScale;
      output[2]*=theInvNormalizationScale;
      
   }
   virtual void xyzToLatLonHeight(const osg::Vec3d& input,
                        osg::Vec3d& output)const
   {
      
      theModel.XYZToLatLonHeight(input[0]*theNormalizationScale,
                                 input[1]*theNormalizationScale,
                                 input[2]*theNormalizationScale,
                                 output[0],
                                 output[1],
                                 output[2]);
   }
   virtual void lsrMatrix(const osg::Vec3d& latLonHeight,
                          osg::Matrixd& output,
                          bool rotationOnlyFlag=false)const
   {
      osg::Vec3d xyz;
      ossimLsrSpace lsrSpace;
      lsrSpace = ossimLsrSpace(ossimGpt(latLonHeight[0],
                                        latLonHeight[1],
                                        latLonHeight[2]),
                               0.0);//,
      if(!rotationOnlyFlag)
      {// make sure that the x,y,z are normalized.
         latLonHeightToXyz(latLonHeight, xyz);
         xyz[0] *= theNormalizationScale;
         xyz[1] *= theNormalizationScale;
         xyz[2] *= theNormalizationScale;
      }
      ossimMatrix4x4 lsrMatrix(lsrSpace.lsrToEcefRotMatrix());
      NEWMAT::Matrix compositeMatrix = lsrMatrix.getData();
/*       compositeMatrix = compositeMatrix.t(); */
      output = osg::Matrixd(compositeMatrix[0][0], compositeMatrix[1][0], compositeMatrix[2][0], 0.0,
                            compositeMatrix[0][1], compositeMatrix[1][1], compositeMatrix[2][1], 0.0,
                            compositeMatrix[0][2], compositeMatrix[1][2], compositeMatrix[2][2], 0.0,
                            xyz[0], xyz[1], xyz[2], 1.0);
   }
   virtual double getNormalizationScale()const
   {
      return theNormalizationScale;
   }
   virtual double getInvNormalizationScale()const
   {
      return theInvNormalizationScale;
   }

protected:
   double theNormalizationScale;
   double theInvNormalizationScale;
};

#if 0
class ossimPlanetFlatLandModel : public ossimPlanetLandModel
{
public:
   ossimPlanetFlatLandModel(){}
   virtual void latLonHeightToXyz(const osg::Vec3d& input,
                        osg::Vec3d& output)const
   {
      output[0] = input[1];
      output[1] = input[0];
      output[2] = input[2];
   }
   virtual void computeLocalToWorldTransform(const osg::Vec3d& /* input */,
                                             osg::Matrixd& /* output */)const
   {
      // left blank for now so the matrix will not change
   }
   virtual void xyzToLatLonHeight(const osg::Vec3d& input,
                                  osg::Vec3d& output)const
   {
      output[0] = input[1];
      output[1] = input[0];
      output[2] = input[2];
   }
   virtual void normal(const osg::Vec3d& input,
                       osg::Vec3d& output)const
   {
      if(theNormalModel.valid())
      {
         osg::Vec3d tempVec;
         osg::Vec3d tempVec2;
         xyzToLatLonHeight(input, tempVec);
         theNormalModel->forward(tempVec, tempVec2);
         theNormalModel->normal(tempVec2,
                                output);
         output[0] *=-1.0;
         output[1] *=-1.0;
         output[2] *=-1.0;
      }
      else
      {
         output[0] = 0.0;
         output[1] = 0.0;
         output[2] = -1.0;
      }
   }
   virtual void changeNormalModel(osg::ref_ptr<ossimPlanetLandModel> normalModel)
   {
      theNormalModel = normalModel;
   }
   
   osg::ref_ptr<ossimPlanetLandModel> theNormalModel;
};
#endif

#endif
