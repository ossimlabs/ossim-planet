#include <ossimPlanet/ossimPlanetAnimatedPointModel.h>
#include <ossimPlanet/ossimPlanetLayer.h>

ossimPlanetAnimatedPointModel::ossimPlanetAnimatedPointModel()
:theShowPathFlag(true),
theShowModelFlag(true),
theAnimationPathColor(1.0,0.0,0.0,.5),
theAnimationPathLineThickness(2.0)
{
   setCullingActive(false);
   thePathMatrixTransform = new osg::MatrixTransform();
   thePathColor           = new osg::Vec4Array();
   thePathVertices        = new osg::Vec3Array();
   thePathGeode           = new osg::Geode();
   thePathGeometry        = new osg::Geometry();
   theLineWidth           = new osg::LineWidth(theAnimationPathLineThickness);
   thePathColor->push_back(theAnimationPathColor);
   // add the points geometry to the geode.
   thePathGeode->addDrawable(thePathGeometry.get());
   thePathMatrixTransform->addChild(thePathGeode.get());
   
   theAnimationPathCallback = new ossimPlanetAnimatedPointModel::PathCallback();
}

ossimPlanetAnimatedPointModel::~ossimPlanetAnimatedPointModel()
{
   if(thePointModel.valid())
   {
      thePointModel->setUpdateCallback(0);
   }
   if(theAnimationPathCallback.valid())
   {
      theAnimationPathCallback->setAnimationPath(0);
   }
}

void ossimPlanetAnimatedPointModel::execute(const ossimPlanetAction& action)
{
   ossimPlanetAnnotationLayerNode::execute(action);
}

void ossimPlanetAnimatedPointModel::setAnimationPath(ossimPlanetAnimationPath* path)
{
   setDirtyBit((DirtyBit)(COLOR_DIRTY|COORDINATE_DIRTY)); // this will dirty everything so all is regenerated
   theAnimationPath = path;
   
   theAnimationPathCallback->setAnimationPath(theAnimationPath.get());
}

void ossimPlanetAnimatedPointModel::setAnimationPathColor(const osg::Vec4f& value)
{
   setDirtyBit(COLOR_DIRTY); // this will rest color and thickness for the path
   theAnimationPathColor = value;
}

void ossimPlanetAnimatedPointModel::setAnimationPathLineThickness(ossim_float32 value)
{
   setDirtyBit(COLOR_DIRTY);  // let's just reset color and thickness always
   theAnimationPathLineThickness = value;
}

void ossimPlanetAnimatedPointModel::setShowPathFlag(bool flag)
{
   theShowPathFlag = flag;
}

void ossimPlanetAnimatedPointModel::setShowModelFlag(bool flag)
{
   theShowModelFlag = flag;
}

void ossimPlanetAnimatedPointModel::setPointModel(osg::Node* value)
{
   if(thePointModel.valid())
   {
      thePointModel->setUpdateCallback(0);
      // need to test update callback to our animation update callback.
      // thePointModel->getUpdateCallback();
   }
   thePointModel = value;
   if(value)
   {
      value->setUpdateCallback(theAnimationPathCallback.get());
   }
}

void ossimPlanetAnimatedPointModel::setTimeScale(ossim_float64 scale)
{
   theAnimationPathCallback->setTimeMultiplier(scale);
}

void ossimPlanetAnimatedPointModel::setTimeOffset(ossim_float64 offset)
{
   theAnimationPathCallback->setTimeOffset(offset);
}

void ossimPlanetAnimatedPointModel::traverse(osg::NodeVisitor& nv)
{
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if(theAnimationPath.valid()&&!theAnimationPath->geoRefModel())
         {
            if(layer()&&layer()->model())
            {
               theAnimationPath->setGeoRefModel(layer()->model());
               // let's reset everything
               setDirtyBit(COORDINATE_DIRTY);
               setDirtyBit(COLOR_DIRTY);
            }
         }
         if(isDirtyBitSet(COLOR_DIRTY))
         {
            updateColor();
         }
         if(isDirtyBitSet(COORDINATE_DIRTY))
         {
            updateCoordinates();
         }
         break;
      }
      default:
      {
         break;
      }
   }
   if(thePathVertices->size()>0)
   {
      if(nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
      {
         if(theShowPathFlag)
         {
            // draw the path
            thePathMatrixTransform->accept(nv);
         }
      }
      else
      {
         thePathMatrixTransform->accept(nv);
      }
   }
   if(thePointModel.valid())
   {
      if(nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
      {
         if(theShowModelFlag)
         {
            thePointModel->accept(nv);
         }
      }
      else
      {
         thePointModel->accept(nv);
      }
   }
}

void ossimPlanetAnimatedPointModel::stage()
{
   updateCoordinates();
   updateColor();
   setStagedFlag(true);
}

void ossimPlanetAnimatedPointModel::updateCoordinates()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theUpdateCoordinatesMutex);
   if(theAnimationPath.valid())
   {
      ossimPlanetAnimationPath::PointList pointList;
      theAnimationPath->generateWorldCoordinates(pointList);
      bool updatePrimitiveSetFlag = false;
      if(thePathVertices->size() != pointList.size())
      {
         updatePrimitiveSetFlag = true;
         thePathVertices->resize(pointList.size());
      }
      ossim_uint32 idx = 0;
      ossim_uint32 size = pointList.size();
      ossim_float64 normalizer = 1.0/static_cast<ossim_float64>(size);
      osg::Vec3d theCenter;
      if(pointList.size())
      {
         theCenter = pointList[0]*normalizer;
      }
      for(idx=1; idx < size; ++idx)
      {
         theCenter += pointList[idx]*normalizer;
      }
      osg::Matrixd m;
      m.setTrans(theCenter);
      thePathMatrixTransform->setMatrix(m);
      for(idx=0; idx < size; ++idx)
      {
         osg::Vec3f localPoint = pointList[idx] - theCenter;
         (*thePathVertices)[idx] = localPoint;
      }
      thePathGeometry->setVertexArray(thePathVertices.get());
      if(updatePrimitiveSetFlag)
      {
         if(thePathGeometry->getNumPrimitiveSets() > 0)
         {
            thePathGeometry->setPrimitiveSet(0, new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, pointList.size())); 
         }
         else
         {
            thePathGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP,0,pointList.size())); 
         }
      }
      
      thePathGeode->dirtyBound();
      thePathMatrixTransform->dirtyBound();
   }
   
   clearDirtyBit(COORDINATE_DIRTY);
}

void ossimPlanetAnimatedPointModel::updateColor()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theUpdateColorMutex);
   if(thePathColor->size() != 1)
   {
      thePathColor->push_back(theAnimationPathColor);
   }
   else
   {
      (*thePathColor)[0] = theAnimationPathColor;
   }
   osg::StateSet* stateset = thePathGeometry->getOrCreateStateSet();
   theLineWidth->setWidth(theAnimationPathLineThickness);
   thePathGeometry->setColorArray(thePathColor.get());
   thePathGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
   stateset->setAttribute(theLineWidth.get());
   
   thePathGeometry->setUseVertexBufferObjects(true);
   thePathGeometry->setUseDisplayList(false);
//   thePathGeometry->setFastPathHint(true);
   // for the path we will turn the lighting off
   stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
   
   stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
   stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
   
   clearDirtyBit(COLOR_DIRTY);
}


ossimPlanetAnimatedPointModel::PathCallback::PathCallback()
:osg::AnimationPathCallback()
{
   
}
ossimPlanetAnimatedPointModel::PathCallback::PathCallback(const PathCallback& apc,
             const osg::CopyOp& copyop)
:osg::AnimationPathCallback(apc, copyop)
{
}

ossimPlanetAnimatedPointModel::PathCallback::PathCallback(osg::AnimationPath* ap,
                                                          double timeOffset,
                                                          double timeMultiplier)
:osg::AnimationPathCallback(ap, timeOffset, timeMultiplier)
{
}

void ossimPlanetAnimatedPointModel::PathCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
   ossimPlanetPointModel* pointModel = dynamic_cast<ossimPlanetPointModel*>(node);
   ossimPlanetLsrSpaceTransform* lsrSpaceTransform = dynamic_cast<ossimPlanetLsrSpaceTransform*>(node);
   if(pointModel||lsrSpaceTransform)
   {
      if (_animationPath.valid() && 
          nv->getVisitorType()==osg::NodeVisitor::UPDATE_VISITOR && 
          nv->getFrameStamp())
      {
         double time = nv->getFrameStamp()->getSimulationTime();
         // we might want to simulate both sim time and if real time by getting the first time to be real time
         // but for now we will assume firstTime relative to 0 for sim time.
         _latestTime = time;
         _firstTime = 0.0;
        
         if (!_pause)
         {
            // Only update _firstTime the first time, when its value is still DBL_MAX
            //if (_firstTime==DBL_MAX) _firstTime = time;
            osg::AnimationPath::ControlPoint cp;
            if(pointModel) lsrSpaceTransform = pointModel->lsrSpace();
            if (_animationPath->getInterpolatedControlPoint(getAnimationTime(),cp))
            {
               osg::Matrixd m;
               cp.getMatrix(m);
               
               lsrSpaceTransform->setMatrix(m);
            }
         }
      }
      
      // must call any nested node callbacks and continue subgraph traversal.
      NodeCallback::traverse(node,nv);
   }
   else
   {
      osg::AnimationPathCallback::operator()(node, nv);
   }
}

