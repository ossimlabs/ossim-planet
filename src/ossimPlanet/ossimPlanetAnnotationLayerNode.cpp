#include <ossimPlanet/ossimPlanetAnnotationLayerNode.h>
#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanetBillboardIcon.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetAnnotationLayer.h>
#include <ossimPlanet/ossimPlanetIconGeom.h>
#include <osgUtil/IntersectVisitor>
#include <sstream>
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetXmlAction.h>
static inline void annotationAdjustHeight(osg::Vec3d& input,
														ossimPlanetAltitudeMode mode,
														ossimPlanetGeoRefModel* modelTransform)
{
	switch(mode)
	{
		case ossimPlanetAltitudeMode_CLAMP_TO_GROUND:
		{
			input[2] = modelTransform->getHeightAboveEllipsoid(input[0], input[1]);
			break;
		}
		case ossimPlanetAltitudeMode_RELATIVE_TO_GROUND:
		{
			input[2]+=modelTransform->getHeightAboveEllipsoid(input[0], input[1]);
			break;
		}
		case ossimPlanetAltitudeMode_ABSOLUTE:
		{
			input[2] += modelTransform->getGeoidOffset(input[0], input[2]);
			break;
		}
		default:
		{
			input[2] = modelTransform->getHeightAboveEllipsoid(input[0], input[1]);
			break;
		}
	}			
}

class AnnotationUpdater : public ossimPlanetOperation
	{
	public:
		AnnotationUpdater()
		{}
		AnnotationUpdater(ossimPlanetAnnotationPlacemark* placemark)
		:theNode(placemark)
		{
			
		}
		virtual void run()
		{
			theNode->update();
		}
	protected:
		osg::ref_ptr<ossimPlanetAnnotationPlacemark> theNode;
	};



ossimPlanetAnnotationLayerNode::ossimPlanetAnnotationLayerNode()
:ossimPlanetNode(),
theDirtyBit(NOT_DIRTY),
theStagedFlag(false)
{
}

void ossimPlanetAnnotationLayerNode::traverse(osg::NodeVisitor& nv)
{
   ossimPlanetNode::traverse(nv);
}  


void ossimPlanetAnnotationLayerNode::execute(const ossimPlanetAction& action)
{
	ossimPlanetNode::execute(action);
}

ossimPlanetAnnotationPlacemark::ossimPlanetAnnotationPlacemark()
:ossimPlanetAnnotationLayerNode(),
theLabelStyle(new ossimPlanetAnnotationLabelStyle)
{
}

ossimPlanetAnnotationPlacemark::ossimPlanetAnnotationPlacemark(const osg::Vec3d& location,
                                                               ossimPlanetAltitudeMode altitudeMode,
                                                               const ossimString& nameStr,
                                                               const ossimString& descriptionStr)
:ossimPlanetAnnotationLayerNode(),
theLabelStyle(new ossimPlanetAnnotationLabelStyle)
{
   setName(nameStr);
   setDescription(descriptionStr);
   ossimPlanetAnnotationPoint* point = new ossimPlanetAnnotationPoint(location);
   point->setAltitudeMode(altitudeMode);
   setGeometry(point);
}

void ossimPlanetAnnotationPlacemark::traverse(osg::NodeVisitor& nv)
{
	if(!enableFlag()) return;
   if(!isStaged())
   {
      return;
   }
   bool needRedraw = false;
	OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theUpdateMutex);
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
			if(isDirtyBitSet(LABEL_DIRTY)&&theLabel.valid())
         {
            needRedraw = true;
            if(theLabel.valid())
            {
               theLabel->setText(name().c_str());
            }
				clearDirtyBit(LABEL_DIRTY);
         }
			if(isDirtyBitSet(COLOR_DIRTY))
			{
				if(theLabelStyle.valid()&&theLabel.valid())
				{
					needRedraw = true;
					if(theLabelStyle->colorMode() == ossimPlanetAnnotationColorMode_NORMAL)
					{
						theLabel->setColor(theLabelStyle->color());
					}
					else
					{
						theLabel->setColor(osg::Vec4d((ossim_float64)rand()/(ossim_float64)RAND_MAX,
																(ossim_float64)rand()/(ossim_float64)RAND_MAX,
																(ossim_float64)rand()/(ossim_float64)RAND_MAX,
																theLabel->getColor()[3]));
					}
				}
				clearDirtyBit(COLOR_DIRTY);
			}
         if(theExpireTime.valid())
         {
            if(theExpireTime->hasExpired())
            {
               if(theLayer)
               {
                  theLayer->needsRemoving(this);
               }
            }
            needRedraw = true;
         }
         break;
      }
      default:
      {
         break;
      }
   }
   if(theExpireTime.valid())
   {
      setRedrawFlag(true);
   }
   osg::ref_ptr<ossimPlanetAnnotationGeometry> geom = geometry();
   if(geom.valid())
   {
      geom->traverse(nv);
   }
   ossimPlanetAnnotationLayerNode::traverse(nv);
   if(needRedraw) setRedrawFlag(true);
}


void ossimPlanetAnnotationPlacemark::execute(const ossimPlanetAction& action)
{
   std::string command = action.command();
   const ossimPlanetXmlAction* xmlAction = action.toXmlAction();
   ossimPlanetAnnotationLayerNode::execute(action);
   if(command == "Set")
   {
		bool coordinateUpdating = false;
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theUpdateMutex);
      if(xmlAction&&xmlAction->xmlNode().valid())
      {
         ossim_uint32 idx = 0;
         const ossimXmlNode::ChildListType& childNodes = xmlAction->xmlNode()->getChildNodes();
			const ossimXmlNode::ChildListType* properties = 0;
			// at this point we should just have a single object in the first child of the set
			//
			if(childNodes.size() > 0)
			{
            if((childNodes[0]->getTag() == "Placemark")||
               (childNodes[0]->getTag() == "Object"))
            {
               properties = &(childNodes[0]->getChildNodes());
            }
            else
            {
               properties = &childNodes;
            }
			}
			if(!properties) return;
         if(properties->size()==0)
         {
            properties = &childNodes; // we have direct properties first
         }
         for(idx = 0;idx<properties->size();++idx)
         {
            ossimString tag = (*properties)[idx]->getTag();
            if(tag == "Style")
            {
               const ossimXmlNode::ChildListType& childStyleNodes = (*properties)[idx]->getChildNodes();  
               ossim_uint32 styleIdx = 0;
               for(styleIdx = 0;styleIdx<childStyleNodes.size();++styleIdx)
               {
                  if(childStyleNodes[styleIdx]->getTag() == "LabelStyle")
                  {
                     ossimString color     = childStyleNodes[styleIdx]->getChildTextValue("color").trim();
                     ossimString colorMode = childStyleNodes[styleIdx]->getChildTextValue("colorMode").trim();
                     ossimString scale     = childStyleNodes[styleIdx]->getChildTextValue("scale").trim();
                     if(!color.empty())
                     {
                        std::vector<ossimString> colorValues;
                        color.split(colorValues, " ");
                        if(colorValues.size() == 4)
                        {
                           theLabelStyle->setColor(osg::Vec4d(colorValues[0].toDouble(),
                                                              colorValues[1].toDouble(),
                                                              colorValues[2].toDouble(),
                                                              colorValues[3].toDouble()));
                        }
                        else if(colorValues.size() == 3)
                        {
                           theLabelStyle->setColor(osg::Vec4d(colorValues[0].toDouble(),
                                                              colorValues[1].toDouble(),
                                                              colorValues[2].toDouble(),
                                                              1.0));
                        }
                        else if(colorValues.size() == 1)
                        {
                           theLabelStyle->setColor(osg::Vec4d(colorValues[0].toDouble(),
                                                              colorValues[0].toDouble(),
                                                              colorValues[0].toDouble(),
                                                              1.0));
                        }
                     }
                     if(!colorMode.empty())
                     {
                        if(colorMode == "random")
                        {
                           theLabelStyle->setColorMode(ossimPlanetAnnotationColorMode_RANDOM);
                        }
                        else if(colorMode == "normal")
                        {
                           theLabelStyle->setColorMode(ossimPlanetAnnotationColorMode_NORMAL);
                        }
                     }
                  }
               }
               setDirtyBit(COLOR_DIRTY);
               setRedrawFlag(true);
            }
            else if(tag == "color")
            {
               ossimString color = (*properties)[idx]->getText();
               std::vector<ossimString> colorValues;
               color.split(colorValues, " ");
               if(colorValues.size() == 4)
               {
                  theLabelStyle->setColor(osg::Vec4d(colorValues[0].toDouble(),
                                                     colorValues[1].toDouble(),
                                                     colorValues[2].toDouble(),
                                                     colorValues[3].toDouble()));
               }
               else if(colorValues.size() == 3)
               {
                  theLabelStyle->setColor(osg::Vec4d(colorValues[0].toDouble(),
                                                     colorValues[1].toDouble(),
                                                     colorValues[2].toDouble(),
                                                     1.0));
               }
               else if(colorValues.size() == 1)
               {
                  theLabelStyle->setColor(osg::Vec4d(colorValues[0].toDouble(),
                                                     colorValues[0].toDouble(),
                                                     colorValues[0].toDouble(),
                                                     1.0));
               }
               setDirtyBit(COLOR_DIRTY);
               setRedrawFlag(true);
            }
            else if(tag == "description")
            {
               setDescription((*properties)[idx]->getText());
            }
            else if(tag == "name")
            {
               setName((*properties)[idx]->getText());
               setDirtyBit(LABEL_DIRTY);
               setRedrawFlag(true);
            }
            else if(tag == "coordinates")
            {
               ossimPlanetAnnotationPoint* pointGeom = dynamic_cast<ossimPlanetAnnotationPoint*>(theGeometry.get());
               if(pointGeom)
               {
                  ossimString coordinates    = (*properties)[idx]->getText().trim();
                  if(!coordinates.empty())
                  {
                     double lon=0.0, lat=0.0, hgt=0.0;
                     ossimString coordinateStr;
                     std::stringstream in(coordinates);
                     in >> coordinateStr;
                     std::vector<ossimString> splitArray;
                     coordinateStr.split(splitArray, ",");
                     if(splitArray.size() >1)
                     {
                        lon = splitArray[0].toDouble();
                        lat = splitArray[1].toDouble();
                        if(splitArray.size() > 2)
                        {
                           hgt = splitArray[2].toDouble();
                        }
                     }
                     coordinateUpdating = true;
                     setDirtyBit(COORDINATE_DIRTY);
                     pointGeom->setCoordinate(osg::Vec3d(lat,lon,hgt));
                     setRedrawFlag(true);
                  }
               }
            }
            else if(tag == "Point")
            {
					coordinateUpdating = true;
               ossimPlanetAnnotationPoint* pointGeom = dynamic_cast<ossimPlanetAnnotationPoint*>(theGeometry.get());
               if(!pointGeom)
               {
                  pointGeom = new ossimPlanetAnnotationPoint;
                  theGeometry = pointGeom;
               }
               ossimString coordinates    = (*properties)[idx]->getChildTextValue("coordinates").trim();
               ossimString altitudeMode   = (*properties)[idx]->getChildTextValue("altitudeMode").trim();
               ossimString extrudeFlag    = (*properties)[idx]->getChildTextValue("extrude").trim();
               if(!coordinates.empty())
               {
                  double lon=0.0, lat=0.0, hgt=0.0;
                  ossimString coordinateStr;
                  std::stringstream in(coordinates);
                  in >> coordinateStr;
                  std::vector<ossimString> splitArray;
                  coordinateStr.split(splitArray, ",");
                  if(splitArray.size() >1)
                  {
                     lon = splitArray[0].toDouble();
                     lat = splitArray[1].toDouble();
                     if(splitArray.size() > 2)
                     {
                        hgt = splitArray[2].toDouble();
                     }
                  }
                  pointGeom->setCoordinate(osg::Vec3d(lat,lon,hgt));
               }
               ossimPlanetAltitudeMode mode = ossimPlanetAltitudeMode_NONE;
               if(!altitudeMode.empty())
               {
                  if(altitudeMode.contains("clamp"))
                  {
                     mode = ossimPlanetAltitudeMode_CLAMP_TO_GROUND;
                  }
                  else if(altitudeMode.contains("relative"))
                  {
                     mode = ossimPlanetAltitudeMode_RELATIVE_TO_GROUND;
                  }
                  else if(altitudeMode.contains("absolute"))
                  {
                     mode = ossimPlanetAltitudeMode_ABSOLUTE;
                  }
               }
               pointGeom->setAltitudeMode(mode);
               if(extrudeFlag == "extrude")
               {
                  pointGeom->setExtrudeFlag(extrudeFlag.toBool());
               }
               setDirtyBit(COORDINATE_DIRTY);
               setRedrawFlag(true);
            }
            else if(tag == "ExpireDuration")
            {
               ossimString value = (*properties)[idx]->getAttributeValue("value").trim();
               ossimString units = (*properties)[idx]->getAttributeValue("units").trim();
               if(!value.empty())
               {
                  ossimPlanetAnnotationExpireDuration* duration = new ossimPlanetAnnotationExpireDuration;
                  duration->setDuration(value.toDouble());
                  theExpireTime = duration;
                  // right now we ignore units.  Will add that support later
               }
               else
               {
                  theExpireTime = 0;
               }
            }
         }
      }
		if(isDirtyBitSet(COORDINATE_DIRTY)&&theStagedFlag)
		{
			if(geometry()->asPoint())
			{
				update();
			}
			else
			{
				ossimPlanetAnnotationLayer* l = dynamic_cast<ossimPlanetAnnotationLayer*>(layer());
				if(l)
				{
					l->updateThreadQueue()->add(new AnnotationUpdater(this));
				}
				else
				{
					update();
				}
			}
		}
      setRedrawFlag(true);
   }
}

void ossimPlanetAnnotationPlacemark::update()
{
	if(!layer()) return;
	if(!theGeometry.valid()) return;
   osg::ref_ptr<ossimPlanetGeoRefModel> model = layer()->model();
   
	if(isDirtyBitSet(ossimPlanetAnnotationLayerNode::COORDINATE_DIRTY))
	{
		if(model.valid())
		{
			osg::ref_ptr<ossimPlanetAnnotationPoint> point = geometry()->asPoint();
			if(point.valid())
			{
				osg::Matrixd localToWorld;
				osg::Vec3d modelCoordinate;
				osg::Vec3d normal;
				osg::Vec3d localNormal;
				osg::Vec3d coordinate = point->coordinate();
				annotationAdjustHeight(coordinate, 
											  point->altitudeMode(),
											  model.get());
				model->forward(coordinate, modelCoordinate);
				normal = modelCoordinate;
				normal.normalize();
				point->setModelCoordinate(modelCoordinate);
				model->lsrMatrix(coordinate, localToWorld);
				osg::ref_ptr<osg::MatrixTransform> m = point->matrixTransform();
				if(clusterCull().valid())
				{
					osg::Vec3d localNormal = osg::Matrixd::transform3x3(localToWorld, normal);
					clusterCull()->set(osg::Vec3d(0.0,0.0,0.0), localNormal, 0.0, -1.0);
				}
				m->setMatrix(localToWorld);
			}
		}
		clearDirtyBit(ossimPlanetAnnotationLayerNode::COORDINATE_DIRTY);
		setRedrawFlag(true);
	}
}

void ossimPlanetAnnotationPlacemark::stage()
{
	
   osg::ref_ptr<ossimPlanetGeoRefModel> model = layer()->model();
	ossimPlanetAnnotationLayer* annotationLayer = dynamic_cast<ossimPlanetAnnotationLayer*>(theLayer);
	if(annotationLayer)
	{
		if(annotationLayer->planet())
		{
			if(model.valid()&&geometry().valid())
			{
				osg::ref_ptr<ossimPlanetAnnotationPoint> point = geometry()->asPoint();
				if(point.valid())
				{
					osg::Matrixd localToWorld;
					osg::Vec3d modelCoordinate;
					osg::Vec3d normal;
					osg::Vec3d localNormal;
					osg::Vec3d coordinate = point->coordinate();
					annotationAdjustHeight(coordinate, 
												  point->altitudeMode(),
												  model.get());
					model->forward(coordinate, modelCoordinate);
					normal = modelCoordinate;
					normal.normalize();
					point->setModelCoordinate(modelCoordinate);
					model->lsrMatrix(coordinate, localToWorld);
					osg::MatrixTransform* m = new osg::MatrixTransform;
					m->setMatrix(localToWorld);
					{
						OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theUpdateMutex);
						osg::Vec3d localNormal = osg::Matrixd::transform3x3(localToWorld, normal);
						theLabel = new ossimPlanetFadeText();
						theLabel->setText(name());
						theLabel->setFont(annotationLayer->defaultFont().get());
						theLabel->setBackdropType(osgText::Text::OUTLINE);
						osg::Vec3d textEcef(0.0,0.0,0.0);
						theLabel->setPosition(textEcef);
						theLabel->setCharacterSize(30000.0f/model->getNormalizationScale());
						theLabel->setCharacterSizeMode(osgText::Text::OBJECT_COORDS_WITH_MAXIMUM_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT);
						theLabel->setAutoRotateToScreen(true);
						theLabel->setAlignment(osgText::Text::CENTER_BOTTOM);
						theClusterCull = new osg::ClusterCullingCallback(textEcef, localNormal, 0.0);
						theLabel->setClusterCullingCallback(theClusterCull.get());
						theLabelGeode = new ossimPlanetAnnotationTextGeode(this, theLabel.get());
						theLabelGeode->addDrawable(theLabel.get());
						theLabelGeode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
						m->addChild(theLabelGeode.get());
						if(theLabelStyle->colorMode() == ossimPlanetAnnotationColorMode_NORMAL)
						{
							theLabel->setColor(theLabelStyle->color());
						}
						else
						{
							theLabel->setColor(osg::Vec4d((ossim_float64)rand()/(ossim_float64)RAND_MAX,
																	(ossim_float64)rand()/(ossim_float64)RAND_MAX,
																	(ossim_float64)rand()/(ossim_float64)RAND_MAX,
																	theLabel->getColor()[3]));
						}
					}
//					ossimPlanetBillboardIcon* icon = new ossimPlanetBillboardIcon;
//					ossimPlanetIconGeom* geom = new ossimPlanetIconGeom;
//					if(annotationLayer)
//					{
//						geom->setTexture(annotationLayer->defaultIconTexture().get());
//					}
//					icon->setGeom(geom);
//					m->addChild(icon);
					point->setMatrixTransform(m);
               if(!lookAt().valid())
               {
                  setLookAt(new ossimPlanetLookAt(coordinate[0],
                                                  coordinate[1],
                                                  coordinate[2],
                                                  0, 45, 0, 5000));
               }
				}
			}
		}
	}
   if(theExpireTime.valid())
   {
      theExpireTime->initTimeStamp();
   }
	clearDirtyBit(ALL_DIRTY);
	setStagedFlag(true);
}

void ossimPlanetAnnotationPlacemark::setName(const ossimString& name)
{
   ossimPlanetAnnotationLayerNode::setName(name);
	setDirtyBit(LABEL_DIRTY);
}
