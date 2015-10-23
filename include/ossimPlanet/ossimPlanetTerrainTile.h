#ifndef ossimPlanetTerrainTile_HEADER
#define ossimPlanetTerrainTile_HEADER
#include <osg/Group>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetGrid.h>
#include <ossimPlanet/ossimPlanetGrid.h>
#include <ossimPlanet/ossimPlanetOperation.h>
#include <ossimPlanet/ossimPlanetTerrainTileId.h>
#include <ossimPlanet/ossimPlanetTerrainLayer.h>
#include <ossimPlanet/ossimPlanetTerrainTechnique.h>
#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <ossimPlanet/ossimPlanetOperation.h>
#include <osg/observer_ptr>
#include <OpenThreads/ReadWriteMutex>

class ossimPlanetTerrain;
class OSSIMPLANET_DLL ossimPlanetTerrainTile : public osg::Group
{
public:
   friend class ossimPlanetTerrain;
   
   class MergeTestVisitor : public osg::NodeVisitor
   {
   public:
      MergeTestVisitor(bool cancelScheduledTasksFlag=true)
      :NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
      theCancelScheduledTasksFlag(cancelScheduledTasksFlag),
      theCanMergeFlag(true)
      {
      }
      virtual void reset()
      {
         theCanMergeFlag = true;
      }
      virtual void apply(osg::Node& node)
      {
         ossimPlanetTerrainTile* tile = dynamic_cast<ossimPlanetTerrainTile*>(&node);
         if(tile)
         {
            if(theCancelScheduledTasksFlag)
            {
               tile->splitRequest()->cancel();
               tile->elevationRequest()->cancel();
               tile->textureRequest()->cancel();
            }
            if((tile->splitRequest()->referenceCount() >1)||
               (tile->elevationRequest()->referenceCount() > 1)||
               (tile->textureRequest()->referenceCount()>1))
            {
               theCanMergeFlag = false;
            }
         }
         
         traverse(node);
      }
      bool canMerge()const{return theCanMergeFlag;}
   protected:
      bool theCancelScheduledTasksFlag;
      bool theCanMergeFlag;
   };
   typedef std::vector<osg::ref_ptr<ossimPlanetTerrainImageLayer> > ImageLayers;
   typedef osg::ref_ptr<ossimPlanetTerrainImageLayer> ElevationLayer;
   typedef std::vector<osg::ref_ptr<ossimPlanetTerrainTile> > TileList;
   ossimPlanetTerrainTile();
   ossimPlanetTerrainTile(const ossimPlanetTerrainTileId& value);
   virtual ~ossimPlanetTerrainTile();

   virtual void init(ossimPlanetTerrainTile* optionalParentOverride=0);
   void traverse(osg::NodeVisitor& nv);
   
   void setTerrainTechnique(ossimPlanetTerrainTechnique* terrainTechnique);
   
   /** Get the TerrainTechnique*/
   ossimPlanetTerrainTechnique* terrainTechnique() { return theTerrainTechnique.get(); }
   
   /** Get the const TerrainTechnique*/
   const ossimPlanetTerrainTechnique* terrainTechnique() const
   { 
      return theTerrainTechnique.get(); 
   }
   void copyCommonParameters(ossimPlanetTerrainTile* src);
   const ossimPlanetTerrainTileId& tileId()const{return theId;}
   void setTileId(const ossimPlanetTerrainTileId& value);
   void setTerrain(ossimPlanetTerrain* value);
   ossimPlanetTerrain* terrain();
   const ossimPlanetTerrain* terrain()const;
   ossimPlanetGrid* grid(){return theGrid.get();}
   const ossimPlanetGrid* grid()const{return theGrid.get();}
   
   virtual void cancelAllOperations();
   virtual bool hasActiveOperations()const;
   
   virtual void resetImageLayers();
   virtual void resetElevationLayer();
/**
    * Will be non destructive set.
    */
   void setNumberOfImageLayers(ossim_uint32 n);
   ossim_uint32 numberOfImageLayers()const;
   bool imageLayersDirty()const;
   ossimPlanetTerrainImageLayer* imageLayer(ossim_uint32 idx=0);
   ossimPlanetTerrainImageLayer* elevationLayer();
   
   ossim_int32 indexOfChild(const ossimPlanetTerrainTileId& id);
   ossimPlanetTerrainTile* child(const ossimPlanetTerrainTileId& id);

   ossimPlanetTerrainTile* parentTile();
   const ossimPlanetTerrainTile* parentTile()const;
   
   virtual void vacantChildIds(ossimPlanetTerrainTechnique::TileIdList& ids)const;
   //virtual void split(bool recurse=true, double availableTime=2.5);
   virtual void merge();

   void setCulledFlag(bool culled);
   bool culledFlag()const;
   
   virtual osg::BoundingSphere computeBound() const;
   
   ossimPlanetOperation* splitRequest()
   {
      return theSplitRequest.get();
   }
   const ossimPlanetOperation* splitRequest()const
   {
      return theSplitRequest.get();
   }
   
   ossimPlanetOperation* elevationRequest()
   {
      return theElevationRequest.get();
   }
   
   ossimPlanetOperation* textureRequest()
   {
      return theTextureRequest.get();
   }
   virtual void releaseGLObjects(osg::State* state=0);
   
   /**
    * @return the last cull time stamp.
    */
   ossim_float64 simTimeStamp()const
   {
      return theSimTimeStamp;
   }
   
   
   /**
    * @return the last cull time stamp.
    */
   ossim_float64 timeStamp()const
   {
      return theTimeStamp;
   }
   
   /**
    * @return the last cull frame stamp
    */
   ossim_int64 frameNumber()const
   {
      return theFrameNumber;
   }
   
   void updateFrameAndTimeStamps(const osg::FrameStamp* stamp)
   {
      if(stamp)
      {
         theFrameNumber = stamp->getFrameNumber();
         theTimeStamp = stamp->getReferenceTime();
         theSimTimeStamp = stamp->getSimulationTime();
      }
   }
protected:
   virtual void childInserted(unsigned int /*pos*/);
   
   mutable ossimPlanetReentrantMutex thePropertyMutex; 
   ossimPlanetTerrainTileId theId;

   osg::ref_ptr<ossimPlanetGrid> theGrid;
   ossimPlanetTerrain* theTerrain;
   osg::ref_ptr<ossimPlanetTerrainTechnique> theTerrainTechnique;
   
   ImageLayers    theImageLayers;
   ElevationLayer theElevationLayer;
   
   bool theCulledFlag;

   /**
    * Job interface to the split queue
    */
   osg::ref_ptr<ossimPlanetOperation> theSplitRequest;

   /**
    * Job interface to the texture queue
    */
   osg::ref_ptr<ossimPlanetOperation> theTextureRequest;

   /**
    * Job interface to the elevation queue
    */
   osg::ref_ptr<ossimPlanetOperation> theElevationRequest;
   
   /**
    * Will hold the frame stamp of the last culled visit.  If it get's culled it should not update
    * it's frame stamp
    */
   ossim_int64   theFrameNumber;

   
   /**
    * Will hold the time stamp of the last culled visit.  If it get's culled it should not update
    * it's time stamp
    */
   ossim_float64 theTimeStamp;
   
   /**
    * Will hold the simulation time stamp of the last culled visit.  If it get's culled it should not update
    * it's time stamp
    */
   ossim_float64 theSimTimeStamp;
};

#endif
