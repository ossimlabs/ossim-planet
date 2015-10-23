#ifndef ossimPlanetLsrSpaceTransform_HEADER
#define ossimPlanetLsrSpaceTransform_HEADER
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetCallback.h>
#include <osg/MatrixTransform>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <OpenThreads/ScopedLock>

class ossimPlanetLsrSpaceTransform;
class OSSIMPLANET_DLL ossimPlanetLsrSpaceTransformCallback : public ossimPlanetCallback
   {
   public:
      virtual void lsrSpaceChanged(ossimPlanetLsrSpaceTransform* lsrSpace)=0;
   };

/**
 * ossimPlanetLsrSpaceTransform allows one to talk to osg::Transform in the form of Euler angles and
 * lat lon altitude where altitude is in meters.  The Lsr stands for Local Space Reference.  This manages
 * a local space axis at te given <lat,lon,altitude> point.
 */
class OSSIMPLANET_DLL ossimPlanetLsrSpaceTransform : public osg::Transform,
                                     public ossimPlanetCallbackListInterface<ossimPlanetLsrSpaceTransformCallback>
{
public:
   /**
    * Constructs a LsrSpace with a given geo ref model and orientation type.
    * @param model The Geo Reference Model to use.
    */
   ossimPlanetLsrSpaceTransform(ossimPlanetGeoRefModel* model=0);
   
   /**
    * Copy contructor.
    */ 
   ossimPlanetLsrSpaceTransform(const ossimPlanetLsrSpaceTransform& lsrTransform,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
   
   /**
    * Allows one to change the geo reference model.  Will apply the new model to the coordinates
    * and dirty the bound.
    * 
    * @param model  The model to use to generates the lsrMatrix.
    */
   void setModel(ossimPlanetGeoRefModel* model);
   
   
   /**
    * Will copy the parameters only form one transform to the next.
    *
    * @param src This is the source to copy from.
    */
   void copyParametersOnly(const ossimPlanetLsrSpaceTransform& src)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      dirtyBound();
      if(!theModel.valid())
      {
         theModel = src.theModel;
      }
      setHeadingPitchRoll(src.headingPitchRoll());
      setLatLonAltitude(src.latLonAltitude());
      setScale(src.scale());
      theOrientationMode = src.theOrientationMode;
   }
   /**
    * @return the pointer to the active model used by the transform.
    */
   const ossimPlanetGeoRefModel* model()const
   {      
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theModel.get();
   }
   
   /**
    * @return the pointer to the active model used by the transform.
    */
   ossimPlanetGeoRefModel* model()
   {      
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theModel.get();
   }
   
   /**
    * This is the local to world  transform matrix.  It will extract out the translation, scale
    * and orientation from the past in matrix and set the Lsr parameters.
    *
    * @param m The matrix to copy and extract the Lsr space parameters.
    */
   void setMatrix(const osg::Matrix& m);
   
   /**
    * @return the latitude position in degrees.
    */
   double lat()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theLatLonAltitude[0];
   }
   
   /**
    * @return the longitutde position in degrees.
    */
   double lon()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theLatLonAltitude[1];
   }
   
   /**
    * @return the altitdude in meters.
    */
   double altitude()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theLatLonAltitude[2];
   }
   
   /**
    * @return latLonAltitude as a vector.
    */
   const osg::Vec3d& latLonAltitude()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theLatLonAltitude;
   }
   
   /**
    * @return the model X coordinate of the model that was fromed from the inverse
    *  of the lat lon altitude.
    */
   double x()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theXYZ[0];
   }
   
   /**
    * @return the model Y coordinate of the model that was fromed from the inverse
    *  of the lat lon altitude.
    */
   double y()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theXYZ[1];
   }
   
   /**
    * @return the model Z coordinate of the model that was fromed from the inverse
    *  of the lat lon altitude.
    */   
   double z()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theXYZ[2];
   }
   
   /**
    * @return the model XYZ coordinate of the model that was fromed from the inverse
    *  of the lat lon altitude.
    */   
   const osg::Vec3d& xyz()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theXYZ;
   }
   
   /**
    * @return the heading orientation in degrees. indicates the rotation about the
    * Nadir axis
    */
   double heading()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theHpr[0];
   }
   
   /**
    * @return the pitch orientation in degrees.  Indicates the rotation about the
    * x-axis.
    */
   double pitch()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theHpr[1];
   }
   
   /**
    * @return the roll orientation in degrees. Indicates the rotation about the
    * y-axis.
    */
   double roll()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theHpr[2];
   }
   
   /**
    * @return all values for heading pitch and roll.
    */
   const osg::Vec3d& headingPitchRoll()const{return theHpr;}
   
   /**
    * @return the x scale factor.
    */
   double scalex()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theScale[0];
   }
   
   /**
    * @return the y scale factor
    */
   double scaley()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theScale[1];
   }
   
   /**
    * @return the z-scale factor.
    */
   double scalez()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theScale[2];
   }
   
   /**
    * @return the scale for x,y,z as a vector
    */
   const osg::Vec3d& scale()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theScale;
   }
   
   /**
    * @param hpr contains the heading pitch and roll in that order for the orientation.
    */
   void setHeadingPitchRoll(const osg::Vec3d& hpr);
   
   /**
    * @param value contains the position lat lon and altitude where lat and lon
    *               are in degrees and altitude is in meters relative to the ellipsoid.
    */
   void setLatLonAltitude(const osg::Vec3d& value);
   
   
   /**
    * @param value contains the position lat lon altitude where lat lon are in degrees
    *              and altitude is in meters relative to the curren geoid contained in the
    *              ossimPlanetGeoRefModel.
    */
   void setLatLonAltitudeMeanSeaLevel(const osg::Vec3d& value);
   
   /**
    * @param value contains the values for the x-scale, y-scale, z-scale
    *              components.
    */
   void setScale(const osg::Vec3d& value);
   
   /**
    * @param xyz is the x,y,z values resulting from the conversion of lat,lon,altitude using the
    *            ossimPlanetGeoRefModel.  
    */
   void setXYZ(const osg::Vec3d& xyz);
   
   /**
    * Override the base class Transform localToWorldMatrix.
    */
   virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const;
   
   /**
    * Override the base class world to local transforms
    */
   virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const;
   
   /**
    * Override the traverse to make sure that the model pointer is set and if the redraw flag is
    * set then pass it up.
    */
   virtual void traverse(osg::NodeVisitor& nv);

protected:
   /**
    * Will take the parameters such as lat,lon,altitude, heading, pitch, and roll, and scales
    * and convert to it's localToWorld matrix form.
    *
    * @return true if successfull and flase otherwise.
    */
   bool parametersToMatrix();
   
   /**
    * Will take the matrix and extract out the parameters such as lat, lon, altitude, and
    * heading pitch and roll and scales.
    *
    * param inputM the input matrix to decompose the parameters from.  It will decompose to 
    * a local space reference axis.
    */
   void matrixToParameters(const osg::Matrix& inputM);
   
   /**
    * Will call the callbacks in the callback list and notify the listeners that 
    * the Lsr space transform changed.
    */
   void notifyLsrSpaceChanged();
   
   /**
    * thePropertyMutex is used to synchronize access to any property value.
    */ 
   mutable ossimPlanetReentrantMutex thePropertyMutex;
   
   /**
    * theOrientationMode specifies how to construct the matrix
    * transform.  This can be in OSSIM_LEFT_HANDED or OSSIM_RIGHT_HANDED.
    */
   ossimCoordSysOrientMode theOrientationMode;
   
   /**
    *  Convenient pointer to hold direct access to the current GeoRefModel.
    */
   osg::ref_ptr<ossimPlanetGeoRefModel> theModel;
   
   /**
    * The position in lat lon altitude where lat and lon are degrees and Altitude is
    * meters.
    */
   osg::Vec3d theLatLonAltitude;
   
   /**
    * theXYZ holds the ossimlanetGeoRefModel result of going form lat, lon, altitude to the current
    * x,y,z representation.
    */
   osg::Vec3d theXYZ;
   
   /**
    * theHpr holds the orientation parameters for heading, pitch and roll in degrees.
    */
   osg::Vec3d theHpr;
   
   /**
    * theScale holds the x,y,z, scale factors
    */
   osg::Vec3d theScale;
   
   /**
    * Convenient values for holding the theLocalToWorld transform.  
    */
   mutable osg::Matrixd theLocalToWorld;
   
   /**
    * Convenient values for holding the inverse of theLocalToWorld transform.  
    */
   mutable osg::Matrixd theInvLocalToWorld;
   
   /**
    * theRedrawFlag holds the value to specify if redraw is needed.  Will be comunicated to
    * the parent if needed.
    */
   bool theRedrawFlag;
};

#endif
