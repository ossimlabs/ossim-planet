#include <ossimPlanet/ossimPlanetCloudLayer.h>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/TexMat>
#include <osg/io_utils>
#include <osgUtil/CullVisitor>
#include <osgUtil/IntersectVisitor>
#include <osgUtil/IntersectionVisitor>
#include <queue>
#include <ossim/base/ossimCommon.h>

ossimPlanetCloudLayer::ossimPlanetCloudLayer()
:theHeading(0.0),
theSpeed(0.0),
theTextureTranslation(0.0,0.0,0.0),
theTextureScale(1.0,1.0,1.0),
theAutoUpdateTextureMatrixFlag(true),
theTextureWidth(0),
theTextureHeight(0),
theMeshLevel(0),
theApproximateMetersPerPixelCoverage(90000.0),// lets just init to a big coverage for cloud
theMaxAltitudeToShowClouds(9999999999999.0)// lets just init to a big coverage for cloud
{
   init();
}

void ossimPlanetCloudLayer::traverse(osg::NodeVisitor& nv)
{
   if(dynamic_cast<osgUtil::IntersectVisitor*> (&nv) ||
      dynamic_cast<osgUtil::IntersectionVisitor*> (&nv)||
      !enableFlag())
   {
      return;
   }
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         updateTextureMatrix(nv.getFrameStamp()->getSimulationTime());
         break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
         osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(&nv);
         if(cv)
         {
            osg::Vec3d eye = cv->getEyePoint();
            if(theModel.valid())
            {
               osg::Vec3d llh;
               theModel->xyzToLatLonHeight(eye, llh);
               if(llh[2] > theMaxAltitudeToShowClouds)
               {
                  return;
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
   ossimPlanetLayer::traverse(nv);
}

void ossimPlanetCloudLayer::init()
{
   theGrid  = new ossimPlanetAdjustableCubeGrid(ossimPlanetAdjustableCubeGrid::HIGH_CAP);
   getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
   getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
   setNodeMask(0xffffffff);
   theTexture = new osg::Texture2D;
   theTexture->setResizeNonPowerOfTwoHint(false);
   theTexture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
   theTexture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
   theTexture->setWrap(osg::Texture::WRAP_S,osg::Texture::REPEAT);
   theTexture->setWrap(osg::Texture::WRAP_T,osg::Texture::REPEAT);
   theTexture->setDataVariance(osg::Object::DYNAMIC);
   theTexture->setUnRefImageDataAfterApply(false);
   getOrCreateStateSet()->setTextureAttributeAndModes(0, 
                                                      theTexture.get(), 
                                                      osg::StateAttribute::ON);
}

void ossimPlanetCloudLayer::splitTiles(ossimPlanetGrid::TileIds& tiles, ossim_uint32 levels)const
{
   if(levels == 0) return;
   std::queue<ossimPlanetTerrainTileId> workQueue;
   ossim_uint32 idx = 0;
   for(idx = 0; idx < tiles.size(); ++idx)
   {
      if(!theGrid->isPolar(tiles[idx]))
      {
         workQueue.push(tiles[idx]);
      }
   }
   tiles.clear();
   ossimPlanetTerrainTileId quad00;
   ossimPlanetTerrainTileId quad10;
   ossimPlanetTerrainTileId quad11;
   ossimPlanetTerrainTileId quad01;
   ossimPlanetTerrainTileId currentTile;
   while(!workQueue.empty())
   {
      currentTile = workQueue.front();
      workQueue.pop();
      currentTile.splitQuad(quad00, quad10, quad11, quad01);
      if(currentTile.level()+1 >= levels)
      {
         tiles.push_back(quad00);
         tiles.push_back(quad10);
         tiles.push_back(quad11);
         tiles.push_back(quad01);
      }
      else
      {
         workQueue.push(quad00);
         workQueue.push(quad10);
         workQueue.push(quad11);
         workQueue.push(quad01);
      }
   }
}

void ossimPlanetCloudLayer::updateTexture(osg::Image* cloudTexture)
{
   theTexture->setImage(cloudTexture);
   if(cloudTexture)
   {
      theTextureWidth  = cloudTexture->s();
      theTextureHeight = cloudTexture->t();
      updateMetersPerPixelCoverage();
  }
   else
   {
      theTextureWidth  = 0;
      theTextureHeight = 0;
   }
}

void ossimPlanetCloudLayer::updateTexture(ossim_int64 seed,
                                          ossim_int32 coverage,
                                          ossim_float64 sharpness)
{
   osg::ref_ptr<ossimPlanetCloud> cloud = new ossimPlanetCloud;
   srand(seed);
   cloud->makeCloud(seed, coverage, sharpness);
   updateTexture(cloud->image());
}

void ossimPlanetCloudLayer::computeMesh(double patchAltitude,
                                        ossim_uint32 patchWidth, 
                                        ossim_uint32 patchHeight,
                                        ossim_uint32 level)
{
   if(!theModel.valid()) return;
   theGeometryArray.clear();
   theCenterLlh = osg::Vec3d(0.0,0.0,patchAltitude);
   ossim_uint32 adjustedWidth  = patchWidth;
   ossim_uint32 adjustedHeight = patchHeight;
   double adjustedAltitude = patchAltitude;
   bool patchFlag = false;
   if(dynamic_cast<Patch*> (theGrid.get()))
   {
      if(level > 0)
      {
         adjustedWidth  <<= level;
         adjustedHeight <<= level;
      }
      // can only do single level meshes with a local patch
      theMeshLevel = 0;
      adjustedAltitude = 0;
      patchFlag = true;
   }
   ossimPlanetGrid::ModelPoint modelPoint;
   ossim_uint32 numCols = adjustedWidth|1; // make sure it's odd
   ossim_uint32 numRows = adjustedHeight|1; // make sure it's odd
   ossim_uint32 numberOfPoints = numCols*numRows;
   bool exceedsUshort = numberOfPoints >= 0x1111;
   ossimPlanetGrid::TileIds tileIds;
   theGrid->getRootIds(tileIds);
   if(theMeshLevel != 0)
   {
      splitTiles(tileIds, theMeshLevel);
   }
   if(getNumChildren())
   {
      removeChildren(0, getNumChildren());
   }
   ossim_uint32 idx = 0;
   double modelHeight = adjustedAltitude*theModel->getInvNormalizationScale();
   for(idx = 0; idx < tileIds.size();++idx)
   {
      if(theGrid->isPolar(tileIds[idx]))
      {
         continue;
      }
      ossimPlanetTerrainTileId& tileId = tileIds[idx];
      osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform;
      osg::ref_ptr<osg::Geode>    geode = new osg::Geode;
      osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
      
      osg::Matrixd localToWorld;
      theModel->lsrMatrix(osg::Vec3d(modelPoint.y(),
                                     modelPoint.x(),
                                     adjustedAltitude),
                          localToWorld);
      osg::Matrixd inverseLocalToWorld;
      inverseLocalToWorld.invert(localToWorld);
      transform->addChild(geode.get());
      geode->addDrawable(geometry.get());
      unsigned int numVerticesInBody = numCols*numRows;
      unsigned int numVertices       = numVerticesInBody;
      
      osg::ref_ptr<osg::Vec2Array> tcoords = new osg::Vec2Array;
      tcoords->reserve(numVertices);
      geometry->setTexCoordArray( 0, tcoords.get());   
      
      osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
      vertices->reserve(numVertices);
      geometry->setVertexArray(vertices.get());
      
      // allocate and assign normals
      osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
      if (normals.valid()) normals->reserve(numVertices);
      geometry->setNormalArray(normals.get());
      geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
      
      // allocate and assign color
      osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(1);
      (*colors)[0].set(1.0f,1.0f,1.0f,1.0f);
      
      geometry->setColorArray(colors.get());
      geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
      theGeometryArray.push_back(geometry.get());
      ossimPlanetGrid::LocalNdcPoint localPoint;
      typedef std::vector<int> Indices;
      Indices indices(numVertices, -1);
      osg::Vec3d xyz;
      osg::Vec3d localXyz;
      osg::Vec3d norm1;
      ossim_uint32 baseIdx = 0;
      unsigned int iv = 0;
      double txInc = 1.0/(numCols-1.0);
      double tyInc = 1.0/(numRows-1.0);
      double ty = 0.0;
      ossim_uint32 colIdx, rowIdx;
      localPoint.setZ(modelHeight);
      for(rowIdx = 0; rowIdx < numRows;++rowIdx)
      {
         double tx = 0.0;
         localPoint.setY(ty);//(double)(rowIdx)/(double)(numRows-1));
         for(colIdx = 0; colIdx < numCols;++colIdx)
         {
            //rowIdx*numCols + colIdx;
            indices[iv] = vertices->size();
            localPoint.setX(tx);
            tcoords->push_back(osg::Vec2d(tx, ty));
            theGrid->localNdcToModel(tileId, localPoint, modelPoint);
            osg::Vec3d llh(modelPoint.y(), modelPoint.x(), adjustedAltitude);
            theModel->latLonHeightToXyz(llh, xyz);
            localXyz = xyz*inverseLocalToWorld;
            (*vertices).push_back(localXyz);
            theModel->normal(xyz, norm1);
            norm1 = osg::Matrixd::transform3x3(localToWorld, norm1);
            norm1.normalize();
            (*normals).push_back(norm1);
            ++iv;
            tx+=txInc;
         }
         ty += tyInc;
      }
      transform->setMatrix(localToWorld);
      
      if(!exceedsUshort)
      {
         osg::DrawElementsUShort& drawElements = *(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES));
         osg::Geometry::PrimitiveSetList primSetList;
         ossim_uint32 i = 0;
         ossim_uint32 j = 0;
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
         primSetList.push_back(&drawElements);
         geometry->setPrimitiveSetList(primSetList);
      }
      else
      {
         osg::DrawElementsUInt& drawElements = *(new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES));
         osg::Geometry::PrimitiveSetList primSetList;
         ossim_uint32 i = 0;
         ossim_uint32 j = 0;
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
         primSetList.push_back(&drawElements);
         geometry->setPrimitiveSetList(primSetList);
      }
      //osgUtil::Simplifier simplifier(.8);
      //simplifier.simplify(*geometry, pointsToProtect);
      geometry->dirtyBound();
      geometry->getBound();
      
      addChild(transform.get());
   }
   theTextureScale[0] = 1.0;
   theTextureScale[1] = 1.0;
   theTextureScale[2] = 1.0;
   theTextureMatrixAttribute = new osg::TexMat();
   updateTextureMatrix();
   updateMetersPerPixelCoverage();
   getOrCreateStateSet()->setTextureAttributeAndModes(0, theTextureMatrixAttribute.get(), osg::StateAttribute::ON);
   
   if(patchFlag)
   {
      moveToLocationLatLonAltitude(theCenterLlh);
   }
}

void ossimPlanetCloudLayer::setAlphaValue(float alpha) 
{
   geometryPtr* geometryArray = &theGeometryArray.front();
   ossim_uint32 length = theGeometryArray.size();
   
   ossim_uint32 idx = 0;
   for(idx = 0; idx < length;++idx)
   {
      osg::Vec4Array* colors = (osg::Vec4Array*)((*geometryArray)->getColorArray());
      (*colors)[0].set(1.0f,1.0f,1.0f,alpha);
      
      // we will let osg dirty and initialize internls s set it back on the geometry
      (*geometryArray)->setColorArray(colors);
      ++geometryArray;
   }
}

void ossimPlanetCloudLayer::moveToLocationLatLonAltitude(const osg::Vec3d& llh)
{
  if(getNumChildren()==1)
   {
      osg::MatrixTransform* mt = dynamic_cast<osg::MatrixTransform*>(getChild(0));
      if(mt)
      {
         osg::Vec3d newLlh = llh;
         osg::Matrixd localToWorld;
         theModel->mslToEllipsoidal(newLlh);
         theModel->lsrMatrix(newLlh,
                             localToWorld);
         mt->setMatrix(localToWorld);
         theCenterLlh = newLlh;
      }
   }
}

void ossimPlanetCloudLayer::updateTextureMatrix(double timeScale)
{
   if(!theAutoUpdateTextureMatrixFlag) return;
   if((theTextureHeight>0)&&
      (!ossim::almostEqual(theSpeed, 0.0)))
   {
      double distance = (((theSpeed*timeScale)/theApproximateMetersPerPixelCoverage))/theTextureHeight;
      theTextureTranslation = ((osg::Vec3d(0.0,1.0,0.0)*
                                osg::Matrixd::rotate(osg::DegreesToRadians(-theHeading), 
                                                     osg::Vec3d(0.0, 0.0, 1.0)))*
                               (-distance));
   }
   else
   {
      theTextureTranslation = osg::Vec3d(0.0,0.0,0.0);  
   }
   theTextureMatrix = osg::Matrixd::scale(theTextureScale);
   theTextureMatrix(3,0) = theTextureTranslation[0];
   theTextureMatrix(3,1) = theTextureTranslation[1];
   theTextureMatrix(3,2) = theTextureTranslation[2];
   
   if(theTextureMatrixAttribute.valid())
   {
      theTextureMatrixAttribute->setMatrix(theTextureMatrix);
   }
}

void ossimPlanetCloudLayer::updateMetersPerPixelCoverage()
{
   if(theTextureHeight>0)
   {
      double factor = 1.0;
      if(theMeshLevel)
      {
         factor /=(ossim_float64)(1<<theMeshLevel);
         
      }
      theApproximateMetersPerPixelCoverage = ((90.0*factor)/(theTextureScale[1]*theTextureHeight))*ossimGpt().metersPerDegree().y;
   }
}

void ossimPlanetCloudLayer::setMaxAltitudeToShowClouds(ossim_float64 maxAltitude)
{
   theMaxAltitudeToShowClouds = maxAltitude;
}

ossim_float64 ossimPlanetCloudLayer::maxAltitudeToShowClouds()const
{
   return theMaxAltitudeToShowClouds;
}

void ossimPlanetCloudLayer::setTextureMatrix(osg::TexMat* texMatrix)
{
   if((theTextureMatrixAttribute.get()!=texMatrix)&&
      (texMatrix))
   {
      theTextureMatrixAttribute = texMatrix;
      theTextureMatrix = texMatrix->getMatrix();
      getOrCreateStateSet()->setTextureAttributeAndModes(0, theTextureMatrixAttribute.get(), osg::StateAttribute::ON);
   }
   
}

ossimPlanetCloud::ossimPlanetCloud(TextureSize size)
{
   switch(size)
   {
      case TEXTURE_SIZE_256_256:
      {
         theCloudDataWidth=theCloudDataHeight=256;
         theNoiseDataWidth=theNoiseDataHeight=32;
         break;
      }
      case TEXTURE_SIZE_512_512:
      {
         theCloudDataWidth=theCloudDataHeight=512;
         theNoiseDataWidth=theNoiseDataHeight=64;
         break;
      }
      case TEXTURE_SIZE_1024_1024:
      {
         theCloudDataWidth=theCloudDataHeight=1024;
         theNoiseDataWidth=theNoiseDataHeight=128;
         break;
      }
      case TEXTURE_SIZE_2048_2048:
      {
         theCloudDataWidth=theCloudDataHeight=2048;
         theNoiseDataWidth=theNoiseDataHeight=256;
         break;
      }
      default:
      {
         break;
      }
   }
   
   theCloudData.resize(theCloudDataWidth*theCloudDataHeight);
   theNoise.resize(theNoiseDataWidth*theNoiseDataHeight);
}

double ossimPlanetCloud::noise(ossim_int32 x, ossim_int32 y, ossim_int32 random)const
{
   ossim_int32 n = x + y * 57 + random * 131;
   n = (n<<13) ^ n;
   return (1.0 - ( (n * (n * n * 15731 + 789221) +
                    1376312589)&0x7fffffff)* 0.000000000931322574615478515625);
}
void ossimPlanetCloud::makeNoise(ossim_int64 seed)
{
   srand(seed);
   std::vector<std::vector<ossim_float64> > temp;
//   ossim_float64 temp[theNoiseDataWidth+2][theNoiseDataHeight+2];
   ossim_float64* noiseMap = &theNoise.front();
   ossim_int32 random=rand() % 100000;
   temp.resize(theNoiseDataHeight+2);
   temp[0].resize(theNoiseDataWidth+2);
   temp[theNoiseDataHeight+1].resize(theNoiseDataWidth+2);
   for (int y=1; y<(theNoiseDataHeight+1); y++) 
   {
	   temp[y].resize(theNoiseDataWidth+2);
      for (int x=1; x<(theNoiseDataWidth+1); x++)
      {
		  temp[y][x] = 128.0 + noise(x,  y,  random)*128.0;
       //temp[x][y] = 128.0 + noise(x,  y,  random)*128.0;
      }
   }
   for (int x=1; x<(theNoiseDataWidth+1); x++)
   {
      temp[x][0] = temp[x][theNoiseDataWidth];
      temp[x][theNoiseDataWidth+1] = temp[x][1];
      temp[0][x] = temp[theNoiseDataHeight][x];
      temp[theNoiseDataWidth+1][x] = temp[1][x];
   }
   temp[0][0] = temp[theNoiseDataHeight][theNoiseDataWidth];
   temp[theNoiseDataWidth+1][theNoiseDataHeight+1] = temp[1][1];
   temp[theNoiseDataHeight+1][0] = temp[1][theNoiseDataWidth];
   temp[0][theNoiseDataWidth+1] = temp[theNoiseDataHeight][1];
   for (int y=1; y<(theNoiseDataHeight+1); y++)
   {
      for (int x=1; x<(theNoiseDataWidth+1); x++)
      {
         ossim_float64 center = temp[y][x]/4.0;
         ossim_float64 sides = (temp[y][x+1] + temp[y][x-1] + temp[y+1][x] + temp[y-1][x])/8.0;
         ossim_float64 corners = (temp[y+1][x+1] + temp[y-1][x+1] + temp[y+1][x-1] + temp[y-1][x-1])/16.0;
         
         noiseMap[((x-1)*theNoiseDataWidth) + (y-1)] = center + sides + corners;
      }
   }
}
ossim_float64 ossimPlanetCloud::interpolate(ossim_float64 x, ossim_float64 y, ossim_float64  *map)
{
   int Xint = (int)x;
   int Yint = (int)y;
   
   ossim_float64 Xfrac = x - Xint;
   ossim_float64 Yfrac = y - Yint;
   int X0 = Xint % theNoiseDataWidth;
   int Y0 = Yint % theNoiseDataHeight;
   int X1 = (Xint + 1) % theNoiseDataWidth;
   int Y1 = (Yint + 1) % theNoiseDataHeight;      
   ossim_float64 bot = map[X0*theNoiseDataWidth + Y0] + Xfrac * (map[X1*theNoiseDataWidth + Y0] - map[X0*theNoiseDataWidth + Y0]);
   ossim_float64 top = map[X0*theNoiseDataWidth + Y1] + Xfrac * (map[X1*theNoiseDataWidth +  Y1] - map[X0*theNoiseDataWidth + Y1]);
   
   return (bot + Yfrac * (top - bot));
}
void ossimPlanetCloud::overlapOctaves()
{
   ossim_float64  *mapNoise = &theNoise.front(); 
   ossim_float64  *mapCloud = &theCloudData.front();
   ossim_uint32 cloudArea = theCloudDataWidth*theCloudDataHeight;
   ossim_int32 x,y, octave;
   for (x=0; x<cloudArea; x++)
   {
      mapCloud[x] = 0;
   }
   for (octave=0; octave<4; octave++)
   {
      for (x=0; x<theCloudDataWidth; x++)
      {
         for (y=0; y<theCloudDataHeight; y++)
         {
            ossim_float64 scale = 1 / pow(2.0, 3.0-octave);
            ossim_float64 noise = interpolate(x*scale, y*scale , mapNoise);
            mapCloud[(y*theCloudDataWidth) + x] += noise / pow(2.0, (ossim_float64)octave);
         }
      }
   }
}
void ossimPlanetCloud::expFilter()
{
//   std::cout << "sharp = " << theSharpness << std::endl;
//   std::cout << "cov = " << theCoverage << std::endl;
   
   ossim_float64  *map = &theCloudData.front();
   for (int x=0; x<theCloudData.size(); x++)
   {
      ossim_float64 c = map[x] - (255.0-theCoverage);
      if (c<0)
      {
         c = 0;
      }
      map[x] = 255.0 - ((ossim_float64)(pow(theSharpness, c))*255.0);
   }
}
osg::Image* ossimPlanetCloud::image()
{
   return theImage.get();
}

void ossimPlanetCloud::makeCloud(ossim_int64 seed=0, ossim_int32 coverage, ossim_float64 sharpness)
{
   theCoverage = coverage;
   theSharpness = sharpness;
   ossim_uint32 area = theCloudDataWidth*theCloudDataHeight;
   unsigned char* newData = new unsigned char[area*4];
   unsigned char* newDataPtr = newData;
   ossim_float64* cloudData = &theCloudData.front();
   makeNoise(seed);
   overlapOctaves();
   expFilter();  
   for(unsigned int idx = 0; idx < area;++idx)
   {
      newDataPtr[0] = (ossim_uint8)ossim::clamp((*cloudData), 0.0, 255.0);
      newDataPtr[1] = newDataPtr[0];
      newDataPtr[2] = newDataPtr[0];
      newDataPtr[3] = newDataPtr[0];
      newDataPtr+=4;
      ++cloudData;
   }      
   theImage = new osg::Image();
   theImage->setImage(theCloudDataWidth, theCloudDataHeight, 1,
                      GL_RGBA,
                      GL_RGBA,
                      GL_UNSIGNED_BYTE,
                      newData,
                      osg::Image::USE_NEW_DELETE);
}
