#include <ossimPlanet/ossimPlanetLandCullCallback.h>
#include <ossimPlanet/ossimPlanetPagedLandLod.h>
#include <ossimPlanet/ossimPlanetDatabasePager.h>
#include <osgUtil/CullVisitor>
#include <osgUtil/IntersectVisitor>
#include <ossim/base/ossimDrect.h>
#include <osg/LineSegment>

ossimPlanetLandCullCallback::ossimPlanetLandCullCallback()
   :osg::NodeCallback(),
    theFreezeRequestFlag(false),
    theCullingFlag(true),
    theSplitMetric(3.0),
    theSplitPriorityType(ossimPlanetLandPriorityType_LINE_OF_SITE_INTERSECTION),
    theLineOfSiteValidFlag(false)
{
}

void ossimPlanetLandCullCallback::setLineOfSite(const osg::Vec3d& lineOfSite)
{
   theLineOfSite = lineOfSite;
   theLineOfSiteValidFlag = false;
   if((!ossim::isnan(theLineOfSite[0]))&&
      (!ossim::isnan(theLineOfSite[1]))&&
      (!ossim::isnan(theLineOfSite[2])))
   {
      theLineOfSiteValidFlag = true;
   }
}

void ossimPlanetLandCullCallback::setLineOfSiteValidFlag(bool flag)
{
   theLineOfSiteValidFlag = flag;
}

bool ossimPlanetLandCullCallback::isLineOfSiteValid()const
{
   return theLineOfSiteValidFlag;
}

void ossimPlanetLandCullCallback::setSplitMetricRatio(double ratio)
{
   theSplitMetric = ratio;
}

double ossimPlanetLandCullCallback::getSplitMetricRatio()const
{
   return theSplitMetric;
}

void ossimPlanetLandCullCallback::setSplitPriorityType(ossimPlanetPriorityType priorityType)
{
   theSplitPriorityType = priorityType;
}

ossimPlanetPriorityType ossimPlanetLandCullCallback::getSplitPriorityType()const
{
   return theSplitPriorityType;
}

void ossimPlanetLandCullCallback::setCullingFlag(bool flag)
{
   theCullingFlag = flag;
}

bool ossimPlanetLandCullCallback::getCullingFlag()const
{
   return theCullingFlag;
}

void ossimPlanetLandCullCallback::setFreezeRequestFlag(bool flag)
{
   theFreezeRequestFlag = flag;
}

bool ossimPlanetLandCullCallback::getFreezRequestFlag()const
{
   return theFreezeRequestFlag;
}

void ossimPlanetLandCullCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
   ossimPlanetPagedLandLod* n = dynamic_cast<ossimPlanetPagedLandLod*>(node);
   if(n)
   {
      applyStandardCull(n, nv);
   }   
}

void ossimPlanetLandCullCallback::applyStandardCull(ossimPlanetPagedLandLod* n, osg::NodeVisitor* nv)
{
   osgUtil::CullVisitor* cullVisitor = dynamic_cast<osgUtil::CullVisitor*>(nv);
   if(n->theRefreshType == ossimPlanetLandRefreshType_PRUNE)
   {
      if(n->getNumChildren() > 0)
      {
         if(n->getChild(0))
         {
            n->getChild(0)->accept(*nv);
         }
      }

      return;
   }
   if(!cullVisitor)
   {
      if(n->isLeaf())
      {
         n->getChild(0)->accept(*nv);
      }
      else
      {
         traverse(n, nv);
      }
      return;
   }
	const osg::BoundingSphere& bs = n->getBound();
   bool canCull = true;
//    bool canCull = n->theLevel >0;//theCullingFlag;
   n->theRemoveChildrenFlag = false;
   bool addChildrenFlag = false;
   double ratio    = theSplitMetric;

   // let's always keep level 0.
   //
//    if(n->theLevel < 1) ratio = 99999.0;
   
   n->theRemoveChildrenFlag = false;
   osg::Vec3 real_eye = cullVisitor->getEyeLocal();

   double fov_y, aspect_ratio, z_near, z_far;
   cullVisitor->getProjectionMatrix()->getPerspective(fov_y, aspect_ratio, z_near, z_far );
   
   osg::Vec3d apparent_eye = real_eye;
   // osg::Vec3d down_eye     = real_eye;

   if( theLineOfSiteValidFlag )
   {
        osg::Vec3d look_vector = theLineOfSite - real_eye;

        double opposite = look_vector.length() * tan( osg::DegreesToRadians(fov_y/2.0) );
        double camera_dist = opposite / tan( osg::DegreesToRadians(50.0/2.0) );

        apparent_eye = theLineOfSite - (look_vector * (camera_dist / look_vector.length()));
   }

    double radius = bs.radius();
//    double radius   = (n->theCullNode->boundingBox()->radius());
//    double distance = n->theCullNode->eyeDistance();
    double real_distance = (real_eye-bs.center()).length();
    double apparent_distance = (apparent_eye-bs.center()).length();
    double distance = min( apparent_distance, real_distance ); // this will keep around tiles close to the eye

//    double pixelSize = cullVisitor->clampedPixelSize(bs);
    n->theCulledFlag = false;
   if(canCull)
   {
      n->theCullNode->accept(*nv);
      n->theCulledFlag = n->theCullNode->isCulled();
   }
   
   if((distance < radius*ratio))
//    if(pixelSize >= 512)
   {
      if(n->getNumChildren() < 5)
      {
         addChildrenFlag  = true;
      }
   }
    else if( distance > 2*radius*ratio )
//    else if(pixelSize < 256)
   {
      n->theRemoveChildrenFlag = true;
   }
   if(addChildrenFlag && ((n->theLevel+1)>n->theMaxLevel))
   {
      addChildrenFlag = false;
   }
   double     priority = 0.0;
   osg::Vec3d priorityPoint = apparent_eye;

   
   if((theSplitPriorityType==ossimPlanetLandPriorityType_LINE_OF_SITE_INTERSECTION)&&theLineOfSiteValidFlag)
   {
      priorityPoint = theLineOfSite;
   }
   
   priority = (priorityPoint-bs.center()).length();
   priority *= 1<<n->theLevel;
   if(priority > 0.0)
   {
      priority = 1.0/priority;
   }
   else
   {
      priority = 1.0/FLT_EPSILON;
   }
   ossimString postFixRefreshType = "";
   if( (n->theRefreshType&ossimPlanetLandRefreshType_GEOM)&&
       (n->theRefreshType&ossimPlanetLandRefreshType_TEXTURE))
   {
      postFixRefreshType = "_GEOM_TEXTURE";
   }
   else if(n->theRefreshType&ossimPlanetLandRefreshType_GEOM)
   {
      postFixRefreshType = "_GEOM";
   }
   else if(n->theRefreshType&ossimPlanetLandRefreshType_TEXTURE)
   {
      postFixRefreshType = "_TEXTURE";
      
   }
   if(!postFixRefreshType.empty()&&!theFreezeRequestFlag)
   {
      if(n->isLeaf())
      {
         cullVisitor->getDatabaseRequestHandler()->requestNodeFile(n->theRequestNameList[0]+postFixRefreshType,
                                                                   cullVisitor->getNodePath(),
                                                                   9999999*priority*((std::pow(2.0, (double)n->theLevel))),
                                                                   nv->getFrameStamp(),
                                                                   n->theRequestRef,
                                                                   0);
      }
      else
      {
         cullVisitor->getDatabaseRequestHandler()->requestNodeFile(n->theRequestNameList[0]+postFixRefreshType,
                                                                   cullVisitor->getNodePath(),
                                                                   priority*((std::pow(2.0, (double)n->theLevel))),
                                                                   nv->getFrameStamp(),
                                                                   n->theRequestRef,
                                                                   0);
      }
      addChildrenFlag = false;
      
   }

   if(addChildrenFlag&&!n->theCulledFlag&&!n->theRemoveChildrenFlag)
   {
      ossim_uint32 idx = 0;
      
      OpenThreads::ScopedLock<OpenThreads::Mutex> lockChildList(n->theChildCullNodeListMutex);
      ossim_uint32 cullCount = 0;
      for(idx = 0; ((idx < n->theChildCullNodeList.size())&&n->theChildCullNodeList[idx].valid()); ++idx)
      {
         n->theChildCullNodeList[idx]->accept(*cullVisitor);

         if(n->theChildCullNodeList[idx]->isCulled())
         {
            ++cullCount;
         }
      }
      if(idx >= n->theChildCullNodeList.size())
      {
         addChildrenFlag = cullCount != n->theChildCullNodeList.size();
      }
   }
   int childIdx = n->thePagedLodList.size();
   std::string requestString = ((n->getNumChildren() < 5)&&(n->thePagedLodList.size()<4))?n->theRequestNameList[childIdx+1]:"";
   if(!theFreezeRequestFlag&&
      (requestString!= "")&&
      cullVisitor->getDatabaseRequestHandler())
   {
      if(!n->theRemoveChildrenFlag &&
         addChildrenFlag&&
         !n->theCulledFlag&&
         (n->getNumChildren() < 5))
      {
         cullVisitor->getDatabaseRequestHandler()->requestNodeFile(requestString,
                                                                   cullVisitor->getNodePath(),
                                                                   priority,
                                                                   nv->getFrameStamp(),
                                                                   n->theRequestRefChildList[childIdx]);
      }
   }
   if(!n->isLeaf())
   {
      ossim_uint32 idx = 0;
      for(idx = 1; idx < n->getNumChildren(); ++idx)
      {
         if(n->getChild(idx))
         {
            n->getChild(idx)->accept(*nv);
         }
      }
//       if(n->areAllChildrenCulled(true)&&!n->theCulledFlag)
//       {
//          if(n->getChild(0))
//          {
//             n->getChild(0)->accept(*nv);
//          }
//       }
   }
   else
   {
      if(n->getNumChildren() > 0)
      {
         if(n->getChild(0))
         {
            if(!n->theCulledFlag)
            {
               n->getChild(0)->accept(*nv);
            }
         }
      }
   }
}

#if 0
void ossimPlanetLandCullCallback::applyOrthoCull(ossimPlanetPagedLandLod* n,
                                                 osg::NodeVisitor* nv)
{
   //OpenThreads::ScopedLock<OpenThreads::Mutex> lock(n->theMutex);

   osgUtil::CullVisitor* cullVisitor = dynamic_cast<osgUtil::CullVisitor*>(nv);
   if(n->theRefreshType == ossimPlanetLandRefreshType_PRUNE)
   {
      if(n->getNumChildren() > 0)
      {
         if(n->getChild(0))
         {
            n->getChild(0)->accept(*nv);
         }
      }
      cullVisitor->getDatabaseRequestHandler()->requestNodeFile(n->theRequestNameList[0]+"_GEOM",
                                                                n,
                                                                99999999,
                                                                nv->getFrameStamp());
      return;
   }
   if(!cullVisitor) return;
   n->theCulledFlag = false;
   bool addChildrenFlag = false;
   n->theRemoveChildrenFlag = false;
   double distance=0.0;
   osg::Matrix wm = cullVisitor->getWindowMatrix();

   const osg::RefMatrix& m = *(cullVisitor->getMVPW());
   double x = cullVisitor->getViewport()->x();
   double y = cullVisitor->getViewport()->y();
   double w = cullVisitor->getViewport()->width();
   double h = cullVisitor->getViewport()->height();
   if(w < 1) w = 1;
   if(h < 1) h = 1;
   ossimDrect viewportRect(x,
                           y,
                           x + (w-1),
                           y + (h-1));
   wm = osg::Matrixd::inverse(*cullVisitor->getProjectionMatrix());

   osg::Vec3d center = (osg::Vec3d(0.0,0.0, 1.0)*wm);
   osg::Vec3d projectedCenter = (n->theCenterPoint*m);
   distance = (center-n->getBound().center()).length();
//    distance = (center-n->getBound().center()).length()/(sqrt(180.0*180.0 + 90.0*90.0));
//    
//    distance = (osg::Vec2d(projectedCenter[0],
//                           projectedCenter[1]) -
//                osg::Vec2d(w/2.0,
//                           h/2.0)).length();
   
   osg::Vec3d ulView = n->theUlPoint*m;
   osg::Vec3d urView = n->theUrPoint*m;
   osg::Vec3d lrView = n->theLrPoint*m;
   osg::Vec3d llView = n->theLlPoint*m;
   ossimDpt ulViewDpt(ulView[0], ulView[1]);
   ossimDpt urViewDpt(urView[0], urView[1]);
   ossimDpt lrViewDpt(lrView[0], lrView[1]);
   ossimDpt llViewDpt(llView[0], llView[1]);
   ossimDrect rect(ulViewDpt,
                   urViewDpt,
                   lrViewDpt,
                   llViewDpt);
   
   n->theCulledFlag = !rect.intersects(viewportRect);
   
   float pixelSize = ossim::max(rect.width(), rect.height());
   if(pixelSize >= 512)
   {
      if(n->isLeaf())
      {
         addChildrenFlag = true;
      }
   }
   else if(pixelSize < 256)
   {
//       if(!n->isLeaf())
      {
         n->theRemoveChildrenFlag = true;
      }
   }
   
   distance *= 1<<n->theLevel;
   double priority = 0.0;
   if(distance > 0.0)
   {
      priority = 1.0/distance;
   }
   else
   {
      priority = 1.0/FLT_EPSILON;
   }
   if((n->theRefreshType == ossimPlanetLandRefreshType_GEOM)&&(!n->theCulledFlag))
   {
      cullVisitor->getDatabaseRequestHandler()->requestNodeFile(n->theRequestNameList[0]+"_GEOM",
                                                                n,
                                                                priority,
                                                                nv->getFrameStamp());
   }
   else if((n->theRefreshType == ossimPlanetLandRefreshType_TEXTURE)&&(!n->theCulledFlag))
   {
      cullVisitor->getDatabaseRequestHandler()->requestNodeFile(n->theRequestNameList[0]+"_TEXTURE",
                                                                n,
                                                                priority,
                                                                nv->getFrameStamp());
   }
   std::string requestString = ((n->getNumChildren() < 5)&&(n->thePagedLodList.size()<4))?n->theRequestNameList[n->thePagedLodList.size()+1]:"";
//   std::string requestString = (n->getNumChildren() < 5)?n->getRequestName(n->getNumChildren()):"";
   if((requestString!= "")&&
      cullVisitor->getDatabaseRequestHandler())
   {
      if((n->theRefreshType == ossimPlanetLandRefreshType_GEOM)&&(!n->theCulledFlag))
      {
         cullVisitor->getDatabaseRequestHandler()->requestNodeFile(n->theRequestNameList[0]+"_GEOM",
                                                                   n,
                                                                   priority,
                                                                   nv->getFrameStamp());
      }
      else if((n->theRefreshType == ossimPlanetLandRefreshType_TEXTURE)&&(!n->theCulledFlag))
      {
         cullVisitor->getDatabaseRequestHandler()->requestNodeFile(n->theRequestNameList[0]+"_TEXTURE",
                                                                   n,
                                                                   priority,
                                                                   nv->getFrameStamp());
      }
      if(!n->theRemoveChildrenFlag &&
         addChildrenFlag&&
         !n->theCulledFlag&&
         (n->getNumChildren() < 5))
      {
         cullVisitor->getDatabaseRequestHandler()->requestNodeFile(requestString,
                                                                   n,
                                                                   priority,
                                                                   nv->getFrameStamp());
      }
      else if((n->theCulledFlag)&&
              (n->isLeaf())&&
              (n->getNumChildren()>1))
      {
			ossimPlanetDatabasePager* pager = dynamic_cast<ossimPlanetDatabasePager*>(cullVisitor->getDatabaseRequestHandler());
          if(pager)
          {
             pager->invalidateRequest(requestString);
          }
      }
   }
   if(!n->theCulledFlag)
   {
      if(n->getNumChildren() >4)
      {
         ossim_uint32 idx = 0;
         for(idx = 1; idx < n->getNumChildren(); ++idx)
         {
            if(n->getChild(idx))
            {
               n->getChild(idx)->accept(*nv);
            }
         }
      }
      else
      {
         if(n->getNumChildren() > 0)
         {
            if(n->getChild(0))
            {
               n->getChild(0)->accept(*nv);
            }
         }
      }
   }
}
#endif

