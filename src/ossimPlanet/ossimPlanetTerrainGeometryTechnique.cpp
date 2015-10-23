#include <ossimPlanet/ossimPlanetTerrainGeometryTechnique.h>
#include <ossimPlanet/ossimPlanetTerrainTile.h>
#include <ossimPlanet/ossimPlanetTerrain.h>
#include <ossimPlanet/ossimPlanetTexture2D.h>
#include <ossimPlanet/ossimPlanet.h>
#include <iostream>
#include <osg/io_utils>
#include <osg/Timer>
#include <osg/CullFace>
#include <osgDB/Registry>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/TriStripVisitor>
#include <queue>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/Simplifier>

void ossimPlanetTerrainGeometryTechnique::UpdateChildTextureVisitor::apply(osg::Node& node)
{
   ossimPlanetTerrainTile* tile = dynamic_cast<ossimPlanetTerrainTile*>(&node);
   if(tile)
   {
      ossimPlanetTerrainGeometryTechnique* technique = (ossimPlanetTerrainGeometryTechnique*)tile->terrainTechnique();
      if(technique)
      {
         BufferData& buffer = technique->readOnlyBuffer();
         if(buffer.theGeode.valid())
         {
            if(!tile->imageLayer(theIdx)->image()||
               (tile->imageLayer(theIdx)->noMoreDataFlag()&&
                tile->imageLayer(theIdx)->refreshFlag())) // no valid image then set to the best parent texture and keep going
            {
               osg::StateSet* stateset = buffer.theGeode->getOrCreateStateSet();
               stateset->setTextureAttributeAndModes(theIdx, theTexture.get(), osg::StateAttribute::ON);
               technique->updateTextureMatrix(stateset, theIdx, tile->tileId(),  theTexture->tileId());
               traverse(node);
            }
         }
      }
   }
}

void ossimPlanetTerrainGeometryTechnique::CullNode::traverse(osg::NodeVisitor& nv)
{
   osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(&nv);
   theCulledFlag = false;
   
   if(!cv) return;
   const osg::Vec3d& center = theBoundingBox->center();
   double radius = theBoundingBox->radius();
   thePixelSize = cv->clampedPixelSize(center, radius);
   osg::Vec3d eye = cv->getEyeLocal();
   osg::Vec3d eyeDirection = cv->getLookVectorLocal();
   const osg::Polytope& frustum = cv->getCurrentCullingSet().getFrustum();
   theEyeDistance = (eye-theBoundingBox->center()).length();
   theEyeToVolumeDistance = (theEyeDistance - theBoundingBox->radius());
   if(theEyeToVolumeDistance < theBoundingBox->radius()) theEyeToVolumeDistance = FLT_EPSILON;
#if 1
   // do cluster cull if enabled
   //
   if (cv->getCullingMode() & osg::CullSettings::CLUSTER_CULLING)
   {
      if(theClusterCullingDeviation >= -1.0)
      {
         // let's use the bounding box center instead of the cluster center
         // seems to be more stable right now to do it this way.
         //
         //osg::Vec3d eyeCp = eye - center;
         osg::Vec3d eyeCp = eye - theClusterCullingControlPoint;
         double radius    = eyeCp.length();
         
         if (radius>=theClusterCullingRadius)
         {
            double deviation = (eyeCp * theClusterCullingNormal)/radius;
            
            theCulledFlag = (deviation < theClusterCullingDeviation);
         }
      }
   }
#endif
   theWithinFrustumFlag = true;
   if(theBoundingBox->isInFront(eye, eyeDirection))
   {
      theWithinFrustumFlag = theBoundingBox->intersects(frustum);
   }
   else
   {
      theWithinFrustumFlag = false;
   }
   
   if((cv->getCullingMode()&osg::CullSettings::VIEW_FRUSTUM_CULLING)&&!theCulledFlag)
   {
      theCulledFlag = !theWithinFrustumFlag;
   }
}

osg::BoundingSphere ossimPlanetTerrainGeometryTechnique::CullNode::computeBound() const
{
   osg::BoundingSphere bsphere(theBoundingBox->center(),
                               theBoundingBox->radius());
   if (!bsphere.valid()) return bsphere;
   
   // note, NULL pointer for NodeVisitor, so compute's need
   // to handle this case gracefully, normally this should not be a problem.
   osg::Matrix l2w;
   
   computeLocalToWorldMatrix(l2w,NULL);
   
   osg::Vec3d xdash = bsphere._center;
   xdash.x() += bsphere._radius;
   xdash = xdash*l2w;
   
   osg::Vec3d ydash = bsphere._center;
   ydash.y() += bsphere._radius;
   ydash = ydash*l2w;
   
   osg::Vec3d zdash = bsphere._center;
   zdash.z() += bsphere._radius;
   zdash = zdash*l2w;
   
   
   bsphere._center = bsphere._center*l2w;
   
   xdash -= bsphere._center;
   double len_xdash = xdash.length();
   
   ydash -= bsphere._center;
   double len_ydash = ydash.length();
   
   zdash -= bsphere._center;
   double len_zdash = zdash.length();
   
   bsphere._radius = len_xdash;
   if (bsphere._radius<len_ydash) bsphere._radius = len_ydash;
   if (bsphere._radius<len_zdash) bsphere._radius = len_zdash;
   
   return bsphere;
   
}

ossimPlanetTerrainGeometryTechnique::ossimPlanetTerrainGeometryTechnique()
:ossimPlanetTerrainTechnique()
{
}

ossimPlanetTerrainGeometryTechnique::ossimPlanetTerrainGeometryTechnique(const ossimPlanetTerrainGeometryTechnique& src,
                                                                         const osg::CopyOp& copyop)
:ossimPlanetTerrainTechnique(src, copyop)
{
}

ossimPlanetTerrainGeometryTechnique::~ossimPlanetTerrainGeometryTechnique()
{
   BufferData& buffer = readOnlyBuffer();
   buffer.theTransform = 0;
   buffer.theGeode = 0;
   buffer.theGeometry = 0;
   buffer.theClusterCullingCallback = 0;
   buffer.theCullNode = 0;
   
   buffer = writeBuffer();
   buffer.theTransform = 0;
   buffer.theGeode = 0;
   buffer.theGeometry = 0;
   buffer.theClusterCullingCallback = 0;
   buffer.theCullNode = 0;
}

void ossimPlanetTerrainGeometryTechnique::setTerrainTile(ossimPlanetTerrainTile* tile)
{
   if(tile)
   {
      tile->setCullingActive(false);
   }
   ossimPlanetTerrainTechnique::setTerrainTile(tile);
}

void ossimPlanetTerrainGeometryTechnique::swapBuffers()
{
   //std::swap(theCurrentReadOnlyBuffer,theCurrentWriteBuffer);
}

void ossimPlanetTerrainGeometryTechnique::update(osgUtil::UpdateVisitor* uv)
{
   BufferData& buffer = readOnlyBuffer();
   if (theTerrainTile) 
   {
      updateRequests(*uv);
      // let's immediately initialize a level 0 tile
      //
      if(theTerrainTile->tileId().level() == 0)
      {
         if(!readOnlyBuffer().theGeometry.valid())
         {
            theTerrainTile->init();
         }
      }
      if(buffer.theTransform.valid())
      {
         buffer.theTransform->accept(*uv);
      }
      theTerrainTile->osg::Group::traverse(*uv);
   }
}

void ossimPlanetTerrainGeometryTechnique::cull(osgUtil::CullVisitor* cv)
{
   BufferData& buffer = readOnlyBuffer();
   
   if(isCulled(buffer,cv))
   {
      return;
   }
   theTerrainTile->updateFrameAndTimeStamps(cv->getFrameStamp());
   thePriorityPoint = cv->getEyeLocal();
   theAdjustedEye   = cv->getEyeLocal();

   if(theTerrainTile->terrain())
   {
     if(theTerrainTile->terrain()->falseEyeFlag())
     {
       theAdjustedEye = theTerrainTile->terrain()->falseEye();
       thePriorityPoint = theAdjustedEye;
     }
     else if(theTerrainTile->terrain()->priorityPointFlag())
     {
       thePriorityPoint = theTerrainTile->terrain()->priorityPoint();
     }
   }

   bool addChildren            = false;
   bool removeChildren         = false;
   
   if(theTerrainTile->terrain()->splitMergeMetricType() == ossimPlanetTerrain::DISTANCE_METRIC)
   {
      double splitMergeLodScale = theTerrainTile->terrain()->splitMergeLodScale();
      double ratio = splitMergeLodScale*thePatchBound.radius();
      double distance = (theAdjustedEye - thePatchBound.center()).length();
      removeChildren = distance  > ratio;
      addChildren = (distance < ratio)&&isLeaf();
   }
   else
   {
      osg::Vec2d deltaXY;
      theTerrainTile->terrain()->grid()->widthHeightInModelSpace(theTerrainTile->tileId(), deltaXY);
      //ossim_float64 maxSize =.5*(deltaXY[0] + deltaXY[1]);
      ossim_float64 maxSize =ossim::min(deltaXY[0], deltaXY[1]);
      ossim_float64 averageMeters = ossimGpt().metersPerDegree().y* sqrt(2.0*maxSize*maxSize);
      ossim_float64 averageRadius = averageMeters*theTerrainTile->terrain()->model()->getInvNormalizationScale();
      ossim_float64 pixelSize     = cv->clampedPixelSize(thePatchBound.center(), averageRadius);
      ossim_float64 pixelWidth    = ossim::max(theTerrainTile->terrain()->textureTileWidth(),
                                               theTerrainTile->terrain()->textureTileHeight());
      ossim_float64 mergeWidth = pixelWidth*theTerrainTile->terrain()->mergePixelMetric();
      ossim_float64 splitWidth = pixelWidth*theTerrainTile->terrain()->splitPixelMetric();
      
      if(pixelSize < mergeWidth)
      {
         removeChildren = true;
      }
      else if(pixelSize > splitWidth)
      {
         addChildren = isLeaf();
      }
   }
   // we can only add children if the previous buffered cull setting are not culled for 
   // all child nodes and if we don't have all the children and if we are not culled
   //
   if(addChildren)
   {
      if(theTerrainTile->tileId().level() > 17)
      {
         addChildren = false;
      }
      else
      {
         theTerrainTile->terrain()->requestSplit(theTerrainTile,
                                                 splitPriority(),
                                                 cv->getFrameStamp(),
                                                 theTerrainTile->splitRequest());
         
      }
   }
   if(isLeaf()||removeChildren)
   {
      if (buffer.theTransform.valid())
      {
         buffer.theTransform->accept(*cv);
      }
   }
   else 
   {
      theTerrainTile->osg::Group::traverse(*cv);
   }
}

void ossimPlanetTerrainGeometryTechnique::updateRequests(osg::NodeVisitor& nv)
{
   // ossimPlanetTerrainTile* parentTile = theTerrainTile->parentTile();
   bool canRequest = true;
   // we need to loop this and add requests for multi textures.
   //
   // if(isLeaf())
   if(canRequest)
   {
      std::vector<ossim_uint32> textureIndices;
      ossimPlanetTerrainTile* current = theTerrainTile;
      // bool doneImage = false;
      bool doneElevation = false;
      if(current)
      {
         ossim_uint32 imageLayerCount = current->numberOfImageLayers();
         ossim_uint32 imageLayerIdx = 0;
         for(imageLayerIdx = 0; imageLayerIdx < imageLayerCount; ++imageLayerIdx)
         {
            ossimPlanetTerrainImageLayer* layer = current->imageLayer(imageLayerIdx);
            if(layer&&
               (!layer->noMoreDataFlag())&&
               (!layer->image()||
                (layer->image()->tileId().level() != current->tileId().level())||
                (layer->refreshFlag())))
            {
               textureIndices.push_back(imageLayerIdx);
               // doneImage = true;
            }
         }
         if(textureIndices.size() > 0)
         {
            theTerrainTile->terrain()->requestTexture(current,
                                                      current->terrainTechnique()->texturePriority(),
                                                      nv.getFrameStamp(),
                                                      textureIndices,
                                                      current->textureRequest());
         }
         ossimPlanetTerrainImageLayer* elevLayer = current->elevationLayer();
         if(!doneElevation&&
            elevLayer&&
            (!elevLayer->noMoreDataFlag())&&
            (!elevLayer->image()||
             (elevLayer->image()->tileId().level()!=current->tileId().level())||
             (elevLayer->refreshFlag())))
         {
            theTerrainTile->terrain()->requestElevation(current,
                                                        current->terrainTechnique()->elevationPriority(),
                                                        nv.getFrameStamp(),
                                                        current->elevationRequest());
            doneElevation = true;
         }
      }
   }
}

void ossimPlanetTerrainGeometryTechnique::traverse(osg::NodeVisitor& nv)
{
   // if app traversal update the frame count.
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         osgUtil::UpdateVisitor* uv = dynamic_cast<osgUtil::UpdateVisitor*>(&nv);
         if (uv)
         {
            update(uv);
            return;
         }        
         break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
         osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(&nv);
         if (cv)
         {
            cull(cv);
            return;
         }
         break;
      }
      default:
      {
         BufferData& buffer = readOnlyBuffer();
         if(nv.getTraversalMode() == osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN)
         {
            if(isLeaf())
            {
               if(buffer.theTransform.valid())
               {
                  buffer.theTransform->accept(nv);
               }
               return;
            }
         }
         else
         {
            if(buffer.theTransform.valid())
            {
               buffer.theTransform->accept(nv);
            }
         }
         break;
      }
   }
   theTerrainTile->osg::Group::traverse(nv);
}

void averageNormal(osg::Vec3d& center,
                   osg::Vec3d& normal,
                   const osg::Vec3d& p0,
                   const osg::Vec3d& p1,
                   const osg::Vec3d& p2,
                   const osg::Vec3d& p3)
{
   center = (p0+p1+p2+p3)*.25;
   osg::Vec3d delta1, delta2;
   delta1 = (p0-center);
   delta2 = (p1-center);
   delta1.normalize();
   delta2.normalize();
   normal = delta1^delta2;
   delta1 = (p1-center);
   delta2 = (p2-center);
   delta1.normalize();
   delta2.normalize();
   normal += (delta1^delta2);
   delta1 = (p2-center);
   delta2 = (p3-center);
   delta1.normalize();
   delta2.normalize();
   normal += (delta1^delta2);
   delta1 = (p3-center);
   delta2 = (p0-center);
   delta1.normalize();
   delta2.normalize();
   normal += (delta1^delta2);
   
   normal.normalize();
}

void ossimPlanetTerrainGeometryTechnique::buildMesh(ossimPlanetTerrainTile* /* optionalParent */)
{
   if(!theModel.valid()||!theGrid.valid()||!theTerrainTile||!theTerrainTile->terrain()) return;
   BufferData& buffer = writeBuffer();
   osg::ref_ptr<ossimPlanetImage> elevationImage = theTerrainTile->elevationLayer()->image();
   //ossim_float32* elevationData = 0;
   ossim_float64 skirtScale = .08;
   unsigned int numRows = theTerrainTile->terrain()->elevationTileHeight();
   unsigned int numCols = theTerrainTile->terrain()->elevationTileWidth();
   ossim_int32 padding = 0;
   if(elevationImage.valid())
   {
      //elevationData = (ossim_float32*)elevationImage->data();
      numCols = elevationImage->widthWithoutPadding();
      numRows = elevationImage->heightWithoutPadding();
      padding = elevationImage->padding();
   }
   //std::cout << "w,h,pad = " << numCols <<", " << numRows << ", " << padding << "\n";
   buffer.theGeode = new osg::Geode;
   buffer.theTransform = new osg::MatrixTransform;
   buffer.theGeometry = new osg::Geometry;
   buffer.theGeode->addDrawable(buffer.theGeometry.get());
   buffer.theClusterCullingCallback = new osg::ClusterCullingCallback();
   osg::Geometry* geometry = buffer.theGeometry.get();
   
   buffer.theTransform->addChild(buffer.theGeode.get());
   buffer.theGeode->addDrawable(buffer.theGeometry.get());
   
   
   unsigned int numVerticesInBody = numCols*numRows;
   unsigned int numVertices       = numVerticesInBody;
   ossimPlanetGrid::GridBound tileBound;
   theGrid->bounds(theTerrainTile->tileId(), tileBound);
   osg::Vec2d metersPerPixel;
   theGrid->getUnitsPerPixel(metersPerPixel, theTerrainTile->tileId(), numCols, numRows, OSSIM_METERS);
   
   ossim_uint32 rowIdx=0,colIdx=0;
   
   // This is the grid model point.  For this technique it should be lon, lat and height format
   // stored in x, y, z
   //
   ossimPlanetGrid::ModelPoint centerModel;
   theGrid->centerModel(theTerrainTile->tileId(), centerModel);
   osg::Matrixd localToWorld;
   // for our lsr let's always use te ellipsoid height of 0.  This will make things easier later
   //
   theModel->lsrMatrix(osg::Vec3d(centerModel.y(),
                                  centerModel.x(),
                                  0.0),//centerModel.z()),
                       localToWorld);
   osg::Matrixd inverseLocalToWorld;
   inverseLocalToWorld.invert(localToWorld);
   theModel->forward(osg::Vec3d(centerModel.y(), centerModel.x(), centerModel.z()), 
                     buffer.theCenterPatch);
   osg::Vec3d centerNormal, rightAxis, upAxis;
   centerNormal = osg::Vec3d(localToWorld(2, 0),
                             localToWorld(2, 1),
                             localToWorld(2, 2));
   rightAxis = osg::Vec3d(localToWorld(0, 0),
                          localToWorld(0, 1),
                          localToWorld(0, 2));
   upAxis    = osg::Vec3d(localToWorld(1, 0),
                          localToWorld(1, 1),
                          localToWorld(1, 2));
   osg::Vec2d normalDistance(0.0,0.0);
   osg::Vec2d rightDistance(0.0,0.0);
   osg::Vec2d upDistance(0.0,0.0);
   double upDelta = 0.0;
   double rightDelta = 0.0;
   double normDelta  = 0.0;
   osg::Vec3d deltaP(0.0,0.0,0.0);
   osg::ref_ptr<osg::Vec2Array> tcoords = new osg::Vec2Array;
 	tcoords->reserve(numVertices);
   geometry->setTexCoordArray( 0, tcoords.get());   
   
   osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
   vertices->reserve(numVertices);
   geometry->setVertexArray(vertices.get());
   
   // allocate and assign normals
   osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
   normals->reserve(numVertices);
   osg::ref_ptr<osg::Vec3Array> skirtNormals = new osg::Vec3Array;
   skirtNormals->reserve(numVertices);
   geometry->setNormalArray(normals.get());
   geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
   
   // allocate and assign color
   osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(1);
   (*colors)[0].set(1.0f,1.0f,1.0f,1.0f);
   
   geometry->setColorArray(colors.get());
   geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
   
   ossimPlanetGrid::ModelPoint modelPoint;
   osg::Vec3d norm1;
   osg::Vec3d xyz;
   osg::Vec3d localXYZ;
   ossimPlanetGrid::LocalNdcPoint localPoint;
   const ossimPlanetTerrainTileId tileId = theTerrainTile->tileId();
   typedef std::vector<int> Indices;
   Indices indices(numVertices, -1);
   osg::Vec3d centerBox = buffer.theCenterPatch;
   ossim_uint32 elevIdx = 0;
   // double minHeight = 1.0/FLT_EPSILON;
   // double maxHeight = -minHeight;
   double elevationExaggeration = theTerrainTile->terrain()->elevationExaggeration();
   if(elevationImage.valid()&&(elevationImage->hasMinMax()))
   {
      // minHeight = (elevationImage->minValue()[0]*elevationExaggeration)*theModel->getInvNormalizationScale();
      // maxHeight = (elevationImage->maxValue()[0]*elevationExaggeration)*theModel->getInvNormalizationScale();
   }
   else
   {
      // minHeight = 0.0;
      // maxHeight = 0.0;
   }
#if 0
   std::cout << "minHeight = " << minHeight << std::endl;
   std::cout << "maxHeight = " << maxHeight << std::endl;
   std::cout << "w = " << numCols << std::endl;
   std::cout << "h = " << numRows << std::endl;
   std::cout << "padding = " << padding << std::endl;
#endif
   ossim_uint32 baseIdx = 0;
   unsigned int iv = 0;
   double txInc = 1.0/(numCols-1.0);
   double tyInc = 1.0/(numRows-1.0);
   double ty = 0.0;
   osgUtil::Simplifier::IndexList pointsToProtect;
   for(rowIdx = 0; rowIdx < numRows;++rowIdx)
   {
      double tx = 0.0;
      localPoint.setY(ty);//(double)(rowIdx)/(double)(numRows-1));
      for(colIdx = 0; colIdx < numCols;++colIdx)
      {
         // if(elevationData)
         if(elevationImage.valid())
         {
            double h = elevationImage->elevationValue(colIdx+padding, rowIdx+padding);
            if((h == OSSIMPLANET_NULL_HEIGHT)||
               (ossim::isnan(h)))
            {
               h = 0;
            }
            else
            {
               h*=elevationExaggeration;
            }
            //            if(h < minHeight) minHeight = h;
            //            if(h > maxHeight) maxHeight = h;
            localPoint.setZ(h);
         }
         //rowIdx*numCols + colIdx;
         indices[iv] = vertices->size();
         if((colIdx == 0)||(rowIdx==0)||(rowIdx==(numRows-1)) || (colIdx == (numCols-1)))
         {
            pointsToProtect.push_back(indices[iv]);
         }
         localPoint.setX(tx);//(double)(colIdx)/(double)(numCols-1));
         tcoords->push_back(osg::Vec2d(tx, ty));//osg::Vec2d(localPoint.x(), localPoint.y()));
         theGrid->localNdcToModel(tileId, localPoint, modelPoint);
         osg::Vec3d llh(modelPoint.y(), modelPoint.x(), modelPoint.z());
         theModel->forward(llh, xyz);
#if 1
         deltaP     = (xyz-centerBox);
         upDelta    = deltaP*upAxis;
         rightDelta = deltaP*rightAxis;
         normDelta  = deltaP*centerNormal;
         
         if(normDelta < normalDistance[1]) normalDistance[1] = normDelta;
         if(normDelta > normalDistance[0]) normalDistance[0] = normDelta;
         if(upDelta < upDistance[1]) upDistance[1] = upDelta; 
         if(upDelta > upDistance[0]) upDistance[0] = upDelta;
         if(rightDelta < rightDistance[1]) rightDistance[1] = rightDelta; 
         if(rightDelta > rightDistance[0]) rightDistance[0] = rightDelta;
#endif
         localXYZ = xyz*inverseLocalToWorld;
         (*vertices).push_back(localXYZ);
         theModel->normal(xyz,
                          norm1);
         osg::Vec3d tempNorm(norm1);
         norm1 = osg::Matrixd::transform3x3(localToWorld, norm1);
         norm1.normalize();
         // first push gravitational normal for skirts so they point straight down
         (*skirtNormals).push_back(norm1);
         
         // now calculate a perturbed normal using the elevation grids
         //
         if(elevationImage.valid())
         {
            osg::Vec2d normDeltas;
            osg::Matrixd tempLocalToWorld;
            theModel->lsrMatrix(osg::Vec3d(llh[0],
                                           llh[1],
                                           llh[2]),
                                tempLocalToWorld);
            osg::Vec3d xAxis(tempLocalToWorld(0,0), tempLocalToWorld(0,1), tempLocalToWorld(0,2));
            osg::Vec3d yAxis(tempLocalToWorld(1,0), tempLocalToWorld(1,1), tempLocalToWorld(1,2));
            osg::Vec3d zAxis(tempLocalToWorld(2,0), tempLocalToWorld(2,1), tempLocalToWorld(2,2));
            osg::Matrixd composite = tempLocalToWorld*inverseLocalToWorld;
            normDeltas = elevationImage->deltas(colIdx+padding, rowIdx+padding);
            normDeltas = normDeltas*elevationExaggeration;
            normDeltas[0]/=(metersPerPixel[0]);
            normDeltas[1]/=(metersPerPixel[1]);
            normDeltas[0] *=theModel->getInvNormalizationScale();
            normDeltas[1] *=theModel->getInvNormalizationScale();
            tempNorm = osg::Vec3d(-normDeltas[0], -normDeltas[1], theModel->getInvNormalizationScale());
            tempNorm.normalize();
            norm1 = zAxis*tempNorm[2] + xAxis*tempNorm[0] + yAxis*tempNorm[1];
            norm1 = osg::Matrixd::transform3x3(localToWorld, norm1);
            norm1.normalize();
         }
         // push perturbed normal.
         (*normals).push_back(norm1);
#if 0      
         deltaP     = (centerBox-centerBox);
         upDelta    = deltaP*upAxis;
         rightDelta = deltaP*rightAxis;
         normDelta  = deltaP*centerNormal;
         
         if(normDelta < normalDistance[1]) normalDistance[1] = normDelta;
         if(normDelta > normalDistance[0]) normalDistance[0] = normDelta;
         if(upDelta < upDistance[1]) upDistance[1] = upDelta; 
         if(upDelta > upDistance[0]) upDistance[0] = upDelta;
         if(rightDelta < rightDistance[1]) rightDistance[1] = rightDelta; 
         if(rightDelta > rightDistance[0]) rightDistance[0] = rightDelta;
#endif
         ++elevIdx;
         ++iv;
         tx+=txInc;
      }
      ty += tyInc;
   }
#if 0
   osg::Vec3d bbp0 = (*vertices)[0];
   osg::Vec3d bbp1 = (*vertices)[numCols-1];
   osg::Vec3d bbp2 = (*vertices)[(numRows-1)*numCols + numCols-1];
   osg::Vec3d bbp3 = (*vertices)[(numRows-1)*numCols];
   osg::Vec3d bboxCenter;
   osg::Vec3d bboxNormal;
   osg::Vec2d bboxNormalDistance;
   bboxNormalDistance[0] = 0.0;
   bboxNormalDistance[1] = 0.0;
   
   averageNormal(bboxCenter, bboxNormal,
                 bbp0,
                 bbp1,
                 bbp2,
                 bbp3);
   ossim_uint32 idx = 0;
   osg::Vec3d tempPoint;
   double projValue = 0.0;
   for(idx = 0; idx < (*vertices).size();++idx)
   {
      tempPoint = osg::Vec3d((*vertices)[idx])-bboxCenter;
      projValue = tempPoint*bboxNormal;
      if(projValue > bboxNormalDistance[0]) bboxNormalDistance[0] = projValue;
      if(projValue < bboxNormalDistance[1]) bboxNormalDistance[1] = projValue;
   }
#endif
   osg::ref_ptr<ossimPlanetBoundingBox> box = new ossimPlanetBoundingBox();
   //   box->extrude(bbp0, 
   //                bbp1, 
   //                bbp2, 
   //                bbp3,
   //                bboxNormal, 
   //                bboxNormalDistance);
   box->extrude(centerBox,//buffer.theCenterPatch,
                upAxis,
                rightAxis,
                centerNormal,
                upDistance,
                rightDistance,
                normalDistance);
   // double defaultDeltaHeight = fabs(normalDistance[0] - normalDistance[1]);
   box->transform(inverseLocalToWorld);
   
   buffer.theTransform->setMatrix(localToWorld);
   
   // now lit's do the skirt if enabled
   // for now it's always enabled but will add a flag later to turn it off and 
   // on
   //
   osg::ref_ptr<osg::Vec3Array> skirtVectors = new osg::Vec3Array((*skirtNormals));
   // let do bottom part
   //
   
   osg::DrawElementsUShort& drawElements = *(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES));
   osg::Geometry::PrimitiveSetList primSetList;
   ossim_uint32 i = 0;
   ossim_uint32 j = 0;
   
#if 0
   for(j = 0; j < numRows-1;++j)
   {
      for(i = 0; i < numCols-1; ++i)
      {
         int i00;
         int i01;
         i00 = j*numCols + i;
         i01 = i00+numCols;
         
         int i10 = i00+1;
         int i11 = i01+1;
         
         // remap indices to final vertex positions
         i00 = indices[i00];
         i01 = indices[i01];
         i10 = indices[i10];
         i11 = indices[i11];
         
         if(i!=j)
         {
            drawElements.push_back(i10);
            drawElements.push_back(i01);
            drawElements.push_back(i00);
            
            drawElements.push_back(i10);
            drawElements.push_back(i11);
            drawElements.push_back(i01);
         }
         else
         {
            drawElements.push_back(i01);
            drawElements.push_back(i00);
            drawElements.push_back(i11);
            
            drawElements.push_back(i00);
            drawElements.push_back(i10);
            drawElements.push_back(i11);
         }
         
      }
   }
#else
   for(i = 1; i < numRows;i+=2)
   {
      for(j = 1; j < numCols;j+=2)
      {
         ossim_uint32 i00 = (i)*numCols + j;
         drawElements.push_back(i00);
         drawElements.push_back(i00 - (numCols+1));
         drawElements.push_back(i00 - (numCols));
         
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 - (numCols));
         drawElements.push_back(i00 - (numCols-1));
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 - (numCols-1));
         drawElements.push_back(i00 + 1);
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 + 1);
         drawElements.push_back(i00 + (numCols+1));
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 + (numCols+1));
         drawElements.push_back(i00 + (numCols));
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 + (numCols));
         drawElements.push_back(i00 + (numCols-1));
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 + (numCols-1));
         drawElements.push_back(i00 - 1);
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 - 1);
         drawElements.push_back(i00 - (numCols+1));
      }
   }
#endif
   primSetList.push_back(&drawElements);
	geometry->setPrimitiveSetList(primSetList);
   //osgUtil::Simplifier simplifier(.8);
   //simplifier.simplify(*geometry, pointsToProtect);
   geometry->dirtyBound();
   geometry->getBound();
   thePatchBound = buffer.theTransform->getBound();
   ossimPlanetGrid::ModelPoint centerModelPoint;
   theGrid->centerModel(theTerrainTile->tileId(), centerModelPoint);
   theModel->latLonHeightToXyz(osg::Vec3d(centerModelPoint.y(), centerModelPoint.x(), 0.0), theCenterGrid);

   buffer.theClusterCullingCallback->computeFrom(geometry);
   //smoothGeometry();
   buffer.theCullNode = new CullNode(theTerrainTile->tileId(),
                                     box.get(),
                                     buffer.theClusterCullingCallback->getControlPoint(),
                                     buffer.theClusterCullingCallback->getNormal(),
                                     (double)buffer.theClusterCullingCallback->getDeviation(),
                                     (double)buffer.theClusterCullingCallback->getRadius());
   buffer.theCullNode->setMatrix(buffer.theTransform->getMatrix());
   buffer.theCullNode->dirtyBound();
   ossim_uint32 c = 0;
   ossim_uint32 r = 0;
   ossim_float32 bottomSkirtHeight = buffer.theTransform->getBound().radius()*skirtScale;
   ossim_float32 rightSkirtHeight  = bottomSkirtHeight;
   ossim_float32 topSkirtHeight    = bottomSkirtHeight;
   ossim_float32 leftSkirtHeight   = bottomSkirtHeight;
   
#if 0
   if(elevationData)
   {
      // bottom variance
      ossim_int32 i = indices[numCols>>1]; // index of original vertex of grid
      osg::Vec3d centerEdgeNormal = (*normals)[i];
      osg::Vec3d centerControlPoint = (*vertices)[i];
      ossim_float32 heightValue = 0.0;
      minHeight = 1.0/FLT_EPSILON;
      maxHeight = -minHeight;
      ossim_float32 heightVariance = 0.0;
      for(c = 0; c < numCols; ++c)
      {
         i = indices[c]; // index of original vertex of grid
         heightValue = ((*vertices)[i]-centerControlPoint)*centerEdgeNormal;
         if(maxHeight < heightValue) maxHeight = heightValue;
         if(minHeight > heightValue) minHeight = heightValue;
      }
      heightVariance = (maxHeight-minHeight);///theModel->getNormalizationScale();
      bottomSkirtHeight = heightVariance;//ossim::max(bottomSkirtHeight, heightVariance);
      
      // right variance
      //
      minHeight = 1.0/FLT_EPSILON;
      maxHeight = -minHeight;
      baseIdx = numCols-1;
      i = indices[baseIdx + numCols*(numRows>>1)];
      centerEdgeNormal = (*normals)[i];
      centerControlPoint = (*vertices)[i];
      for(r = 0; r < numRows; ++r)
      {
         i = indices[baseIdx]; // index of original vertex of grid
         heightValue = ((*vertices)[i]-centerControlPoint)*centerEdgeNormal;
         if(maxHeight < heightValue) maxHeight = heightValue;
         if(minHeight > heightValue) minHeight = heightValue;
         baseIdx+=numCols;
      }
      heightVariance = (maxHeight-minHeight);///theModel->getNormalizationScale();
      rightSkirtHeight = heightVariance;//ossim::max(rightSkirtHeight, heightVariance);
      
      // top variance
      //
      minHeight = 1.0/FLT_EPSILON;
      maxHeight = -minHeight;
      baseIdx = numCols*(numRows-1);
      i = indices[baseIdx + (numCols>>1)];
      centerEdgeNormal = (*normals)[i];
      centerControlPoint = (*vertices)[i];
      for(c = 0; c < numCols; ++c)
      {
         i = indices[baseIdx]; // index of original vertex of grid
         heightValue = ((*vertices)[i]-centerControlPoint)*centerEdgeNormal;
         if(maxHeight < heightValue) maxHeight = heightValue;
         if(minHeight > heightValue) minHeight = heightValue;
         ++baseIdx;
      }
      heightVariance = (maxHeight-minHeight);
      topSkirtHeight = heightVariance;//ossim::max(topSkirtHeight, heightVariance);
      
      // left variance
      //
      minHeight = 1.0/FLT_EPSILON;
      maxHeight = -minHeight;
      baseIdx = 0;
      i = indices[baseIdx + numCols*(numRows>>1)];
      centerEdgeNormal   = (*normals)[i];
      centerControlPoint = (*vertices)[i];
      for(r = 0; r < numRows; ++r)
      {
         i = indices[baseIdx]; // index of original vertex of grid
         heightValue = ((*vertices)[i]-centerControlPoint)*centerEdgeNormal;
         if(maxHeight < heightValue) maxHeight = heightValue;
         if(minHeight > heightValue) minHeight = heightValue;
         baseIdx += numCols;
      }
      heightVariance = (maxHeight-minHeight);
      leftSkirtHeight = heightVariance;//ossim::max(leftSkirtHeight, heightVariance);
   }
#endif
   ossim_uint32 skirtFlags = computeSkirtFlags(theTerrainTile->tileId());
   if(skirtFlags&BOTTOM_SKIRT)
   {
      // let's do the bottom skirt first
      //
      r = 0;
      for(c = 0; c < numCols; ++c)
      {
         ossim_uint32 idx = c;//numCols+c;
         ossim_int32 orig_i = indices[idx]; // index of original vertex of grid
         unsigned int new_i = vertices->size(); // index of new index of added skirt point
         osg::Vec3 new_v = (*vertices)[orig_i] - ((*skirtVectors)[orig_i])*bottomSkirtHeight;
         (*vertices).push_back(new_v);
         (*normals).push_back((*normals)[orig_i]);
         
         tcoords->push_back((*tcoords)[orig_i]);
         if(((c % 2) == 0)&&
            (c>0))
         {
            drawElements.push_back(indices[idx-2]);
            drawElements.push_back(new_i-2);
            drawElements.push_back(new_i-1);
            
            drawElements.push_back(new_i-1);
            drawElements.push_back(indices[idx-1]);
            drawElements.push_back(indices[idx-2]);
            
            drawElements.push_back(indices[idx-1]);
            drawElements.push_back(new_i-1);
            drawElements.push_back(new_i);
            
            drawElements.push_back(new_i);
            drawElements.push_back(indices[idx]);
            drawElements.push_back(indices[idx-1]);
         }
      }
   }
   if(skirtFlags&RIGHT_SKIRT)
   {
      // now let's do right Skirt
      //
      baseIdx = numCols-1;
      for(r = 0; r < numRows; ++r)
      {
         ossim_int32 orig_i = indices[baseIdx]; // index of original vertex of grid
         unsigned int new_i = vertices->size(); // index of new index of added skirt point
         osg::Vec3 new_v = (*vertices)[orig_i] - ((*skirtVectors)[orig_i])*rightSkirtHeight;
         (*vertices).push_back(new_v);
         (*normals).push_back((*normals)[orig_i]);
         tcoords->push_back((*tcoords)[orig_i]);
         if(((r % 2) == 0)&&
            (r>0))
         {
            drawElements.push_back(indices[baseIdx-(numCols<<1)]);
            drawElements.push_back(new_i-2);
            drawElements.push_back(new_i-1);
            
            drawElements.push_back(new_i-1);
            drawElements.push_back(indices[baseIdx-numCols]);
            drawElements.push_back(indices[baseIdx-(numCols<<1)]);
            
            drawElements.push_back(indices[baseIdx-numCols]);
            drawElements.push_back(new_i-1);
            drawElements.push_back(new_i);
            
            drawElements.push_back(new_i);
            drawElements.push_back(indices[baseIdx]);
            drawElements.push_back(indices[baseIdx-numCols]);
         }         
         baseIdx+=numCols;
      }
   }
   
   if(skirtFlags&TOP_SKIRT)
   {
      // top skirt
      baseIdx = numCols*(numRows-1);
      for(c = 0; c < numCols; ++c)
      {
         ossim_int32 orig_i = indices[baseIdx]; // index of original vertex of grid
         unsigned int new_i = vertices->size(); // index of new index of added skirt point
         osg::Vec3 new_v = (*vertices)[orig_i] - ((*skirtVectors)[orig_i])*topSkirtHeight;
         (*vertices).push_back(new_v);
         (*normals).push_back((*normals)[orig_i]);
         tcoords->push_back((*tcoords)[orig_i]);
         if(((c % 2) == 0)&&
            (c>0))
         {
            drawElements.push_back(indices[baseIdx-2]);
            drawElements.push_back(new_i-1);
            drawElements.push_back(new_i-2);
            
            drawElements.push_back(indices[baseIdx-2]);
            drawElements.push_back(indices[baseIdx-1]);
            drawElements.push_back(new_i-1);
            
            drawElements.push_back(indices[baseIdx-1]);
            drawElements.push_back(new_i);
            drawElements.push_back(new_i-1);
            
            drawElements.push_back(indices[baseIdx]-1);
            drawElements.push_back(indices[baseIdx]);
            drawElements.push_back(new_i);
         }
         
         ++baseIdx;
      }
   }
   
   if(skirtFlags&LEFT_SKIRT)
   {
      // now do left skirt
      //
      baseIdx = 0;
      for(r = 0; r < numRows; ++r)
      {
         ossim_int32 orig_i = indices[baseIdx]; // index of original vertex of grid
         unsigned int new_i = vertices->size(); // index of new index of added skirt point
         osg::Vec3 new_v = (*vertices)[orig_i] - ((*skirtVectors)[orig_i])*leftSkirtHeight;
         (*vertices).push_back(new_v);
         (*normals).push_back((*normals)[orig_i]);
         tcoords->push_back((*tcoords)[orig_i]);
         if(((r % 2) == 0)&&
            (r>0))
         {
            drawElements.push_back(indices[baseIdx-(numCols<<1)]);
            drawElements.push_back(new_i-1);
            drawElements.push_back(new_i-2);
            
            drawElements.push_back(indices[baseIdx-(numCols<<1)]);
            drawElements.push_back(indices[baseIdx-numCols]);
            drawElements.push_back(new_i-1);
            
            drawElements.push_back(indices[baseIdx-numCols]);
            drawElements.push_back(new_i);
            drawElements.push_back(new_i-1);
            
            drawElements.push_back(indices[baseIdx-numCols]);
            drawElements.push_back(indices[baseIdx]);
            drawElements.push_back(new_i);
         }    
         baseIdx += numCols;
      }
   }
#if 0
   osg::DrawElementsUShort& drawElements = *(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES));
   osg::Geometry::PrimitiveSetList primSetList;
   ossim_uint32 i = 0;
   ossim_uint32 j = 0;
   ossim_uint32 i00 = 0;
   ossim_uint32 i01 = 0;
   ossim_uint32 i10 = 0;
   ossim_uint32 i11 = 0;
   bool swapOrientation = theTerrainTile->tileId().face() > 3;
   for(j = 0; j < numRows-1;++j)
   {
      for(i = 0; i < numCols-1;++i)
      {
         //         if (swapOrientation)
         //         {
         //            i01 = j*numCols + i;
         //            i00 = i01+numCols;
         //         }
         //         else
         //         {
         i00 = j*numCols + i;
         i01 = i00+numCols;
         //         }
         i10 = i00+1;
         i11 = i01+1;
         //         i00 = indices[i00];
         //         i01 = indices[i01];
         //         i10 = indices[i10];
         //         i11 = indices[i11];
         if(!swapOrientation)
         {
            drawElements.push_back(i01);
            drawElements.push_back(i00);
            drawElements.push_back(i11);
            
            drawElements.push_back(i00);
            drawElements.push_back(i10);
            drawElements.push_back(i11);
         }
         else
         {
            drawElements.push_back(i01);
            drawElements.push_back(i00);
            drawElements.push_back(i10);
            
            drawElements.push_back(i01);
            drawElements.push_back(i10);
            drawElements.push_back(i11);
            
         }
      }
   }
   primSetList.push_back(&drawElements);
	geometry->setPrimitiveSetList(primSetList);
#endif
   
   //smoothGeometry();
   osg::ref_ptr<osg::CullFace> cullFace = new osg::CullFace;
   geometry->getOrCreateStateSet()->setAttributeAndModes(cullFace.get(),
                                                         osg::StateAttribute::ON);
   //   if(theCullSettings&osg::CullSettings::CLUSTER_CULLING)
   //   {
   //      geometry->setCullCallback(buffer.theClusterCullingCallback.get());
   //   }
   geometry->setUseVertexBufferObjects(true);
   geometry->setUseDisplayList(false);
//   geometry->setFastPathHint(true);
   
   
   if (osgDB::Registry::instance()->getBuildKdTreesHint()==osgDB::ReaderWriter::Options::BUILD_KDTREES &&
       osgDB::Registry::instance()->getKdTreeBuilder())
   {
      
      
      //osg::Timer_t before = osg::Timer::instance()->tick();
      //osg::notify(osg::NOTICE)<<"osgTerrain::GeometryTechnique::build kd tree"<<std::endl;
      osg::ref_ptr<osg::KdTreeBuilder> builder = osgDB::Registry::instance()->getKdTreeBuilder()->clone();
      buffer.theGeode->accept(*builder);
      //std::cout << "BUILT!!" << std::endl;
      //osg::Timer_t after = osg::Timer::instance()->tick();
      //osg::notify(osg::NOTICE)<<"KdTree build time "<<osg::Timer::instance()->delta_m(before, after)<<std::endl;
   }
   
   buffer.theGeode->dirtyBound();
   buffer.theTransform->dirtyBound();
   buffer.theGeometry->dirtyBound();
   buffer.theTransform->getBound();
#if 0   
   ossimPlanetTerrainTile* parent = optionalParent?optionalParent:theTerrainTile->parentTile();
   if(theTerrainTile)
   {
      theTerrainTile->dirtyBound();
      theTerrainTile->getBound();
   }
   // now copy cull node settings to the parent so it's always cached
   //
   if(parent)
   {
      ((ossimPlanetTerrainGeometryTechnique*)(parent->terrainTechnique()))->setChildCullParameters(theTerrainTile,
                                                                                                   buffer.theCullNode);
   }
#endif
}

ossimPlanetTexture2D* ossimPlanetTerrainGeometryTechnique::findNearestActiveParentTexture(ossim_uint32 imageLayerIdx, 
                                                                                          ossimPlanetTerrainTile* optionalParent)
{
   if(!theTerrainTile) return 0;
   ossimPlanetTerrainTile* parent = optionalParent?optionalParent:theTerrainTile->parentTile();
   ossimPlanetTerrainGeometryTechnique* currentTechnique = 
   dynamic_cast<ossimPlanetTerrainGeometryTechnique*>(parent?parent->terrainTechnique():0);
   ossimPlanetTexture2D* result = 0;
   while(!result&&currentTechnique)
   {
      BufferData& buffer = currentTechnique->readOnlyBuffer();
      if(buffer.theTransform.valid())
      {
         osg::StateSet* stateset = buffer.theGeode->getStateSet();
         if(stateset)
         {
            result = dynamic_cast<ossimPlanetTexture2D*>(stateset->getTextureAttribute(imageLayerIdx, 
                                                                                       osg::StateAttribute::TEXTURE));
            
            //               if(!result->getImage())
            //               {
            //                  result = 0;
            //               }
         }
         parent = parent->parentTile();
         if(parent)
         {
            currentTechnique = dynamic_cast<ossimPlanetTerrainGeometryTechnique*>(parent->terrainTechnique());
         }
         else
         {
            currentTechnique = 0;
         }
      }
   }
   
   return result;
}

void ossimPlanetTerrainGeometryTechnique::applyColorLayers(ossimPlanetTerrainTile* optionalParent)
{
   if(!theTerrainTile) return;
   BufferData& buffer = writeBuffer();
   if(!buffer.theTransform.valid()) return;
   typedef std::map<ossimPlanetTerrainLayer*, osg::ref_ptr<osg::Texture> > LayerToTextureMap;
   LayerToTextureMap layerToTextureMap;
   
   ossim_uint32 imageLayerIdx = 0;
   for(imageLayerIdx=0; imageLayerIdx<theTerrainTile->numberOfImageLayers(); ++imageLayerIdx)
   {
      ossimPlanetTerrainImageLayer* imageLayer = theTerrainTile->imageLayer(imageLayerIdx);
      if (!imageLayer) continue;
      osg::ref_ptr<ossimPlanetImage> image = imageLayer->image();
      
      osg::ref_ptr<ossimPlanetTexture2D> borrowedTexture = 0;
      if (!image) 
      {
         borrowedTexture = findNearestActiveParentTexture(imageLayerIdx, optionalParent);
         //std::cout << theTerrainTile->tileId().level() << " TRYING TO BORROW\n";
         //std::cout << borrowedTexture.get() << std::endl;
         // borrow gl texture from parent
         //
         if(!borrowedTexture) continue;
      }
      osg::ref_ptr<osg::StateSet> stateset = buffer.theGeode->getOrCreateStateSet();
      osg::ref_ptr<ossimPlanetTexture2D> texture2D = borrowedTexture.get();//borrowedTexture.valid()?borrowedTexture.get():
      //dynamic_cast<ossimPlanetTexture2D*>(layerToTextureMap[imageLayer].get());
      imageLayer->setDirtyFlag(false);
      
      if(!texture2D)
      {
         texture2D = dynamic_cast<ossimPlanetTexture2D*>(stateset->getTextureAttribute(imageLayerIdx, 
                                                                                       osg::StateAttribute::TEXTURE));
         if(!texture2D)
         {
            texture2D = newImageLayerTexture(imageLayerIdx);
            imageLayer->setDirtyFlag(false);
         }
         else if(image.valid())
         {
            if(texture2D->tileId().level()!=image->tileId().level())
            {
               texture2D = newImageLayerTexture(imageLayerIdx);
               imageLayer->setDirtyFlag(false);
            }
         }
      }
      if(texture2D.valid()&&!borrowedTexture.valid())
      {
         texture2D->setFilter(osg::Texture::MIN_FILTER, imageLayer->minFilter());
         texture2D->setFilter(osg::Texture::MAG_FILTER, imageLayer->magFilter());
      }
      //layerToTextureMap[imageLayer] = texture2D.get();
      stateset->setTextureAttributeAndModes(imageLayerIdx, texture2D.get(), osg::StateAttribute::ON);
      
      updateTextureMatrix(stateset.get(), imageLayerIdx, theTerrainTile->tileId(),  texture2D->tileId());
   }
}

void ossimPlanetTerrainGeometryTechnique::applyTransparency(ossimPlanetTerrainTile* /* optionalParent */)
{
   BufferData& buffer = writeBuffer();
   
   bool containsTransparency = false;
   ossim_uint32 colorLayerIdx = 0;
   for(colorLayerIdx=0; colorLayerIdx<theTerrainTile->numberOfImageLayers(); ++colorLayerIdx)
   {
      ossimPlanetTerrainImageLayer* colorLayer = theTerrainTile->imageLayer(colorLayerIdx);
      if (!colorLayer) continue;
      
      osg::ref_ptr<osg::Image> image = colorLayer->image();
      if (image.valid())
      {
         containsTransparency = image->isImageTranslucent();
         break;
      }        
   }
   
   if (containsTransparency&&buffer.theGeode.valid())
   {
      osg::StateSet* stateset = buffer.theGeode->getOrCreateStateSet();
      stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
      stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
   }
}

void ossimPlanetTerrainGeometryTechnique::init(ossimPlanetTerrainTile* optionalParent)
{
   if(!theTerrainTile) return;
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildNodeCullParametersMutex);
      if(!theChildNodeCullParameters.size())
      {
         theChildNodeCullParameters.resize(4);
      }
   }
   if(!readOnlyBuffer().theTransform.valid()) // we need to do a full init and swap
   {
      buildMesh(optionalParent);
      applyColorLayers(optionalParent);
      applyTransparency(optionalParent);
      swapBuffers();
   }
}

void ossimPlanetTerrainGeometryTechnique::childAdded(ossim_uint32 pos)
{
   ossimPlanetTerrainTile* tile = dynamic_cast<ossimPlanetTerrainTile*>(theTerrainTile->getChild(pos));
   
   if(tile)
   {
      // ossimPlanetTerrainGeometryTechnique* technique = dynamic_cast<ossimPlanetTerrainGeometryTechnique*>(tile->terrainTechnique());
      
      // now lets default to a lower resolution texture for
      // any image layers that have not been initialized
      //
      ossim_uint32 idx = 0;
      ossim_uint32 numberOfImageLayers = tile->numberOfImageLayers();
      for(idx = 0; idx < numberOfImageLayers;++idx)
      {
         ossimPlanetTerrainImageLayer* layer = tile->imageLayer(idx);
         if(layer&&!layer->image())
         {
            ossimPlanetTexture2D* texture = findNearestActiveParentTexture(idx, tile);
            if(texture)
            {
               UpdateChildTextureVisitor childTextureVisitor(texture, idx);
               tile->accept(childTextureVisitor);
            }
         }
      }
   }
   
   
   //   theTerrainTile->dirtyBound();
   //   theTerrainTile->getBound();
   
}

void ossimPlanetTerrainGeometryTechnique::merge()
{
   if(!theTerrainTile) return;
   
   ossimPlanetTerrainTile::MergeTestVisitor visitor(true);
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theTerrainTile->getNumChildren();++idx)
   {
      theTerrainTile->getChild(idx)->accept(visitor);
   }
   if(visitor.canMerge())
   {
      theTerrainTile->removeChildren(0, theTerrainTile->getNumChildren());
   }
}

void ossimPlanetTerrainGeometryTechnique::removeCulledChildren()
{
   ossim_uint32 idx = 0;
   ossim_uint32 bound = theChildNodeCullParameters.size();
   for(idx = 0; idx < bound; ++idx)
   {
      if(theChildNodeCullParameters[idx].valid())
      {
         
         ossimPlanetTerrainTile* tile = theTerrainTile->child(theChildNodeCullParameters[idx]->tileId());
         if(tile&&theChildNodeCullParameters[idx]->isCulled())
         {
            theTerrainTile->removeChild(tile);
         }
      }
   }
}

bool ossimPlanetTerrainGeometryTechnique::hasCulledChildren()const
{
   if(theTerrainTile->getNumChildren() < 1) return false;
   ossim_uint32 idx = 0;
   ossim_uint32 bounds = theTerrainTile->getNumChildren();
   for(idx = 0; idx < bounds; ++idx)
   {
      const ossimPlanetTerrainTile* tile = dynamic_cast<const ossimPlanetTerrainTile*>(theTerrainTile->getChild(idx));
      if(tile)
      {
         if(tile->culledFlag()) return true;
      }
   }
   
   return false;   
}


bool ossimPlanetTerrainGeometryTechnique::isChildCulled(ossim_uint32 childIdx)const
{
   if(theChildNodeCullParameters[childIdx].valid())
   {
      return theChildNodeCullParameters[childIdx]->isCulled();
   }
   
   return false;
}

double ossimPlanetTerrainGeometryTechnique::texturePriority()const
{
   if(!theTerrainTile) return 0.0;
   
   // const BufferData& buffer = readOnlyBuffer();
   double result = 0.0;
   // double distance = 0.0;
  // result = (thePriorityPoint - thePatchBound.center()).length();

   result =(thePriorityPoint-theCenterGrid).length();

   result = -result;

    return result;
}

double ossimPlanetTerrainGeometryTechnique::elevationPriority()const
{
   return texturePriority();
}

double ossimPlanetTerrainGeometryTechnique::mergePriority()const
{
   return texturePriority();
}

double ossimPlanetTerrainGeometryTechnique::splitPriority()const
{
   return texturePriority();
}

void ossimPlanetTerrainGeometryTechnique::smoothGeometry()
{
   BufferData& buffer = writeBuffer();
   
   if (buffer.theGeometry.valid())
   {
      osgUtil::SmoothingVisitor smoother;
      smoother.smooth(*buffer.theGeometry);
   }
}

osg::BoundingSphere ossimPlanetTerrainGeometryTechnique::computeBound() const
{
   const BufferData& buffer = readOnlyBuffer();
   if(buffer.theTransform.valid())
   {
      return readOnlyBuffer().theTransform->getBound();
   }
   return osg::BoundingSphere(osg::Vec3d(0.0,0.0,0.0), -1);
   
}

bool ossimPlanetTerrainGeometryTechnique::isCulled(BufferData& buffer, osgUtil::CullVisitor* cv)const
{
   if(cv->getCullingMode() == osg::CullSettings::NO_CULLING) return false;
   
   buffer.theCullNode->accept(*cv);
   bool result = buffer.theCullNode->isCulled();
   if(!result)
   {
      result = cv->isCulled(thePatchBound);
   }
   return result;
}

void ossimPlanetTerrainGeometryTechnique::setChildCullParameters(ossimPlanetTerrainTile* tile,
                                                                 osg::ref_ptr<CullNode> cullNode)
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theChildNodeCullParametersMutex);
      
      if(theChildNodeCullParameters.size() != 4)
      {
         theChildNodeCullParameters.resize(4);
      }
      ossim_uint32 idx = childIndex(tile->tileId());
      // this is a quad tree so look at eve odd values.
      //
      if(idx < theChildNodeCullParameters.size())
      {
         theChildNodeCullParameters[idx] = cullNode.get();
      }      
   }
}

void ossimPlanetTerrainGeometryTechnique::markOnlyNeededChildImageLayersDirty()
{
   ossim_uint32 idx = 0;
   ossim_uint32 bound = theTerrainTile->getNumChildren();
   
   for(idx = 0; idx < bound; ++idx)
   {
      ossimPlanetTerrainTile* tile = dynamic_cast<ossimPlanetTerrainTile*>(theTerrainTile->getChild(idx));
      if(tile)
      {
         ossim_uint32 idxLayer = 0;
         ossim_uint32 imageLayerBound = tile->numberOfImageLayers();
         for(idxLayer = 0; idxLayer < imageLayerBound; ++idxLayer)
         {
            //if(theTerrainTile->imageLayer(idxLayer)->isDirty())
            {
               osg::ref_ptr<ossimPlanetImage> image = tile->imageLayer(idxLayer)->image();
               
               // if he is borrowing from a parent Texture make sure he get's reset
               //
               if(!image || (image->tileId().level()!=tile->tileId().level()))
               {
                  tile->imageLayer(idxLayer)->setDirtyFlag(true);
               }
            }
         }
      }
   }
}

void ossimPlanetTerrainGeometryTechnique::updateElevationMesh()
{
   BufferData& buffer = writeBuffer();
   osg::ref_ptr<osg::StateSet> stateset;
   if(buffer.theGeode.valid())
   {
      stateset = buffer.theGeode->getStateSet();
   }
   buildMesh();
   if(stateset.valid())
   {
      if(stateset.valid()&&buffer.theGeode.valid())
      {
         buffer.theGeode->setStateSet(stateset.get());
      }
   }
}

ossimPlanetTexture2D* ossimPlanetTerrainGeometryTechnique::newImageLayerTexture(ossim_uint32 imageLayerIdx)
{
   ossimPlanetTexture2D* texture2D = new ossimPlanetTexture2D;
   osg::ref_ptr<ossimPlanetImage> image;
   ossimPlanetTerrainImageLayer* layer =0;
   if(theTerrainTile)
   {
      layer = theTerrainTile->imageLayer(imageLayerIdx);
      if(layer)
      {
         image = layer->image();
      }
   }
   if(image.valid())
   {
      texture2D->setImage(image.get());
   }
   // texture2D->setMaxAnisotropy(16.0f);
   texture2D->setResizeNonPowerOfTwoHint(false);
   if(layer)
   {
      texture2D->setFilter(osg::Texture2D::MIN_FILTER,layer->minFilter());
      texture2D->setFilter(osg::Texture2D::MAG_FILTER,layer->magFilter());
   }
   texture2D->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
   texture2D->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
   texture2D->setDataVariance(osg::Object::DYNAMIC);
   texture2D->setUnRefImageDataAfterApply(false);
   
   return texture2D;
}

void ossimPlanetTerrainGeometryTechnique::compileGlObjects(osg::State* state)
{
   BufferData& buffer = readOnlyBuffer();
   if(buffer.theGeode.valid())
   {
      ossimPlanetTerrainTechnique::CompileObjects visitor;
      visitor.setState(state);
      buffer.theTransform->accept(visitor);
   }
}

void ossimPlanetTerrainGeometryTechnique::setImageLayerTexture(ossimPlanetTexture2D* texture, 
                                                               ossim_uint32 imageLayerIdx)
{
   BufferData& buffer = readOnlyBuffer();
   if(texture&&buffer.theGeode.valid())
   {
      osg::StateSet* stateset = buffer.theGeode->getOrCreateStateSet();
      if(texture->tileId().level() == theTerrainTile->tileId().level())
      {
         ossimPlanetImage* image = dynamic_cast<ossimPlanetImage*>(texture->getImage());
         if(image)
         {
            ossimPlanetTerrainImageLayer* imageLayer = theTerrainTile->imageLayer(imageLayerIdx);
            if(imageLayer)
            {
               imageLayer->setImage(image);
               imageLayer->setDirtyFlag(false);
               imageLayer->setRefreshFlag(false);
            }
         }
      }
      if(stateset)
      {
         stateset->setTextureAttributeAndModes(imageLayerIdx, texture, osg::StateAttribute::ON);
         updateTextureMatrix(stateset, imageLayerIdx, theTerrainTile->tileId(),  texture->tileId());
      }
      UpdateChildTextureVisitor visitor(texture, imageLayerIdx);
      ossim_uint32 idx = 0;
      for(idx = 0; idx < theTerrainTile->getNumChildren();++idx)
      {
         theTerrainTile->getChild(idx)->accept(visitor);
      }
   }
}

void ossimPlanetTerrainGeometryTechnique::updateTextureMatrix(osg::StateSet* stateset, 
                                                              ossim_uint32 imageLayerIdx,
                                                              const ossimPlanetTerrainTileId& startId,
                                                              const ossimPlanetTerrainTileId& endId)
{
   if(startId.level() != endId.level())
   {
      osg::Matrixd texMat;
      solveTextureMatrixMappingToParent(startId, endId, texMat);
      osg::TexMat* texMatAttribute = new osg::TexMat(texMat);
      stateset->setTextureAttributeAndModes(imageLayerIdx, texMatAttribute, osg::StateAttribute::ON);
   }
   else
   {
      stateset->removeTextureAttribute(imageLayerIdx, osg::StateAttribute::TEXMAT);
   }
}

void ossimPlanetTerrainGeometryTechnique::setElevationMeshFrom(ossimPlanetTerrainTile* tile)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theTerrainTileMutex);
   
   if(tile&&theTerrainTile)
   {
      ossimPlanetTerrainGeometryTechnique* geometryTechnique = dynamic_cast<ossimPlanetTerrainGeometryTechnique*>(tile->terrainTechnique());
      osg::ref_ptr<ossimPlanetImage> elevation = tile->elevationLayer()->image();
      if(geometryTechnique)
      {
         BufferData& bufferFrom = geometryTechnique->readOnlyBuffer();
         
         BufferData& bufferTo = writeBuffer();
         
         if(bufferFrom.theTransform.valid())
         {
            if(theTerrainTile->elevationLayer()&&elevation.valid())
            {
               theTerrainTile->elevationLayer()->setImage(elevation.get());
               theTerrainTile->elevationLayer()->setDirtyFlag(false);
               theTerrainTile->elevationLayer()->setRefreshFlag(false);
            }
            bufferTo.theClusterCullingCallback = bufferFrom.theClusterCullingCallback;
            osg::ref_ptr<osg::StateSet> bufferToTransformStateSet = bufferTo.theTransform->getStateSet();
            osg::ref_ptr<osg::StateSet> bufferToGeometryStateSet = bufferTo.theGeometry->getStateSet();
            osg::ref_ptr<osg::StateSet> bufferToGeodeStateSet = bufferTo.theGeode->getStateSet();
            
            bufferTo.theTransform = bufferFrom.theTransform;
            bufferTo.theGeometry  = bufferFrom.theGeometry;
            bufferTo.theGeode     = bufferFrom.theGeode;
            bufferTo.theCenterPatch = bufferFrom.theCenterPatch;
            bufferTo.theClusterCullingCallback = bufferFrom.theClusterCullingCallback;
            
            bufferTo.theTransform->setStateSet(bufferToTransformStateSet.get());
            bufferTo.theGeometry->setStateSet(bufferToGeometryStateSet.get());
            bufferTo.theGeode->setStateSet(bufferToGeodeStateSet.get());
            
            if(theTerrainTile)
            {
               theTerrainTile->dirtyBound();
               theTerrainTile->getBound();
            }
            // now copy cull node settings to the parent so it's always cached
            //
            // ossimPlanetTerrainTile* parent = theTerrainTile->parentTile();
            swapBuffers();
            // markOnlyNeededChildImageLayersDirty();
         }
      }
   }
}

void ossimPlanetTerrainGeometryTechnique::vacantChildIds(TileIdList& ids)const
{
   ossimPlanetTerrainTechnique::vacantChildIds(ids);
}

ossim_uint32 ossimPlanetTerrainGeometryTechnique::computeSkirtFlags(const ossimPlanetTerrainTileId& /* childId */)const
{
#if 1
   return LEFT_SKIRT|RIGHT_SKIRT|BOTTOM_SKIRT|TOP_SKIRT;
#else   
   if(childId.level() == 0) return LEFT_SKIRT|RIGHT_SKIRT|BOTTOM_SKIRT|TOP_SKIRT;
   ossim_uint32 result = NO_SKIRT;
   switch(childIndex(childId))
   {
      case BOTTOM_LEFT: // bottom Left quadrant
      {
         result = BOTTOM_SKIRT|LEFT_SKIRT;
         break;
      }
      case BOTTOM_RIGHT: // bottom right
      {
         result = BOTTOM_SKIRT|RIGHT_SKIRT;
         break;
      }
      case TOP_LEFT: // top left child
      {
         result = TOP_SKIRT|LEFT_SKIRT;
         break;
      }
      case TOP_RIGHT: // top right child
      {
         result = TOP_SKIRT|RIGHT_SKIRT;
         break;
      }
      default:
      {
         break;
      }
   }
   
   return result;
#endif
}

void ossimPlanetTerrainGeometryTechnique::releaseGLObjects(osg::State* state)
{
   BufferData& buffer = readOnlyBuffer();
   if(buffer.theTransform.valid())
   {
      buffer.theTransform->releaseGLObjects(state);
      buffer.theGeode->releaseGLObjects(state);
      buffer.theGeometry->releaseGLObjects(state);
   }
}
