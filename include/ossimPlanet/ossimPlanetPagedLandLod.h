#ifndef ossimPlanetPagedLandLod_HEADER
#define ossimPlanetPagedLandLod_HEADER
#include <osg/NodeVisitor>
#include <osg/Group>
#include <osg/PagedLOD>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <OpenThreads/ScopedLock>
#include <ossim/base/ossimConstants.h>
#include <ossimPlanet/ossimPlanetConstants.h>
#include "ossimPlanet/ossimPlanetBoundingBox.h"
#include <ossimPlanet/ossimPlanetLandTextureRequest.h>
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetPagedLandLodCullNode : public osg::MatrixTransform
{
public:
   ossimPlanetPagedLandLodCullNode():
      osg::MatrixTransform(),
      theCulledFlag(false)
   {}
      ossimPlanetPagedLandLodCullNode(osg::ref_ptr<ossimPlanetBoundingBox> boundingBox,
                                      bool useClusterCulling,
                                      const osg::Vec3d& clusterCullingControlPoint,
                                      const osg::Vec3d& clusterCullingNormal,
                                      double clusterCullingDeviation,
                                      double clusterCullingRadius):
         osg::MatrixTransform(),
         theCulledFlag(false),
         theBoundingBox(boundingBox),
         theUseClusterCulling(useClusterCulling),
         theClusterCullingControlPoint(clusterCullingControlPoint),
         theClusterCullingNormal(clusterCullingNormal),
         theClusterCullingDeviation(clusterCullingDeviation),
         theClusterCullingRadius(clusterCullingRadius)
   {}
   ossimPlanetPagedLandLodCullNode(const ossimPlanetPagedLandLodCullNode& src):
      osg::MatrixTransform(src),
      theCulledFlag(src.theCulledFlag),
      theBoundingBox(new ossimPlanetBoundingBox(*src.theBoundingBox)),
      theUseClusterCulling(src.theUseClusterCulling),
      theClusterCullingControlPoint(src.theClusterCullingControlPoint),
      theClusterCullingNormal(src.theClusterCullingNormal),
      theClusterCullingDeviation(src.theClusterCullingDeviation),
      theClusterCullingRadius(src.theClusterCullingRadius)
   {
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
   virtual void traverse(osg::NodeVisitor& nv);

   bool isCulled()const
   {
      return theCulledFlag;
   }
  
protected:
   bool theCulledFlag;
   osg::ref_ptr<ossimPlanetBoundingBox> theBoundingBox;
   bool theUseClusterCulling;
   osg::Vec3d theClusterCullingControlPoint;
   osg::Vec3d theClusterCullingNormal;
   double theClusterCullingDeviation;
   double theClusterCullingRadius;

   double theEyeDistance;
};

class ossimPlanetLandCullCallback;
class ossimPlanetLand;
class OSSIMPLANET_EXPORT ossimPlanetPagedLandLod : public osg::Group//public osg::PagedLOD
{
public:

   friend class ossimPlanetLandCullCallback;
   friend class ossimPlanetPagedLandLodUpdateCallback;
   friend class ossimPlanetLandReaderWriter;
   friend class ossimPlanetPagedLandLodClearPointers;

   virtual osg::Object* cloneType() const { return new ossimPlanetPagedLandLod(); }
   virtual osg::Object* clone(const osg::CopyOp& copyop) const
   {
      return new ossimPlanetPagedLandLod(*this,copyop);
   }
   virtual bool isSameKindAs(const osg::Object* obj) const
   {
      return dynamic_cast<const ossimPlanetPagedLandLod *>(obj)!=0;
   }
   virtual const char* className() const { return "PlanetPagedLandLod"; }
   virtual const char* libraryName() const { return ""; }
   ossimPlanetPagedLandLod(ossim_uint32 level=0,
                         ossim_uint32 row=0,
                         ossim_uint32 col=0,
                         const std::string& requestName="");
   ossimPlanetPagedLandLod(const ossimPlanetPagedLandLod& src,
                         const osg::CopyOp& copyop);
    virtual ~ossimPlanetPagedLandLod();
    void setRequestName(ossim_uint32 idx,
                          const std::string& name)
    {
      theRequestNameList[idx] = name;
   }
   std::string getRequestName(ossim_uint32 idx)const
   {
      return theRequestNameList[idx];
   }
   bool getRemoveChildrenFlag()const
   {
      return theRemoveChildrenFlag;
   }
   bool getCulledFlag()const
   {
      return theCulledFlag;
   }

   virtual bool addChild( Node *child );

   ossim_uint32 getLevel()const
      {
         return theLevel;
      }
   ossim_uint32 getRow()const
      {
         return theRow;
      }
   ossim_uint32 getCol()const
      {
         return theCol;
      }
   bool isLeaf()const
   {
      return (getNumChildren() != 5);
   }
   bool hasCulledChildren()const;
   bool areAllChildrenCulled(bool applyToAddedChildrenOnly=false)const;
   bool areAllChildrenLeaves()const;
   OpenThreads::Mutex& mutex()const
   {
      return theMutex;
   }

   virtual void traverse(osg::NodeVisitor& nv);

   void setRefreshType(ossimPlanetLandRefreshType refreshType);
   ossimPlanetLandRefreshType refreshType()const;
protected:
   ossimPlanetLand* landLayer();
   const ossimPlanetLand* landLayer()const;

   osg::ref_ptr<osg::Geode> theGeode;
   ossim_uint32 theLevel;
   ossim_uint32 theRow;
   ossim_uint32 theCol;
/*    osg::ref_ptr<ossimPlanetBoundingBox> theBoundingBox; */
   std::vector<std::string> theRequestNameList;
   osg::ref_ptr<osg::Referenced> theRequestRefChildList[4];
   osg::ref_ptr<osg::Referenced> theRequestRef;
   bool theCulledFlag;
   bool theRemoveChildrenFlag;
   ossimPlanetLandRefreshType theRefreshType;
   mutable ossimPlanetReentrantMutex theMutex;
   mutable ossimPlanetReentrantMutex thePagedLodListMutex;
   mutable ossimPlanetReentrantMutex theChildCullNodeListMutex;
   mutable ossimPlanetReentrantMutex theTextureRequestMutex;
   osg::ref_ptr<ossimPlanetPagedLandLodCullNode> theCullNode;
      
/*    osg::Node* theTransform; */
   ossim_uint32 theMaxLevel;
   osg::Vec3d theCenterPoint;
   osg::Vec3d theUlPoint;
   osg::Vec3d theUrPoint;
   osg::Vec3d theLrPoint;
   osg::Vec3d theLlPoint;
   osg::Vec3d theCenterNormal;
   osg::Vec3d theUlNormal;
   osg::Vec3d theUrNormal;
   osg::Vec3d theLrNormal;
   osg::Vec3d theLlNormal;

   
// lists to add during update calls
   //
   std::vector<osg::ref_ptr<ossimPlanetPagedLandLod> >          thePagedLodList;
   std::vector<osg::ref_ptr<ossimPlanetPagedLandLodCullNode> >  theChildCullNodeList;
   osg::ref_ptr<ossimPlanetLandTextureRequest>                  theTextureRequest;
};

#endif
