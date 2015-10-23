#ifndef ossimPlanetLandNode_HEADER
#define ossimPlanetLandNode_HEADER
#include <osg/PagedLOD>
#include <osg/Texture2D>
#include <osg/CoordinateSystemNode>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <ossimPlanet/ossimPlanetLandGridNode.h>
#include <ossimPlanet/ossimPlanetPagedRequestNode.h>
#include <osg/Array>
#include <osg/ref_ptr>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <ossim/base/ossimConstants.h>
#include "ossimPlanetExport.h"
#include "ossimPlanetLandSettings.h"
#include "ossimPlanetBoundingBox.h"
#include <ossimPlanet/ossimPlanetExport.h>

namespace osgUtil
{
   class CullVisitor;
}
class OSSIMPLANET_DLL ossimPlanetLandNode : public osg::Group
{
public:
   friend class ossimPlanetLand;
   friend class ossimPlanetLandNodeDrawableCallback;
   ossimPlanetLandNode(ossim_uint32 level = 0,
                     ossim_uint32 row   = 0,
                     ossim_uint32 col   = 0);
   ossimPlanetLandNode(const ossimPlanetLandNode& plod,const osg::CopyOp& copyop);
   virtual ~ossimPlanetLandNode();
   virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const ossimPlanetLandNode *>(obj)!=NULL; }
   virtual const char* className() const { return "PlanetLandNode"; } 
   virtual const char* libraryName() const { return ""; }

   void setLandSettings(const osg::ref_ptr<ossimPlanetLandSettings>& settings);
   void setTextureRequest(osg::ref_ptr<ossimPlanetPagedRequestNode> texture);
   osg::ref_ptr<osg::Texture2D> getTexture();

   ossim_uint32 getLevel()const;
   ossim_uint32 getRow()const;
   ossim_uint32 getCol()const;
   bool getRemoveAllChildrenFlag()const
   {
      return theRemoveAllChildrenFlag;
   }
   void setRemoveAllChildrenFlag(bool flag)
   {
      theRemoveAllChildrenFlag = flag;
   }
   void createPatchGeometry();
   void setPatchLocation(ossim_uint32 level,
                         ossim_uint32 row,
                         ossim_uint32 col,
                         bool createPatchGeometryFlag=true);
  
   virtual bool isLeaf()const;
   virtual bool areAllChildrenLeaves()const;
   virtual bool isCulled()const;
   virtual bool areAllChildrenCulled()const;
protected:
   friend class ossimPlanetLandNodeUpdateCallback;
   unsigned int theLevel;
   unsigned int theRow;
   unsigned int theCol;
   osg::ref_ptr<ossimPlanetPagedRequestNode> theTextureRequest;
   
   bool                                    theTextureChangedFlag;
   bool                                    theElevationChangedFlag;
   osg::ref_ptr<ossimPlanetLandSettings>     theLandSettings;
   
   mutable ossimPlanetReentrantMutex theMutex;
   mutable osg::ref_ptr<osg::Geometry> theGeometry;

   mutable bool theAddAllChildrenFlag;
   mutable bool theRemoveAllChildrenFlag;
   mutable bool theCulledFlag;
   mutable bool thePreviousCulledFlag;
   osg::Vec3d theCenterNormal;
   osg::Vec3d theUlNormal;
   osg::Vec3d theUrNormal;
   osg::Vec3d theLrNormal;
   osg::Vec3d theLlNormal;

   osg::Vec3d theCenterPoint;
   osg::Vec3d theUlPoint;
   osg::Vec3d theUrPoint;
   osg::Vec3d theLrPoint;
   osg::Vec3d theLlPoint;
   osg::ref_ptr<ossimPlanetBoundingBox> theBoundingBox;
   osg::ref_ptr<ossimPlanetElevationGrid> theElevation;
   
   virtual void createPoints(osg::Vec3Array *verts,
                             osg::Vec3Array *norms,
                             osg::Vec2Array *tcoords)=0;
   virtual void handleUpdateTraversal(osg::NodeVisitor& nv);
   virtual void handleCullTraversal(osg::NodeVisitor& nv);
   virtual void traverse(osg::NodeVisitor& nv);
   virtual void setupBoundaryNormals()=0;
   virtual void applyElevation()=0;
   virtual void adjustTexCoordinates();

//   void computeLocalToWorldTransformFromXYZ(double X, double Y, double Z, osg::Matrixd& localToWorld) const;
//   osg::Vec3d computeLocalPosition(const osg::Matrixd& worldToLocal, double X, double Y, double Z)const;

   virtual bool addChild(Node *child);
   virtual std::string createDbName()const;
   virtual std::string createLevel0DbName()const;
   const osg::ref_ptr<osg::Geometry> getParentGeometry()const;
   osg::ref_ptr<osg::Geometry> getParentGeometry();
   ossim_uint32 distanceToLeaf()const;
};

#endif
