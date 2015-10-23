
#include <ossimPlanet/ossimPlanetViewMatrixBuilder.h>
#include <ossimPlanet/ossimPlanetLsrSpaceTransform.h>
#include <ossimPlanet/ossimPlanetPointModel.h>
#include <ossimPlanet/mkUtils.h>
#include <osg/io_utils>
ossimPlanetViewMatrixBuilder::Visitor::Visitor()
:osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}
void ossimPlanetViewMatrixBuilder::Visitor::apply(osg::Node& node)
{
   ossimPlanetPointModel* pointModel = dynamic_cast<ossimPlanetPointModel*>(&node);
   if(pointModel)
   {
      thePointModel        = pointModel;
      theLsrSpaceTransform = thePointModel->lsrSpace();
      return;
   }
   else
   {
      ossimPlanetLsrSpaceTransform* lsrSpace = dynamic_cast<ossimPlanetLsrSpaceTransform*>(&node);
      if(lsrSpace)
      {
         theLsrSpaceTransform = lsrSpace;
         return;
      }
   }
   
   traverse(node);
}

ossimPlanetViewMatrixBuilder::ossimPlanetViewMatrixBuilder(ossimPlanetGeoRefModel* geoRefModel)
:theModel(geoRefModel),
theLookAxis(LOOK_AXIS_Y),
theFromInformationSetFlag(false),
theFromNode(0),
theFromPositionLLH(osg::Vec3d(0.0,0.0,0.0)),
theFromRelativeHpr(osg::Vec3d(0.0,0.0,0.0)),
theFromRelativeOrientationFlags(ALL_ORIENTATION),
theFromHpr(osg::Vec3d(0.0,0.0,0.0)),
theFromRange(0.0),
theToInformationSetFlag(false),
theToNode(0),
theToPositionLLH(osg::Vec3d(0.0,0.0,0.0)),
theToDisplacement(osg::Vec3d(0.0,0.0,0.0)),
theToRange(0.0),
theAttitudeHpr(osg::Vec3d(0.0,0.0,0.0)),
theRange(0.0),
theViewMatrix(osg::Matrixd()),
theInverseViewMatrix(osg::Matrixd()),
theComputeViewMatrixFlag(false)
{
   
}

ossimPlanetViewMatrixBuilder::ossimPlanetViewMatrixBuilder(const osg::Matrixd& m, ossimPlanetGeoRefModel* geoRefModel)
:theModel(geoRefModel),
theLookAxis(LOOK_AXIS_Y),
theFromInformationSetFlag(false),
theFromNode(0),
theFromPositionLLH(osg::Vec3d(0.0,0.0,0.0)),
theFromRelativeHpr(osg::Vec3d(0.0,0.0,0.0)),
theFromRelativeOrientationFlags(ALL_ORIENTATION),
theFromHpr(osg::Vec3d(0.0,0.0,0.0)),
theFromRange(0.0),
theToInformationSetFlag(false),
theToNode(0),
theToPositionLLH(osg::Vec3d(0.0,0.0,0.0)),
theToDisplacement(osg::Vec3d(0.0,0.0,0.0)),
theToRange(0.0),
theAttitudeHpr(osg::Vec3d(0.0,0.0,0.0)),
theRange(0.0),
theViewMatrix(osg::Matrixd()),
theInverseViewMatrix(osg::Matrixd()),
theComputeViewMatrixFlag(false)
{
   setParametersByMatrix(m);
}

ossimPlanetViewMatrixBuilder::ossimPlanetViewMatrixBuilder(const ossimPlanetViewMatrixBuilder& src)
:theModel(src.theModel),
theLookAxis(src.theLookAxis),
theFromInformationSetFlag(src.theFromInformationSetFlag),
theFromNode(src.theFromNode),
theFromPositionLLH(src.theFromPositionLLH),
theFromRelativeHpr(src.theFromRelativeHpr),
theFromRelativeOrientationFlags(src.theFromRelativeOrientationFlags),
theFromHpr(src.theFromHpr),
theFromRange(src.theFromRange),
theFromDisplacement(src.theFromDisplacement),
theToInformationSetFlag(src.theToInformationSetFlag),
theToNode(src.theToNode),
theToPositionLLH(src.theToPositionLLH),
theToDisplacement(src.theToDisplacement),
theToRange(src.theToRange),
theAttitudeHpr(src.theAttitudeHpr),
theRange(src.theRange),
theViewMatrix(src.theViewMatrix),
theInverseViewMatrix(src.theInverseViewMatrix),
theComputeViewMatrixFlag(src.theComputeViewMatrixFlag)
{
}

ossimPlanetViewMatrixBuilder::~ossimPlanetViewMatrixBuilder()
{
   
}

void ossimPlanetViewMatrixBuilder::setGeoRefModel(ossimPlanetGeoRefModel* model)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theModel = model;
   setComputeViewMatrixFlag(true);
}

void ossimPlanetViewMatrixBuilder::updateFromLocalDisplacement(const osg::Vec3d& displacement)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theFromDisplacement = displacement;
   setComputeViewMatrixFlag(true);
}

void ossimPlanetViewMatrixBuilder::setLookFromNodeOffset(osg::Node* node,
                                                         const osg::Vec3d& hpr,
                                                         double range,
                                                         int relativeOrientationFlag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theFromInformationSetFlag = false;
   theFromNode = node;
   if(!node||!theModel.valid()) return;
   
   Visitor v;
   node->accept(v);
   theFromRange = range;
   theFromRelativeOrientationFlags = static_cast<OrientationFlags>(relativeOrientationFlag);
   theFromRelativeHpr = hpr;
   theFromInformationSetFlag = false;
   if(v.theLsrSpaceTransform.valid())
   {
      if(!theModel.valid())
      {
         theModel = v.theLsrSpaceTransform->model();
      }
      theFromInformationSetFlag = true;
      theFromPositionLLH = osg::Vec3d(v.theLsrSpaceTransform->lat(),
                                      v.theLsrSpaceTransform->lon(),
                                      v.theLsrSpaceTransform->altitude());
      theFromHpr = v.theLsrSpaceTransform->headingPitchRoll();
      
      setComputeViewMatrixFlag(true);
   }
}

void ossimPlanetViewMatrixBuilder::setLookFrom(const osg::Vec3d& llh,
                                               const osg::Vec3d& hpr,
                                               double range)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theFromNode = 0;
   theFromRange = range; // we displace along the look
   theFromPositionLLH = llh;
   theFromInformationSetFlag = true;
   theFromRelativeOrientationFlags = ALL_ORIENTATION;
   theFromRelativeHpr = hpr;
   theFromHpr = osg::Vec3d(0.0,0.0,0.0);
   setComputeViewMatrixFlag(true);
}

osg::Vec3d ossimPlanetViewMatrixBuilder::computeFromOrientation()const
{
   osg::Vec3d hpr = theFromRelativeHpr;
   if(theFromRelativeOrientationFlags&HEADING)
   {
      // we use the hpr of the node
      hpr[0] += theFromHpr[0];
   }
   if(theFromRelativeOrientationFlags&PITCH)
   {
      // we use the hpr of the node
      hpr[1] += theFromHpr[1];
   }
   if(theFromRelativeOrientationFlags&ROLL)
   {
      // we use the hpr of the node
      hpr[2] += theFromHpr[2];
   }
   return hpr;
}

void ossimPlanetViewMatrixBuilder::setLookToNode(osg::Node* node)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theToNode = node;
   theToInformationSetFlag = false;
   if(node)
   {
      Visitor v;
      node->accept(v);
      if(v.theLsrSpaceTransform.valid())
      {
         theToPositionLLH = osg::Vec3d(v.theLsrSpaceTransform->lat(),
                                       v.theLsrSpaceTransform->lon(),
                                       v.theLsrSpaceTransform->altitude());
         theToInformationSetFlag = true;
      }
      else
      {
         // for now set back to null since we can't do anything with it
         theToNode = 0;
      }
   }
   
   setComputeViewMatrixFlag(true);
}

void ossimPlanetViewMatrixBuilder::setLookTo(const osg::Vec3d& llh)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theToNode = 0;
   theToPositionLLH = llh;
   theToInformationSetFlag = true;
   setComputeViewMatrixFlag(true);
}

void ossimPlanetViewMatrixBuilder::setLookToLocalDisplacement(const osg::Vec3d& displacement)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theToDisplacement = displacement;
   setComputeViewMatrixFlag(true);
}
void ossimPlanetViewMatrixBuilder::setLookToRange(double range)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theToRange = range;
   setComputeViewMatrixFlag(true);
}

void ossimPlanetViewMatrixBuilder::setRange(double range)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theRange = range;
   setComputeViewMatrixFlag(true);
}

double ossimPlanetViewMatrixBuilder::range()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theRange;
}

bool ossimPlanetViewMatrixBuilder::extractCompositedLlhHprParameters(osg::Vec3d& llh,
                                                                    osg::Vec3d& hpr)const
{
   // no calculate the HPR and position of the from
   //
   ossimPlanetViewMatrixBuilder thisBuilder(*this);
   ossimPlanetViewMatrixBuilder tempBuilder(theModel.get());
   thisBuilder.setLookAxis(ossimPlanetViewMatrixBuilder::LOOK_AXIS_NEGATIVE_Z);
   osg::Matrixd m = thisBuilder.viewMatrix();
   osg::Vec3d eyeXyz(m(3,0), m(3,1), m(3,2));
   theModel->xyzToLatLonHeight(eyeXyz, llh);
   tempBuilder.setLookAxis(ossimPlanetViewMatrixBuilder::LOOK_AXIS_NEGATIVE_Z);
   tempBuilder.setLookFrom(llh, osg::Vec3d(0.0,0.0,0.0), 0.0);
   osg::Matrixd lsrM = tempBuilder.viewMatrix();
   osg::Vec3d newHpr;
   mkUtils::matrixToHpr(newHpr, lsrM, m);
   hpr = newHpr;
   
   return true;
}

void ossimPlanetViewMatrixBuilder::setParametersByMatrix(const osg::Matrixd& m)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if(!theModel.valid())
   {
      // nothing to do
      return;
   }
   theAttitudeHpr = osg::Vec3d(0.0,0.0,0.0);
   theFromRelativeOrientationFlags = ALL_ORIENTATION;
   theFromRelativeHpr        = osg::Vec3d(0.0,0.0,0.0);
   theFromRange              = 0.0;
   theFromDisplacement       = osg::Vec3d(0.0,0.0,0.0);
   theToInformationSetFlag   = false;
   theFromInformationSetFlag = true;
   theToNode                 = 0;
   theFromNode               = 0;
   theToDisplacement         = osg::Vec3d(0.0,0.0,0.0);   
   theToRange                = 0.0; 
   theRange                  = 0.0;
   // no calculate the HPR and position of the from
   //
   osg::Vec3d eyeXyz(m(3,0), m(3,1), m(3,2));
   osg::Matrixd lsrM;
   theModel->xyzToLatLonHeight(eyeXyz, theFromPositionLLH);
   theModel->orientationLsrMatrix(lsrM, theFromPositionLLH, 0.0, 0.0, 0.0);
   osg::Vec3d newHpr;
   mkUtils::matrixToHpr(newHpr, lsrM, m);
   newHpr[1] -= 90.0;
   newHpr[1]  = ossim::wrap(newHpr[1], -180.0, 180.0);
   theFromHpr = newHpr;
   
   
   setComputeViewMatrixFlag(true);
}

void ossimPlanetViewMatrixBuilder::convertToAFromViewMatrix(bool flattenRangeFlag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if(!theModel.valid())
   {
      // nothing to do
      return;
   }
   if(flattenRangeFlag&&!theToInformationSetFlag)
   {
      if(theFromRange != 0.0)
      {
         osg::Vec3d savedAttitudeHpr = theAttitudeHpr;
         theAttitudeHpr = osg::Vec3d(0.0,0.0,0.0);
         setComputeViewMatrixFlag(true);
         osg::Matrixd currentM = viewMatrix();
         theAttitudeHpr = savedAttitudeHpr;
         setComputeViewMatrixFlag(true);
         osg::Matrixd lsrM;
         osg::Vec3d eyeXyz(currentM(3,0), currentM(3,1), currentM(3,2));
         osg::Vec3d eyeLlh;
         theModel->xyzToLatLonHeight(eyeXyz, eyeLlh);
         theModel->orientationLsrMatrix(lsrM, eyeLlh, 0.0, 0.0, 0.0);
         theFromRange = 0.0;
         theFromPositionLLH = eyeLlh;
         osg::Vec3d newHpr;
         mkUtils::matrixToHpr(newHpr, lsrM, currentM);
         newHpr[1] -= 90.0;
         newHpr[1]  = ossim::wrap(newHpr[1], -180.0, 180.0);
         // newHpr[2] = 0.0;
         theFromHpr = newHpr;
      }
   }
   else
   {
      osg::Vec3d newHpr;
      osg::Vec3d toXyz;
      theModel->latLonHeightToXyz(theToPositionLLH, toXyz);
      osg::Vec3d savedAttitudeHpr = theAttitudeHpr;
      theAttitudeHpr = osg::Vec3d(0.0,0.0,0.0);
      setComputeViewMatrixFlag(true);
      osg::Matrixd currentM = viewMatrix();
      theAttitudeHpr = savedAttitudeHpr;
      setComputeViewMatrixFlag(true);
      osg::Matrixd lsrM;
      osg::Vec3d eyeXyz(currentM(3,0), currentM(3,1), currentM(3,2));
      osg::Vec3d eyeLlh;
      theModel->xyzToLatLonHeight(eyeXyz, eyeLlh);
      
      if(flattenRangeFlag)
      {
         theModel->orientationLsrMatrix(lsrM, eyeLlh, 0.0, 0.0, 0.0);
         theFromRange = 0.0;
         theFromPositionLLH = eyeLlh;
      }
      else
      {
         double range = theModel->calculateUnnormalizedLengthXyz(eyeXyz, toXyz);
         theModel->orientationLsrMatrix(lsrM, theToPositionLLH, 0.0, 0.0, 0.0);
         theFromRange = -range; // we displace along the look
         theFromPositionLLH = theToPositionLLH;
      }
      mkUtils::matrixToHpr(newHpr, lsrM, currentM);
      
      
      //   osg::Matrixd viewM = viewMatrix();
      newHpr[1] -= 90.0;
      newHpr[1]  = ossim::wrap(newHpr[1], -180.0, 180.0);
      // newHpr[2] = 0.0;
      theFromHpr = newHpr;
   }
   theFromRelativeOrientationFlags = ALL_ORIENTATION;
   theFromRelativeHpr = osg::Vec3d(0.0,0.0,0.0);
   theFromDisplacement = osg::Vec3d(0.0,0.0,0.0);
   theToInformationSetFlag = false;
   theFromInformationSetFlag = true;
   theToNode = 0;
   theFromNode = 0;
   theRange = 0.0;
   setComputeViewMatrixFlag(true);
}

void ossimPlanetViewMatrixBuilder::computeMatrices()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if(!theModel) return;
   // need to compute the view matrix
   if(theFromInformationSetFlag)
   {
      osg::Matrixd tempM;
      osg::Vec3d hpr = computeFromOrientation();
      theModel->orientationLsrMatrix(tempM,
                                     theFromPositionLLH,
                                     hpr[0],
                                     hpr[1],
                                     hpr[2]);
      osg::Vec3d xAxis(tempM(0,0), tempM(0,1), tempM(0,2));
      osg::Vec3d yAxis(tempM(1,0), tempM(1,1), tempM(1,2));
      osg::Vec3d zAxis(tempM(2,0), tempM(2,1), tempM(2,2));
      osg::Vec3d eye(tempM(3,0),   tempM(3,1), tempM(3,2));
      osg::Vec3d newEye;
      if(!ossim::almostEqual(theFromDisplacement.length2(), 0.0, 1e-15))
      {
         osg::Matrixd displacementM;
         
         theModel->orientationLsrMatrix(displacementM,
                                        theFromPositionLLH,
                                        theFromHpr[0],
                                        theFromHpr[1],
                                        theFromHpr[2]);
         
         // we will only displace along the axis of the orientation matrix
         // relative to the model to solve the new eye origin
         //
         osg::Vec3d displacement = theFromDisplacement*theModel->getInvNormalizationScale();
         osg::Vec3d xAxis(displacementM(0,0), displacementM(0,1), displacementM(0,2));
         osg::Vec3d yAxis(displacementM(1,0), displacementM(1,1), displacementM(1,2));
         osg::Vec3d zAxis(displacementM(2,0), displacementM(2,1), displacementM(2,2));
         osg::Vec3d eye(displacementM(3,0),   displacementM(3,1), displacementM(3,2));
         newEye = (eye +
                   xAxis*displacement[0]+
                   yAxis*displacement[1]+
                   zAxis*displacement[2]);
      }
      else
      {
         newEye = eye;
      }
      osg::Vec3d lookVector;
      osg::Vec3d upAxis;
      switch(theLookAxis)
      {
         case LOOK_AXIS_X:
         {
            lookVector = (newEye + xAxis) - newEye;
            upAxis = zAxis;
            break;
         }
         case LOOK_AXIS_Y:
         {
            lookVector = (newEye + yAxis) - newEye;
            upAxis = zAxis;
            //std::cout << "HPR build = " << theFromHpr << std::endl;
            break;
         }
         case LOOK_AXIS_Z:
         {
            lookVector = (newEye + zAxis) - newEye;
            upAxis = yAxis;
            break;
         }
         case LOOK_AXIS_NEGATIVE_X:
         {
            lookVector = (newEye - xAxis) - newEye;
            upAxis = zAxis;
            break;
         }
         case LOOK_AXIS_NEGATIVE_Y:
         {
            lookVector = (newEye - yAxis) - newEye;
            upAxis = zAxis;
            break;
         }
         case LOOK_AXIS_NEGATIVE_Z:
         {
            lookVector = (newEye - zAxis) - newEye;
            upAxis = yAxis;
            break;
         }
         default:
         {
            // for now we only support Y Axis;
            lookVector = (newEye + zAxis) - newEye;
            upAxis = yAxis;
            break;
         }
      }
      lookVector.normalize();
      
      // now displace along the look axis a range value
      //
      newEye = newEye + lookVector*(theModel->getInvNormalizationScale()*theFromRange);
      
      // now let's make a look at based on the orientation axis and the final eye
      // position. We should have the center displacment and range integrated into
      // the final look
      //
      if(!theToInformationSetFlag)
      {
         if(!ossim::almostEqual(theRange, 0.0))
         {
            newEye = newEye + lookVector*(theModel->getInvNormalizationScale()*theRange);
            theInverseViewMatrix.makeLookAt(newEye, newEye + lookVector, upAxis);
         }
         else
         {
            theInverseViewMatrix.makeLookAt(newEye, newEye + lookVector, upAxis);
         }
         theViewMatrix.invert(theInverseViewMatrix);
      }
      else
      {
         osg::Vec3d toXyz;
         theModel->latLonHeightToXyz(theToPositionLLH, toXyz);
         
         double delta = (newEye-toXyz).length();
         // sanity check
         //
         if(ossim::almostEqual(0.0,delta))
         {
            if(!ossim::almostEqual(theRange, 0.0))
            {
               newEye = newEye + lookVector*(theModel->getInvNormalizationScale()*theRange);
               theInverseViewMatrix.makeLookAt(newEye, newEye + lookVector, upAxis);
            }
            else
            {
               theInverseViewMatrix.makeLookAt(newEye, newEye + lookVector, upAxis);
            }
            theViewMatrix.invert(theInverseViewMatrix);
         }
         else
         {
            upAxis = zAxis;
            lookVector = toXyz-newEye;
            lookVector.normalize();
         
            if(!ossim::almostEqual(theToDisplacement.length2(), 0.0, 1e-15)||
               !ossim::almostEqual(theToRange,0.0,1e-15)||
               !ossim::almostEqual(theRange, 0.0, 1e-15))
            {
               osg::Vec3d displacement = theToDisplacement*theModel->getInvNormalizationScale();
               double range = (theToRange+theRange)*theModel->getInvNormalizationScale();
 //              double range = (theToRange)*theModel->getInvNormalizationScale();
               theInverseViewMatrix.makeLookAt(newEye, toXyz, upAxis);
               theViewMatrix.invert(theInverseViewMatrix);
               osg::Vec3d xAxis(theViewMatrix(0,0), theViewMatrix(0,1), theViewMatrix(0,2));
               osg::Vec3d yAxis(theViewMatrix(1,0), theViewMatrix(1,1), theViewMatrix(1,2));
               osg::Vec3d zAxis(-theViewMatrix(2,0), -theViewMatrix(2,1), -theViewMatrix(2,2));
               osg::Vec3d eye(theViewMatrix(3,0),   theViewMatrix(3,1), theViewMatrix(3,2));
               newEye = (eye +
                         xAxis*displacement[0]+
                         yAxis*displacement[1]+
                         zAxis*(displacement[2]+range));
            }
            theInverseViewMatrix.makeLookAt(newEye, toXyz, upAxis);
            
            theViewMatrix.invert(theInverseViewMatrix);
         }
         // create a forward vector to the desired look point
      }
      
      if(!ossim::almostEqual(theAttitudeHpr.length2(), 0.0, 1e-15))
      {
         // now let's apply the final orientation matrix
         // we will see if we can integrate further in the chain so we can limit the number
         // of inverts we have to do.
         //
         xAxis = upAxis^lookVector;
         zAxis = xAxis^lookVector;
         
         osg::Matrixd rot;
         rot.makeIdentity();
         rot.makeRotate(osg::DegreesToRadians(theAttitudeHpr[0]), zAxis, // heading
                        osg::DegreesToRadians(theAttitudeHpr[1]), xAxis, // pitch
                        osg::DegreesToRadians(theAttitudeHpr[2]), lookVector); // roll
         osg::Matrixd result(theViewMatrix);
         mkUtils::mult3x3(result, theViewMatrix, rot);
         theViewMatrix = result;
         theInverseViewMatrix.invert(theViewMatrix);
      }

   }
#if 0 // debug stuff
   {
      osg::Vec3d xAxis(theInverseViewMatrix(0,0), theInverseViewMatrix(0,1), theInverseViewMatrix(0,2));
      osg::Vec3d yAxis(theInverseViewMatrix(1,0), theInverseViewMatrix(1,1), theInverseViewMatrix(1,2));
      osg::Vec3d zAxis(theInverseViewMatrix(2,0), theInverseViewMatrix(2,1), theInverseViewMatrix(2,2));
      std::cout << "INVERSE x = " << xAxis << std::endl;
      std::cout << "INVERSE Y = " << yAxis << std::endl;
      std::cout << "INVERSE z = " << zAxis << std::endl;
   }
#endif
   theComputeViewMatrixFlag = false;
}


const osg::Matrix& ossimPlanetViewMatrixBuilder::viewMatrix()const
{
   if(computeViewMatrixFlag())
   {
      computeMatrices();
   }
   
   return theViewMatrix;
}

const osg::Matrix& ossimPlanetViewMatrixBuilder::inverseViewMatrix()const
{
   if(computeViewMatrixFlag())
   {
      computeMatrices();
   }
   
   return theInverseViewMatrix;
}
