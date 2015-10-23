#ifndef ossimPlanetKmlPlacemarkNode_HEADER
#define ossimPlanetKmlPlacemarkNode_HEADER
#include <ossimPlanet/ossimPlanetKmlLayerNode.h>
#include <ossimPlanet/ossimPlanetLabelGeom.h>
#include <osg/MatrixTransform>
#include <ossimPlanet/ossimPlanetBillboardIcon.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetFadeText.h>
#include <osg/ClusterCullingCallback>
#include <ossimPlanet/ossimPlanetKml.h>

class ossimPlanetKmlPlacemarkNode : public ossimPlanetKmlLayerNode
{
public:
   ossimPlanetKmlPlacemarkNode(ossimPlanetKmlLayer* layer = 0,
                               ossimPlanetKmlObject* obj  = 0);
   virtual void traverse(osg::NodeVisitor& nv);

   virtual bool init();
   
protected:
   class PlacemarkGeometryDraw : public osg::Drawable::DrawCallback
   {
   public:
      PlacemarkGeometryDraw()
         :theOpacity(1.0)
      {
         
      }
      virtual void drawImplementation(osg::RenderInfo& /*renderInfo*/,
                                      const osg::Drawable* /*drawable*/)const;
   public:
      float theOpacity;
   };
   double convertHeight(const osg::Vec3d& kmlWorldPoint,
                        ossimPlanetAltitudeMode altMode,
                        ossimPlanetGeoRefModel* landModel)const;
                        
   void convertPointsToLocalCoordinates(osg::Vec3Array* result,
                                        const ossimPlanetKmlGeometry::PointListType& pointList,
                                        const osg::Matrixd& worldToLocalTransform,
                                        ossimPlanetGeoRefModel* landModel,
                                        ossimPlanetAltitudeMode altMode,
                                        double& minHeight,
                                        double& maxHeight)const;

   void extrude(osg::ref_ptr<osg::Geometry> result,
                osg::Vec3Array* verts,
                osg::Vec3Array* extrusionVerts,
                const std::vector<std::pair<ossim_uint32, ossim_uint32> >& extrusionGroups)const;



   double                              theNormalizationScale;
   osg::ref_ptr<osg::Group>            theKmlGeometries;
   osg::ref_ptr<osg::Group>            theKmlPickableGeometries;
   bool                                theCulledFlag;

   // we will shift the center of the bounds to the ground level
   // and use this for determining pixel coverage
   //
   ossim_float64                       theRadius;
   osg::Vec3d                          theCenter;

   ossim_float32                       theFadeAlpha;
   osg::ref_ptr<PlacemarkGeometryDraw> theDraw;
   osg::ref_ptr<ossimPlanetKmlRegion> theRegion;
   osg::ref_ptr<ossimPlanetKmlLod>    theLod;
};


#endif
