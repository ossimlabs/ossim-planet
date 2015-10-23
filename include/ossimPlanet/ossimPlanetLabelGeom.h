#ifndef ossimPlanetLabelGeom_HEADER
#define ossimPlanetLabelGeom_HEADER
#include <osg/Node>
#include <osg/CoordinateSystemNode>
#include <osg/MatrixTransform>
#include <osg/Billboard>
#include <osgText/Text>
#include <osg/Matrixd>
#include <osg/Vec4f>
#include <ossim/base/ossimString.h>

class ossimPlanetLabelGeom : public osg::Node
{
public:
   enum ShowState
   {
      SHOW_NO_STATE      = 0,
      SHOW_WHEN_FULL_RES = 1, // default
      SHOW_ALWAYS        = 2
   };
   class ossimPlanetLabelGeomUpdateCallback : public osg::NodeCallback
   {
      virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
         {
            ossimPlanetLabelGeom* n = dynamic_cast<ossimPlanetLabelGeom*>(node);
            if(n)
            {
               n->traverse(*nv);
            }
         }
   };
   ossimPlanetLabelGeom(const ossimString& label="",
                        double characterSizeInObjectSpace=1.0/osg::WGS_84_RADIUS_EQUATOR);
   virtual ~ossimPlanetLabelGeom();
   void setAlignment(osgText::Text::AlignmentType alignment);
   void setCharacterSizeInObjectSpace(double objectSize);
   osg::ref_ptr<osgText::Text> getLabel();
   void setLabel(const ossimString& label);
   /**
    * Setting to 0 or negative says there is no max height.
    */ 
   void setMaxHeightInPixels(double pixelHeight);
   /**
    * Setting to 0 or negative value says there is no minimum height.
    */ 
   void setMinHeightInPixels(double pixelHeight);
   
   void setColor(float r, float g, float b, float a=1.0);
   void setEnableFlag(bool enableFlag);
   void setShowFlag(bool show);
   virtual void traverse(osg::NodeVisitor& nv);
   virtual osg::BoundingSphere computeBound() const;
protected:
   /**
    * Specifies how the label should be shown.  You can show it once it reaches full resolution of the pixel
    * identified by the .
    */ 
   ossimPlanetLabelGeom::ShowState     theShowState;
   mutable bool                        theShowFlag;
   mutable bool                        theUpdateTransformFlag;
   bool                                theEnableFlag;
   double                              theMaxHeightInPixels;
   double                              theMinHeightInPixels;

   /**
    * used to make sure the font is at desired size in pixel if the
    * max or min font heights are specified.
    */ 
   osg::ref_ptr<osg::MatrixTransform>  theTransform;  
   osg::ref_ptr<osg::Billboard>        theBillboard;
   osg::ref_ptr<osgText::Text>         theLabel;
   osg::Matrixd                        theMatrix;
   osg::Vec4f                          theColor;
};


#endif
