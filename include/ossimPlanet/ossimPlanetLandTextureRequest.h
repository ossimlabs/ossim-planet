#ifndef ossimPlanetLandTextureRequest_HEADER
#define ossimPlanetLandTextureRequest_HEADER
#include <vector>
#include <osg/Node>
#include <osg/ref_ptr>
#include <osg/Texture2D>
#include <osg/NodeCallback>
#include <osg/MatrixTransform>
#include <ossim/base/ossimConstants.h>
#include <ossimPlanet/ossimPlanetBoundingBox.h>
#include <iostream>
#include <ossimPlanet/ossimPlanetTexture2D.h>
#include <ossimPlanet/ossimPlanetImage.h>

class ossimPlanetLandTextureRequest : public osg::Node
{
public:
   ossimPlanetLandTextureRequest(ossim_uint32 level=0,
                                 ossim_uint32 row=0,
                                 ossim_uint32 col=0,
                                 const std::vector<osg::ref_ptr<osg::Texture2D> >& textures=std::vector<osg::ref_ptr<osg::Texture2D> >())
      :osg::Node(),
      theLevel(level),
      theRow(row),
      theCol(col),
      theTextures(textures)/* , */
/*       theClusterParametersSet(false) */
      {
      }
   virtual ~ossimPlanetLandTextureRequest()
      {
      }
   void setTextures(const std::vector<osg::ref_ptr<osg::Texture2D> >& textures)
      {
         theTextures = textures;
      }
   void setTransform(osg::ref_ptr<osg::Node> transform)
      {
         theTransform = transform;
      }
   void setCullCallback(osg::ref_ptr<osg::NodeCallback> callback)
   {
      theCullCallback = callback;
   }
   osg::ref_ptr<osg::Node> getTransform()
   {
      return theTransform;
   }
   std::vector<osg::ref_ptr<osg::Texture2D> >& getTextures()
   {
      return theTextures;
   }
   osg::ref_ptr<osg::NodeCallback> getCullCallback()
   {
      return theCullCallback;
   }
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

   void setCenterPoint(osg::Vec3d& centerPoint)
   {
      theCenterPoint = centerPoint;
   }
   void setUlPoint(osg::Vec3d& ulPoint)
   {
      theUlPoint = ulPoint;
   }
   void setUrPoint(osg::Vec3d& urPoint)
   {
      theUrPoint = urPoint;
   }
   void setLrPoint(osg::Vec3d& lrPoint)
   {
      theLrPoint = lrPoint;
   }
   void setLlPoint(osg::Vec3d& llPoint)
   {
      theLlPoint = llPoint;
   }
   void setCenterNormal(osg::Vec3d& centerNormal)
   {
      theCenterNormal = centerNormal;
   }
   void setUlNormal(osg::Vec3d& ulNormal)
   {
      theUlNormal = ulNormal;
   }
   void setUrNormal(osg::Vec3d& urNormal)
   {
      theUrNormal = urNormal;
   }
   void setLrNormal(osg::Vec3d& lrNormal)
   {
      theLrNormal = lrNormal;
   }
   void setLlNormal(osg::Vec3d& llNormal)
   {
      theLlNormal = llNormal;
   }
   osg::Vec3d centerPoint()const
   {
      return theCenterPoint;
   }
   osg::Vec3d ulPoint()const
   {
      return theUlPoint;
   }
   osg::Vec3d urPoint()const
   {
      return theUrPoint;
   }
   osg::Vec3d lrPoint()const
   {
      return theLrPoint;
   }
   osg::Vec3d llPoint()const
   {
      return theLlPoint;
   }
   osg::Vec3d centerNormal()const
   {
      return theCenterNormal;
   }
   osg::Vec3d ulNormal()const
   {
      return theUlPoint;
   }
   osg::Vec3d urNormal()const
   {
      return theUrNormal;
   }
   osg::Vec3d lrNormal()const
   {
      return theLrNormal;
   }
   osg::Vec3d llNormal()const
   {
      return theLlNormal;
   }

   void setBoundingBox(osg::ref_ptr<ossimPlanetBoundingBox> bounds)
   {
      theBoundingBox = bounds;
   }
   osg::ref_ptr<ossimPlanetBoundingBox> boundingBox()
   {
      return theBoundingBox;
   }
   const osg::ref_ptr<ossimPlanetBoundingBox> boundingBox()const
   {
      return theBoundingBox;
   }
   void setChildrenBounds(const std::vector<ossimPlanetBoundingBox>& childrenBounds)
   {
      theChildrenBounds = childrenBounds;
   }
   const std::vector<ossimPlanetBoundingBox>& childrenBounds()const
   {
      return theChildrenBounds;
   }
   ossimPlanetImage::ossimPlanetImageStateType getTextureState()const
   {
      return theTextureState;
   }
   void setTextureState(ossimPlanetImage::ossimPlanetImageStateType state)
   {
      theTextureState = state;
   }

/*    void setClusterCullValues(bool useClusterCulling, */
/*                              const osg::Vec3& controlPoint, */
/*                              const osg::Vec3& normal, */
/*                              double deviation, */
/*                              double radius) */
/*    { */
/*       theClusterParametersSet = true; */
/*       theUseClusterCulling    = useClusterCulling; */
/*       theClusterCullingControlPoint = controlPoint; */
/*       theClusterCullingNormal = normal; */
/*       theClusterCullingDeviation = deviation; */
/*       theClusterCullingRadius = radius; */
/*    } */
   
protected:
   ossim_uint32 theLevel;
   ossim_uint32 theRow;
   ossim_uint32 theCol;
   std::vector<osg::ref_ptr<osg::Texture2D> > theTextures;
   osg::ref_ptr<osg::Node> theTransform;
   osg::ref_ptr<osg::NodeCallback> theCullCallback;
   
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
   osg::ref_ptr<ossimPlanetBoundingBox> theBoundingBox;
   std::vector<ossimPlanetBoundingBox>  theChildrenBounds;
   ossimPlanetImage::ossimPlanetImageStateType theTextureState;


/*    bool theClusterParametersSet; */
/*    bool theUseClusterCulling; */
/*    osg::Vec3d theClusterCullingControlPoint; */
/*    osg::Vec3d theClusterCullingNormal; */
/*    double theClusterCullingDeviation; */
/*    double theClusterCullingRadius; */
   
};

#endif
