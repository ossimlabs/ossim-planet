#ifndef ossimPlanetTerrainGeometryTechnique_HEADER
#define ossimPlanetTerrainGeometryTechnique_HEADER
#include <ossimPlanet/ossimPlanetTerrainTechnique.h>
#include <osg/MatrixTransform>
#include <osgUtil/CullVisitor>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ClusterCullingCallback>
#include <ossimPlanet/ossimPlanetBoundingBox.h>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>

class ossimPlanetTerrainTile;
class ossimPlanetTexture2D;
class OSSIMPLANET_DLL ossimPlanetTerrainGeometryTechnique : public ossimPlanetTerrainTechnique
{
public:
   META_Object(ossimPlanet, ossimPlanetTerrainGeometryTechnique);
     
   class UpdateChildTextureVisitor : public osg::NodeVisitor
      {
      public:
         UpdateChildTextureVisitor(ossimPlanetTexture2D* texture,
                                   ossim_uint32 idx)
         : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
         theTexture(texture),
         theIdx(idx)
         {
         }
         virtual void apply(osg::Node& node);
      protected:
         osg::ref_ptr<ossimPlanetTexture2D> theTexture;
         ossim_uint32 theIdx;
      };
   /**
    * Local space cull node
    */ 
   class OSSIMPLANET_DLL CullNode : public osg::MatrixTransform
   {
   public:
      CullNode(const ossimPlanetTerrainTileId& tileId):
      osg::MatrixTransform(),
      theTileId(tileId),
      theCulledFlag(false),
      theWithinFrustumFlag(true),
      theEyeDistance(0.0),
      theEyeToVolumeDistance(0.0),
      thePixelSize(0.0)
      {
         setCullingActive(false);
      }
      CullNode(const ossimPlanetTerrainTileId& tileId,
               osg::ref_ptr<ossimPlanetBoundingBox> boundingBox,
               const osg::Vec3d& clusterCullingControlPoint,
               const osg::Vec3d& clusterCullingNormal,
               double clusterCullingDeviation,
               double clusterCullingRadius):
      osg::MatrixTransform(),
      theTileId(tileId),
      theCulledFlag(false),
      theWithinFrustumFlag(true),
      theBoundingBox(boundingBox),
      theClusterCullingControlPoint(clusterCullingControlPoint),
      theClusterCullingNormal(clusterCullingNormal),
      theClusterCullingDeviation(clusterCullingDeviation),
      theClusterCullingRadius(clusterCullingRadius),
      theEyeDistance(0.0),
      theEyeToVolumeDistance(0.0),
      thePixelSize(0.0)
      {
         setCullingActive(false);
      }
      CullNode(const CullNode& src, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
      :osg::MatrixTransform(src, copyop),
      theTileId(src.theTileId),
      theCulledFlag(src.theCulledFlag),
      theWithinFrustumFlag(src.theWithinFrustumFlag),
      theBoundingBox(new ossimPlanetBoundingBox(*src.theBoundingBox)),
      theClusterCullingControlPoint(src.theClusterCullingControlPoint),
      theClusterCullingNormal(src.theClusterCullingNormal),
      theClusterCullingDeviation(src.theClusterCullingDeviation),
      theClusterCullingRadius(src.theClusterCullingRadius),
      theEyeDistance(0.0),
      theEyeToVolumeDistance(0.0),
      thePixelSize(0.0)
     {
         setCullingActive(false);
      }
      const ossimPlanetTerrainTileId& tileId()const
      {
         return theTileId;
      }
      void setTileId(const ossimPlanetTerrainTileId& id)
      {
         theTileId = id;
      }
      void setBoundingBox(osg::ref_ptr<ossimPlanetBoundingBox> box)
      {
         theBoundingBox = box.get();
      }
      
      const ossimPlanetBoundingBox* boundingBox()const
      {
         return theBoundingBox.get();
      }
      double eyeDistance()const
      {
         return theEyeDistance;
      }
      double eyeToVolumeDistance()const
      {
         return theEyeToVolumeDistance;
      }
      virtual void traverse(osg::NodeVisitor& nv);
      
      void setCulledFlag(bool flag)
      {
         theCulledFlag = flag;
      }
      bool isCulled()const
      {
         return theCulledFlag;
      }
      virtual osg::BoundingSphere computeBound() const;
      
      double pixelSize()const
      {
         return thePixelSize;
      }
      bool withinFrustumFlag()const
      {
         return theWithinFrustumFlag;
      }
   protected:
      ossimPlanetTerrainTileId theTileId;
      bool theCulledFlag;
      bool theWithinFrustumFlag;
      osg::ref_ptr<ossimPlanetBoundingBox> theBoundingBox;
      osg::Vec3d theClusterCullingControlPoint;
      osg::Vec3d theClusterCullingNormal;
      double theClusterCullingDeviation;
      double theClusterCullingRadius;
      double theEyeDistance;
      double theEyeToVolumeDistance;
      double thePixelSize;
  };
   friend class CullCallback;
   ossimPlanetTerrainGeometryTechnique();
   ossimPlanetTerrainGeometryTechnique(const ossimPlanetTerrainGeometryTechnique& src,
                                       const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
  virtual ~ossimPlanetTerrainGeometryTechnique();
   virtual void setTerrainTile(ossimPlanetTerrainTile* tile);
   virtual void update(osgUtil::UpdateVisitor* nv);
   
   virtual void cull(osgUtil::CullVisitor* nv);
   virtual void traverse(osg::NodeVisitor& nv);
  
   virtual void buildMesh(ossimPlanetTerrainTile* optionalParent=0);
   
   virtual void applyColorLayers(ossimPlanetTerrainTile* optionalParent=0);
   
   virtual void applyTransparency(ossimPlanetTerrainTile* optionalParent=0);
   
   virtual void smoothGeometry();
   virtual void init(ossimPlanetTerrainTile* optionalParent=0);
   //void split(bool recurse, double availableTime=2.5);
   
   virtual void childAdded(ossim_uint32 pos);
   
   virtual void merge();
   virtual void removeCulledChildren();
   bool hasCulledChildren()const;
   bool isChildCulled(ossim_uint32 childIdx)const;

   virtual double elevationPriority()const;
   virtual double texturePriority()const;
   virtual double mergePriority()const;
   virtual double splitPriority()const;
   virtual osg::BoundingSphere computeBound() const;

   virtual void updateElevationMesh();
   
   virtual ossimPlanetTexture2D* newImageLayerTexture(ossim_uint32 imageLayerIdx);
   virtual void setChildCullParameters(ossimPlanetTerrainTile* child,
                                       osg::ref_ptr<CullNode> cullNode);
   virtual void compileGlObjects(osg::State* state);
   virtual void setImageLayerTexture(ossimPlanetTexture2D* texture, 
                                     ossim_uint32 imageLayerIdx);
   virtual void setElevationMeshFrom(ossimPlanetTerrainTile* tile);
   virtual void vacantChildIds(TileIdList& ids)const;

   
   // setup parameters
   //
   
   
   virtual void releaseGLObjects(osg::State* state=0);
   
protected:
   struct BufferData
   {
      osg::ref_ptr<osg::MatrixTransform>   theTransform;
      osg::ref_ptr<osg::Geode>             theGeode;
      osg::ref_ptr<osg::Geometry>          theGeometry;
      osg::Vec3d                           theCenterPatch;
      osg::ref_ptr<osg::ClusterCullingCallback> theClusterCullingCallback;
      osg::ref_ptr<CullNode>               theCullNode;
   };
   enum ChildLocation
   {
      BOTTOM_LEFT  = 0,
      BOTTOM_RIGHT = 1,
      TOP_LEFT     = 2,
      TOP_RIGHT    = 3
   };
   enum SkirtFlags
   {
      NO_SKIRT     = 0,
      BOTTOM_SKIRT = 1,
      RIGHT_SKIRT  = 2,
      TOP_SKIRT    = 4,
      LEFT_SKIRT   = 8
  };
   ossimPlanetTexture2D* findNearestActiveParentTexture(ossim_uint32 idx, ossimPlanetTerrainTile* optionalParent=0);
   void swapBuffers();
   inline BufferData& readOnlyBuffer() { return theBufferData;}//theBufferData[theCurrentReadOnlyBuffer]; }
   inline const BufferData& readOnlyBuffer()const { return theBufferData;}//[theCurrentReadOnlyBuffer]; }
   inline BufferData& writeBuffer() { return theBufferData;}//theBufferData[theCurrentWriteBuffer]; }
   void updateTextureMatrix(osg::StateSet* stateset, 
                            ossim_uint32 imageLayerIdx,
                            const ossimPlanetTerrainTileId& startId,
                            const ossimPlanetTerrainTileId& endId);
   bool isCulled(BufferData& buffer, osgUtil::CullVisitor* cv)const;
   void updateChildCullSettings(osgUtil::CullVisitor* cv);
   void markOnlyNeededChildImageLayersDirty();
   ossim_uint32 computeSkirtFlags(const ossimPlanetTerrainTileId& childId)const;
   void updateRequests(osg::NodeVisitor& nv);
   
//   unsigned int                        theCurrentReadOnlyBuffer;
//   unsigned int                        theCurrentWriteBuffer;
   BufferData                            theBufferData;
   ossimPlanetReentrantMutex theUpdateMutex;
   
   ossimPlanetReentrantMutex            theChildNodeCullParametersMutex;
   std::vector<osg::ref_ptr<CullNode> > theChildNodeCullParameters;
   
   OpenThreads::Mutex                   theInitMutex;
   
   osg::Vec3d                           theAdjustedEye;
   osg::Vec3d                           thePriorityPoint;
   osg::BoundingSphere                  thePatchBound;
   osg::Vec3d                           theCenterGrid;
};

#endif
   
