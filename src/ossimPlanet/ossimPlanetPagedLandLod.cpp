#include <ossimPlanet/ossimPlanetPagedLandLod.h>
#include <ossimPlanet/ossimPlanetLand.h>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <osg/Geode>
//#include <ossimPlanet/ossimPlanetDatabasePager.h>
#include <osgUtil/CullVisitor>

//#define OSGPLANET_ENABLE_ALLOCATION_COUNT
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
static ossim_uint32 allocated = 0;
#endif

void ossimPlanetPagedLandLodCullNode::traverse(osg::NodeVisitor& nv)
{
   osgUtil::CullVisitor* cullVisitor = dynamic_cast<osgUtil::CullVisitor*>(&nv);
   theCulledFlag = false;
   if(!cullVisitor||!theBoundingBox.valid()) return;

   
   osg::Vec3d eye = cullVisitor->getEyeLocal();
   osg::Vec3d eyeDirection = cullVisitor->getLookVectorLocal();
   const osg::Polytope& frustum = cullVisitor->getCurrentCullingSet().getFrustum();
   theEyeDistance = (eye-theBoundingBox->center()).length();
   if(theUseClusterCulling)
   {
      if(theClusterCullingDeviation >= -1.0)
      {
         osg::Vec3d eyeCp = eye - theClusterCullingControlPoint;
         double radius    = eyeCp.length();
         
         if (radius>=theClusterCullingRadius)
         {
            
            double deviation = (eyeCp * theClusterCullingNormal)/radius;
            
            
            theCulledFlag = (deviation < theClusterCullingDeviation);
         }
      }
   }
   if(!theCulledFlag)
   {
      if(theBoundingBox->isInFront(eye, eyeDirection))
      {
         theCulledFlag = !theBoundingBox->intersects(frustum);
      }
      else
      {
         theCulledFlag = true;
      }
   }
}

class ossimPlanetPagedLandLodTextureVisitor : public osg::NodeVisitor
{
public:
   ossimPlanetPagedLandLodTextureVisitor(osg::ref_ptr<ossimPlanetLandTextureRequest> tex,
                                       ossimPlanetPagedLandLod* lod)
      :osg::NodeVisitor(NODE_VISITOR,
                        TRAVERSE_ALL_CHILDREN),
       theRequest(tex),
       theLod(lod),
       theHasTextureFlag(false)
      {
      }
   virtual void apply(osg::Geode& node)
      {
         theHasTextureFlag = false;
         if(node.getNumDrawables() > 0)
         {
            osg::Geometry* geom = (node.getDrawable(0)->asGeometry());
            if(geom)
            {
               osg::StateSet* dstate = geom->getOrCreateStateSet();
               if(dstate)
               {
                  ossim_uint32 size = dstate->getTextureAttributeList().size();
                  ossim_uint32 idx = 0;
                  // first remove all texture states befoe adding the new states.
                  for(idx = 0; idx < size; ++idx)
                  {
                     dstate->removeTextureAttribute(idx,
                                                    osg::StateAttribute::TEXTURE);
                  }
                  std::vector<osg::ref_ptr<osg::Texture2D> >& textures = theRequest->getTextures();
                  if(textures.size())
                  {
                     for(idx = 0;idx < textures.size(); ++idx)
                     {
                        dstate->setTextureAttributeAndModes(idx, textures[idx].get(),
                                                            osg::StateAttribute::ON);
                        theHasTextureFlag = true;
                     }
                  }
                  else
                  {
                     theHasTextureFlag = false;
                     
                  }
               }
            }
         }
      }
   
   bool hasTexture()const
      {
         return theHasTextureFlag;
      }
protected:   
   osg::ref_ptr<ossimPlanetLandTextureRequest> theRequest;
   ossimPlanetPagedLandLod* theLod;
   bool theHasTextureFlag;
};

class ossimPlanetPagedLandLodUpdateCallback : public osg::NodeCallback
{
public:
   virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
      {
         ossimPlanetPagedLandLod* n = dynamic_cast<ossimPlanetPagedLandLod*>(node);
         if(n)
         {
           // ossimPlanetDatabasePager* pager = dynamic_cast<ossimPlanetDatabasePager*>(nv->getDatabaseRequestHandler());
            //OpenThreads::ScopedLock<OpenThreads::Mutex> lock(n->theMutex);
            if(n->theRefreshType == ossimPlanetLandRefreshType_TEXTURE)
            {
               OpenThreads::ScopedLock<OpenThreads::Mutex> lock(n->thePagedLodListMutex);
               n->thePagedLodList.clear();
            }
            if(n->getNumChildren() > 0)
            {
               OpenThreads::ScopedLock<OpenThreads::Mutex> lock(n->theTextureRequestMutex);
               ossimPlanetPagedLandLodTextureVisitor visitor(n->theTextureRequest, n);
               if(n->theTextureRequest.valid())
               {
                  if(n->theTextureRequest->getCullCallback().valid())
                  {
                     n->setCullCallback(n->theTextureRequest->getCullCallback().get());
                  }
                  if(n->theTextureRequest->getTransform().valid())
                  {
                     n->setChild(0, n->theTextureRequest->getTransform().get());
                     n->theCulledFlag = false;
                     n->theRemoveChildrenFlag = false;
                     n->theCenterPoint = n->theTextureRequest->centerPoint();
                     n->theUlPoint = n->theTextureRequest->ulPoint();
                     n->theUrPoint = n->theTextureRequest->urPoint();
                     n->theLrPoint = n->theTextureRequest->lrPoint();
                     n->theLlPoint = n->theTextureRequest->llPoint();
                     n->theCenterNormal = n->theTextureRequest->centerNormal();
                     n->theUlNormal = n->theTextureRequest->ulNormal();
                     n->theUrNormal = n->theTextureRequest->urNormal();
                     n->theLrNormal = n->theTextureRequest->lrNormal();
                     n->theLlNormal = n->theTextureRequest->llNormal();
//                      if(n->theTextureRequest->boundingBox().valid())
//                      {
//                         n->theCullNode->setBoundingBox(n->theTextureRequest->boundingBox().get());
//                      }
                  }
                  
                  n->getChild(0)->accept(visitor);
                  if(n->theTextureRequest->getTextureState() == ossimPlanetImage::ossimPlanetImageStateType_NEEDS_LOADING)
                  {
                     if(n->theRefreshType != ossimPlanetLandRefreshType_PRUNE)
                     {
                        n->theRefreshType = ossimPlanetLandRefreshType_TEXTURE;
                     }
                  }
                  else
                  {
                     n->theRefreshType = ossimPlanetLandRefreshType_NONE;
                  }
                  n->dirtyBound();
                  n->getBound();
                  n->theTextureRequest = 0;
                  ossimPlanetLand* land = n->landLayer();
                  if(land)
                  {
                     land->pagedLodModified(n);
                  }
              }
            }
            ossim_uint32 numChildren = n->getNumChildren();
            ossimPlanetLand* land = n->landLayer();
            if(n->theRemoveChildrenFlag||
               n->theCulledFlag||
               n->areAllChildrenCulled()||
               (n->theRefreshType == ossimPlanetLandRefreshType_PRUNE))
            {
               OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(n->thePagedLodListMutex);
               ossim_uint32 idx = 0;
               if(numChildren > 1)
               {
                  for(idx = 1; idx < numChildren;++idx)
                  {
                     if(n->getChild(idx))
                     {
                        if(land)
                        {
                           land->pagedLodRemoved(n->getChild(idx));
                        }
#if 0
                        if(pager)
                        {
                           pager->addToDeleteList(n->getChild(idx));
                        }
#endif
                        //osg::Group* grp = dynamic_cast<osg::Group*>(n->getChild(idx));
                        //if( grp ) pager->removeRequest( grp );
                     }
                  }
                  
                  n->removeChild(1, numChildren-1);
               }

               n->thePagedLodList.clear();
               if(numChildren > 0)
               {
                  n->getChild(0)->accept(*nv);
               }
               if(n->theRefreshType == ossimPlanetLandRefreshType_PRUNE)
               {
                  n->theRefreshType = ossimPlanetLandRefreshType_NONE;
               }
               return;
            }
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(n->thePagedLodListMutex);
            if(n->thePagedLodList.size()==4)
            {
               if(n->getNumChildren() == 1) 
               {
                  for( unsigned int i = 0 ; i < n->thePagedLodList.size(); i++ )
                  {
                     n->insertChild(n->getNumChildren(), n->thePagedLodList[i].get());
                  }
                  
                  if(n->getNumChildren() == 5)
                  {
                     n->thePagedLodList.clear();
                  }
               }
               else
               {
                  n->thePagedLodList.clear();
               }
            }
            ossim_uint32 idx =0;
            for(idx = 0; idx < n->thePagedLodList.size(); ++idx)
            {
               n->thePagedLodList[idx]->accept(*nv);
            }
         }
		 
         traverse(node, nv);
      }
};

ossimPlanetPagedLandLod::ossimPlanetPagedLandLod(ossim_uint32 level,
                                                 ossim_uint32 row,
                                                 ossim_uint32 col,
                                                 const std::string& requestName)
   :
   theGeode(0),
   theLevel(level),
   theRow(row),
   theCol(col),
   theRequestNameList(5),
   theCulledFlag(false),
   theRemoveChildrenFlag(false),
   theMaxLevel(99999999),
   theChildCullNodeList(4)

{
   theRefreshType = ossimPlanetLandRefreshType_NONE;
   theRequestNameList[0] = requestName;
   setUpdateCallback( new ossimPlanetPagedLandLodUpdateCallback );
   theCullNode = new ossimPlanetPagedLandLodCullNode();
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   ++allocated;
   std::cout << "ossimPlanetPagedLandLod count: " << allocated << std::endl;
#endif
}

ossimPlanetPagedLandLod::~ossimPlanetPagedLandLod()
{
   theGeode = 0;

#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   --allocated;
   std::cout << "ossimPlanetPagedLandLod count: " << allocated << std::endl;
#endif
}

ossimPlanetPagedLandLod::ossimPlanetPagedLandLod(const ossimPlanetPagedLandLod& src,
                                             const osg::CopyOp& copyop)
   :osg::Group(src, copyop),
    theLevel(src.theLevel),
    theRow(src.theRow), 
    theCol(src.theCol),
    theRequestNameList(src.theRequestNameList),
    theCulledFlag(src.theCulledFlag),
    theRemoveChildrenFlag(src.theRemoveChildrenFlag),
    theChildCullNodeList(4)
{
}

bool ossimPlanetPagedLandLod::areAllChildrenLeaves()const
{
   if(getNumChildren()!=5) return false;
   ossim_uint32 idx;
   ossim_uint32 bounds = getNumChildren();
   for(idx = 1; idx < bounds; ++idx)
   {
      const ossimPlanetPagedLandLod* lod = dynamic_cast<const ossimPlanetPagedLandLod*>(getChild(idx));
      if(lod)
      {
         if(!lod->isLeaf()) return false;
      }
   }
   
   return true;
}

bool ossimPlanetPagedLandLod::hasCulledChildren()const
{
   if(getNumChildren()<2) return false;
   ossim_uint32 idx = 0;
   ossim_uint32 bounds = getNumChildren();
   for(idx = 1; idx < bounds; ++idx)
   {
      const ossimPlanetPagedLandLod* lod = dynamic_cast<const ossimPlanetPagedLandLod*>(getChild(idx));
      if(lod)
      {
         if(lod->getCulledFlag()) return true;
      }
   }

   return false;   
}

bool ossimPlanetPagedLandLod::areAllChildrenCulled(bool applyToAddedChildrenOnly)const
{
   if(applyToAddedChildrenOnly)
   {
      if(getNumChildren()!=5) return false;
   }
   if(getNumChildren()==1) return false;
   ossim_uint32 idx = 0;
   ossim_uint32 bounds = getNumChildren();
   for(idx = 1; idx < bounds; ++idx)
   {
      const ossimPlanetPagedLandLod* lod = dynamic_cast<const ossimPlanetPagedLandLod*>(getChild(idx));
      if(lod)
      {
         if(!lod->getCulledFlag()) return false;
      }
   }

   return true;
}

bool ossimPlanetPagedLandLod::addChild( Node *child )
{
//   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   if(!child) return false;
	
   ossimPlanetPagedLandLod* lod = dynamic_cast<ossimPlanetPagedLandLod*>(child);
   ossimPlanetLandTextureRequest *request = dynamic_cast<ossimPlanetLandTextureRequest*>(child);
   bool result = false;
   if(lod)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePagedLodListMutex);
      if(getNumChildren() >4)
      {
         return false;
      }
	     
      if(thePagedLodList.size() < 4)
      {
         
         if(lod->theRequestNameList[0] != theRequestNameList[thePagedLodList.size()+1])
         {
            return false;
         }
      }
      else
      {
         return false;
      }
      
      ossim_uint32 idx = thePagedLodList.size();
      thePagedLodList.push_back(lod);
      OpenThreads::ScopedLock<OpenThreads::Mutex> lockChildList(theChildCullNodeListMutex);
      theChildCullNodeList[idx] = new ossimPlanetPagedLandLodCullNode(*lod->theCullNode);
      ossimPlanetLand* land = landLayer();
      if(land)
      {
         land->pagedLodAdded(this, theChildCullNodeList[idx].get());
      }
      return true;
   }
   else if(request)
   {
      if((request->getLevel() == theLevel)&&
         (request->getRow()   == theRow)&&
         (request->getCol()   == theCol))
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTextureRequestMutex);
         theTextureRequest = request;
         ossimPlanetLand* land = landLayer();
         if(land)
         {
            land->pagedLodModified(this);
         }
        
         return true;
      }
      return false;
   }
   result = Group::addChild(child);
   
   return result;
}

void ossimPlanetPagedLandLod::traverse(osg::NodeVisitor& nv)
{  
   switch(nv.getTraversalMode())
   {
      case osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN:
      {
         if(isLeaf())
         {
            if(getChild(0))
            {
               getChild(0)->accept(nv);
            }
         }
         else
         {
            ossim_uint32 idx = 0;
            for(idx = 1; idx < getNumChildren(); ++idx)
            {
               getChild(idx)->accept(nv);
            }
         }
         break;
      }
      default:
      {
         Group::traverse(nv);
         break;
      }
  }
}

void ossimPlanetPagedLandLod::setRefreshType(ossimPlanetLandRefreshType refreshType)
{
   theRefreshType = refreshType;
	if(theRefreshType != ossimPlanetLandRefreshType_NONE)
	{
		if(landLayer()) landLayer()->setRedrawFlag(true);
		theTextureRequest = 0;
	}
}

ossimPlanetLandRefreshType ossimPlanetPagedLandLod::refreshType()const
{
   return theRefreshType;
}



ossimPlanetLand* ossimPlanetPagedLandLod::landLayer()
{
   osg::Node* parentNode = this;
   while(parentNode->getNumParents())
   {
      parentNode = parentNode->getParent(0);
      ossimPlanetLand* landPtr = dynamic_cast<ossimPlanetLand*>(parentNode);
      if(landPtr)
      {
         return landPtr;
      }
   }
   
   return 0;
}

const ossimPlanetLand* ossimPlanetPagedLandLod::landLayer()const
{
   const osg::Node* parentNode = this;
   while(parentNode->getNumParents())
   {
      parentNode = parentNode->getParent(0);
      const ossimPlanetLand* landPtr = dynamic_cast<const ossimPlanetLand*>(parentNode);
      if(landPtr)
      {
         return landPtr;
      }
   }while(parentNode);

   return 0;
}

