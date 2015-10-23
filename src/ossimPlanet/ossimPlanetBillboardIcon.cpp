#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanetBillboardIcon.h>
#include <osgUtil/CullVisitor>
#include <iostream>

ossimPlanetBillboardIcon::ossimPlanetBillboardIcon(double objectGroundSize)
{
   thePixelWidth    = 32;
   thePixelHeight   = 32;
   theMaxPixelSize  = 32; // will be reset to the images max width height
   theMinPixelSize  = 4;
   theNeedsUpdateFlag = true;
   theBillboard = new osg::Billboard;
   theBillboard->setMode(osg::Billboard::POINT_ROT_EYE);
   theGeom = new ossimPlanetIconGeom();
   theBillboard->addDrawable(theGeom.get());
   theBillboardTransform = new osg::MatrixTransform;
   theBillboardTransform->addChild(theBillboard);
   theFudgeFactorTransform = new osg::MatrixTransform;
   theFudgeFactorTransform->addChild(theBillboardTransform.get());
   setUpdateCallback(new ossimPlanetBillboardIconUpdateCallback);
                                      // we will set the scale for now so the icon comes in
   // at full res at approximately 5 kilomeer coverage.
   theBillboardTransform->setMatrix(osg::Matrixd::scale(objectGroundSize, objectGroundSize, objectGroundSize));
}

void ossimPlanetBillboardIcon::setGroundObjectSize(double groundSize)
{
   theBillboardTransform->setMatrix(osg::Matrixd::scale(groundSize,
                                                        groundSize,
                                                        groundSize));
}

void ossimPlanetBillboardIcon::setIcon(osg::ref_ptr<osg::Image> img)
{
   thePixelWidth   = img->s();
   thePixelHeight  = img->t();
   theMaxPixelSize = ossim::max(thePixelWidth, thePixelHeight);
   theGeom->setTexture(img.get());
   dirtyBound();
}

void ossimPlanetBillboardIcon::setIcon(osg::ref_ptr<osg::Texture2D> img)
{
   theGeom->setTexture(img.get());

   dirtyBound();
}

void ossimPlanetBillboardIcon::setGeom(osg::ref_ptr<ossimPlanetIconGeom> geom)
{
   theBillboard->setDrawable(0, geom.get());
   theGeom = geom.get();
   if(geom->texture().valid())
   {
      osg::Image* img = geom->texture()->getImage();
      if(img)
      {
         thePixelWidth   = img->s();
         thePixelHeight  = img->t();
         theMaxPixelSize = ossim::max(thePixelWidth, thePixelHeight);
      }
   }
   dirtyBound();
}

void ossimPlanetBillboardIcon::traverse(osg::NodeVisitor& nv)
{
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if(theNeedsUpdateFlag)
         {
            theFudgeFactorTransform->setMatrix(theFudgeFactorMatrix);
            theNeedsUpdateFlag = false;
            dirtyBound();
         }
         theFudgeFactorTransform->accept(nv);
         break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
         osgUtil::CullVisitor* cullVisitor = (osgUtil::CullVisitor*)(&nv);
         // make sure we compute scales from the original bill board geom and not from the current
         // bounds of the theTransform node.
         //
         const osg::BoundingSphere& bs = theBillboardTransform->getBound();
         double pixelSize = ossim::abs(cullVisitor->pixelSize(bs));
         double scaleFactor = theMaxPixelSize/pixelSize;
         //do some sanity checks
         if(pixelSize < theMinPixelSize)
         {
            scaleFactor = theMinPixelSize/pixelSize;
         }
         else if(pixelSize < theMaxPixelSize)
         {
            scaleFactor = 1.0;
         }
         theTranslate =osg::Vec3d(0.0,0.0,bs.radius()*.5*scaleFactor);
         theFudgeFactorMatrix = (osg::Matrixd::scale(osg::Vec3d(scaleFactor,
                                                                scaleFactor,
                                                                scaleFactor))*
                                 osg::Matrixd::translate(theTranslate));
         theNeedsUpdateFlag = true;
         theCulledFlag = cullVisitor->isCulled(*theFudgeFactorTransform);
         if(!theCulledFlag)
         {
           theFudgeFactorTransform->accept(nv);
         }
         break;
      }
      default:
      {
         theFudgeFactorTransform->accept(nv);
         break;
      }
   }
}       

osg::BoundingSphere ossimPlanetBillboardIcon::computeBound() const
{
   return theFudgeFactorTransform->getBound();
}

bool ossimPlanetBillboardIcon::isCulled()const
{
   return theCulledFlag;
}
