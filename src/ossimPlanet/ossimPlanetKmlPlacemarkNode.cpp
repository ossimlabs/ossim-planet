#include <ossimPlanet/ossimPlanetKmlPlacemarkNode.h>
#include <osgUtil/IntersectVisitor>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/ossimPlanetKmlLayer.h>
#include <ossimPlanet/ossimPlanetVisitors.h>
#include <ossimPlanet/ossimPlanetCubeGrid.h>
#include <osg/LineWidth>
#include <osg/PolygonOffset>
#include <osg/PolygonMode>
#include <osg/CullSettings>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/TriStripVisitor>
#include <osgUtil/CullVisitor>
#include <osgUtil/Tessellator>
#include <osg/TriangleFunctor>
#include <osg/ShadeModel>
#include <osg/Point>
#include <osg/Node>
#include <osgDB/ReadFile>


void ossimPlanetKmlPlacemarkNode::PlacemarkGeometryDraw::drawImplementation(osg::RenderInfo& renderInfo,
                                                                            const osg::Drawable* drawable)const
{
   osg::Drawable* nonConstDrawable = const_cast<osg::Drawable*>(drawable);
   osg::Geometry* geom = dynamic_cast<osg::Geometry*>(nonConstDrawable);
   if(geom)
   {
      osg::Vec4Array* array = dynamic_cast<osg::Vec4Array*>(geom->getColorArray());
      if(array&&array->size()==1)
      {
         float previous = (*array)[0][3];
         (*array)[0][3] *=theOpacity;
         geom->setColorArray(array);
         drawable->drawImplementation(renderInfo);
         (*array)[0][3] = previous;
         geom->setColorArray(array);
         
      }
      else
      {
         drawable->drawImplementation(renderInfo);
      }
   }
   else
   {
      drawable->drawImplementation(renderInfo);
   }
}

ossimPlanetKmlPlacemarkNode::ossimPlanetKmlPlacemarkNode(ossimPlanetKmlLayer* layer,
                                                         ossimPlanetKmlObject* obj)
   :ossimPlanetKmlLayerNode(layer, obj)
{
//    setCullingActive(false);
   getOrCreateStateSet()->setMode( GL_LIGHTING,
                                   osg::StateAttribute::PROTECTED | osg::StateAttribute::OFF );
   theCulledFlag = false;
}


void ossimPlanetKmlPlacemarkNode::traverse(osg::NodeVisitor& nv)
{
   if(!enableFlag()) return;
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::CULL_VISITOR:
      {
         theCulledFlag = false;
         osgUtil::CullVisitor* cullVisitor = dynamic_cast<osgUtil::CullVisitor*>(&nv);
         if(theLod.valid())
         {
//            float opacity = 1.0;
            // divide by root 2
            //  this is a unit square so the radius is more than likely similar to the radius along the
            // diagonal.  We just need a crude approximation to give the visual afect we want.
            //
            float pixelSize = cullVisitor->clampedPixelSize(theCenter, theRadius)/1.41421356237309504880;
            if(pixelSize > theLod->minLodPixels())
            {
               ossim_int32 maxLod = theLod->maxLodPixels();
               if((maxLod > 0)&&(pixelSize > maxLod))
               {
                  theCulledFlag = true;
                  return;
               }
            }
            else
            {
               theCulledFlag = true;
               return;
            }
            if(theLod->minFadeExtent()&&
               theLod->maxFadeExtent()&&
               (theLod->minFadeExtent()<theLod->maxFadeExtent()))
            {
               if (pixelSize < theLod->minLodPixels())
               {
                  theFadeAlpha=0;
               }
               else if((pixelSize < (theLod->minLodPixels() + theLod->minFadeExtent())))
               {
                  theFadeAlpha=(pixelSize - theLod->minLodPixels())/theLod->minFadeExtent();
               }
               else if(pixelSize < (theLod->maxLodPixels() - theLod->maxFadeExtent()))
               {
                  theFadeAlpha=1;
               }
               else if (pixelSize < theLod->maxLodPixels())
               {
                  theFadeAlpha=(theLod->maxLodPixels()-pixelSize)/theLod->maxFadeExtent();
               }
               else
               {
                  theFadeAlpha=0;
               }
               if(theDraw.valid())
               {
                  theDraw->theOpacity = theFadeAlpha;
               }
               if(theFadeAlpha==0)
               {
                  theCulledFlag = true;
                  return;
               }
            }
            else
            {
               if(theDraw.valid())
               {
                  theDraw->theOpacity = 1.0;
               }
            }
         }
         break;
      }
      default:
      {
         if(theCulledFlag)
         {
            return;
         }
      }
   }
   if(theKmlGeometries.valid())
   {
      if(dynamic_cast<osgUtil::IntersectVisitor*>(&nv))
      {
         if(theKmlPickableGeometries.valid())
         {
            theKmlPickableGeometries->accept(nv);
         }
      }
      else
      {
         theKmlGeometries->accept(nv);
      }
   }
}
#if 0
static std::ostream& operator << (std::ostream& out, const osg::Vec3d& src)
{
   out << "<" << src[0] <<", " << src[1] << ", " << src[2];
   return out;
}
static std::ostream& operator << (std::ostream& out, const osg::Vec4& src)
{
   out << "<" << src[0] <<", " << src[1] << ", " << src[2] << ", " << src[3];
   return out;
}
#endif

class ossimPlanetKmlTextShiftUpdate : public osg::Drawable::UpdateCallback
{
public:
   ossimPlanetKmlTextShiftUpdate(ossimPlanetBillboardIcon* icon,
                                 osg::ClusterCullingCallback* clusterCallback,
                                 osg::ref_ptr<osg::Drawable::UpdateCallback> oldCallback=0)
      :theIcon(icon),
       theCallback(clusterCallback),
       theOldCallback(oldCallback)
      {}
   virtual void update(osg::NodeVisitor* nv, osg::Drawable* drawable)
      {
         osgText::Text* text = dynamic_cast<osgText::Text*>(drawable);
         if(text&&theIcon)
         {
            if(theOldCallback.valid())
            {
               theOldCallback->update(nv, drawable);
            }
            osg::Vec3d labelShift(0.0,0.0,0.0);
            osg::BoundingSphere bs = theIcon->getBound();
            if(bs.valid())
            {
               labelShift[2] = bs.radius()*1.5;
            }
            
            if(!ossim::almostEqual(labelShift[2],
                                   (double)text->getPosition()[2],
                                   DBL_EPSILON))
            {
               text->setPosition(labelShift);
//                if(theCallback)
//                {
//                   theCallback->set(labelShift,
//                                    theCallback->getNormal(),
//                                    theCallback->getDeviation(),
//                                    theCallback->getRadius());
//                }
               text->dirtyBound();
            }
         }
      }
   ossimPlanetBillboardIcon* theIcon;
   osg::ClusterCullingCallback* theCallback;
   osg::ref_ptr<osg::Drawable::UpdateCallback> theOldCallback;
};
bool ossimPlanetKmlPlacemarkNode::init()
{
   if(!theLayer) return false;
   ossimPlanetGeoRefModel* landModel = theLayer->landModel();
   ossimPlanetKmlPlacemark* placemark = dynamic_cast<ossimPlanetKmlPlacemark*>(theKmlObject.get());
   if(!placemark||!landModel) return false;
   
   std::vector<osg::ref_ptr<ossimPlanetKmlGeometry> > primitiveGeomList;
   theKmlGeometries = new osg::Group;
   theKmlPickableGeometries = new osg::Group;
   
   // load placemark feature
   osg::ref_ptr<ossimPlanetKmlGeometry> geom = placemark->getGeometry();
   if(geom.valid())
   {
      if(geom->toMultiGeometry())
      {
         std::vector<osg::ref_ptr<ossimPlanetKmlGeometry> >& multiGeomList = geom->toMultiGeometry()->geomtryList();
         
         primitiveGeomList.insert(primitiveGeomList.begin(),
                                  multiGeomList.begin(),
                                  multiGeomList.end());
      }
      else
      {
         primitiveGeomList.push_back(geom.get());
      }
   }
   
   theRegion = placemark->region();
   if(theRegion.valid())
   {
      theLod = theRegion->lod();
   }
   else
   {
      theLod = 0;
   }
   // let's setup the global stuff that all geometries will need access to to initialize itself
   //
   static osg::ref_ptr<osgText::Font> defaultFont = osgText::readFontFile("fonts/arial.ttf");
   ossimPlanetKml* kml = dynamic_cast<ossimPlanetKml*>(placemark->getRoot(placemark));
   theNormalizationScale = landModel->getNormalizationScale();
   const ossimPlanetKmlStyleSelector* styleSelector = placemark->getStyleSelector().get();
   ossimString styleUrl = placemark->styleUrl();
   
   if(kml)
   {
//       std::cout << "STYLE URL == " << styleUrl << std::endl;
      // check for the inline style
      if(!styleSelector&&!styleUrl.empty())
      {
         if(*(styleUrl.begin()) == '#')
         {
            // no inline style
            // Now check for shared style
            const osg::ref_ptr<ossimPlanetKmlObject> obj = kml->findById(styleUrl.after("#"));
            if(obj.valid())
            {
               styleSelector = dynamic_cast<const ossimPlanetKmlStyleSelector*>(obj.get());
            }
         }
      }
   }

   const ossimPlanetKmlStyle*        normalStyle = 0;
   const ossimPlanetKmlStyle*        highlightStyle = 0;
   const ossimPlanetKmlIconStyle*    normalIconStyle = 0;
   const ossimPlanetKmlLabelStyle*   normalLabelStyle = 0;
   // const ossimPlanetKmlLineStyle*    normalLineStyle = 0;
   const ossimPlanetKmlPolyStyle*    normalPolyStyle = 0;
   // const ossimPlanetKmlBalloonStyle* normalBalloonStyle = 0;
   const ossimPlanetKmlStyleMap*     styleMap = 0;

   if(styleSelector)
   {
      styleMap = styleSelector->toStyleMap();
      if(styleMap&&kml)
      {
         const ossimPlanetKmlObject* obj = kml->findById(styleMap->normalUrl().after("#")).get();
         styleSelector = dynamic_cast<const ossimPlanetKmlStyleSelector*>(obj);
         if(styleSelector)
         {
//             std::cout << "Found normal style!" << std::endl;
            normalStyle = styleSelector->toStyle();
         }
         obj = kml->findById(styleMap->highlightUrl().after("#")).get();
         styleSelector = dynamic_cast<const ossimPlanetKmlStyleSelector*>(obj);
         if(styleSelector)
         {
            highlightStyle = styleSelector->toStyle();
         }
      }
      else 
      {
         normalStyle = styleSelector->toStyle();
      }
      
      if(normalStyle)
      {
         normalIconStyle = normalStyle->iconStyle().get();
         normalLabelStyle = normalStyle->labelStyle().get();
         // normalLineStyle = normalStyle->lineStyle().get();
         normalPolyStyle = normalStyle->polyStyle().get();
         // normalBalloonStyle = normalStyle->balloonStyle().get();
         if(normalPolyStyle)
         {
//             std::cout << "OUTLINE? " << normalPolyStyle->getOutlineFlag() << std::endl;
         }
      }
   }
   if(highlightStyle)
   {
      if(highlightStyle->iconStyle().valid())
      {
      }
      if(highlightStyle->labelStyle().valid())
      {
      }
      if(highlightStyle->lineStyle().valid())
      {
      }
   }
//    if(placemark->getCenter(lat, lon, alt))
//    {
//       llh = osg::Vec3d(lat, lon, alt);
//       double geoidOffset = landModel->getGeoidOffset(lat,
//                                                      lon);
//       if(!ossim::isnan(geoidOffset))
//       {
//          llh[2]+=geoidOffset;
//       }
//    }
   ossimPlanetCubeGrid grid;
   ossimPlanetGridUtility::GridPoint gridPt;
   for(std::vector<osg::ref_ptr<ossimPlanetKmlGeometry> >::const_iterator iter = primitiveGeomList.begin();
       iter != primitiveGeomList.end();
       ++iter)
   {
      bool needsReTessFlag = false;
      double lat, lon, alt;
      osg::Vec3d llh;;
      if(!(*iter)->getCenter(lat, lon, alt))
      {
         continue;
      }
      llh = osg::Vec3d(lat, lon, alt);
      llh[2] += landModel->getGeoidOffset(lat,
                                          lon);
      osg::Matrixd localToWorld;
      osg::Matrixd inverseLocalToWorld;
      landModel->lsrMatrix(llh, localToWorld);
      osg::ref_ptr<osg::MatrixTransform> localToWorldTransform         = new osg::MatrixTransform;
      osg::ref_ptr<osg::MatrixTransform> localToWorldPickableTransform = new osg::MatrixTransform;
      localToWorldTransform->setMatrix(localToWorld);
      localToWorldPickableTransform->setMatrix(localToWorld);
      inverseLocalToWorld.invert(localToWorld);
      osg::Vec3d ecef;
      osg::Vec3d normal;
      landModel->forward(llh, ecef);
      landModel->normal(ecef, normal);
      normal.normalize();
//       osg::Vec3d centerGridLatLon;
//       std::cout << "llh before = " << llh[0] << ", " << llh[1] << ", " << llh[2] <<  "\n";
//       grid.getGridPoint(gridPt,
//                         llh);
//       std::cout << "gridPt before = " << gridPt.theGlobalGridPoint[0] << ", " << gridPt.theGlobalGridPoint[1] <<  "\n";
//       grid.getCenterGridPoint(gridPt,
//                               5,
//                               llh);
//       grid.getLatLon(centerGridLatLon,
//                      gridPt);
      
//       centerGridLatLon[2] = landModel->getHeightAboveEllipsoid(centerGridLatLon[0],
//                                                                centerGridLatLon[1]);
 //      landModel->lsrMatrix(centerGridLatLon, localToWorld);
//       localToWorldTransform->setMatrix(localToWorld);
//       localToWorldPickableTransform->setMatrix(localToWorld);
//       inverseLocalToWorld.invert(localToWorld);

//       std::cout << "llh after = " << centerGridLatLon[0] << ", " << centerGridLatLon[1] << ", " << centerGridLatLon[2] <<  "\n";
      osg::Vec3d localNormal = osg::Matrixd::transform3x3(localToWorld, normal);
      osg::ref_ptr<ossimPlanetKmlGeometry> geom = *iter;
      osg::ref_ptr<osg::Vec3Array> verts         = new osg::Vec3Array;
      osg::ref_ptr<osg::Vec3Array> extrusionVerts = new osg::Vec3Array;
      std::vector<std::pair<ossim_uint32, ossim_uint32> > extrusionGroups; // will be used for extrusions
      osg::ref_ptr<osg::Geometry> kmlPrimitiveGeom      = new osg::Geometry;
      double minHeight, maxHeight;
      ossimPlanetAltitudeMode altMode = geom->altitudeMode();
      bool needsOutline = false;
      double offset = 0.0;
      osg::ref_ptr<osg::Geode> geometryNode = new osg::Geode;
      // bool computeNormals = false;
      bool extrudeFlag = false;
      // bool hasInnerRings = false;
      
      if(geom->toPoint())
      {
         ossimPlanetKmlPoint* point = geom->toPoint();
         const ossimPlanetKmlGeometry::PointListType& pointList = point->pointList();
         if(pointList.size())
         {            
            // we actually need to adjust the Matrix transform for points since we need to center
            // the location based on altitude type
            //
            //
            osg::Vec3d tempLlh (pointList[0][1],
                                pointList[0][0],
                                convertHeight(pointList[0], altMode, landModel));
            landModel->lsrMatrix(tempLlh, localToWorld);
            localToWorldTransform->setMatrix(localToWorld);
            localToWorldPickableTransform->setMatrix(localToWorld);
            inverseLocalToWorld.invert(localToWorld);

            convertPointsToLocalCoordinates(verts.get(),
                                            pointList,
                                            inverseLocalToWorld,
                                            landModel,
                                            altMode,
                                            minHeight,
                                            maxHeight);
            
            if(point->extrudeFlag()&&(altMode!=ossimPlanetAltitudeMode_CLAMP_TO_GROUND))
            {
               extrudeFlag = true;
               // computeNormals = false;
               convertPointsToLocalCoordinates(extrusionVerts.get(),
                                               pointList,
                                               inverseLocalToWorld,
                                               landModel,
                                               ossimPlanetAltitudeMode_CLAMP_TO_GROUND,
                                               minHeight,
                                               maxHeight);
               extrusionGroups.push_back(std::make_pair(0, 1));
            }
         }
         osg::ref_ptr<ossimPlanetFadeText> text;
         osg::ref_ptr<osg::ClusterCullingCallback> textCull;
         osg::ref_ptr<osg::Geode> textGeometry;
         osg::ref_ptr<ossimPlanetBillboardIcon> billboardIcon;
         if(!placemark->name().empty())
         {
            text = new ossimPlanetFadeText();
            text->setText(placemark->name());
            text->setFont(defaultFont.get());
            text->setBackdropType(osgText::Text::OUTLINE);
            osg::Vec3d textEcef(0.0,0.0,0.0);//1000.0/landModel->getNormalizationScale());
            text->setPosition(textEcef);
            text->setCharacterSize(30000.0f/landModel->getNormalizationScale());
            text->setCharacterSizeMode(osgText::Text::OBJECT_COORDS_WITH_MAXIMUM_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT);
            text->setAutoRotateToScreen(true);
            text->setAlignment(osgText::Text::CENTER_BOTTOM);
            textCull = new osg::ClusterCullingCallback(textEcef, localNormal, 0.0);
            text->setClusterCullingCallback(textCull.get());
            textGeometry = new osg::Geode;
            textGeometry->addDrawable(text.get());
            textGeometry->getOrCreateStateSet()->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
         }
         if(normalIconStyle)
         {
           
            const osg::ref_ptr<ossimPlanetKmlIcon> icon = normalIconStyle->icon();
            if(icon.valid())
            {
               ossimFilename file = icon->download();
               osg::ref_ptr<osg::Image> img;
               if(file.exists())
               {
                  osg::ref_ptr<ossimPlanetIconGeom> geom = theLayer->getOrCreateIconEntry(file);
                  
                  if(geom.valid())
                  {
                     billboardIcon = new ossimPlanetBillboardIcon;
                     billboardIcon->setGeom(geom.get());
                  }
               }
            }
         }

         localToWorldPickableTransform->setMatrix(localToWorld);
         if(billboardIcon.valid())
         {
            
            localToWorldTransform->addChild(billboardIcon.get());
            localToWorldPickableTransform->addChild(billboardIcon.get());
            if(text.get()&&textCull.get())
            {               
               text->setUpdateCallback(new ossimPlanetKmlTextShiftUpdate(billboardIcon.get(),
                                                                         textCull.get(),
                                                                         text->getUpdateCallback()));
            }
         }
         if(textGeometry.valid())
         {
            localToWorldTransform->addChild(textGeometry.get());
         }
         if(!billboardIcon.valid())
         {
            localToWorldPickableTransform->addChild(textGeometry.get());
            
         }
         
         if(normalLabelStyle&&text.valid())
         {
            ossimString color = normalLabelStyle->color();
            if(color != "")
            {
               ossim_uint8 r, g, b, a;
               ossimPlanetKmlColorToRGBA(r,g,b,a, color);
               if(normalLabelStyle->colorMode() == ossimPlanetKmlColorMode_RANDOM)
               {
                  double tr = (double)rand()/(double)RAND_MAX;
                  double tg = (double)rand()/(double)RAND_MAX;
                  double tb = (double)rand()/(double)RAND_MAX;
                  r = (ossim_uint8)(tr*r);
                  g = (ossim_uint8)(tg*g);
                  b = (ossim_uint8)(tb*b);
               }
               text->setColor(osg::Vec4(r/255.0,g/255.0,b/255.0,a/255.0) );
            }
         }
         if(localToWorldPickableTransform.valid())
         {
            theKmlPickableGeometries->addChild(localToWorldPickableTransform.get());
         }
      } // end toPoint
      else if(geom->toLineString())
      {
         ossimPlanetKmlLineString* lineString = geom->toLineString();
         const ossimPlanetKmlGeometry::PointListType& pointList = lineString->pointList();
         if(pointList.size())
         {
            convertPointsToLocalCoordinates(verts.get(),
                                            pointList,
                                            inverseLocalToWorld,
                                            landModel,
                                            altMode,
                                            minHeight,
                                            maxHeight);
            if(lineString->extrudeFlag()&&(altMode!=ossimPlanetAltitudeMode_CLAMP_TO_GROUND))
            {
               extrudeFlag = true;
               // computeNormals = true;
               convertPointsToLocalCoordinates(extrusionVerts.get(),
                                               pointList,
                                               inverseLocalToWorld,
                                               landModel,
                                               ossimPlanetAltitudeMode_CLAMP_TO_GROUND,
                                               minHeight,
                                               maxHeight);
               extrusionGroups.push_back(std::make_pair((ossim_uint32)0,
                                                        (ossim_uint32)verts->size()));
            }
            kmlPrimitiveGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP,0,verts->size()));
         }
      }
      else if(geom->toLinearRing())
      {
         ossimPlanetKmlLinearRing* linearRing = geom->toLinearRing();
         
         const ossimPlanetKmlGeometry::PointListType& pointList = linearRing->pointList();
         if(pointList.size())
         {
            convertPointsToLocalCoordinates(verts.get(),
                                            pointList,
                                            inverseLocalToWorld,
                                            landModel,
                                            altMode,
                                            minHeight,
                                            maxHeight);
            if(linearRing->extrudeFlag()&&(altMode!=ossimPlanetAltitudeMode_CLAMP_TO_GROUND))
            {
               extrudeFlag = true;
               // computeNormals = true;
               convertPointsToLocalCoordinates(extrusionVerts.get(),
                                               pointList,
                                               inverseLocalToWorld,
                                               landModel,
                                               ossimPlanetAltitudeMode_CLAMP_TO_GROUND,
                                               minHeight,
                                               maxHeight);
               extrusionGroups.push_back(std::make_pair((ossim_uint32)0,
                                                        (ossim_uint32)verts->size()));
               
            }
            kmlPrimitiveGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP,0,verts->size()));
         }
      }
      else if(geom->toPolygon())
      {
         ossimPlanetKmlLinearRing* linearRing = geom->toPolygon()->outerBoundary().get();
         const ossimPlanetKmlPolygon::InnerBoundaryList& innerRings = geom->toPolygon()->innerBoundaryList();
         extrudeFlag = geom->toPolygon()->extrudeFlag();
         ossim_uint32 startGroupIdx = 0;
         
         if(linearRing)
         {
            const ossimPlanetKmlGeometry::PointListType& pointList = linearRing->pointList();
            if(pointList.size())
            {
               convertPointsToLocalCoordinates(verts.get(),
                                               pointList,
                                               inverseLocalToWorld,
                                               landModel,
                                               altMode,
                                               minHeight,
                                               maxHeight);
               if(extrudeFlag&&(altMode!=ossimPlanetAltitudeMode_CLAMP_TO_GROUND))
               {
                     needsReTessFlag = true;
                     // computeNormals = true;
                     convertPointsToLocalCoordinates(extrusionVerts.get(),
                                                     pointList,
                                                     inverseLocalToWorld,
                                                     landModel,
                                                     ossimPlanetAltitudeMode_CLAMP_TO_GROUND,
                                                     minHeight,
                                                     maxHeight);
                     extrusionGroups.push_back(std::make_pair((ossim_uint32)startGroupIdx,
                                                              (ossim_uint32)pointList.size()));
                     startGroupIdx += pointList.size();
               }
               else
               {
                  offset = ossim::max((maxHeight-minHeight)*.25, 5.0);
               }
            }
         }
         if(innerRings.size() > 0)
         {
            // hasInnerRings = true;
            needsReTessFlag = true;
            ossim_uint32 innerRingIdx = 0;
            for(innerRingIdx = 0; innerRingIdx < innerRings.size(); ++innerRingIdx)
            {
               const ossimPlanetKmlGeometry::PointListType& pointList = innerRings[innerRingIdx]->pointList();
               convertPointsToLocalCoordinates(verts.get(),
                                               pointList,
                                               inverseLocalToWorld,
                                               landModel,
                                               altMode,
                                               minHeight,
                                               maxHeight);
               if(extrudeFlag)
               {
                  convertPointsToLocalCoordinates(extrusionVerts.get(),
                                                  pointList,
                                                  inverseLocalToWorld,
                                                  landModel,
                                                  ossimPlanetAltitudeMode_CLAMP_TO_GROUND,
                                                  minHeight,
                                                  maxHeight);
                  extrusionGroups.push_back(std::make_pair((ossim_uint32)startGroupIdx,
                                                           (ossim_uint32)(pointList.size())));
                  startGroupIdx += pointList.size();
                  
               }
               if(!extrudeFlag)
               {
                  offset = ossim::max(offset, ossim::max((maxHeight-minHeight)*.25, 5.0));
               }
            }
         }
         kmlPrimitiveGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,0,verts->size()));

         if(!needsReTessFlag)
         {
            if(normalStyle->polyStyle().get())
            { 
               const osg::ref_ptr<ossimPlanetKmlPolyStyle> polyStyle = normalStyle->polyStyle();
               if(polyStyle.valid())
               {
                polyStyle->theColor = placemark->getPColor();
                polyStyle->clearFields();
                  if(polyStyle->getFillFlag())
                  {
                     needsReTessFlag = true;
                  }
               }
            }
         }
      }// toPolygon
      else if(geom->toModel())
      {
         const ossimPlanetKmlModel* model = geom->toModel();
         osg::ref_ptr<ossimPlanetKmlLocation> location = model->location();
         osg::ref_ptr<ossimPlanetKmlOrientation> orientation = model->orientation();
         const osg::ref_ptr<ossimPlanetKmlScale> scale = model->scale();
         osg::ref_ptr<ossimPlanetKmlLink> link     = model->link();
         double heading = 0.0, pitch = 0.0, roll = 0.0;
         double scalex = 1.0, scaley=1.0, scalez=1.0;
         if(orientation.valid())
         {
            heading = orientation->heading();
            pitch = orientation->pitch();
            roll  = orientation->roll();
         }
         if(scale.valid())
         {
            scalex = scale->x();
            scaley = scale->y();
            scalez = scale->z();
         }
         if(location.get()&&link.get())
         {
            osg::Vec3d kmlPoint(location->longitude(),
                                location->latitude(),
                                location->altitude());
            osg::Vec3d tempLlh(kmlPoint[1],
                               kmlPoint[0],
                               convertHeight(kmlPoint, altMode, landModel));
//             std::cout << "H = " << tempLlh[2] << std::endl;
            landModel->lsrMatrix(tempLlh, localToWorld, heading);
            localToWorld = (osg::Matrixd::scale(scalex, scaley, scalez)*
                            osg::Matrixd::rotate(pitch, osg::Vec3d(1.0, 0.0, 0.0))*
                            osg::Matrixd::rotate(roll, osg::Vec3d(0.0, 1.0, 0.0)))*localToWorld;
            localToWorldTransform->setMatrix(localToWorld);
            localToWorldPickableTransform->setMatrix(localToWorld);
            ossimFilename modelName = link->download();
            if(modelName.exists())
            {
               osg::NotifySeverity severity = osg::getNotifyLevel();
               osg::setNotifyLevel(osg::ALWAYS);
               osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(std::string(modelName.c_str()));
               osg::setNotifyLevel(severity);
//                osgDB::ReaderWriter::ReadResult result = osgDB::Registry::instance()->readNode(modelName.c_str(), 0);
               if(loadedModel.valid())
               {
                  std::ifstream in(modelName.c_str());
                  std::vector<char> buf(4048);
                  in.read(&buf.front(), buf.size());
                  double scaleMultiplier = 1.0;
                  if(in.gcount() > 0)
                  {
                     ossimString s(buf.begin(),
                                   buf.begin() + in.gcount());
                     ossimString::size_type beginPos = s.find("<asset>");
                     if(beginPos != std::string::npos)
                     {
                        ossimString assetXmlNode;
                        ossimString::size_type endPos = s.find("</asset>");
                        if(endPos != std::string::npos)
                        {
                           assetXmlNode = ossimString(buf.begin() + beginPos,
                                                      buf.begin() + endPos+8);
                           ossimRefPtr<ossimXmlNode> xmlNode = new ossimXmlNode;
                           std::istringstream inStringStream(assetXmlNode);
                           if(xmlNode->read(inStringStream))
                           {
                              ossimRefPtr<ossimXmlNode> unitNode =  xmlNode->findFirstNode("unit");
                              if(unitNode.valid())
                              {
                                 ossimString scaleValue;
                                 if(unitNode->getAttributeValue(scaleValue, "meter"))
                                 {
                                    scaleMultiplier = scaleValue.toDouble();
                                 }
                              }
                           }
                        }
                     }
                  }
//                   osg::Vec3d v;
//                   landModel->forward(tempLlh, v);
                  osg::MatrixTransform* modelTransform = new osg::MatrixTransform;
                  double scale2 = (1.0/landModel->getNormalizationScale());
                  modelTransform->setMatrix(osg::Matrixd::scale((scale2*scaleMultiplier),
                                                                (scale2*scaleMultiplier),
                                                                (scale2*scaleMultiplier)));
                  modelTransform->addChild(loadedModel.get());
                  modelTransform->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL,osg::StateAttribute::ON);        
                  
//                   modelScaleTransform->addChild(modelTransform);
                  localToWorldTransform->addChild(modelTransform);
                  ossimSetNonPowerOfTwoTextureVisitor nv;
                  localToWorldTransform->accept(nv);
               }
            }
         }
      }
//    } 
      
      if(extrudeFlag)
      {
         extrude(kmlPrimitiveGeom.get(),
                 verts.get(),
                 extrusionVerts.get(),
                 extrusionGroups);
      }
      
      if(altMode==ossimPlanetAltitudeMode_CLAMP_TO_GROUND&&offset > 0.0)
      {
         kmlPrimitiveGeom->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonOffset(-offset, -offset),
                                                                       osg::StateAttribute::ON);
      }
      else
      {
         offset = 0.0;
      }
         
      
      if(offset > 0.0)//||(altMode == ossimPlanetAltitudeMode_CLAMP_TO_GROUND))
      {
         kmlPrimitiveGeom->setCullCallback(new osg::ClusterCullingCallback(osg::Vec3d(0.0,0.0,0.0),
                                                                           localNormal,
                                                                           0.0));
      }

      
      kmlPrimitiveGeom->setSupportsDisplayList(true);
      kmlPrimitiveGeom->setUseDisplayList(true);
      
//       kmlPrimitiveGeom->setSupportsDisplayList(!theLod.valid());
      kmlPrimitiveGeom->setVertexArray(verts.get());
      
      osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
      float lineWidth = 2.0;
      osg::Vec4 lineColor(1.0,1.0,1.0,1.0);
      osg::Vec4 polyColor(1.0,1.0,1.0,1.0);
      osg::PolygonMode::Mode polygonStyleMode = osg::PolygonMode::LINE;
      bool polyStyleOutline = false;
      if(normalStyle)
      {
         // bool lineColorSetFlag;
         const osg::ref_ptr<ossimPlanetKmlLineStyle> lineStyle = normalStyle->lineStyle();
         if(lineStyle.valid())
         {
            ossim_uint8 r,g,b,a;
            ossimPlanetKmlColorToRGBA(r,g,b,a,lineStyle->color());
            // lineColorSetFlag = true;
            lineColor = osg::Vec4(r/255.0,
                                  g/255.0,
                                  b/255.0,
                                  a/255.0);
            lineWidth = lineStyle->width();
            if(lineWidth != 0.0)
            {
               kmlPrimitiveGeom->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(lineWidth),
                                                                             osg::StateAttribute::ON);
            }
         }
         if(normalStyle->polyStyle().get())
         {
            const osg::ref_ptr<ossimPlanetKmlPolyStyle> polyStyle = normalStyle->polyStyle();
            if(polyStyle.valid())
            {
               ossim_uint8 r,g,b,a;
               ossimPlanetKmlColorToRGBA(r,g,b,a, polyStyle->color());
               // lineColorSetFlag = true;
               polyColor = osg::Vec4(r/255.0,
                                     g/255.0,
                                     b/255.0,
                                     a/255.0);
               
               if(polyStyle->getFillFlag())
               {
                  polygonStyleMode = osg::PolygonMode::FILL;
               }
               if(polyStyle->getOutlineFlag())
               {
                  polyStyleOutline = true;//(polygonStyleMode == osg::PolygonMode::FILL);
//                         ossimNotify(ossimNotifyLevel_WARN) << "Outlining not supported yet in Kml drawing" << std::endl;
               }
            }
            kmlPrimitiveGeom->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
                                                                                               polygonStyleMode),
                                                                          osg::StateAttribute::ON);
         }
      }
      // check to see if we need to do an outline
      // If we are extruded or a plygon and have the outline flag enabled
      // 
      if(geom->toPolygon())
      {
         if(polyStyleOutline&&(polygonStyleMode==osg::PolygonMode::FILL))
         {
            needsOutline = true;
         }
      }
      else if(extrudeFlag&&!geom->toPoint()&&!geom->toPolygon())
      {
         needsOutline = true;
      }
      
      
      if((extrudeFlag&&!geom->toPoint())||
         ((geom->toPolygon()&&(polygonStyleMode==osg::PolygonMode::FILL))))
      {
         color->push_back(polyColor);
      }
      else
      {
         color->push_back(lineColor);
      }
//      if((*color)[0][3] < 1.0)
      {
         kmlPrimitiveGeom->getOrCreateStateSet()->setMode(GL_BLEND,osg::StateAttribute::ON);
      }
      kmlPrimitiveGeom->getOrCreateStateSet()->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
      kmlPrimitiveGeom->setColorBinding(osg::Geometry::BIND_OVERALL);
      kmlPrimitiveGeom->setColorArray(color.get());


      osg::ref_ptr<osg::Geometry> outlineGeom;
#if 1
      if(needsOutline)
      {
         outlineGeom = new osg::Geometry(*kmlPrimitiveGeom, osg::CopyOp::DEEP_COPY_ALL);
         if(!geom->toPoint()&&extrudeFlag)
         {
            // we will only keep the walls around.  Walls are after the root primitive
            // when extruding
            //
            outlineGeom->removePrimitiveSet(0);
         }
         outlineGeom->getOrCreateStateSet()->setMode(GL_BLEND,osg::StateAttribute::OFF);
         outlineGeom->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
                                                                                       osg::PolygonMode::LINE),
                                                                  osg::StateAttribute::ON);
         outlineGeom->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonOffset(-(offset+1), -(offset+1)),
                                                                  osg::StateAttribute::ON);
         outlineGeom->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(lineWidth),
                                                                  osg::StateAttribute::ON);
         color = new osg::Vec4Array;
         color->push_back(lineColor);
         if((*color)[0][3] < 1.0)
         {
            outlineGeom->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
            outlineGeom->getOrCreateStateSet()->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
         }
         outlineGeom->setColorArray(color.get());
         outlineGeom->setCullCallback(new osg::ClusterCullingCallback(osg::Vec3d(0.0,0.0,0.0),
                                                                      localNormal,
                                                                      0.0));
        geometryNode->addDrawable(outlineGeom.get());
        outlineGeom->setSupportsDisplayList(true);
        outlineGeom->setUseDisplayList(true);
      }
#endif
      if(needsReTessFlag)
      {
         osgUtil::Tessellator tes;
         
         tes.setTessellationType(osgUtil::Tessellator::TESS_TYPE_POLYGONS);
         tes.setBoundaryOnly(false);
         tes.setWindingType( osgUtil::Tessellator::TESS_WINDING_ODD);
         tes.retessellatePolygons(*kmlPrimitiveGeom);
      }

#if 0
      if(computeNormals)
      {
//          if(outlineGeom.valid())
         {
            osgUtil::SmoothingVisitor sv;
            sv.smooth(*kmlPrimitiveGeom);
//                FaceNormalVisitor sv;
//                sv.createFaceNormals(*kmlPrimitiveGeom);  // this will replace the normal vector with a new one
            kmlPrimitiveGeom->getOrCreateStateSet()->setMode(GL_LIGHTING,
                                                             osg::StateAttribute::ON);
            kmlPrimitiveGeom->getOrCreateStateSet()->setAttributeAndModes(new osg::ShadeModel(osg::ShadeModel::FLAT),
                                                                          osg::StateAttribute::ON);
         }
      }
#endif
      if(theLod.valid()) // only do a custom draw if we need to
      {
         theDraw = new ossimPlanetKmlPlacemarkNode::PlacemarkGeometryDraw;
         kmlPrimitiveGeom->setDrawCallback(theDraw.get());
      }
      if(outlineGeom.valid())
      {
         outlineGeom->setDrawCallback(theDraw.get());
      }
      if(kmlPrimitiveGeom->getNumPrimitiveSets() > 0)
      {
         geometryNode->addDrawable(kmlPrimitiveGeom.get());
      }
      if(geometryNode->getNumDrawables()> 0)
      {
         localToWorldTransform->addChild(geometryNode.get());
      }
      theKmlGeometries->addChild(localToWorldTransform.get());      
   } // end for loop for each geomtry primitive
   osg::BoundingSphere bs = theKmlGeometries->getBound();
   theRadius = bs.radius();
   theCenter = bs.center();
   osg::Vec3d tempLlh;
   landModel->inverse(theCenter, tempLlh);
   tempLlh[2] = 0.0;
   landModel->forward(tempLlh, theCenter);

//    std::cout << "<" << tempLlh[0] << ", " << tempLlh[1] << ", " << tempLlh[2] << std::endl;
//          osgUtil::TriStripVisitor tsv;
//          tsv.setMinStripSize(3);
//          tsv.stripify(*kmlPrimitiveGeom);
   // Now add the geometry
//    theKmlGeometries->addChild(theGeometry.get());
   
   return true;
}

double ossimPlanetKmlPlacemarkNode::convertHeight(const osg::Vec3d& kmlWorldPoint,
                                                ossimPlanetAltitudeMode altMode,
                                                ossimPlanetGeoRefModel* landModel)const
{
   osg::Vec3d worldPoint(kmlWorldPoint[1],
                         kmlWorldPoint[0],
                         kmlWorldPoint[2]);
   switch(altMode)
   {
      case ossimPlanetAltitudeMode_CLAMP_TO_GROUND:
      {
         // note KML is lon then lat so let's reverse
         return landModel->getHeightAboveEllipsoid(kmlWorldPoint[1],
                                                   kmlWorldPoint[0]);
         break;
      }
      case ossimPlanetAltitudeMode_RELATIVE_TO_GROUND:
      {
         return kmlWorldPoint[2] + landModel->getHeightAboveEllipsoid(kmlWorldPoint[1],
                                                                      kmlWorldPoint[0]);
         break;
      }
      default:
      {
         return kmlWorldPoint[2] + landModel->getGeoidOffset(kmlWorldPoint[1],
                                                             kmlWorldPoint[0]);
         break;
      }
   }
   
   return 0.0;
}

void ossimPlanetKmlPlacemarkNode::convertPointsToLocalCoordinates(osg::Vec3Array* result,
                                                                const ossimPlanetKmlGeometry::PointListType& pointList,
                                                                const osg::Matrixd& worldToLocalTransform,
                                                                ossimPlanetGeoRefModel* landModel,
                                                                ossimPlanetAltitudeMode altMode,
                                                                double& minHeight,
                                                                double& maxHeight)const
{
   minHeight = 99999999.0;
   maxHeight = -99999999.0;
   ossim_uint32 idx = 0;
   ossim_uint32 upper = pointList.size();
   osg::Vec3d xyz;
   osg::Vec3d localPoint;
   for(idx = 0; idx < upper; ++idx)
   {
      osg::Vec3d worldPoint(pointList[idx][1],
                            pointList[idx][0],
                            convertHeight(pointList[idx],
                                          altMode,
                                          landModel));
      if(worldPoint[2] < minHeight) minHeight = worldPoint[2];
      if(worldPoint[2] > maxHeight) maxHeight = worldPoint[2];
      landModel->forward(worldPoint, xyz);
      localPoint = xyz*worldToLocalTransform;
      result->push_back(localPoint);
   }
}

// void ossimPlanetKmlPlacemarkNode::extrude(osg::ref_ptr<osg::Geometry> result,
//                                         osg::Vec3Array* verts,
//                                         bool polygonFlag)const
// {
//    if(verts->size() > 1)
//    {
//       ossim_uint32 topSequenceMaxIdx    = verts->size()/2; // top layer
//       ossim_uint32 startTopIdx = 0;
//       ossim_uint32 startBottomIdx = topSequenceMaxIdx;
//       ossim_uint32 idx = 0;
//       ossim_uint32 idxMax = verts->size();
      
//       // lt's do the wall first
//       osg::DrawElementsUShort& drawElements = *(new osg::DrawElementsUShort(GL_QUAD_STRIP, idxMax));
      
//       for(idx = 0; idx < idxMax; ++idx)
//       {
//          if((idx%2)==0) // do top
//          {
//             drawElements[idx] = startTopIdx;
//             ++startTopIdx;
//          }
//          else // do bottom
//          {
//             drawElements[idx] = startBottomIdx;
//             ++startBottomIdx;
//          }
//       }
//       result->addPrimitiveSet(&drawElements);
//       if(polygonFlag)
//       {
//          osg::DrawElementsUShort& drawElements = *(new osg::DrawElementsUShort(GL_POLYGON, topSequenceMaxIdx));
//          for(idx = 0; idx < topSequenceMaxIdx; ++idx)
//          {
//             drawElements[idx] = idx;
//          }
//          result->addPrimitiveSet(&drawElements);
//       }
//       result->setSupportsDisplayList(true);
//    }
//    else
//    {
//       result->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, verts->size()));  
//    }
// }
void ossimPlanetKmlPlacemarkNode::extrude(osg::ref_ptr<osg::Geometry> result,
                                          osg::Vec3Array* verts,
                                          osg::Vec3Array* extrusionVerts,
                                          const std::vector<std::pair<ossim_uint32, ossim_uint32> >& extrusionGroups)const
{
   if(verts->size() == 1)
   {
      verts->insert(verts->end(),
                    extrusionVerts->begin(),
                    extrusionVerts->end());
      result->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, verts->size()));  
   }
   else
   {
      ossim_uint32 offsetIdx = verts->size();
      ossim_uint32 extrusionIdx = 0;
      // create walls for geometry
      verts->insert(verts->end(),
                    extrusionVerts->begin(),
                    extrusionVerts->end());
      for(extrusionIdx = 0; extrusionIdx<extrusionGroups.size();++extrusionIdx)
      {
         ossim_uint32 startIdx = extrusionGroups[extrusionIdx].first;
         ossim_uint32 startTopIdx = startIdx;
         ossim_uint32 startBottomIdx = startTopIdx + offsetIdx;
         ossim_uint32 idx = 0;
         ossim_uint32 idxMax = extrusionGroups[extrusionIdx].second*2;
         // makeWalls
         osg::DrawElementsUShort& drawElements = *(new osg::DrawElementsUShort(GL_QUAD_STRIP, idxMax));
         for(idx = 0;idx < idxMax;++idx)
         {
            if((idx%2)==0)
            {
               drawElements[idx] = startTopIdx;
               ++startTopIdx;
               // do top point
            }
            else
            {
               drawElements[idx] = startBottomIdx;
               ++startBottomIdx;
               // do bottom point
            }
         }
         result->addPrimitiveSet(&drawElements);
      }
   }
   
}
