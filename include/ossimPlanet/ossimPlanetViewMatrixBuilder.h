#ifndef ossimPlanetViewMatrixBuilder_HEADER
#define ossimPlanetViewMatrixBuilder_HEADER
#include <osg/Matrix>
#include <osg/Node>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetCallback.h>
#include <ossimPlanet/ossimPlanetPointModel.h>
#include <ossimPlanet/ossimPlanetLsrSpaceTransform.h>
#include <osg/ref_ptr>

/**
 * The view matrix builder serves several purposes. It can be used to allow one to do free form manipulation of the scene
 * based on position and euler heading pitch roll orientation.  Also, one can use the class to "track" another node whether
 * you are a fixed point looking at a moving object or a moving object looking at another moving object or a moving object
 * looking at a fixed point or looking from a fixed point to a fixed point.  To support all these scenarios we have decomposed
 * the stages internally to build the matrix into self contained paramters where each can be adjusted by a maniopulator.
 * Tracking in the true since of the word is really done by the ossimPlanetManipulator which owns a ViewMatrixBuilder and listens
 * for changes in nodes that either we are looking from or looking to and adjusts the paramters of the view matrix accordingly.
 *
 *
 * Each parameter updates the matrix at it's defined stage.  The multiplication or building up the final view matrix is done  
 * in the following steps:
 * <pre>
 *  Step 1 - Compute a Lsr Matrix with the current "from" Lat lon height position and a composited orientation
 *           created based on the "from" Heading Pitch and Roll and the Relative Heading Pitch and Roll and relative flags.
 *  Step 2 - Apply the From displacement to the current oriented axis.  The from displacement will shift the
 *           "from" eye position to a displacement relative to the position and orientation of the matrix from Step 1.  Each
 *           X, Y, Z value in the displacement vector is multiplied by the axis of the rotational matrix and added to 
 *           the Step 1's origin defined by the from Lat lon height in world  X, Y, Z space.
 *  Step 3 - Calculate a look vector at this point based on the Look vector alignment.  The default alignment is
 *           to position the look direction along the positive Y-Axis.  We will use this vector for other calculations
 *           in later steps.
 *  Step 4 - Displacement along the look axis calculated in Step 3 a "from range" distance.
 *  Step 5 - If there is no "to" information set then we create a lookAt view matrix using OSG's make look at by using the 
 *           look vector calculated in Step 3.  If there is a look "to" information set then goto Step 8.
 *  Step 6 - If the "to" information is set then first redefine a look vector that looks along the axis to the point you
 *           are looking at.
 *  Step 7 - Now apply 2 displacments.  The "ToRange" is a convenient displacement that could be achieved by using just
 *           the z component of the to displacment. The "to range" is added to the Z component of the "to displacement".
 *           the displacement is then applied to the axis created by using the look vector calculated in Step 6 and the makeLookAt 
 *           utility supplied to us by OSG.  This will create an orientation axis and we display the current displaced eye point
 *           calculated in the from stage along the new axis.
 * Step 8 - Apply the final Attitude to allow one to look around.
 * </pre>
 *
 * How to use the view matrix builder.
 * 
 * - Free form manipulation where you are just wanting to move the eye around. the paramters typically used are the 
 *   "from Lat lon Height" and the from Heading pitch and roll.  The from Realtive flags should be set to ALL and the relative
 *   heading pitch roll and all other values should be zeroed out.  The to information should be invalidated and not set.
 * 
 * - fixed point. you can set the from information to a fixed location and allow yourself to spin around the fixed point
 *   by adjusting the from range and look axis and the from heading pitch and roll.  I would default the relative orientation to 0's 
 *   and the relative orientation flags to ALL.
 *
 * - fixed point to fixed point.  There are 2 ways to implement this type of situation.  The little bit harder one to setup
 *   is to use it all as a from point with a range displacement.  So you could orient the From HPR axis and do a negative
 *   range displacement along the look direction for the from orientation.  The easier to control and setup is to treat the point
 *   to point type implementation as a "from to" setting.  So we set the from position Lat Lon Height and the point you are looking
 *   at Lat Lon Height.  We then set the orientaiton information for the from axis to all 0 and relative to all.  This will use the 
 *   normal of the local tangent axis at the from position for the calculation of a "make look at".  This will also keep the horizon 
 *   level.
 *
 * - fixed point to a node.  Currently we only support orientation and positional lookups for nodes that are or contain a node of 
 *   type ossimPlanetPointModel or of type ossimPlanetLsrSpace.  We will later add some of OSG's position attitude models.  The 
 *   view matrix does not auto track but auto tracking is done outside the class (@see ossimPlanetManipulator). Since the node is
 *   the "to" node the orientation information is not used from the passed in node.  We only use the transform to get the global 
 *   position of the node in Lat Lon Height.  The from position is fixed and should use te set routines that do not take a from node.
 *   to keep the horizon pretty much level you can set the "from fixed point" orientation to all 0 for from HPR and from Relative HPR
 *   and make relative flags set to all relative.  This will use the normal to the tangent plane as the up axis used in the calculation 
 *   of the makeLookAt call.
 *
 * - node to fixed point. How the look to the fixed point is oriented will depend on the settings of the relative orientation flags.
 *   the from point is created from the nodes location and then the orientation is grabbed from the node.  We currently only support
 *   locating an ossimPlanetLsrSpace somewhere as a child of node and we also can detect ossimPlanetPointModel.  
 *   The orientation of the lsr space and the location of the point model is used to populate the from information.  
 *   The relative flags used will dtermine how the eye tilts when doing a lookat vector to the fixed point.  If you want it to 
 *   not allow the horizon to move but stay level then I would do a NOR ORIENTATION FLAG and the relative HPR values are actually
 *   used in the orientation and so giving them the value of all 0 should keep the horizon level.
 *
 * - node to node. Node to node is nothing different than the node to fixed point but in the ossimPlanetManipulator it just keeps
 *   tracking the "to" node.  The tilting that occurs depends on how you set up the from information orientation and relative 
 *   orientation and the relative orientation flags.
 */
class OSSIMPLANET_DLL ossimPlanetViewMatrixBuilder : public osg::Referenced

{
public:
   class OSSIMPLANET_DLL Visitor : public osg::NodeVisitor
   {
   public:
      Visitor();
      virtual void apply(osg::Node& node);
      virtual void reset()
      {
         thePointModel = 0;
         theLsrSpaceTransform = 0;
      }
      osg::ref_ptr<ossimPlanetPointModel> thePointModel;
      osg::ref_ptr<ossimPlanetLsrSpaceTransform> theLsrSpaceTransform;
   };
   /**
    * These are used as flags for defining relative orientation axis for passed in
    * heading, pitch, and roll.
    */
   enum OrientationFlags
   {
      NO_ORIENTATION = 0,
      HEADING        = 1,
      PITCH          = 2,
      ROLL           = 4,
      ALL_ORIENTATION = (HEADING|PITCH|ROLL)
   };

   /**
    * Look axis.  Once a defined orientation is created you have the option to define a LOOK axis.
    */
   enum LookAxis
   {
      LOOK_AXIS_X = 0,
      LOOK_AXIS_NEGATIVE_X,
      LOOK_AXIS_Y,
      LOOK_AXIS_NEGATIVE_Y,
      LOOK_AXIS_Z,
      LOOK_AXIS_NEGATIVE_Z
   };
   /**
    * Allow one to construct an empty geo ref model.  We need it to have this option
    * For some nodes in a graph that use this will not know about a model until it's added
    * to a graph.
    *
    * @param geoRefModel Pass in an optional geo ref model during construction time.  If not
    *                    set here you need to set it with the setGeoRefModel before calling
    *                    the viewMatrix and inverseViewMatrix methods.
    */
   ossimPlanetViewMatrixBuilder(ossimPlanetGeoRefModel* geoRefModel=0);

   ossimPlanetViewMatrixBuilder(const osg::Matrixd& m, ossimPlanetGeoRefModel* geoRefModel);
   /**
    * Copy constructor
    */
   ossimPlanetViewMatrixBuilder(const ossimPlanetViewMatrixBuilder& src);

   /**
    * clone method.  Will return an allocated copy of this object.
    */
   virtual ossimPlanetViewMatrixBuilder* clone(){return new ossimPlanetViewMatrixBuilder(*this);}

   /**
    * Standard destructor
    */
   virtual ~ossimPlanetViewMatrixBuilder();

   /**
    * Sets the GeoRefModel used for coordinate transformations.
    *
    * @param geoRefModel the current model used for coordinate transformations
    *
    */
   void setGeoRefModel(ossimPlanetGeoRefModel* geoRefModel);

   /**
    * Basic access method.
    *
    * @return the stored geo ref model
    */
   const ossimPlanetGeoRefModel* geoRefModel()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theModel.get();
   }
   ossimPlanetGeoRefModel* geoRefModel()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theModel.get();
   }
   
   /**
    * This method allows you to define a view that tracks a node.  You can eather sit
    * at the node's defined center of mass or displace a distance "range" along line of site and/or 3d vector displacement.
    * the relative orientation flag defines how the heading pitch roll argument is to be used
    * to calculate the final orientation.  if any flag is set for Heading, Pitch, and/or roll then
    * that orientation parameter is relative to that orientation defined in the axis of the node.  If
    * any flag is not set then it is relative to the East North Up axis.  So if you want a camera to
    * always say rotate with the heading of that node then you will just set the HEADING flag in
    * the relativeOrientationFlag.
    *
    * @param node  This is the node you wish this ViewMatrixBuilder to Lock to.
    * @param hpr   The passed in Heading Pitch and Roll to apply in angular degrees.
    * @param range The meter distance along the look axis.
    * @param relativeOrientationFlag Which orientation parameter(s) do you wish to be relative
    *                                to the Node's Axes.  A value of ALL_ORIENTATION and a hpr
    *                                parameter of all 0's will allow you to rotate with the object.
    */
   void setLookFromNodeOffset(osg::Node* node,
                              const osg::Vec3d& hpr,
                              double range,
                              int relativeOrientationFlag);

   /**
    * This allows for a look from displacement.  So for instance if you want to
    * position the camera truly in the cockpit and not the center of mass used for the local
    * space transform you can do so.  Currently this displacment will follow the local point
    * center of mass relative to the orientation axis of the plane.
    *
    * @param displacement This takes a local space displacement in meters and positions along
    *                     the orientation of the node or current relative axis the amount to
    *                     displace to the new center.
    */
   void setLookFromLocalDisplacement(const osg::Vec3d& displacement)
   {
      updateFromLocalDisplacement(displacement);
   }
   
   /**
    * This was added to keep a name consistency problem.
    */
   void updateFromLocalDisplacement(const osg::Vec3d& displacement);
   
   void updateFromPositionLlh(const osg::Vec3d& llh)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theFromNode        = 0;
      theFromPositionLLH = llh;
   }
   void updateFromRange(double range)
   {
     OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
     theFromRange = range;
     setComputeViewMatrixFlag(true);
   }
   void updateFromRelativeHpr(const osg::Vec3d& hpr)
   {
     OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
     theFromRelativeHpr = hpr;
     setComputeViewMatrixFlag(true);
   }
   void updateFromHpr(const osg::Vec3d& hpr)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theFromHpr = hpr;
      setComputeViewMatrixFlag(true);
   }
   void updateFromRelativeOrientationFlag(int relativeOrientationFlag)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theFromRelativeOrientationFlags = (OrientationFlags)relativeOrientationFlag;
      setComputeViewMatrixFlag(true);
   }
   void updateFromNode(osg::Node* node)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theFromNode = node;
      setComputeViewMatrixFlag(true);
   }
   /**
    * @return The current from node.
    */
   osg::Node* fromNode()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theFromNode.get();
   }
   const osg::Vec3d& fromLocalDisplacement()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theFromDisplacement;
   }
   
   /**
    * This is the starting from position in lat lon height form.  This does not
    * include the displacement information or range information.
    *
    * @return the from position in lat lon height.
    */
   const osg::Vec3d& fromPositionLlh()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theFromPositionLLH;
   }
   
   /**
    * This is for the relative heading pitch and roll setting done in the set
    * relative to a Node call.  Also,  the internal matrix builder will set
    * the relative positioning if setting using the fixed point method.
    *
    * @return the realtive Hpr setting.
    */
   const osg::Vec3d& fromRelativeHpr()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theFromRelativeHpr;
   }
   
   /**
    * @return the current setting for the relative orientation flags.
    */
   OrientationFlags fromRelativeOrientationFlags()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theFromRelativeOrientationFlags;
   }
   
   /**
    * This will give a composited from orientation based on the Relative flags.  Internally the source orientation could
    * be changing each frame and the rleative orientation offsets the source orientation vector based on the orientation flags.
    * this is a utility function to allow one to call this in a single call.
    *
    * @return the composited orientation vector for heading pitch and roll in degrees.
    */
   osg::Vec3d computeFromOrientation()const;
   
   /**
    * This is the from source orietation vector.
    *
    * @return the from Heading pitch ad roll as a vec3d.
    */
   const osg::Vec3d& fromHpr()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theFromHpr;
   }
   
   /**
    * @return the from range value in meters.
    */
   double fromRange()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theFromRange;
   }
   
   /**
    * @return The current from node as a constant value
    */
   const osg::Node* fromNode()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theFromNode.get();
   }
   
   /**
    * This method allow you to set fixed point to look from with a defined orientation and
    * a displacement along the look as defined by the "range" parameter.
    *
    * @param llh This is the <lattitude, longitude, height> where the lat and lon are expressed
    *            in angular degrees and the height is in meters.
    *
    * @param hpr This is the <Heading, Pitch, Roll> parameters for the orietation axis relative
    *            to the local tangent plane defined by the point llh.  The values are in
    *            angular degrees.
    *
    * @param range This indicates the displacement from the axis origin along the line of site or look
    *              vector.  Negative value displaces backwards from the look and positive displaces
    *              along the direction of the resulting look.
    */
   void setLookFrom(const osg::Vec3d& llh,
                    const osg::Vec3d& hpr,
                    double range);

   /**
    * If a look to node is specified some of the look from parameters are overriden since the orientation
    * is dictated by the resulting look vector to the "look to" node that is passed in here.
    *
    * @param node The node you wish to look to.  It will take the node passed in and use it's
    *             center point information to define to help define a look direction as a vector
    *             going from the "from" location to the "to" node.
    *
    */
   void setLookToNode(osg::Node* node);

   /**
    * This is a method to define a fixed look to point.
    *
    * @param llh This is a <lattitude, longitude, height> vector and the lat and lon are in
    *            angular degrees and the height is in meters.
    */
   void setLookTo(const osg::Vec3d& llh);


   /**
    * This allows for a look to displacement. This displaces along the look axis the amount along
    * the x, y, and z.  This is applied after the from axis orientation and positioning is performed
    *
    * @param displacement This takes a local space displacement in meters and positions along
    *                     the orientation of the node or current relative axis the amount to
    *                     displace to the new center.
    */
   void setLookToLocalDisplacement(const osg::Vec3d& dispalcement);

   /**
    *  This allows one to specify a displacement along the look axis to the point you are looking.
    *  This is applied after the look to local displacement.
    */
   void setLookToRange(double range);

   /**
    * Sets the global range.  This is applied in the direction of the view matrix look.
    *
    * @param range The range is in meters.
    */
   void setRange(double range);
   
   /**
    * Returns the range
    */
   double range()const;
   /**
    * This is the last applied transform and will generate the final orientation matrix for
    * the view.  This is done after a look from and to calculations and allows one to move the
    * head around.
    *
    * @param hpr  This is the relative heading pitch and roll about the final orientation axis.
    *
    */
   void setAttitudeHpr(const osg::Vec3d& hpr)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theAttitudeHpr = hpr;
      setComputeViewMatrixFlag(true);

   }

   /**
    * @return The attitude of the final look position.  This is the Heading pitch and roll
    *         relative to the final orientation axes.
    */
   const osg::Vec3d& attitudeHpr()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theAttitudeHpr;
   }

    /**
    * @return The to information set flag.  A value of true specifies that
    *         the look to information was set
    */
   bool toInformationSetFlag()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theToInformationSetFlag;
   }
   /**
    *  @return The current to node.
    */
   osg::Node* toNode()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theToNode.get();
   }

   /**
    *  @return The current to node as a constant value.
    */
   const osg::Node* toNode()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theToNode.get();
   }

   /**
    * This should only be called if a look to was set.
    *
    * @return the Look to position lat lon height as a Vec3d.
    */
   const osg::Vec3d& toPositionLlh()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theToPositionLLH;
   }
   
   /**
    * This is a displacement along the current Rotational axis.
    */
   const osg::Vec3d& toDisplacement()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theToDisplacement;
   }
   
   /**
    */
   double toRange()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theToRange;
   }

   /**
    * This allows one to change the look axis you wish to look down.
    * After the orientation is applied you can set the look axis you wish
    * to use.  This is not used if you are looking at another node or a fixed point
    * for the look direction is already known.  This is used only when you are in a
    * from location only and will allow you to swap between axis to look down.  So you
    * can easily look up, down, left, right, ... etc
    *
    * @param LookAxis The axis to look down,  see LookAxis enumeration for possible
    *                  values.
    */
   void setLookAxis(LookAxis axis)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theLookAxis = axis;
      setComputeViewMatrixFlag(true);
   }
   /**
    * @return The current look axis.
    */
   LookAxis lookAxis()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theLookAxis;
   }

   /**
    * This matrix you can think of as placing objects out into the scene.
    * Basically a local to world transform.
    * @return the composited view Matrix as defined by the look to and from parameter settings.
    */
   const osg::Matrixd& viewMatrix()const;

   /**
    * This takes world coordinates and puts it into this local orientation.  This is typically
    * used to set a Camera Matrix.  Call this to set a osg::Camera matrix or to set a manipulators
    * setByMatrix.
    *
    * @return The inverse of the view matrix.
    */
   const osg::Matrixd& inverseViewMatrix()const;

   /**
    * Returns true or false if there is enough information to compute a matrix.  We will need the
    * geoRefModel for coodrinate transformations and at least the From point information
    * needs to be set.
    *
    * @return true if a call to viewMatrix would be valid or false otherwise.
    */
   bool isValid()const{return (theModel.valid()&&theFromInformationSetFlag);}

   void setFromInformationSetFlag(bool flag)
    {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theFromInformationSetFlag = flag;
    }
   void setToInformationSetFlag(bool flag)
    {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theToInformationSetFlag = flag;
    }
   
   /**
    * @see setParametersByMatrix.
    *
    * This just wraps the call to the setParamtersByMatrix to invert the 
    * passed in matrix.
    *
    * @param m Inverted matrix.
    */
   void setParametersByInverseMatrix(const osg::Matrixd& m)
   {
      osg::Matrixd newM(osg::Matrixd::inverse(m));
      setParametersByMatrix(newM);
   }
   
   /**
    *
    * Allows one to extract the absolute position and orientation of the composited
    * view matrix.  We still have a problem if the viewMatrix has a roll component influencing
    * the solution.  We don't get an exact inverse.
    *
    */
   virtual bool extractCompositedLlhHprParameters(osg::Vec3d& llh,
                                                 osg::Vec3d& hpr)const;
   
   /**
    * This will take a matrix and flatten the parameters of the ViewMatrixBuilder to be purely a from
    * unlocked type ViewMatrixBuilder.  It will take the matrix passed in and extract out the eye position
    * and orientation hpr.  All other values such as range any to values will be zeroed out.  All nodes
    * that we were locked to will be nulled out.
    *
    * @param m The matrix to use to convert the ViewMatrixBuilder to.
    *
    */
   virtual void setParametersByMatrix(const osg::Matrixd& m);
   
   /**
    * Converts the paramters to a from only.  If the range is to be preserved by setting
    * the flatten range flag to false and there is a to point that we are looking at then 
    * it will preserve the distance in the from range and setup a from only paramter ViewMAtrixBuilder.
    *
    * When using this call the Attitude orientation is not taken into account since this is used as a final 
    * displacement.  So,  the Attitude values are preserved but are removed from the orientation calculation
    * for the new from HPR.
    *
    */
   virtual void convertToAFromViewMatrix(bool flattenRangeFlag = false);
   void invalidateFrom()
   {
     OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
     theFromInformationSetFlag = false;
     theFromNode = 0;
   }
   void invalidateTo()
   {
     OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theToInformationSetFlag = false;
      theToNode = 0;
   }
   void invalidate()
   {
     OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
     theFromInformationSetFlag = false;
      theFromNode = 0;
      theToNode   = 0;
   }
protected:
   /**
    * Sets an internal flag to say that the view matrix needs to be recomputed.
    *
    * @param flag a true value will have the next call to viewMatrix or viewInverseMatrix
    *        recalculate the matrices.
    */
   void setComputeViewMatrixFlag(bool flag)
   {
      theComputeViewMatrixFlag = flag;
   }

   /**
    * Returns the view matrix flag
    */
   bool computeViewMatrixFlag()const
   {
      return theComputeViewMatrixFlag;
   }

   /**
    * Recomputes the view matrix based on all the from and to paraemters.
    */
   virtual void computeMatrices()const;

   /**
    * Thread sync mutex for multiple threads hitting the object.
    */
   mutable OpenThreads::ReentrantMutex thePropertyMutex;

   /**
    * A referenced pointer to the current Model in planet.
    */
   osg::ref_ptr<ossimPlanetGeoRefModel> theModel;

   /**
    * The look axis for defining whihc axis to look down when in
    * view mode only.
    */
   LookAxis                theLookAxis;

   bool                    theFromInformationSetFlag;
   osg::ref_ptr<osg::Node> theFromNode;
   osg::Vec3d              theFromPositionLLH;
   osg::Vec3d              theFromRelativeHpr;
   OrientationFlags        theFromRelativeOrientationFlags;
   osg::Vec3d              theFromHpr;
   double                  theFromRange;
   osg::Vec3d              theFromDisplacement;

   bool                    theToInformationSetFlag;
   osg::ref_ptr<osg::Node> theToNode;
   osg::Vec3d              theToPositionLLH;
   osg::Vec3d              theToDisplacement;
   double                  theToRange;
   
   osg::Vec3d              theAttitudeHpr;
   double                  theRange;
   mutable osg::Matrix     theViewMatrix;
   mutable osg::Matrix     theInverseViewMatrix;
   mutable bool            theComputeViewMatrixFlag;
};

#endif // OSSIMPLANETVIEWMATRIXBUILDER_H_
