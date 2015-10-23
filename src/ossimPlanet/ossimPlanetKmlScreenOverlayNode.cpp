#include <ossimPlanet/ossimPlanetKmlScreenOverlayNode.h>
#include <osg/Projection>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/Matrixd>
#include <osgUtil/CullVisitor>
#include <ossimPlanet/ossimPlanetKmlLayer.h>
#include <osgUtil/IntersectVisitor>

ossimPlanetKmlScreenOverlayNode::ossimPlanetKmlScreenOverlayNode(ossimPlanetKmlLayer* layer,
                                                                 ossimPlanetKmlObject* obj)
   :ossimPlanetKmlLayerNode(layer, obj)
{
   theCameraNode = new osg::CameraNode;
   theGroup = new osg::Group;
   theCameraNode->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
   theCameraNode->setViewMatrix(osg::Matrix::identity());
   theCameraNode->setClearMask(GL_DEPTH_BUFFER_BIT);
   theCameraNode->setRenderOrder(osg::CameraNode::POST_RENDER);
   theGroup->addChild(theCameraNode.get());
   theViewportChangedFlag = false;
   theNeedsUpdateFlag = false;
   osg::StateSet* stateset = theCameraNode->getOrCreateStateSet();
   stateset->setMode(GL_LIGHTING,
                     osg::StateAttribute::OFF);
//   stateset->setMode(GL_COLOR_MATERIAL,
//                     osg::StateAttribute::OFF);
   stateset->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
//    stateset->setRenderBinDetails(12,"RenderBin");
   stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
   stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
}

void ossimPlanetKmlScreenOverlayNode::traverse(osg::NodeVisitor& nv)
{
   if(!enableFlag()) return;
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if(theViewport.valid()&&theViewportChangedFlag)
         {
            theCameraNode->setProjectionMatrix(osg::Matrix::ortho2D(theViewport->x(),
                                                                    theViewport->width(),
                                                                    theViewport->y(),
                                                                    theViewport->height()));
            theViewportChangedFlag = false;
            update();
        }

         if(theNeedsUpdateFlag)
         {
            update();
         }
         break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
         osgUtil::CullVisitor* cullVisitor = dynamic_cast<osgUtil::CullVisitor*>(&nv);
         if(cullVisitor)
         {
            double x = cullVisitor->getViewport()->x();
            double y = (int)cullVisitor->getViewport()->y();
            double w = (int)cullVisitor->getViewport()->width();
            double h = (int)cullVisitor->getViewport()->height();
            if(!theViewport.valid())
            {
               theViewport = new osg::Viewport(x,y,w,h);
               theViewportChangedFlag = true;
            }
            else
            {
               if( !ossim::almostEqual(theViewport->x(), x)||
                   !ossim::almostEqual(theViewport->y(), y)||
                   !ossim::almostEqual(theViewport->width(), w)||
                   !ossim::almostEqual(theViewport->height(), h))
               {
                  theViewport->setViewport(x,y,w,h);
                  theViewportChangedFlag = true;
               }
            }
         }
         break;
      }
      default:
      {
         if(dynamic_cast<osgUtil::IntersectVisitor*>(&nv))
         {
            return;
         }
         break;
      }
   }
         // if the viewport is valid and we have been setup to draw for the current viewport
         //
   if(theViewport.valid())
   {
      theGroup->accept(nv);
   }
}

bool ossimPlanetKmlScreenOverlayNode::init()
{
   osg::ref_ptr<ossimPlanetKmlScreenOverlay> overlay = dynamic_cast<ossimPlanetKmlScreenOverlay*> (theKmlObject.get());
   if(!overlay.valid()) return false;
   theViewportChangedFlag = true;
   theOverlayOrigin[0] = overlay->overlayX();
   theOverlayOrigin[1] = overlay->overlayY();
   theOverlayXUnits    = overlay->overlayXUnits();
   theOverlayYUnits    = overlay->overlayYUnits();
   theScreenOrigin[0] = overlay->screenX();
   theScreenOrigin[1] = overlay->screenY();
   theScreenXUnits    = overlay->screenXUnits();
   theScreenYUnits    = overlay->screenYUnits();
   theRotationOrigin[0] = overlay->rotationX();
   theRotationOrigin[1] = overlay->rotationY();
   theRotationXUnits    = overlay->rotationXUnits();
   theRotationYUnits    = overlay->rotationYUnits();
   theSize[0] = overlay->sizeX();
   theSize[1] = overlay->sizeY();
   theSizeXUnits = overlay->sizeXUnits();
   theSizeYUnits = overlay->sizeYUnits();
   theRotation   = overlay->rotation();
   
   if(overlay->icon().valid())
   {
//       std::cout << "Downoading overlay image" << std::endl;
      ossimFilename file = overlay->icon()->download();
//      std::cout << "file = " << file << std::endl;
      if(file.exists())
      {
         theIconGeode = new osg::Geode;
         
         theIconGeom = theLayer->getOrCreateIconEntry(file);
         theIconGeom->setGeometry(osg::Vec3d(0.0,0.0,0.0),
                                  osg::Vec3d(0.0, 0.0, 0.0),
                                  osg::Vec3d(0.0, 0.0, 0.0));
         theIconGeode->addDrawable(theIconGeom.get());
         theCameraNode->addChild(theIconGeode.get());
      }
      else
      {
         return false;
      }
   }

   return overlay->icon().valid();
}

void ossimPlanetKmlScreenOverlayNode::update()
{
   if((!theIconGeom.valid())||(!theIconGeom->texture().valid())) return;
   if(theIconGeom->texture()->getImage())
   {
      ossim_uint32 w = theIconGeom->texture()->getImage()->s();
      ossim_uint32 h = theIconGeom->texture()->getImage()->t();
      if(w||h)
      {
         osg::Vec3d screenOrigin;
         osg::Vec3d overlayOrigin;
         osg::Vec3d size;
         computeScreenXY(screenOrigin);
         computeOverlayXY(overlayOrigin);
         computeSize(size);
//          std::cout << "oringx = " << origin[0] << std::endl;
//          std::cout << "oringy = " << origin[1] << std::endl;
         theIconGeom->setGeometry(screenOrigin-overlayOrigin,
                                  osg::Vec3d(size[0], 0.0, 0.0),
                                  osg::Vec3d(0.0, size[1], 0.0));
//       std::cout << "W = " << theIconGeom->texture()->getImage()->s() << std::endl;
         theNeedsUpdateFlag = false;
      }
      else
      {
         theNeedsUpdateFlag = true;
      }
   }
   else
   {
      theNeedsUpdateFlag = true;
//       std::cout << "NOT VALID!!!" << std::endl;
   }
}

void ossimPlanetKmlScreenOverlayNode::computeOverlayXY(osg::Vec3d& position)
{
   osg::Vec3d size;
   computeSize(size);
   switch(theOverlayXUnits)
   {
      case ossimPlanetKmlUnits_FRACTION:
      {
         position[0] = theOverlayOrigin[0]*size[0];
         break;
      }
      case ossimPlanetKmlUnits_PIXELS:
      {
         position[0] = theOverlayOrigin[0];
         break;
      }
      case ossimPlanetKmlUnits_INSET_PIXELS:
      {
         break;
      }
      default:
      {
         break;
      }
   }
   switch(theScreenYUnits)
   {
      case ossimPlanetKmlUnits_FRACTION:
      {
         position[1] = theOverlayOrigin[1]*size[1];
         
         break;
      }
      case ossimPlanetKmlUnits_PIXELS:
      {
         position[1] = theOverlayOrigin[1];
         break;
      }
      case ossimPlanetKmlUnits_INSET_PIXELS:
      {
         break;
      }
      default:
      {
         break;
      }
   }   
}

void ossimPlanetKmlScreenOverlayNode::computeScreenXY(osg::Vec3d& position)
{
   switch(theScreenXUnits)
   {
      case ossimPlanetKmlUnits_FRACTION:
      {
         position[0] = theViewport->x() + theScreenOrigin[0]*theViewport->width();
         break;
      }
      case ossimPlanetKmlUnits_PIXELS:
      {
         position[0] = theViewport->x() + theScreenOrigin[0];
         break;
      }
      case ossimPlanetKmlUnits_INSET_PIXELS:
      {
         break;
      }
      default:
      {
         break;
      }
   }
   switch(theScreenYUnits)
   {
      case ossimPlanetKmlUnits_FRACTION:
      {
         position[1] = theViewport->y() + theScreenOrigin[1]*theViewport->height();
         
         break;
      }
      case ossimPlanetKmlUnits_PIXELS:
      {
         position[1] = theViewport->y() + theScreenOrigin[1];
         break;
      }
      case ossimPlanetKmlUnits_INSET_PIXELS:
      {
         break;
      }
      default:
      {
         break;
      }
   }
}

void ossimPlanetKmlScreenOverlayNode::computeSize(osg::Vec3d& size)
{
   ossim_uint32 iw = (ossim_uint32)theIconGeom->texture()->getImage()->s();
   ossim_uint32 ih = (ossim_uint32)theIconGeom->texture()->getImage()->t();
   ossim_uint32 vw = (ossim_uint32)theViewport->width();
   ossim_uint32 vh = (ossim_uint32)theViewport->height();
   if(theSize[0] < 0)
   {
      size[0] = iw;
   }
   else if(theSize[0] > 0)
   {
      switch(theSizeXUnits)
      {
         case ossimPlanetKmlUnits_FRACTION:
         {
            size[0] = theSize[0]*vw;
            break;
         }
         case ossimPlanetKmlUnits_PIXELS:
         {
            size[0] = theSize[0];
            break;
         }
         case ossimPlanetKmlUnits_INSET_PIXELS:
         {
            break;
         }
         default:
         {
            break;
         }
      }
   }
   if(theSize[1] < 0)
   {
      size[1] = ih;
   }
   else if(theSize[1] > 0)
   {
      switch(theSizeXUnits)
      {
         case ossimPlanetKmlUnits_FRACTION:
         {
            size[1] = theSize[1]*vh;
            break;
         }
         case ossimPlanetKmlUnits_PIXELS:
         {
            size[1] = theSize[1];
            break;
         }
         case ossimPlanetKmlUnits_INSET_PIXELS:
         {
            break;
         }
         default:
         {
            break;
         }
      }
   }

   if(theSize[0] == 0 && theSize[1] == 0)
   {
      size[0] = iw;
      size[1] = ih;
   }
   else if(theSize[0] == 0)
   {
      double scale = (double)size[1]/(double)ih;
      size[0] = scale*iw;
   }
   else if(theSize[1] == 0)
   {
      double scale = (double)size[0]/(double)iw;
      size[1] = scale*ih;
   }
}
