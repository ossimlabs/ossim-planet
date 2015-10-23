#include <ossimPlanet/ossimPlanetTerrainTechnique.h>
#include <ossimPlanet/ossimPlanetTerrainTile.h>
#include <queue>
#include <osg/Timer>
#include <osg/Geometry>
#if ((OSG_VERSION_INT)>182)
#define USE_OLD_VBO_COMPILE 0
#else
#define USE_OLD_VBO_COMPILE 1
#endif

void ossimPlanetTerrainTechnique::CompileObjects::apply(osg::Drawable& drawable)
{
   osg::Geometry* geometry = drawable.asGeometry();
   if(geometry)
   { 
      osg::Geometry::ArrayList arrayList;
      geometry->getArrayList(arrayList);
      ossim_uint32 idx = 0;
      for(idx = 0; idx < arrayList.size();++idx)
      {
#if 0
         if(arrayList[idx]->getVertexBufferObject())
         {
            arrayList[idx]->getVertexBufferObject()->compileBuffer(*_renderInfo.getState());
         }
#endif
//         if(arrayList[idx]->getVertexBufferObject())
//         {
//            osg::GLBufferObject* glBufferObj = arrayList[idx]->getVertexBufferObject()->getOrCreateGLBufferObject(_renderInfo.getState()->getContextID());
//            if(glBufferObj&&glBufferObj->isDirty())
//            {
//               glBufferObj->compileBuffer();
//            }
     }
 //     }
   }
   osgUtil::GLObjectsVisitor::apply(drawable);
}

void ossimPlanetTerrainTechnique::CompileObjects::apply(osg::Node& node)
{
   if (node.getStateSet())
   {
      apply(*(node.getStateSet()));
   }
   
   traverse(node);
}
void ossimPlanetTerrainTechnique::CompileObjects::apply(osg::Geode& node)
{
   if (node.getStateSet())
   {
      apply(*(node.getStateSet()));
   }
   for(unsigned int i=0;i<node.getNumDrawables();++i)
   {
      osg::Drawable* drawable = node.getDrawable(i);
      if (drawable)
      {
         apply(*drawable);
         if (drawable->getStateSet())
         {
            apply(*(drawable->getStateSet()));
         }
      }
   }
   
   // now let's compile all buffer objects
   //
   
}

void ossimPlanetTerrainTechnique::CompileObjects::apply(osg::StateSet& stateset)
{
   if (_stateSetAppliedSet.count(&stateset)!=0) return;
   _stateSetAppliedSet.insert(&stateset);
   
   if (_mode & COMPILE_STATE_ATTRIBUTES && _renderInfo.getState())
   {
      osg::StateSet::AttributeList& attributeList = stateset.getAttributeList();
      for(osg::StateSet::AttributeList::const_iterator itr = attributeList.begin();
          itr!=attributeList.end();
          ++itr)
      {
         itr->second.first->compileGLObjects(*_renderInfo.getState());
      }
      
      for (unsigned i = 0;
           i < stateset.getTextureAttributeList().size();
           ++i)
      {
         const osg::Texture* texture
         = dynamic_cast<const osg::Texture*>(stateset.getTextureAttribute(i,osg::StateAttribute::TEXTURE));
         if (texture && (texture->getTextureParameterDirty(_renderInfo.getContextID())||(texture->getTextureObject(_renderInfo.getContextID())==0)))
         {
            texture->compileGLObjects(*_renderInfo.getState());
         }
      }
   }
   if (_mode & CHECK_BLACK_LISTED_MODES)
   {
      stateset.checkValidityOfAssociatedModes(*_renderInfo.getState());
   }
}

ossimPlanetTerrainTechnique::ossimPlanetTerrainTechnique()
:theTerrainTile(0)
{
   setThreadSafeRefUnref(true);
}
ossimPlanetTerrainTechnique::ossimPlanetTerrainTechnique(const ossimPlanetTerrainTechnique& src,
                                                         const osg::CopyOp& /* copyop */)
:
   theModel(src.theModel.get()), // this is just shared
   theGrid(src.theGrid.get()),   // this is just shared
   theTerrainTile(0)             // can't duplicate this
{
}

ossimPlanetTerrainTechnique::~ossimPlanetTerrainTechnique()
{
}

void ossimPlanetTerrainTechnique::setTerrainTile(ossimPlanetTerrainTile* tile)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTerrainTileMutex);
   theTerrainTile = tile;
   if(theTerrainTile)
   {
      theTerrainTile->setCullCallback(new ossimPlanetTerrainTechnique::CullCallback());
   }
}

void ossimPlanetTerrainTechnique::init(ossimPlanetTerrainTile* /* optionalParent */)
{
}

void ossimPlanetTerrainTechnique::update(osgUtil::UpdateVisitor* uv)
{
   if (theTerrainTile) theTerrainTile->osg::Group::traverse(*uv);
}

void ossimPlanetTerrainTechnique::cull(osgUtil::CullVisitor* cv)
{
   if (theTerrainTile) theTerrainTile->osg::Group::traverse(*cv);
}

void ossimPlanetTerrainTechnique::traverse(osg::NodeVisitor& nv)
{
   if (!theTerrainTile) return;
   
   // if app traversal update the frame count.
   if (nv.getVisitorType()==osg::NodeVisitor::UPDATE_VISITOR)
   {
      osgUtil::UpdateVisitor* uv = dynamic_cast<osgUtil::UpdateVisitor*>(&nv);
      if (uv)
      {
         update(uv);
         return;
      }        
      
   }
   else if (nv.getVisitorType()==osg::NodeVisitor::CULL_VISITOR)
   {
      osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(&nv);
      if (cv)
      {
         cull(cv);
         return;
      }
   }
   
   // otherwise fallback to the Group::traverse()
   theTerrainTile->osg::Group::traverse(nv);
}

bool ossimPlanetTerrainTechnique::isLeaf()const
{
   if(theTerrainTile)
   {
      return (theTerrainTile->getNumChildren() != 4);
   }
   
   return true;
}

bool ossimPlanetTerrainTechnique::areAllChildrenLeaves()const
{
   if(!theTerrainTile) return false;
   if(theTerrainTile->getNumChildren()!=4) return false;
   ossim_uint32 idx;
   ossim_uint32 bounds = theTerrainTile->getNumChildren();
   for(idx = 1; idx < bounds; ++idx)
   {
      const ossimPlanetTerrainTile* tile = dynamic_cast<const ossimPlanetTerrainTile*>(theTerrainTile->getChild(idx));
      if(tile)
      {
         if(!tile->terrainTechnique()->isLeaf()) return false;
      }
   }
   
   return true;
}

void ossimPlanetTerrainTechnique::vacantChildIds(TileIdList& ids)const
{
   if(!theTerrainTile)
   {
      return;
   }
   ossim_uint32 idx = 0;
   const ossimPlanetTerrainTileId& srcId = theTerrainTile->tileId();
   ossim_uint32 nextLevel = srcId.level()+1;
   for(idx = 0; idx < 4; ++idx)
   {
      ossim_uint32 xOrigin=srcId.x()<<1;
      ossim_uint32 yOrigin=srcId.y()<<1;
      if(idx == 1)
      {
         ++xOrigin;
      }
      else if(idx == 2)
      {
         ++yOrigin;
      }
      else if(idx == 3)
      {
         ++xOrigin;
         ++yOrigin;
      }
      ossimPlanetTerrainTileId tileId(srcId.face(),
                                      nextLevel,
                                      xOrigin,
                                      yOrigin);     
      if(!theTerrainTile->child(tileId))
      {
         ids.push_back(tileId);
      }
   }
}

void ossimPlanetTerrainTechnique::merge()
{
   if(theTerrainTile)
   {
      // we really need to go through the sub tree and free each one or 
      // remove from parent
      //
   }
}

ossim_uint32 ossimPlanetTerrainTechnique::childIndex(const ossimPlanetTerrainTileId& tileId)const
{
   return (((tileId.y()&1)<<1)|
           (tileId.x()&1));
}

void ossimPlanetTerrainTechnique::childTreePosition(const ossimPlanetTerrainTileId& tileId,
                                                    ossim_uint32& x, ossim_uint32& y)const
{
   x = tileId.x()&1;
   y = tileId.y()&1;
}
      
void ossimPlanetTerrainTechnique::solveTextureMatrixMappingToParent(const ossimPlanetTerrainTileId& tileId,
                                                                    osg::Matrixd& m)const
{
   switch(childIndex(tileId))
   {
         // during our split routine an index of 0 is the bottom left child and origin is 0,0 in parent
         // we basically map local space 0 to 1 ro parent 0 to .5 for both x and y direction.
         //
      case 0: 
      {
         m.set(0.5, 0.0, 0.0, 0.0,
               0.0, 0.5, 0.0, 0.0,
               0.0, 0.0, 1.0, 0.0,
               0.0, 0.0, 0.0, 1.0);
         break;
      }
         // during our split routine an index of 1 is the bottom right child and origin is .5,0 in parent
         // so we map local space 0 to 1 along x to .5 to 1 in parent for the x axis and we map
         // y axis to 0 to .5
         //
      case 1:
      {
         m.set(0.5, 0.0, 0.0, 0.0,
               0.0, 0.5, 0.0, 0.0,
               0.0, 0.0, 1.0, 0.0,
               0.5, 0.0, 0.0, 1.0);
         break;
      }
         
         // during our split routine an index of 0 is the top left child and origin is 0,.5 in parent
         // so we map local space 0 to 1 along x to 0 to .5 in parent for the x axis and we map
         // y axis to .5 to 1
         //
      case 2:
      {
         m.set(0.5, 0.0, 0.0, 0.0,
               0.0, 0.5, 0.0, 0.0,
               0.0, 0.0, 1.0, 0.0,
               0.0, 0.5, 0.0, 1.0);
         break;
      }
         // during our split routine an index of 0 is the top right child and origin is .5,.5 in parent
         // so we map local space 0 to 1 along both axis to .5 to 1 in parent.
         //
      case 3:
      {
         m.set(0.5, 0.0, 0.0, 0.0,
               0.0, 0.5, 0.0, 0.0,
               0.0, 0.0, 1.0, 0.0,
               0.5, 0.5, 0.0, 1.0);
         break;
      }      
   }
}

void ossimPlanetTerrainTechnique::solveTextureMatrixMappingToParent(const ossimPlanetTerrainTileId& startId,
                                                                    const ossimPlanetTerrainTileId& endId,
                                                                    osg::Matrixd& m)const
{
   m.makeIdentity();
   if(startId.level() != 0)
   {
      if(startId.level() > endId.level())
      {
         osg::Matrixd mParent;
         ossimPlanetTerrainTileId currentId = startId;
         while(currentId.level() != endId.level())
         {
            solveTextureMatrixMappingToParent(currentId, mParent);
            m*=mParent;
            currentId.setId(currentId.face(),
                            currentId.level()-1,
                            currentId.x()>>1,
                            currentId.y()>>1);
         }
      }
   }
}
