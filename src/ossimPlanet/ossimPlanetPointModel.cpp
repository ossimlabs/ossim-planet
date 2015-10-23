#include <ossimPlanet/ossimPlanetPointModel.h>
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/mkUtils.h>
#include <OpenThreads/ScopedLock>
#include <ossim/base/ossimNotify.h>
#include <osg/io_utils>

ossimPlanetPointModel::ossimPlanetPointModel()
:theNodeChangedFlag(false)
{
   theLsrSpaceCallback = new LsrSpaceCallback(this);
   theLsrSpaceTransform = new ossimPlanetLsrSpaceTransform();
   theLsrSpaceTransform->addCallback(theLsrSpaceCallback.get());
   theLsrSpaceTransform->setUpdateCallback(new ossimPlanetTraverseCallback());
}

ossimPlanetPointModel::~ossimPlanetPointModel()
{
   if(theLsrSpaceTransform.valid())
   {
      theLsrSpaceTransform->removeCallback(theLsrSpaceCallback.get());
   }
}

void ossimPlanetPointModel::traverse(osg::NodeVisitor& nv)
{
   if(!enableFlag()||!theNode.valid())
   {
      ossimPlanetAnnotationLayerNode::traverse(nv);
      return;
   }
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         setRedrawFlag(false); // let's reset the redraw notification
         if(!theLsrSpaceTransform->model()&&layer()) theLsrSpaceTransform->setModel(layer()->model());
         
 //        if(checkPointers())
         {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePointModelPropertyMutex);
            if(theNodeChangedFlag)
            {
               if(theLsrSpaceTransform->getNumChildren() > 0)
               {
                  theLsrSpaceTransform->removeChildren(0, theLsrSpaceTransform->getNumChildren());
               }
               theLsrSpaceTransform->addChild(theNode.get());
               theNodeChangedFlag = false;
           }
         }
         break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
         break;
      }
      default:
      {
         break;
      }
   }
   
   theLsrSpaceTransform->accept(nv);
   ossimPlanetAnnotationLayerNode::traverse(nv);
}

void ossimPlanetPointModel::setNode(osg::Node* node)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePointModelPropertyMutex);
   setRedrawFlag(true);
  
   theNode = node;
   theNodeChangedFlag = true;
}

void ossimPlanetPointModel::copyLsrSpaceParameters(const ossimPlanetLsrSpaceTransform& lsr)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePointModelPropertyMutex);
   // only need the martrix of the lsr space and all other parameters wil be derived from it.
   theLsrSpaceTransform->copyParametersOnly(lsr);
   setRedrawFlag(true);
}

void ossimPlanetPointModel::setLayer(ossimPlanetLayer* layer)
{
   ossimPlanetAnnotationLayerNode::setLayer(layer);
   if(layer)
   {
      theLsrSpaceTransform->setModel(layer->model());
      setRedrawFlag(true);
   }
}

bool ossimPlanetPointModel::checkPointers()
{
   if(layer())
   {
      if(layer()->model())
      {
         return true;
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN) << "ossimPlanetPointModel::checkPointers() ERROR: model is not found.  Try added the annotation layer to planet and add the PointModelNode to the annotation layer";
      }
   }
   else
   {
      ossimNotify(ossimNotifyLevel_WARN) << "ossimPlanetPointModel::checkPointers() ERROR: parent layer is not found.";
   }
   
   return false;
}

