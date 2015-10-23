#ifndef ossimPlanetTerrainTechnique_HEADER
#define ossimPlanetTerrainTechnique_HEADER
#include <ossimPlanet/ossimPlanetExport.h>
#include <osg/Referenced>
#include <osgUtil/UpdateVisitor>
#include <osgUtil/CullVisitor>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetGrid.h>
#include <ossimPlanet/ossimPlanetTexture2D.h>
#include <osgUtil/GLObjectsVisitor>

class ossimPlanetTerrainTile;
class OSSIMPLANET_DLL ossimPlanetTerrainTechnique : public osg::Object
{
public:
   META_Object(ossimPlanet, ossimPlanetTerrainTechnique);
   class OSSIMPLANET_DLL CompileObjects : public osgUtil::GLObjectsVisitor
   {
   public:
      CompileObjects()
      : osgUtil::GLObjectsVisitor(COMPILE_STATE_ATTRIBUTES|COMPILE_DISPLAY_LISTS|CHECK_BLACK_LISTED_MODES)
      {
         
      }
      virtual void apply(osg::Node& node);
      virtual void apply(osg::Geode& node);
      virtual void apply(osg::Drawable& drawable);
      virtual void apply(osg::StateSet& stateset);
   };
   
   class CullCallback : public osg::NodeCallback
   {
   public: 
      CullCallback(){}
      virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
      {
         ossimPlanetTerrainTechnique* technique = dynamic_cast<ossimPlanetTerrainTechnique*>(node);
         osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
         if(technique&&cv)
         {
            technique->cull(cv);
         }
         else if(node)
         {
            node->traverse(*nv);
         }
      }
   };
   friend class ossimPlanetTerrainTile;
   typedef std::vector<ossimPlanetTerrainTileId> TileIdList;
   ossimPlanetTerrainTechnique();
   ossimPlanetTerrainTechnique(const ossimPlanetTerrainTechnique& src,
                               const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
   
   virtual void setTerrainTile(ossimPlanetTerrainTile* tile);
   ossimPlanetTerrainTile* terrainTile() 
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTerrainTileMutex);
      return theTerrainTile; 
   }
   const ossimPlanetTerrainTile* terrainTile() const 
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTerrainTileMutex);
      return theTerrainTile; 
   }
   
   void setGrid(ossimPlanetGrid* grid){theGrid = grid;}
   void setModel(ossimPlanetGeoRefModel* model){theModel = model;}
   
   const ossimPlanetGrid* grid()const
   {
      return theGrid.get();
   }
   const ossimPlanetGeoRefModel* model()const{return theModel.get();}
   
   virtual bool isLeaf()const;
   virtual bool areAllChildrenLeaves()const;
   virtual void init(ossimPlanetTerrainTile* optionalParent=0);
   
//   virtual void applyImageLayer(ossim_int32 idx=-1)=0;
//   virtual void applyElevation()=0;
   
   virtual void update(osgUtil::UpdateVisitor* nv);
   
   virtual void cull(osgUtil::CullVisitor* nv);
   
   /** Traverse the terrain subgraph.*/
   virtual void traverse(osg::NodeVisitor& nv);
   
//   virtual void split(bool recurse, double availableTime=2.5);
   virtual void childAdded(ossim_uint32 /*pos*/)
   {
      
   }

   virtual void merge();
 
   virtual double texturePriority()const
   {
      return 0.0;
   }

   virtual double elevationPriority()const
   {
      return texturePriority();
   }

   virtual double splitPriority()const
   {
      return texturePriority();
   }
   virtual double mergePriority()const
   {
      return texturePriority();
   }
  
   /**
    *
    */
   virtual void solveTextureMatrixMappingToParent(const ossimPlanetTerrainTileId& tileId,
                                                  osg::Matrixd& m)const;
   virtual void solveTextureMatrixMappingToParent(const ossimPlanetTerrainTileId& startId,
                                                  const ossimPlanetTerrainTileId& endId,
                                                  osg::Matrixd& m)const;
   
   virtual osg::BoundingSphere computeBound() const
   {
      return osg::BoundingSphere(osg::Vec3d(0.0,0.0,0.0), -1);
   }
   
   virtual void compileGlObjects(osg::State* /*state*/)
   {
      // intentianally left blank
   }
   virtual void setImageLayerTexture(ossimPlanetTexture2D* /*texture*/, 
                                     ossim_uint32 /*imageLayerIdx*/)
   {
      // intentianally left blank
   }
   virtual void setElevationMeshFrom(ossimPlanetTerrainTile* /*tile*/)
   {
      // intentianally left blank
   }
   virtual ossimPlanetTexture2D* newImageLayerTexture(ossim_uint32 /*imageLayerIdx*/)
   {
      return 0;
   }
   virtual ossim_uint32 childIndex(const ossimPlanetTerrainTileId& tileId)const;
   virtual void childTreePosition(const ossimPlanetTerrainTileId& tileId,
                                  ossim_uint32& x, ossim_uint32& y)const;
   virtual void vacantChildIds(TileIdList& ids)const;
   virtual void releaseGLObjects(osg::State* /*state*/=0)
   {
      
   }
protected:
   virtual ~ossimPlanetTerrainTechnique();
   mutable OpenThreads::Mutex theTerrainTileMutex;
   /**
    * this ossimPlanetGeoRefModel will be replaced soon with a general
    * GeoRefModel
    */
   osg::ref_ptr<ossimPlanetGeoRefModel> theModel;
   
   osg::ref_ptr<ossimPlanetGrid> theGrid;
   ossimPlanetTerrainTile*    theTerrainTile;
   
};

#endif
