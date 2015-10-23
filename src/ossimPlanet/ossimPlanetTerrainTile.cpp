#include <ossimPlanet/ossimPlanetTerrainTile.h>
#include <ossimPlanet/ossimPlanetTerrain.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/ossimPlanetVisitors.h>
#include <OpenThreads/ScopedLock>
#include <osg/io_utils>

//#define OSGPLANET_ENABLE_ALLOCATION_COUNT
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
static OpenThreads::Mutex objectCountMutex;
static ossim_uint32 terrainTileCount = 0;
#endif
ossimPlanetTerrainTile::ossimPlanetTerrainTile()
:osg::Group(),
theTerrain(0),
theCulledFlag(false),
theFrameNumber(0),
theTimeStamp(0.0),
theSimTimeStamp(0.0)
{
   setDataVariance(osg::Object::DYNAMIC);
   theSplitRequest     = new ossimPlanetSplitRequest();
   theTextureRequest   = new ossimPlanetTextureRequest();
   theElevationRequest = new ossimPlanetElevationRequest();
   setThreadSafeRefUnref(true);
   setUpdateCallback(new ossimPlanetTraverseCallback);
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(objectCountMutex);
   ++terrainTileCount;
   std::cout << "ossimPlanetTerrainTile count = " << terrainTileCount << std::endl;
#endif
}

ossimPlanetTerrainTile::ossimPlanetTerrainTile(const ossimPlanetTerrainTileId& value)
:osg::Group(),
theId(value),
theTerrain(0),
theCulledFlag(false),
theFrameNumber(0),
theTimeStamp(0.0),
theSimTimeStamp(0.0)
{
   setDataVariance(osg::Object::DYNAMIC);
   theSplitRequest     = new ossimPlanetSplitRequest();
   theTextureRequest   = new ossimPlanetTextureRequest();
   theElevationRequest = new ossimPlanetElevationRequest();
   
   setUpdateCallback(new ossimPlanetTraverseCallback);
   setThreadSafeRefUnref(true);
   //setCullingActive(false); // we will do our own culling
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(objectCountMutex);
   ++terrainTileCount;
   std::cout << "ossimPlanetTerrainTile count = " << terrainTileCount << std::endl;
#endif
}
ossimPlanetTerrainTile::~ossimPlanetTerrainTile()
{
   if(theTerrain)
   {
      theTerrain->unregisterTile(this);
   }
   setCullCallback(0);
   setTerrain(0);
   theTerrainTechnique->setTerrainTile(0);
   theTerrainTechnique = 0;
   theGrid = 0;
   theElevationLayer = 0;
   theImageLayers.clear();
   theTerrain = 0;
   theSplitRequest     = 0;
   theTextureRequest   = 0;
   theElevationRequest = 0;
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(objectCountMutex);
   --terrainTileCount;
   std::cout << "ossimPlanetTerrainTile count = " << terrainTileCount << std::endl;
#endif
}

void ossimPlanetTerrainTile::init(ossimPlanetTerrainTile* optionalParentOverride)
{
   if(theTerrainTechnique.valid())
   {
      theTerrainTechnique->init(optionalParentOverride);
   }
}

void ossimPlanetTerrainTile::traverse(osg::NodeVisitor& nv)
{
   if(!theTerrain)
   {
      setTerrain(ossimPlanetTerrain::findTerrain(nv.getNodePath()));
   }
   if(theTerrainTechnique.valid())
   {
      theTerrainTechnique->traverse(nv);
   }
   else
   {
      osg::Group::traverse(nv);
   }
}
void ossimPlanetTerrainTile::setTerrainTechnique(ossimPlanetTerrainTechnique* terrainTechnique)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theTerrainTechnique = terrainTechnique;
   if(theTerrainTechnique.valid())
   {
      theTerrainTechnique->setTerrainTile(this);
   }
}

void ossimPlanetTerrainTile::copyCommonParameters(ossimPlanetTerrainTile* src)
{
   if(src)
   {
      setTerrain(src->terrain());
   }
}

void ossimPlanetTerrainTile::setTileId(const ossimPlanetTerrainTileId& value)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theId = value;
}

void ossimPlanetTerrainTile::setTerrain(ossimPlanetTerrain* value)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if(theTerrain)
   {
      theTerrain->unregisterTile(this);
   }
   theTerrain = value;
   if(theTerrain)
   {
      theTerrainTechnique = theTerrain->newTechnique();
      theTerrainTechnique->theTerrainTile = this;
      theGrid = theTerrain->grid();
      theTerrain->registerTile(this);
      
      resetImageLayers();
      resetElevationLayer();
   }
}

ossimPlanetTerrain* ossimPlanetTerrainTile::terrain()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theTerrain;
}

const ossimPlanetTerrain* ossimPlanetTerrainTile::terrain()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theTerrain;
}

void ossimPlanetTerrainTile::resetImageLayers()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   ossim_uint32 idx;
  if(numberOfImageLayers()!= theTerrain->numberOfTextureLayers())
  {
      setNumberOfImageLayers(theTerrain->numberOfTextureLayers());
   }
   for(idx = 0; idx < theImageLayers.size(); ++idx)
   {
      if(!theImageLayers[idx].valid())
      {
         theImageLayers[idx] = new ossimPlanetTerrainImageLayer();
      }
      theImageLayers[idx]->setImage(0);
   }
}

void ossimPlanetTerrainTile::resetElevationLayer()
{
   if(!elevationLayer())
   {
      theElevationLayer = new ossimPlanetTerrainImageLayer();
   }
   //theElevationLayer->setRowsColumns(9,9);
   theElevationLayer->setImage(0);
}
   
void ossimPlanetTerrainTile::setNumberOfImageLayers(ossim_uint32 n)
{

   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);

   if(n==0)
   {
      theImageLayers.clear();
   }
   else
   {
      theImageLayers.resize(n);
   }
}

ossim_uint32 ossimPlanetTerrainTile::numberOfImageLayers()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return (ossim_uint32)theImageLayers.size();
}

bool ossimPlanetTerrainTile::imageLayersDirty()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theImageLayers.size();++idx)
   {
      if(theImageLayers[idx].get())
      {
         if(theImageLayers[idx]->isDirty())
         {
            return true;
         }
      }
   }
   
   return false;
}

ossimPlanetTerrainImageLayer* ossimPlanetTerrainTile::imageLayer(ossim_uint32 idx)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   
   ossimPlanetTerrainImageLayer* result = 0;
   if(idx <theImageLayers.size())
   {
      result = theImageLayers[idx].get();
   }
   
   return result;
}

ossimPlanetTerrainImageLayer* ossimPlanetTerrainTile::elevationLayer()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theElevationLayer.get();
}

ossim_int32 ossimPlanetTerrainTile::indexOfChild(const ossimPlanetTerrainTileId& id)
{
   ossim_uint32 idx = 0;
   ossim_uint32 bound = getNumChildren();
   for(idx = 0; idx < bound;++idx)
   {
      ossimPlanetTerrainTile* tile = dynamic_cast<ossimPlanetTerrainTile*>(getChild(idx));
      if(tile&&(tile->tileId()==id))
      {
         return (ossim_int32)idx;
      }
   }
   return -1;
}

ossimPlanetTerrainTile* ossimPlanetTerrainTile::child(const ossimPlanetTerrainTileId& id)
{
   ossim_uint32 idx = 0;
   ossim_uint32 bound = getNumChildren();
   for(idx = 0; idx < bound;++idx)
   {
      ossimPlanetTerrainTile* tile = dynamic_cast<ossimPlanetTerrainTile*>(getChild(idx));
      if(tile&&(tile->tileId()==id))
      {
         return tile;
      }
   }
   return 0;

}

void ossimPlanetTerrainTile::cancelAllOperations()
{
   theElevationRequest->cancel();
   theSplitRequest->cancel();
   theTextureRequest->cancel();
}

bool ossimPlanetTerrainTile::hasActiveOperations()const
{
   return ((theElevationRequest->referenceCount() > 1)||
           (theSplitRequest->referenceCount() > 1)||
           (theTextureRequest->referenceCount() > 1));
}

ossimPlanetTerrainTile* ossimPlanetTerrainTile::parentTile()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if(getNumParents() < 1) return 0;
   return dynamic_cast<ossimPlanetTerrainTile*>(getParent(0));
}

const ossimPlanetTerrainTile* ossimPlanetTerrainTile::parentTile()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if(getNumParents() < 1) return 0;
   return dynamic_cast<const ossimPlanetTerrainTile*>(getParent(0));
}

void ossimPlanetTerrainTile::vacantChildIds(ossimPlanetTerrainTechnique::TileIdList& ids)const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if(theTerrainTechnique.valid())
   {
      theTerrainTechnique->vacantChildIds(ids);
   }
}

void ossimPlanetTerrainTile::merge()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   if(theTerrainTechnique.valid())
   {
      theTerrainTechnique->merge();
   }
   terrain()->setRedrawFlag(true);
}

osg::BoundingSphere ossimPlanetTerrainTile::computeBound() const
{
   if(theTerrainTechnique.valid())
   {
      return theTerrainTechnique->computeBound();
   }
   return osg::BoundingSphere(osg::Vec3d(0,0,0), -1);
}

void ossimPlanetTerrainTile::setCulledFlag(bool culled)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   theCulledFlag = culled;
}

bool ossimPlanetTerrainTile::culledFlag()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
   return theCulledFlag;
}
void ossimPlanetTerrainTile::releaseGLObjects(osg::State* state)
{
   osg::Group::releaseGLObjects(state);
   if(theTerrainTechnique.valid())
   {
      theTerrainTechnique->releaseGLObjects(state);
   }
}

void ossimPlanetTerrainTile::childInserted(unsigned int pos)
{
   if(theTerrainTechnique.valid())
   {
      theTerrainTechnique->childAdded(pos);
   }
}
