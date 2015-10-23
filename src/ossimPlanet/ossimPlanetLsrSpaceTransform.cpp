#include <ossimPlanet/ossimPlanetLsrSpaceTransform.h>
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/mkUtils.h>
#include <osg/io_utils>

ossimPlanetLsrSpaceTransform::ossimPlanetLsrSpaceTransform(ossimPlanetGeoRefModel* model)
:theModel(model),
theRedrawFlag(true)
{
   theScale[0] = theScale[1] = theScale[2] = 1.0;
   theHpr[0] = theHpr[1] = theHpr[2] = 0.0;
   _referenceFrame = RELATIVE_RF;
}

ossimPlanetLsrSpaceTransform::ossimPlanetLsrSpaceTransform(const ossimPlanetLsrSpaceTransform& src,const osg::CopyOp& copyop)
:osg::Transform(src, copyop),
theModel(src.theModel),
theLatLonAltitude(src.theLatLonAltitude),
theXYZ(src.theXYZ),
theHpr(src.theHpr),
theScale(src.theScale),
theLocalToWorld(src.theLocalToWorld),
theInvLocalToWorld(src.theInvLocalToWorld)
{
}

void ossimPlanetLsrSpaceTransform::setMatrix(const osg::Matrix& m)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   dirtyBound();
   theLocalToWorld = m;
   theInvLocalToWorld.invert(m);
   matrixToParameters(m);
}

void ossimPlanetLsrSpaceTransform::setModel(ossimPlanetGeoRefModel* model)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theModel = model;
   parametersToMatrix();
   dirtyBound();
}

void ossimPlanetLsrSpaceTransform::setHeadingPitchRoll(const osg::Vec3d& hpr)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theHpr = hpr;
   parametersToMatrix();
   dirtyBound();
}

void ossimPlanetLsrSpaceTransform::setLatLonAltitude(const osg::Vec3d& value)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theLatLonAltitude = value;
   parametersToMatrix();
   dirtyBound();
}

void ossimPlanetLsrSpaceTransform::setLatLonAltitudeMeanSeaLevel(const osg::Vec3d& value)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if(theModel.valid())
   {
      theLatLonAltitude = value;
      theModel->mslToEllipsoidal(theLatLonAltitude);
   }
   parametersToMatrix();
   dirtyBound();
}

void ossimPlanetLsrSpaceTransform::setScale(const osg::Vec3d& value)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theScale = value;
   parametersToMatrix();
   dirtyBound();
}

void ossimPlanetLsrSpaceTransform::setXYZ(const osg::Vec3d& xyz)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if(theModel.valid())
   {
      theXYZ = xyz;
      theModel->inverse(xyz, theLatLonAltitude);
   }
   dirtyBound();
   parametersToMatrix();
}

void ossimPlanetLsrSpaceTransform::traverse(osg::NodeVisitor& nv)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
        if(theRedrawFlag)
         {
            ossimPlanetNode* node = ossimPlanetNode::findNode(nv.getNodePath());
            if(node)
            {
               node->setRedrawFlag(true);
            }
            theRedrawFlag = false;
         }
         if(!theModel.valid())
         {
            ossimPlanetLayer* layer = ossimPlanetLayer::findLayer(nv.getNodePath());
            if(layer)
            {
               theModel = layer->model();
            }
         }
         break;
      }
      default:
      {
         break;
      }
   }
   osg::Transform::traverse(nv);
}

void ossimPlanetLsrSpaceTransform::matrixToParameters(const osg::Matrix& inputM)
{
   if(theModel.valid())
   {
      osg::Matrixd m;
      osg::Vec3d translation;
      osg::Quat rotation;
      osg::Quat s;
      // deomcpose some of the parts we need
      //
      inputM.decompose(translation, rotation, theScale, s);

      // no solve the relative heading pitch and roll to the
      // tangent plane at the point translation
      //
      theXYZ = translation;
      theModel->inverse(translation, theLatLonAltitude);
      theModel->lsrMatrix(theLatLonAltitude, m);//, theOrientationMode);
      mkUtils::matrixToHpr(theHpr, m, inputM);
      theRedrawFlag = true;
      notifyLsrSpaceChanged();
      
 //     std::cout << "hpr = " << theHpr            << std::endl;
//      std::cout << "llh = " << theLatLonAltitude << std::endl;
      
   }
}

bool ossimPlanetLsrSpaceTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if (_referenceFrame==RELATIVE_RF)
   {
      matrix.preMult(theLocalToWorld);
   }
   else // absolute
   {
      matrix = theLocalToWorld;
   }
   return true;
}

bool ossimPlanetLsrSpaceTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if (_referenceFrame==RELATIVE_RF)
   {
      matrix.postMult(theInvLocalToWorld);
   }
   else // absolute
   {
      matrix = theInvLocalToWorld;
   }
   return true;
}

bool ossimPlanetLsrSpaceTransform::parametersToMatrix()
{
   if(theScale.x() == 0.0 || theScale.y() == 0.0 || theScale.z()==0.0) return false;
   if(!theModel.valid()) return false;
   osg::Matrixd m;
   theModel->orientationLsrMatrix(m, theLatLonAltitude, theHpr[0], theHpr[1], theHpr[2]);//, theOrientationMode);
   theLocalToWorld = osg::Matrixd::scale(osg::Vec3d(theScale[0], theScale[1], theScale[2]))*m;
   theInvLocalToWorld.invert(theLocalToWorld);
   theRedrawFlag = true;
   notifyLsrSpaceChanged();
   return true;
}

void ossimPlanetLsrSpaceTransform::notifyLsrSpaceChanged()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx = 0;
   ossim_uint32 upper = theCallbackList.size();

   for(idx = 0; idx < upper; ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->lsrSpaceChanged(this);
      }
   }
}

