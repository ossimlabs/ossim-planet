#ifndef ossimPlanetBillboardIcon_HEADER
#define ossimPlanetBillboardIcon_HEADER
#include <osg/Node>
#include <osg/NodeVisitor>
#include <ossimPlanet/ossimPlanetConstants.h>
#include <osg/Billboard>
#include <osg/MatrixTransform>

#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimCommon.h>
#include <ossimPlanet/ossimPlanetIconGeom.h>

class ossimPlanetBillboardIcon :public osg::Node
{
public:
   class ossimPlanetBillboardIconUpdateCallback : public osg::NodeCallback
   {
      virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
         {
            ossimPlanetBillboardIcon* n = dynamic_cast<ossimPlanetBillboardIcon*>(node);
            if(n)
            {
               n->traverse(*nv);
            }
         }
   };
   /**
    * The normalization factor is used to scale the bill board into the units of
    * the scenegraph. This is done by the billboard transform.  This transform is wrapped
    * with a fudge factor transform to make sure that the icon stays on top of the terrain
    */ 
   ossimPlanetBillboardIcon(double objectGroundSize = 50000.0/OSSIMPLANET_WGS_84_RADIUS_EQUATOR);
   void setGroundObjectSize(double groundSize);
   void setIcon(osg::ref_ptr<osg::Image> img);
   void setIcon(osg::ref_ptr<osg::Texture2D> img);
   osg::ref_ptr<ossimPlanetIconGeom> getGeom()
   {
      return theGeom.get();
   }
   void setGeom(osg::ref_ptr<ossimPlanetIconGeom> geom);
   /**
    * This turns on auto scaling so that the full texture size is reached once
    * the gsd is approximately the passed in meters per pixel.
    */ 
   virtual void traverse(osg::NodeVisitor& nv);
   virtual osg::BoundingSphere computeBound() const;
   bool isCulled()const;
   void setMinPixelSize(ossim_uint32 size)
   {
      theMinPixelSize = size;
   }
   void setMaxPixelSize(ossim_uint32 size)
   {
      theMaxPixelSize = size;
   }
protected:
   osg::Billboard*                    theBillboard;
   osg::ref_ptr<ossimPlanetIconGeom>  theGeom;
   osg::ref_ptr<osg::MatrixTransform> theBillboardTransform;
   osg::ref_ptr<osg::MatrixTransform> theFudgeFactorTransform;
   osg::ref_ptr<osg::MatrixTransform> theLabelTransform;
   osg::Matrixd                       theFudgeFactorMatrix;
   bool                               theNeedsUpdateFlag;
   bool                               theCulledFlag;
   ossim_uint32                       theMinPixelSize;
   ossim_uint32                       theMaxPixelSize;
   ossim_uint32                       thePixelWidth;
   ossim_uint32                       thePixelHeight;
   osg::Vec3d                         theTranslate;
   
   bool                               theScaleFactorChangedFlag;
   ossim_float64                      theScaleFactor;
};

#endif
