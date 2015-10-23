#include <osg/Timer>
#include <osg/BoundingSphere>
#include <osg/CoordinateSystemNode>
#include <osgDB/DatabasePager>
#include <osgDB/Registry>
#include <osgUtil/IntersectVisitor>
#include <iostream>
#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetLand.h>
#include <ossim/base/ossimEcefPoint.h>
#include <ossim/base/ossimGpt.h>
#include <ossimPlanet/ossimPlanetLayerRegistry.h>
#include <ossimPlanet/ossimPlanetLookAt.h>
#include <iostream>
// #include <ossimPlanet/ossimPlanetUtility.h>

// #include <ossimPlanet/ossimPlanetLatLonHud.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>

#include <OpenThreads/ScopedLock>
ossimPlanet::LayerListener::LayerListener(ossimPlanet* planet)
:thePlanet(planet)
{
}
void ossimPlanet::LayerListener::needsRedraw(ossimPlanetNode* node)
{
   if(thePlanet)
   {
      thePlanet->setRedrawFlag(true);
      thePlanet->notifyNeedsRedraw(node);
   }
}
void ossimPlanet::LayerListener::setPlanet(ossimPlanet* planet)
{
   thePlanet = planet;
}

class ossimPlanetFinder : public osg::NodeVisitor
{
public:
   ossimPlanetFinder()
      :osg::NodeVisitor(NODE_VISITOR,
                        TRAVERSE_ALL_CHILDREN)
      {
         thePlanet = 0;
      }

   virtual void apply(osg::Node& node)
      {
         if(!thePlanet)
         {
            thePlanet = dynamic_cast<ossimPlanet*>(&node);
         }
         else
         {
            return;
         }
         osg::NodeVisitor::apply(node);
      }
   
   ossimPlanet* thePlanet;
};

class ossimPlanetResetRedrawFlag : public osg::NodeVisitor
{
public:
	ossimPlanetResetRedrawFlag()
	:osg::NodeVisitor(NODE_VISITOR,
							TRAVERSE_ALL_CHILDREN)
	{
		
	}
   virtual void apply(osg::Node& node)
	{
		ossimPlanetLayer* layer = dynamic_cast<ossimPlanetLayer*>(&node);
		if(layer)
		{
			layer->setRedrawFlag(false);
		}
		
		osg::NodeVisitor::apply(node);		
	}
	
};

osg::BoundingSphere ossimPlanet::computeBound() const
{
   if(getNumChildren() == 0)
   {
      return osg::BoundingSphere(osg::Vec3(0.0,0.0,0.0),
                                     1.0);
   }
   return osg::MatrixTransform::computeBound();

}

//bool ossimPlanet::computeBound() const
//{
//  if(getNumChildren() == 0)
//  {
//       _bsphere = osg::BoundingSphere(osg::Vec3(0.0,0.0,0.0),
//                                      1.0);
//    }
//    else
//    {
//       return osg::MatrixTransform::computeBound();
//    }
//    return true;
// }

#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
static ossim_uint32 allocated = 0;
#endif
ossimPlanet::ossimPlanet()
   :osg::MatrixTransform()
{
   theNadirPoint                  = osg::Vec3d(0.0,0.0,ossim::nan());
   theLineOfSitePoint             = osg::Vec3d(0.0,0.0,ossim::nan());
   theNadirLatLonHeightPoint      = osg::Vec3d(0.0,0.0,ossim::nan());
   theLineOfSiteLatLonHeightPoint = osg::Vec3d(0.0,0.0,ossim::nan());
   theLayerListener = new ossimPlanet::LayerListener(this);
   theRedrawFlag = false;
   theComputeIntersectionFlag = true;
   setUpdateCallback(new ossimPlanetTraverseCallback);
   setEventCallback(new ossimPlanetTraverseCallback);
   setCullCallback(new ossimPlanetTraverseCallback);

   theModel = new ossimPlanetNormalizedEllipsoidModel;
   
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   ++allocated;
   std::cout << "ossimPlanet count: " << allocated << std::endl;
#endif
}

ossimPlanet::~ossimPlanet()
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   --allocated;
   std::cout << "ossimPlanet count: " << allocated << std::endl;
#endif

}
void ossimPlanet::setupDefaults()
{
}


void ossimPlanet::traverse(osg::NodeVisitor& nv)
{
//   osg::Timer_t tick = osg::Timer::instance()->tick();
   switch(nv.getVisitorType())
   {
     case osg::NodeVisitor::CULL_VISITOR:
      {
         osg::MatrixTransform::traverse(nv);
         return;
      }
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         // we are starting a redraw for the update,cull,draw sequence so reset the redraw flag for next frame
         theLayersToAddListMutex.lock();
         if(theLayersToAddList.size())
         {
            ossim_uint32 idx = 0;
            for(idx = 0; idx < theLayersToAddList.size();++idx)
            {
               addChild(theLayersToAddList[idx].get());
            }
            theLayersToAddList.clear();
         }
         theLayersToAddListMutex.unlock();
         
         break;
      }
      case osg::NodeVisitor::EVENT_VISITOR:
      {
         break;
      }
      default:
      {
         break;
      }
   }
   osg::MatrixTransform::traverse(nv);
#if 0
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::EVENT_VISITOR:
      {
         std::cout << "event TIME == " 
         << osg::Timer::instance()->delta_m(tick, osg::Timer::instance()->tick()) << std::endl;
         break;
      }
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         std::cout << "update TIME == " 
         << osg::Timer::instance()->delta_m(tick, osg::Timer::instance()->tick()) << std::endl;
         break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
         std::cout << "CULLING TIME == " 
         << osg::Timer::instance()->delta_m(tick, osg::Timer::instance()->tick()) << std::endl;
         break;
      }
   }
#endif
}

const osg::Vec3d& ossimPlanet::getEyePositionLatLonHeight()const
{
   return theEyePositionLatLonHeight;
}

osg::Vec3d ossimPlanet::getNadirPoint()const
{
   return theNadirPoint;
}

osg::Vec3d ossimPlanet::getLineOfSitePoint()const
{
   return theLineOfSitePoint;
}


osg::Vec3d ossimPlanet::getNadirLatLonHeightPoint()const
{
   return theNadirLatLonHeightPoint;
}

osg::Vec3d ossimPlanet::getLineOfSiteLatLonHeightPoint()const
{
   return theLineOfSiteLatLonHeightPoint;
}

const osg::Vec3d& ossimPlanet::hpr()const
{
   return theLsrHeadingPitchRoll;
}

void ossimPlanet::setComputeIntersectionFlag(bool flag)
{
   theComputeIntersectionFlag = flag;
}

bool ossimPlanet::getComputeIntersectionFlag()const
{
   return theComputeIntersectionFlag;
}

void ossimPlanet::computeIntersection(osgUtil::CullVisitor* nv)
{
   theNadirPoint                  = osg::Vec3d(0.0,0.0,ossim::nan());
   theLineOfSitePoint             = osg::Vec3d(0.0,0.0,ossim::nan());
   theNadirLatLonHeightPoint      = osg::Vec3d(0.0,0.0,ossim::nan());
   theLineOfSiteLatLonHeightPoint = osg::Vec3d(0.0,0.0,ossim::nan());
   if(!nv)
   {
      return;
   }
   osg::Node* node = 0;
   if(getNumChildren() > 0)
   {
      node = getChild(0);
   }
   if(node)
   {
      if(theModel.valid())
      {
         osgUtil::IntersectVisitor iv;
         osg::Vec3d eyeDirection = nv->getLookVectorLocal();
         osg::Vec3d eye = nv->getEyePoint();
         osg::Vec3d normal;
         osg::Vec3d eyeLlh;
         theModel->inverse(eye,
                           eyeLlh);
         theModel->normal(eyeLlh,
                          normal);
         normal.normalize();
         osg::Vec3d endPt1 = getBound().center();
         osg::Vec3d endPt2  = eye + eyeDirection*(1/FLT_EPSILON);
         osg::Vec3d startPt3  = eye + normal*(1/FLT_EPSILON);
         osg::ref_ptr<osg::LineSegment> nadirLookVector      = new osg::LineSegment(startPt3, endPt1);
         osg::ref_ptr<osg::LineSegment> lineOfSiteLookVector = new osg::LineSegment(eye, endPt2);
         iv.addLineSegment(nadirLookVector.get());
         iv.addLineSegment(lineOfSiteLookVector.get());
         node->accept(iv);
//          this->accept(iv);
         if(iv.hits())
         {
//             osg::Vec3d eyeLocal = nv->getEyeLocal();

//             theNadirPoint = eyeLocal;
//             land()->model()->inverse(theNadirPoint,
//                                            theNadirLatLonHeightPoint);
            
            osgUtil::IntersectVisitor::HitList& hitList = iv.getHitList(lineOfSiteLookVector.get());
            if (!hitList.empty())
            {
               theLineOfSitePoint = hitList.front().getWorldIntersectPoint();
               theModel->inverse(theLineOfSitePoint,
											theLineOfSiteLatLonHeightPoint);
            }
            osgUtil::IntersectVisitor::HitList& hitListNadir = iv.getHitList(nadirLookVector.get());
            if (!hitListNadir.empty())
            {
               theNadirPoint = hitListNadir.front().getWorldIntersectPoint();
               theModel->inverse(theNadirPoint,
											theNadirLatLonHeightPoint);
            }
         }
      }
   }
}

ossimPlanet* ossimPlanet::findPlanet(osg::Node* startNode)
{
   osg::Node* rootNode = startNode;
   osg::Node* rootNonNullNode = startNode;

   while(rootNode)
   {
      rootNonNullNode = rootNode;
      if(dynamic_cast<ossimPlanet*>(rootNode))
      {
         return (ossimPlanet*)rootNode;
      }
      if(rootNode->getNumParents() > 0)
      {
         rootNode = rootNode->getParent(0);
      }
      else
      {
         rootNode = 0;
      }
   }
   if(rootNonNullNode)
   {
      ossimPlanetFinder finder;
      rootNonNullNode->accept(finder);
      if(finder.thePlanet)
      {
         return finder.thePlanet;
      }
   }

   return 0;
}

ossimPlanet* ossimPlanet::findPlanet(osg::NodePath& currentNodePath)
{
   if(currentNodePath.empty())
   {
      return 0;
   }
   for(osg::NodePath::reverse_iterator itr = currentNodePath.rbegin();
       itr != currentNodePath.rend();
       ++itr)
   {
      ossimPlanet* node = dynamic_cast<ossimPlanet*>(*itr);
      if (node) 
      {
         return node;
      }
   }
   
   return 0;
}

void ossimPlanet::resetAllRedrawFlags()
{
	ossimPlanetResetRedrawFlag	resetRedraw;
	setRedrawFlag(false);
	accept(resetRedraw);
}


void ossimPlanet::execute(const ossimPlanetAction& action)
{
   const ossimPlanetXmlAction* xmlAction = action.toXmlAction();
	
	if(xmlAction)
	{
		xmlExecute(*xmlAction);
      if(action.command() == "Add")
      {
         
      }
	}
}

void ossimPlanet::xmlExecute(const ossimPlanetXmlAction& action)
{
   if(action.command() == "Add")
   {
   }
}

bool ossimPlanet::removeChildren(unsigned int pos,unsigned int numChildrenToRemove)
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
      if(theBlockCallbacksFlag)
      {
         return osg::MatrixTransform::removeChildren(pos, numChildrenToRemove);
      }
   }
   
   if (pos<_children.size() && numChildrenToRemove>0)
   {
      unsigned int endOfRemoveRange = pos+numChildrenToRemove;
      if (endOfRemoveRange>_children.size())
      {
         endOfRemoveRange=_children.size();
      }
      ossim_uint32 idx = 0;
      for(idx = 0; idx < endOfRemoveRange;++idx)
      {
         ossimPlanetLayer* layer = dynamic_cast<ossimPlanetLayer*>(_children[idx].get());
         if(layer)
         {
            notifyLayerRemoved(layer);
         }
      }
      return osg::MatrixTransform::removeChildren(pos, numChildrenToRemove);
   }
   else return false;
}

void ossimPlanet::childInserted(unsigned int pos)
{
   ossimPlanetLayer* n = dynamic_cast<ossimPlanetLayer*>(getChild(pos));
//   if(!theLand.valid())
//   {
//      ossimPlanetLand* land = dynamic_cast<ossimPlanetLand*>(n);
//      if(land)
//      {
//         theLand = land;
//		   theModel = theLand->model();
//      }
//   }
   {
      if(theBlockCallbacksFlag) return;
   }
   if(n)
   {
      n->setPlanet(this);
      n->setModel(model().get());
      setRedrawFlag(true);
      n->addCallback(theLayerListener.get());
      notifyLayerAdded(n);
   }
}

void ossimPlanet::childRemoved(unsigned int /*pos*/, unsigned int /*numChildrenToRemove*/)
{
	setRedrawFlag(true);
}

void ossimPlanet::notifyLayerAdded(ossimPlanetLayer* layer)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;

   ossim_uint32 idx = 0;
   for(idx = 0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->nodeAdded(layer);
      }
   }
}

void ossimPlanet::notifyLayerRemoved(ossimPlanetLayer* layer)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->nodeRemoved(layer);
      }
   }
   
}

void ossimPlanet::notifyNeedsRedraw(ossimPlanetNode* node)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx = 0;
   ossim_uint32 upper = theCallbackList.size();
   
   for(idx = 0; idx < upper; ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->needsRedraw(node); 
      }
   }   
}

