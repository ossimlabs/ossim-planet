#include <iostream>
#include <sstream>
#include <ossimPlanet/ossimPlanetLatLonHud.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetLand.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetViewer.h>
#include <osg/StateSet>
#include <osg/Matrixd>
#include <osg/Material>
#include <osg/Projection>
#include <osg/MatrixTransform>
#include <osg/LineWidth>
#include <osg/PolygonMode>
#include <osgUtil/CullVisitor>
#include <osgUtil/IntersectVisitor>
#include <OpenThreads/ScopedLock>
#include <ossim/base/ossimDms.h>
#include <osgGA/EventVisitor>

class ossimPlanetLatLonHudUpdateCallback : public osg::NodeCallback
{
   virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
      {
         ossimPlanetLatLonHud* n = dynamic_cast<ossimPlanetLatLonHud*>(node);
         if(n)
         {
            if(n->enableFlag())
            {
               n->traverse(*nv);
            }
            return;
         }
         traverse(node, nv);
      }
};

ossimPlanetLatLonHud::ossimPlanetLatLonHud()
{
   setUpdateCallback(new ossimPlanetLatLonHudUpdateCallback);
   theInitializedFlag = false;
   theAutoUpdateFlag = true;
   theFontName = "arial.ttf";
   theFont = osgText::readFontFile(theFontName.c_str());
   theFontChanged = true;

   theCharacterSize = 16;
   theCharacterSizeDirtyFlag = true;
   theLookLabel  = "Look At:  ";
   theEyeLabel   = "Eye:      ";
   theRangeLabel = "Range:    ";
   theLatDisplayString = "dd.ddddddddddddC";
   theLonDisplayString = "ddd.ddddddddddddC";

   theCompass = new ossimPlanetCompass;
   theCompass->setRotateByViewMatrix(true);
   theCompass->buildCompass();
   theCompass->getOrCreateStateSet()->setAttribute(new osg::PolygonMode(), 
                                                   osg::StateAttribute::PROTECTED); // don't allow to go to wireframe

   theViewportChangedFlag = false;
}

void ossimPlanetLatLonHud::traverse(osg::NodeVisitor& nv)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   if(!theEnableFlag) return;
   bool traverseChildren = theViewport.valid();
   if(!thePlanet)
   {
      thePlanet = ossimPlanet::findPlanet(this);
   }
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if(!theInitializedFlag)
         {
            initialize();
         }
         if(theViewportChangedFlag&&theViewport.valid())
         {
            theCameraNode->setProjectionMatrix(osg::Matrix::ortho2D(theViewport->x(),
                                                                    theViewport->width(),
                                                                    theViewport->y(),
                                                                    theViewport->height()));
            theViewportChangedFlag = false;
            setRedrawFlag(true);
         }
         if(theAutoUpdateFlag&&thePlanet)
         {
 //           osg::Vec3d latLonHeight    = thePlanet->getLineOfSiteLatLonHeightPoint();
 //           osg::Vec3d nadir           = thePlanet->getNadirLatLonHeightPoint();

 //           theLatLonHeight[0] = latLonHeight[0];
 //           theLatLonHeight[1] = latLonHeight[1];
 //           theNadirLatLonHeight[0] = nadir[0];
 //           theNadirLatLonHeight[1] = nadir[1];
         }
         if(theViewport.valid())
         {
            updatePosition();
            Group::traverse(nv);            
         }
         break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
         if(!theInitializedFlag) return;
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
                  setRedrawFlag(true);
                  theViewportChangedFlag = true;
               }
            }
         }
         if(traverseChildren)
         {
            Group::traverse(nv);
         }
         break;
      }
      case osg::NodeVisitor::EVENT_VISITOR:
      {
         osgGA::EventVisitor* ev = dynamic_cast<osgGA::EventVisitor*>(&nv);
         if(ev)
         {
            ossimPlanetViewer* viewer = dynamic_cast<ossimPlanetViewer*>(ev->getActionAdapter());
            if(viewer&&viewer->currentCamera()&&viewer->currentLookAt())
            {
               theNadirLatLon[0] = viewer->currentCamera()->lat(); 
               theNadirLatLon[1] = viewer->currentCamera()->lon(); 
               theNadirLatLon[2] = viewer->currentCamera()->altitude(); 
               theAltitude = viewer->currentCamera()->altitude(); 
               theLineOfSiteLatLon[0] = viewer->currentLookAt()->lat(); 
               theLineOfSiteLatLon[1] = viewer->currentLookAt()->lon(); 
               theLineOfSiteLatLon[2] = viewer->currentLookAt()->altitude(); 
               if(theModel.valid())
               {
                  theModel->ellipsoidalToMsl(theNadirLatLon);
                  theModel->ellipsoidalToMsl(theLineOfSiteLatLon);
               }
               theRange = viewer->currentLookAt()->range(); 
               if(viewer->model())
               {
                  theAltitude -= viewer->model()->getGeoidOffset(theNadirLatLon[0],theNadirLatLon[1]);
               }
               if(theCompass.valid())
               {
                  theCompass->setHeading(viewer->currentCamera()->heading());
                  theCompass->setPitch(viewer->currentCamera()->pitch());
                  theCompass->setRoll(viewer->currentCamera()->roll());
               }
            }
         }
         break;
      }
      default:
      {
         break;
      }
   }
}

void ossimPlanetLatLonHud::initialize()
{   
   theCrosshairColor  = osg::Vec4d(1.0,1.0,1.0,0.6);
   theTextColor       = osg::Vec4d(1.0,1.0,1.0,0.6);
   theGeode     = new osg::Geode();
   theCrosshair = new osg::Geometry;
   theCrosshairLineWidth = new osg::LineWidth(3);
   osg::StateSet* stateset = theGeode->getOrCreateStateSet();
   stateset->setMode(GL_LIGHTING,
                     osg::StateAttribute::OFF);
//   stateset->setMode(GL_COLOR_MATERIAL,
//                     osg::StateAttribute::OFF);
   stateset->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
   stateset->setRenderBinDetails(11,"RenderBin");
   stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
   stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
   stateset->setAttribute(theCrosshairLineWidth.get(),osg::StateAttribute::ON);
   stateset->setAttribute(new osg::PolygonMode(), 
                          osg::StateAttribute::PROTECTED); // don't allow to go to wireframe
   
   theLookText         = new osgText::Text;
   theEyeText          = new osgText::Text;
   theRangeText        = new osgText::Text;
   
   theLookText->setBackdropType(osgText::Text::OUTLINE);
   theEyeText->setBackdropType(osgText::Text::OUTLINE);
   theRangeText->setBackdropType(osgText::Text::OUTLINE);
   
   
   theGeode->addDrawable(theLookText.get());
   theGeode->addDrawable(theEyeText.get());
   theGeode->addDrawable(theRangeText.get());
   theGeode->addDrawable(theCrosshair.get());

   theCameraNode = new osg::CameraNode;
   
//    osg::MatrixTransform* modelview_abs = new osg::MatrixTransform;
   theCameraNode->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
   theCameraNode->setProjectionMatrix(osg::Matrix::ortho2D(0,1024,0,1024));
   theCameraNode->setViewMatrix(osg::Matrix::identity());
   theCameraNode->setClearMask(GL_DEPTH_BUFFER_BIT);
   theCameraNode->setRenderOrder(osg::CameraNode::POST_RENDER);
   theCameraNode->addChild(theGeode.get());
   
//    theProjection = new osg::Projection;
//    theProjection->setMatrix(osg::Matrix::ortho2D(0,1024,0,1024));
//    theProjection->addChild(modelview_abs);

   
   addChild(theCameraNode.get());

   theLookText->setSupportsDisplayList(false);
   theEyeText->setSupportsDisplayList(false);
   theRangeText->setSupportsDisplayList(false);
   theCrosshair->setSupportsDisplayList(false);

   theGeode->setStateSet(stateset);

   theCameraNode->addChild(theCompass.get());
   
   
   theInitializedFlag = true;
}

void ossimPlanetLatLonHud::updatePosition()
{
   char tempBuf[255];
   if(theCompass.valid())
   {
      theCompass->setPosition(osg::Vec3d(theViewport->width()-128,
                                         theViewport->height()-128,
                                         0.0));
      theCompass->setScale(128);
      //      if(thePlanet)
      //      {
      //         theCompass->setHeading(thePlanet->hpr()[0]);
      //        theCompass->setPitch(thePlanet->hpr()[1]);
      //        theCompass->setRoll(thePlanet->hpr()[2]);
      //     }
   }
   if(ossim::isnan(theLineOfSiteLatLon[0]) ||
      ossim::isnan(theLineOfSiteLatLon[1]))
   {
      theLookText->setText(theLookLabel + "NaN");
      //      theLonText->setText(theLookLabel + "NaN");
   }
   else
   {
      ossimDms latDms(theLineOfSiteLatLon[0]);
      ossimDms lonDms(theLineOfSiteLatLon[1], false);
      ossimString text = (theLookLabel + latDms.toString(theLatDisplayString.c_str()) + ", " 
                          + lonDms.toString(theLonDisplayString.c_str())+ ", " 
                          + ossimString::toString(theLineOfSiteLatLon[2]));
      theLookText->setText(text);
   }
   if(ossim::isnan(theNadirLatLon[0]) ||
      ossim::isnan(theNadirLatLon[1]))
   {
      theEyeText->setText(theLookLabel + "NaN");
      //      theLonText->setText(theLookLabel + "NaN");
   }
   else
   {
      ossimDms latDms(theNadirLatLon[0]);
      ossimDms lonDms(theNadirLatLon[1], false);
      ossimString text = (theEyeLabel + latDms.toString(theLatDisplayString.c_str()) + ", " 
                          + lonDms.toString(theLonDisplayString.c_str())+ ", " 
                          + ossimString::toString(theNadirLatLon[2]));
      theEyeText->setText(text);
   }
   if(!ossim::isnan(theRange))
   {
      sprintf(tempBuf,"%lf meters", theRange);
      theRangeText->setText((theRangeLabel + tempBuf).c_str());
   }
   else
   {
      sprintf(tempBuf,"%s", "NaN");
      theRangeText->setText((theRangeLabel + tempBuf).c_str());
   }
   if((theFont.valid())&&
      (theFontChanged))
   {
      theLookText->setFont(theFont.get());
      theEyeText->setFont(theFont.get());
      theRangeText->setFont(theFont.get());
      
//      theLonText->setFont(theFont.get());
//      theHeightText->setFont(theFont.get());
//      theLineOfSiteText->setFont(theFont.get());
      theFontChanged = false;
   }
   if(theCharacterSizeDirtyFlag)
   {
      theLookText->setCharacterSize(theCharacterSize);
      theEyeText->setCharacterSize(theCharacterSize);
      theRangeText->setCharacterSize(theCharacterSize);
//      theLonText->setCharacterSize(theCharacterSize);
//      theHeightText->setCharacterSize(theCharacterSize);
//      theLineOfSiteText->setCharacterSize(theCharacterSize);
//      theCharacterSizeDirtyFlag = false;
   }
   double ulx = theViewport->x();
   double uly = theViewport->y();
   double x = ulx;
   double y = uly;
   double w = theViewport->width();
   double h = theViewport->height();

   osg::BoundingBox bb;

//   bb = osg::BoundingBox();
   osg::Vec3d pos = osg::Vec3d(x, y+5, 0.0);
//   bb.expandBy(theRangeText->getBound());
   bb = theRangeText->getBound();
   theRangeText->setPosition(pos);
   int height = (int)(bb.yMax() - bb.yMin()) + 5;
   pos += osg::Vec3d(0.0,(height), 0.0);
   theLookText->setPosition(pos);
   bb = theLookText->getBound();
//   bb.expandBy(theLookText->getBound());
   height = (int)(bb.yMax() - bb.yMin()) + 5;
   pos += osg::Vec3d(0.0,(height), 0.0);
   theEyeText->setPosition(pos);

   theLookText->setColor(theTextColor);
   theEyeText->setColor(theTextColor);
   theRangeText->setColor(theTextColor);
   
   osg::Vec2Array* vertices = 0;
   osg::Vec4Array* colorArray = 0;
   if(!theCrosshair->getVertexArray())
   {
      vertices = new osg::Vec2Array;
      vertices->push_back(osg::Vec2(0.0,0.0));
      vertices->push_back(osg::Vec2(0.0,0.0));
      vertices->push_back(osg::Vec2(0.0,0.0));
      vertices->push_back(osg::Vec2(0.0,0.0));
      vertices->push_back(osg::Vec2(0.0,0.0));
      vertices->push_back(osg::Vec2(0.0,0.0));
      vertices->push_back(osg::Vec2(0.0,0.0));
      vertices->push_back(osg::Vec2(0.0,0.0));
      theCrosshair->setVertexArray(vertices);
      theCrosshair->addPrimitiveSet(new osg::DrawArrays(GL_LINES,0,8));
      colorArray = new osg::Vec4Array;
      colorArray->push_back(theCrosshairColor);
      theCrosshair->setColorArray(colorArray);
      theCrosshair->setColorBinding(osg::Geometry::BIND_OVERALL);
   }
   else
   {
      vertices = dynamic_cast<osg::Vec2Array*>(theCrosshair->getVertexArray());
      colorArray = dynamic_cast<osg::Vec4Array*>(theCrosshair->getColorArray());
      
   }
   if(colorArray)
   {
      (*colorArray)[0] = theCrosshairColor;
   }
   if(vertices)
   {
      int centerx = (int)(((double)ulx + (double)w/2));
      int centery = (int)(((double)uly + (double)h/2));
      
      (*vertices)[0][0] = centerx - 20;
      (*vertices)[0][1] = centery;
      (*vertices)[1][0] = centerx - 5;
      (*vertices)[1][1] = centery;
      (*vertices)[2][0] = centerx + 5;
      (*vertices)[2][1] = centery;
      (*vertices)[3][0] = centerx + 20;
      (*vertices)[3][1] = centery;
      
      (*vertices)[4][0] = centerx;
      (*vertices)[4][1] = centery + 20;
      (*vertices)[5][0] = centerx;
      (*vertices)[5][1] = centery + 5;
      (*vertices)[6][0] = centerx;
      (*vertices)[6][1] = centery - 5;
      (*vertices)[7][0] = centerx;
      (*vertices)[7][1] = centery - 20;
   }
}

osg::Vec4 ossimPlanetLatLonHud::getCrossHairColor()const
{
   return theCrosshairColor;
}

osg::Vec4 ossimPlanetLatLonHud::getTextColor()const
{
   return theTextColor;
}

void ossimPlanetLatLonHud::setCrosshairColor(const osg::Vec4& color)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theCrosshairColor = color;
}

void ossimPlanetLatLonHud::setFont(const ossimString& fontFile)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theFontChanged = theFontName != fontFile;
   theFontName = fontFile;
   theFont = osgText::readFontFile(fontFile.c_str());
}

void ossimPlanetLatLonHud::setTextColor(const osg::Vec4& color)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theTextColor = color;
}

void ossimPlanetLatLonHud::setLatDisplayString(const ossimString& latDisplayString)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theLatDisplayString = latDisplayString;
}

void ossimPlanetLatLonHud::setLonDisplayString(const ossimString& lonDisplayString)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theLonDisplayString = lonDisplayString;
}

void ossimPlanetLatLonHud::setCharacterSize(float size)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theCharacterSizeDirtyFlag = size != theCharacterSize;
   theCharacterSize = size;
   
}

void ossimPlanetLatLonHud::setViewport(osg::ref_ptr<osg::Viewport> viewport)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theViewport = viewport;
}

void ossimPlanetLatLonHud::setAutoUpdateFlag(bool flag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theAutoUpdateFlag = flag;
}

void ossimPlanetLatLonHud::setCompassTexture(const ossimFilename& compass)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   if(theCompass.valid())
   {
      theCompass->setCompassTexture(compass);
      theCompass->updateCompass();
      setRedrawFlag(true);
   }
}

#if 0
void ossimPlanetLatLonHud::setCompassTexture(const ossimFilename& ring,
                                             const ossimFilename& interior)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   if(theCompass.valid())
   {
      theCompass->setCompassTexture(ring, interior);
      theCompass->updateCompass();
      setRedrawFlag(true);
   }
}
#endif

void ossimPlanetLatLonHud::execute(const ossimPlanetAction& action)
{
#if 0
   if(action.command() == "init")
   {
      ossimPlanetLayer::execute(action); // initialize the base.
   }
   else
   {
      ossimPlanetLayer::execute(action);
   }
#endif  
}
