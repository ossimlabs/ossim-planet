#include <ossimPlanet/ossimPlanetLabelGeom.h>
#include <osgUtil/CullVisitor>

ossimPlanetLabelGeom::ossimPlanetLabelGeom(const ossimString& label,
                                           double characterSizeInObjectSpace)
{
   theColor = osg::Vec4f(1.0f,1.0f, 1.0f, 1.0f);
   theLabel = new osgText::Text;
   theLabel->setAxisAlignment(osgText::Text:: XZ_PLANE);
   theLabel->setColor(theColor);
   theLabel->setCharacterSize(characterSizeInObjectSpace);
   theLabel->setLayout(osgText::Text::LEFT_TO_RIGHT);
   theLabel->setPosition(osg::Vec3d(0.0,0.0,0.0));
   theLabel->setCharacterSizeMode(osgText::Text::OBJECT_COORDS);
   theLabel->setAlignment(osgText::Text:: CENTER_BOTTOM);
   theLabel->setMaximumWidth(-1.0);
   theLabel->setMaximumHeight(-1.0);
   theLabel->setText(label);
   theBillboard = new osg::Billboard;
   theBillboard->addDrawable(theLabel.get());
   theBillboard->setMode(osg::Billboard::POINT_ROT_WORLD);
   theTransform = new osg::MatrixTransform;
   theShowState = ossimPlanetLabelGeom::SHOW_WHEN_FULL_RES;
   theShowFlag = true;
   theEnableFlag = true;
   theTransform->addChild(theBillboard.get());
   
   
   setUpdateCallback(new ossimPlanetLabelGeomUpdateCallback());
   dirtyBound();   
}

ossimPlanetLabelGeom::~ossimPlanetLabelGeom()
{
   theLabel     = 0;
   theBillboard = 0;
   theTransform = 0;
}

void ossimPlanetLabelGeom::setAlignment(osgText::Text::AlignmentType alignment)
{
   theLabel->setAlignment(alignment);
   dirtyBound();
}

void ossimPlanetLabelGeom::setCharacterSizeInObjectSpace(double objectSize)
{
   theLabel->setCharacterSize(objectSize);
   dirtyBound();
}

osg::ref_ptr<osgText::Text> ossimPlanetLabelGeom::getLabel()
{
   return theLabel.get();
}

void ossimPlanetLabelGeom::setLabel(const ossimString& label)
{
   theLabel->setText(label);
   dirtyBound();
}
/**
 * Setting to 0 or negative says there is no max height.
 */ 
void ossimPlanetLabelGeom::setMaxHeightInPixels(double pixelHeight)
{
   theMaxHeightInPixels = pixelHeight;
   dirtyBound();
}
/**
 * Setting to 0 or negative value says there is no minimum height.
 */ 
void ossimPlanetLabelGeom::setMinHeightInPixels(double pixelHeight)
{
   theMinHeightInPixels = pixelHeight;
   dirtyBound();
}
   
void ossimPlanetLabelGeom::setColor(float r, float g, float b, float a)
{
   theColor[0] = r;
   theColor[1] = g;
   theColor[2] = b;
   theColor[3] = a;
   
   theLabel->setColor(theColor);
}

void ossimPlanetLabelGeom::setEnableFlag(bool enableFlag)
{
   theEnableFlag = enableFlag;
}

void ossimPlanetLabelGeom::setShowFlag(bool show)
{
   theShowFlag = show;
}

void ossimPlanetLabelGeom::traverse(osg::NodeVisitor& nv)
{
   if(!theEnableFlag) return;
   
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if(theUpdateTransformFlag)
         {
            theTransform->setMatrix(theMatrix);
            dirtyBound();
            theUpdateTransformFlag = false;
         }
         theTransform->accept(nv);
         break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
         osgUtil::CullVisitor* cullVisitor = (osgUtil::CullVisitor*)(&nv);
         double pixelSize = cullVisitor->pixelSize(osg::Vec3d(0.0,0.0,0.0), theLabel->getCharacterHeight());
         if(theMaxHeightInPixels > 0.0)
         {
            if(pixelSize > theMaxHeightInPixels)
            {
               theUpdateTransformFlag = true;
               double scale = theMaxHeightInPixels/pixelSize;
               theMatrix = osg::Matrixd::scale(osg::Vec3d(scale, scale, scale));
               if(theShowState == ossimPlanetLabelGeom::SHOW_WHEN_FULL_RES)
               {
                  theShowFlag = true;
               }
            }
            else if(theShowState == ossimPlanetLabelGeom::SHOW_WHEN_FULL_RES)
            {
               theShowFlag = false;
            }
         }
         else
         {
            if((theMinHeightInPixels > 0.0)&&
               (pixelSize < theMinHeightInPixels))
            {
               theUpdateTransformFlag = true;
               double scale = theMinHeightInPixels/pixelSize;
               theMatrix = osg::Matrixd::scale(osg::Vec3d(scale, scale, scale));
            }
            
            theShowFlag = true;
         }
         if(theShowFlag)
         {
            theTransform->accept(nv);
         }
         break;
      }
      default:
      {
         theTransform->accept(nv);
         break;
      }
   }
}

osg::BoundingSphere ossimPlanetLabelGeom::computeBound() const
{
   return theTransform->getBound();
}
