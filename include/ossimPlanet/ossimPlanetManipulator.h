/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2003 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
 */

#ifndef OSSIM_PLANETMANIPULATOR
#define OSSIM_PLANETMANIPULATOR 1

//#include <osgGA/MatrixManipulator>
#include <osgGA/CameraManipulator>
#include <osg/Quat>
#include <ossim/base/ossimGpt.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetNavigator.h>
#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <ossimPlanet/ossimPlanetViewMatrixBuilder.h>
#include <osg/observer_ptr>
#include <osg/Node>

class ossimPlanet;

//class OSSIMPLANET_DLL ossimPlanetManipulator : public osgGA::MatrixManipulator,
class OSSIMPLANET_DLL ossimPlanetManipulator : public osgGA::CameraManipulator,
public ossimPlanetActionReceiver
{
public:
   ossimPlanetManipulator();

   // osgGA::MatrixManipulator methods

   virtual const char* className() const { return "ossimPlanetManipulator"; }

   virtual void setByMatrix(const osg::Matrixd& matrix);
   /** set the position of the matrix manipulator using a 4x4 Matrix.*/

   virtual void setByInverseMatrix(const osg::Matrixd& matrix) { setByMatrix(osg::Matrixd::inverse(matrix)); }
   /** set the position of the matrix manipulator using a 4x4 Matrix.*/

   virtual osg::Matrixd getMatrix() const;
   /** get the position of the manipulator as 4x4 Matrix.*/

   virtual osg::Matrixd getInverseMatrix() const;
   /** get the position of the manipulator as a inverse matrix of the manipulator, typically used as a model view matrix.*/

   virtual void setNode(osg::Node*);
   /** Get the FusionDistanceMode. Used by SceneView for setting up stereo convergence.*/
   virtual osgUtil::SceneView::FusionDistanceMode getFusionDistanceMode() const { return osgUtil::SceneView::USE_FUSION_DISTANCE_VALUE; }

   /** Get the FusionDistanceValue. Used by SceneView for setting up stereo convergence.*/
   virtual float getFusionDistanceValue() const { return theFusionDistance; }

   /** Attach a node to the manipulator.
    Automatically detaches previously attached node.
    setNode(NULL) detaches previously nodes.
    Is ignored by manipulators which do not require a reference model.*/

   virtual const osg::Node* getNode() const;
   /** Return node if attached.*/

   virtual osg::Node* getNode();
   /** Return node if attached.*/

   virtual void home(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& us);
   /** Move the camera to the default position.
    May be ignored by manipulators if home functionality is not appropriate.*/

   virtual void init(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& us);
   /** Start/restart the manipulator.*/

   virtual bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& us);
   /** handle events, return true if handled, false otherwise.*/

   virtual void getUsage(osg::ApplicationUsage& usage) const;
   /** Get the keyboard and mouse usage of this manipulator.*/


   virtual void setLockToNode(osg::Node* node);
   // new methods

   virtual void getLatLonHgtHPR(double& lat, double& lon, double& hgt, double& heading, double& pitch, double& roll)const;

   virtual void solveLookAt(double losLat, double losLon, double losHeight, double& heading, double& pitch, double& roll, double& range)const;

   void playRecording();
   void startRecording();
   void stopRecording();
   void saveRecording(std::ostream& out);
   bool loadRecording(std::istream& in);

	void extractLookFromParameters(ossimXmlNode* node,
                                 double& lat,
                                 double& lon,
                                 double& alt,
                                 double& h,
                                 double& p,
                                 double& r);



   void setEventHandlingFlag(bool flag);
   void setUseFrameEventForUpdateFlag(bool flag);
   void setAutoCalculateIntersectionFlag(bool flag);
   void setLosXYZ(const osg::Vec3d& losXYZ);

   void updateNavigator();
   // call theNavigator->update()
   // Cocoa version temporarily needs this for now
   ossimPlanetNavigator* navigator()
   {
      return theNavigator.get();
   }

   /**
    * Will set the latitude for the view matrix.
    *
    * @param value is the latitude position of the view in degrees.
    */
   void setLatitude(double value);

   /**
    * Will set the longitude for the view matrix.
    *
    * @param value is the longitude position of the view in degrees.
    */
   void setLongitude(double value);

   /**
    * Will set the altitude for the view matrix.
    *
    * @param value is the altitude position of the view in meters.
    *              Currently this is absolute position on the ellipsoid.
    */
   void setAltitude(double value);


   /**
    * Will allow you to set the latitude, longitude and altitude in a single operation
    *
    * @param lat is the latitude position of the view in degrees.
    * @param lon is the longitude position of the view in degrees.
    * @param alt is the altitude position of the view in meters.
    *                 Currently this is absolute position on the ellipsoid.
    */
   void setLatitudeLongitudeAltitude(double lat, double lon, double alt);

   /**
    * This is a convenience method that allows one to put the positional information
    * in a single osg::Vec3d data type.
    *
    * @param pos is the is the latitude, longitude, altitude where the latitude is in
    *            pos[0] and longitude is in pos[1] and altitude is in pos[2].  the lat and lon
    *            values are in degrees where the altitude is in meters.
    */
   void setPosition(const osg::Vec3d& pos);

   /**
    * sets the heading orientation for the view.  Note this indicates a rotation about the
    * Z-axis.
    *
    * @param value indicates the heading in degrees to rotate around the Z axis.
    */
   void setHeading(double value);

   /**
    * Sets the pitch orientation for the view.  Note this indicates a rotation about the
    * X-axis.
    *
    * @param value indicates the pitch in degrees to rotate around the X-axis.
    */
   void setPitch(double value);

   /**
    * Set the roll orietation for the view.  Note this indicates a rotation about the
    * Y-axis.
    */
   void setRoll(double value);

   /**
    * sets the heading pitch and rool as a single operation.
    *
    * @param h indicates the heading in degrees to rotate around the Z axis.
    * @param p indicates the pitch in degrees to rotate around the X axis.
    * @param r indicates the roll in degrees to rotate around the y axis.
    */
   void setHeadingPitchRoll(double h, double p, double r);

   /**
    * This is a convenience method that allows one to put the positional information
    * in a single osg::Vec3d data type.
    *
    * @param orien is the heading, pitch, and roll orientation of the view in degrees.
    *              the heading is in orien[0] and pitch is in orien[1] and the roll is in
    *              orien[2].
    */
    void setOrientation(const osg::Vec3d& orien);

   /**
    * Allows one to set the position and orientation as a single operation.
    *
    * @param lat is the latitude position of the view in degrees.
    * @param lon is the longitude position of the view in degrees.
    * @param alt is the altitude position of the view in meters.
    *                 Currently this is absolute position on the ellipsoid.
    * @param h indicates the heading in degrees to rotate around the Z axis.
    * @param p indicates the pitch in degrees to rotate around the X axis.
    * @param r indicates the roll in degrees to rotate around the y axis.
    *
    */
   void setPositionAndOrientation(double lat, double lon, double altitude,
                                  double h, double p, double r);

   /**
    * This is an added convenience method to take 2 3-d vectors as position
    * and orientation
    *
    * @param pos is the is the latitude, longitude, altitude where the latitude is in
    *            pos[0] and longitude is in pos[1] and altitude is in pos[2].  the lat and lon
    *            values are in degrees where the altitude is in meters.
    * @param orien is the heading, pitch, and roll orientation of the view in degrees.
    *              the heading is in orien[0] and pitch is in orien[1] and the roll is in
    *              orien[2].
    *
    */
   void setPositionAndOrientation(const osg::Vec3d& pos,
                                  const osg::Vec3d& orien);

   ossimPlanetViewMatrixBuilder* viewMatrixBuilder(){return theViewMatrixBuilder.get();}
   const ossimPlanetViewMatrixBuilder* viewMatrixBuilder()const{return theViewMatrixBuilder.get();}
   virtual void execute(const ossimPlanetAction& a);

   void initializeDefaultBindings(const ossimString& pathName=":navigator");
   
protected:

   class FromNodeCallback : public ossimPlanetLsrSpaceTransformCallback
      {
      public:
         FromNodeCallback(ossimPlanetViewMatrixBuilder* vb)
         :theViewMatrixBuilder(vb)
         {
         }
         virtual void lsrSpaceChanged(ossimPlanetLsrSpaceTransform* /*lsrSpace*/)
         {
            if(theViewMatrixBuilder)
            {
               theViewMatrixBuilder->setLookFromNodeOffset(theViewMatrixBuilder->fromNode(),
                                                           theViewMatrixBuilder->fromRelativeHpr(),
                                                           theViewMatrixBuilder->fromRange(),
                                                           theViewMatrixBuilder->fromRelativeOrientationFlags());
            }
         }
         ossimPlanetViewMatrixBuilder* theViewMatrixBuilder;
      };

   class ToNodeCallback : public ossimPlanetLsrSpaceTransformCallback
      {
      public:
         ToNodeCallback(ossimPlanetViewMatrixBuilder* vb)
         :theViewMatrixBuilder(vb)
         {

         }
         virtual void lsrSpaceChanged(ossimPlanetLsrSpaceTransform* lsrSpace)
         {
            if(theViewMatrixBuilder)
            {
               theViewMatrixBuilder->setLookToNode(lsrSpace);
            }
         }
         ossimPlanetViewMatrixBuilder* theViewMatrixBuilder;
      };

   virtual ~ossimPlanetManipulator();
   virtual void updateViewMatrixNodes();

   bool calculateLineOfSiteLatLonHeight(osg::Vec3d& latLonHeight);

   osg::Vec3d eyePosition()const;

   osg::ref_ptr<ossimPlanetNavigator> theNavigator;
   mutable ossimPlanetReentrantMutex theMutex;
   osg::ref_ptr<osg::Node> theNode;
   bool theEventHandlingFlag;
   bool theUseFrameEventForUpdateFlag;
   bool theAutoCalculateIntersectionFlag;
   ossimPlanet* thePlanet;
   ossim_float32 theFusionDistance;

   osg::ref_ptr<ossimPlanetViewMatrixBuilder> theViewMatrixBuilder;

   osg::ref_ptr<osg::Node> theLockToNode;
   osg::ref_ptr<osg::Node> theLockFromNode;

   osg::ref_ptr<FromNodeCallback> theFromNodeCallback;
   osg::ref_ptr<ToNodeCallback> theToNodeCallback;
};


#endif

