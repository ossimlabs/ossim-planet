#include <ossimPlanet/ossimPlanetLandReaderWriter.h>
#include <ossimPlanet/ossimPlanetPlaneGrid.h>
#include <ossimPlanet/ossimPlanetCubeGrid.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <osg/CullFace>
#include <osg/Program>
#include <osg/Uniform>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/GLObjectsVisitor>
#include <osgUtil/Optimizer>
#include <osgUtil/CullVisitor>
#include <osg/PagedLOD>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/TriStripVisitor>
#include <osgDB/Registry>
#include <OpenThreads/ScopedLock>
#include <ossimPlanet/ossimPlanetBoundingBox.h>
#include <ossimPlanet/ossimPlanetLandTextureRequest.h>
#include <ossimPlanet/ossimPlanetDatabasePager.h>
#include <ossim/elevation/ossimElevManager.h>
// #include <ossimPlanet/ossimPlanetElevationGrid.h>
#include <ossimPlanet/mkUtils.h>
#include <ossimPlanet/ossimPlanetPagedLandLod.h>
#include <ossimPlanet/ossimPlanetTexture2D.h>
#include <ossim/base/ossimDrect.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <osgUtil/Simplifier>
#include <ossim/base/ossimGeoidManager.h>

static ossim_uint32 landReaderWriterId = 0;

ossimPlanetLandReaderWriter::ossimPlanetLandReaderWriter()
   :theId(landReaderWriterId++)
{  
   theLandType = ossimPlanetLandType_NORMALIZED_ELLIPSOID;
   theMaxLevelDetail    = 18;
   theHeightExag        = 1.0;
   theElevationPatchSize= 17;
   theElevationEnabledFlag   = true;
   theMultiTextureEnableFlag = false;
   theMipMappingFlag = true;
   theGrid          = new ossimPlanetCubeGrid;
   theElevationGrid = new ossimPlanetCubeGrid;
}

const char* ossimPlanetLandReaderWriter::className()const
{
   return "ossimPlanetLandReaderWriter";
}

osgDB::ReaderWriter::ReadResult ossimPlanetLandReaderWriter::readNode(const std::string& filename, const Options* /* options */ )const
{

   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   if(!theModel.valid()) return 0;
  // std::cout << "ossimPlanetLandReaderWriter::readNode filename = " << filename << std::endl;
   osgDB::ReaderWriter::ReadResult result = 0;
   if(filename == "") return result;
   ossim_uint32 level;
   ossim_uint32 row;
   ossim_uint32 col;
   ossim_uint32 id;
   ossimPlanetLandReaderWriterGeomType geomType = ossimPlanetLandReaderWriterGeomType_NONE;

   if(!extractValues(filename,
                     level,
                     row,
                     col,
                     id,
                     geomType))
   {
      return result;
   }
//    std::cout << "level = " << level << std::endl;
   if(id != theId)
   {
      return result;
   }
   osg::Vec3d centerPoint;
   osg::Vec3d ulPoint;
   osg::Vec3d urPoint;
   osg::Vec3d lrPoint;
   osg::Vec3d llPoint;
   osg::Vec3d centerNormal;
   osg::Vec3d ulNormal;
   osg::Vec3d urNormal;
   osg::Vec3d lrNormal;
   osg::Vec3d llNormal;
   osg::ref_ptr<ossimPlanetBoundingBox> box = new ossimPlanetBoundingBox;
   double minLat;
   double minLon;
   double maxLat;
   double maxLon;
//    unsigned int w = theUtility.getTileWidth();
//    unsigned int h = theUtility.getTileHeight();
   theGrid->getLatLonBounds(minLat,
                            minLon,
                            maxLat,
                            maxLon,
                            level,
                            row, // row 
                            col);
   ossim_uint64 tileId = theGrid->getId(level, row, col);
   osg::ref_ptr<ossimPlanetLandCacheNode> cacheNode = theLandCache->getNode(tileId, true);
   if(cacheNode.valid())
   {
      double dx, dy;
      theGrid->getPixelScaleAsMeters(dx, dy, level, row, col);
      double scale = (dx+dy)*.5;
      ossimPlanetExtents* extents = new ossimPlanetExtents;
      extents->setMinMaxLatLon(minLat, minLon, maxLat, maxLon);
      extents->setMinMaxScale(scale*.5, scale*2.0);
      
      cacheNode->setExtents(extents);
   }
   
   ossimPlanetImage::ossimPlanetImageStateType textureState = ossimPlanetImage::ossimPlanetImageStateType_LOADED;
   osg::ref_ptr<ossimPlanetImage> elevationGrid;

   std::vector<osg::ref_ptr<osg::Texture2D> > textures;
   newTexture(textures,
              level,
              row,
              col,
              textureState);
   
   if(geomType & ossimPlanetLandReaderWriterGeomType_TEXTURE)
   {
      ossimPlanetLandTextureRequest* landRequest = new ossimPlanetLandTextureRequest(level, row, col, textures);
      osgUtil::GLObjectsVisitor v;
      landRequest->setTextureState(textureState);
      
      landRequest->accept(v);
      
      return landRequest;
   }
   if(geomType & (ossimPlanetLandReaderWriterGeomType_GEOM|ossimPlanetLandReaderWriterGeomType_LOD))
   {
      elevationGrid = newElevation(level, row, col);
   }
   osg::ref_ptr<ossimPlanetPagedLandLod> pagedLod;
   if(geomType & ossimPlanetLandReaderWriterGeomType_LOD)
   {
      pagedLod = new  ossimPlanetPagedLandLod(level, row, col, filename);
   }
   osg::ref_ptr<ossimPlanetBoundingBox> boundingBox = new ossimPlanetBoundingBox;
   bool useClusterCulling = false;
   osg::Vec3d clusterControlPoint ;
   osg::Vec3d clusterCenterNormal;
   double minDotProduct;
   double maxClusterCullingRadius;
   osg::ref_ptr<osg::MatrixTransform> trans = newGeometry(level,
                                                          row,
                                                          col,
                                                          textures,
                                                          elevationGrid,
                                                          *boundingBox,
                                                          useClusterCulling,
                                                          clusterControlPoint,
                                                          clusterCenterNormal,
                                                          minDotProduct,
                                                          maxClusterCullingRadius);
   std::vector<ossimPlanetBoundingBox> childrenBounds;
   
   if(!pagedLod.valid())
   {
      ossimPlanetLandTextureRequest* landRequest = new ossimPlanetLandTextureRequest(level, row, col, textures);
      landRequest->setTextureState(textureState);
//       landRequest->setTransform(trans);
      landRequest->setCullCallback(theCullNodeCallback.get());
      initSupportAttributes(level,
                            row,
                            col,
                            centerPoint,
                            ulPoint,
                            urPoint,
                            lrPoint,
                            llPoint,
                            centerNormal,
                            ulNormal,
                            urNormal,
                            lrNormal,
                            llNormal,
                            *box,
                            childrenBounds,
                            *trans);
      landRequest->setCenterPoint(centerPoint);
      landRequest->setUlPoint(ulPoint);
      landRequest->setUrPoint(urPoint);
      landRequest->setLrPoint(lrPoint);
      landRequest->setLlPoint(llPoint);
      landRequest->setCenterNormal(centerNormal);
      landRequest->setUlNormal(ulNormal);
      landRequest->setUrNormal(urNormal);
      landRequest->setLrNormal(lrNormal);
      landRequest->setLlNormal(llNormal);
//       if(level == 0)
//       {
//          landRequest->setBoundingBox(box);
//       }
//       else
//       {
      landRequest->setBoundingBox(boundingBox);
//       }
//       landRequest->setChildrenBounds(childrenBounds);
//       osgUtil::GLObjectsVisitor v;
      
//       landRequest->accept(v);

//       if(useClusterCulling)
//       {
//          landRequest->setClusterCullValues(useClusterCulling,
//                                            clusterControlPoint,
//                                            clusterCenterNormal,
//                                            minDotProduct,
//                                            maxClusterCullingRadius);
//       }
      return landRequest;
   }

   if(theGrid->getNumberOfFaces() <3)
   {
      ossimPlanetGridUtility::GridPoint centerGridPoint;
      
      theGrid->getCenterGridPoint(centerGridPoint, level, row, col);
      
      theGrid->getLatLon(centerPoint, centerGridPoint);
      if(centerPoint[0] > 60.0)
      {
         pagedLod->theMaxLevel = ((minLat > 80.0)?4:99999999);
      }
      else if(centerPoint[0] < -60)
      {
         pagedLod->theMaxLevel = ((maxLat < -80.0)?4:99999999);
      }
      else
      {
         pagedLod->theMaxLevel = 99999999;
      }
   }
//    pagedLod->theTransform = trans.get();

   osg::ref_ptr<osg::Geode> geode = dynamic_cast<osg::Geode*>(trans->getChild(0));
   if(geode.valid())
   {
//      geode->getDrawable(0)->setDrawCallback(new ossimPlanetPagedLandDrawableCallback(pagedLod.get()));
//      geode->getDrawable(0)->setDrawCallback(new ossimPlanetPagedLandDrawableCallback(level, row, col));
   }
   
   initSupportAttributes(level,
                         row,
                         col,
                         centerPoint,
                         ulPoint,
                         urPoint,
                         lrPoint,
                         llPoint,
                         centerNormal,
                         ulNormal,
                         urNormal,
                         lrNormal,
                         llNormal,
                         *box,
                         childrenBounds,
                         *trans);
   pagedLod->theGeode = geode;
   pagedLod->theCenterPoint = centerPoint;
   pagedLod->theUlPoint = ulPoint;
   pagedLod->theUrPoint = urPoint;
   pagedLod->theLrPoint = lrPoint;
   pagedLod->theLlPoint = llPoint;
   pagedLod->theCenterNormal = centerNormal;
   pagedLod->theUlNormal = ulNormal;
   pagedLod->theUrNormal = urNormal;
   pagedLod->theLrNormal = lrNormal;
   pagedLod->theLlNormal = llNormal;

   pagedLod->theCullNode = new ossimPlanetPagedLandLodCullNode(boundingBox,
                                                               useClusterCulling,
                                                               clusterControlPoint,
                                                               clusterCenterNormal,
                                                               minDotProduct,
                                                               maxClusterCullingRadius);
   pagedLod->theCullNode->setMatrix(trans->getMatrix());
   
   int face = theGrid->getFace(level, row, col);
   ossim_uint64 wide;
   ossim_uint64 high;
   ossim_uint64 originX = col;
//   ossim_uint64 originY = row;
   
   theGrid->getNumberOfTilesWideHighPerFace(wide, high, level);

   originX -= face*wide;
   theGrid->getNumberOfTilesWideHighPerFace(wide, high, level+1);
   
   pagedLod->setCullCallback(theCullNodeCallback.get());
   pagedLod->addChild(trans.get());

    
   pagedLod->setRequestName(0,
                            createDbString(level,
                                           row,
                                           col));
   if(textureState == ossimPlanetImage::ossimPlanetImageStateType_NEEDS_LOADING)
   {
      pagedLod->setRefreshType(ossimPlanetLandRefreshType_TEXTURE);
   }
   if(level+1 <= theMaxLevelDetail)
   {
      pagedLod->setRequestName(1,
                               createDbString(level+1,
                                              row<<1,
                                              (originX<<1)+(face*wide)));
      pagedLod->setRequestName(2,
                               createDbString(level+1,
                                              row<<1,
                                              ((originX<<1) + 1)+(face*wide)));
      pagedLod->setRequestName(3,
                               createDbString(level+1,
                                              (row<<1)+1,
                                              ((originX<<1)+1)+(face*wide)));
      pagedLod->setRequestName(4,
                               createDbString(level+1,
                                              (row<<1)+1,
                                              (originX<<1)+(face*wide)));
   }
   else
   {
      pagedLod->setRequestName(1,
                               "");
      pagedLod->setRequestName(2,
                               "");
      pagedLod->setRequestName(3,
                               "");
      pagedLod->setRequestName(4,
                               "");
   }
   
//    osgUtil::GLObjectsVisitor v;
   
//    pagedLod->accept(v);
//    osgUtil::Optimizer opt; 
//    opt.optimize(pagedLod.get());

//    osgUtil::SmoothingVisitor smooth;

//    pagedLod->accept(smooth);
   return pagedLod.get();
}


// osgDB::ReaderWriter::ReadResult ossimPlanetLandReaderWriter::local_readNode(const std::string& filename,
//                                                                           const Options* options)const
// {
// }

bool ossimPlanetLandReaderWriter::extractValues(const std::string& filename,
                                              ossim_uint32 &level,
                                              ossim_uint32 &row,
                                              ossim_uint32 &col,
                                              ossim_uint32 &id,
                                              ossimPlanetLandReaderWriterGeomType &geomType)const
{
   ossimString tempString = filename;
   tempString = tempString.substitute("_", "\n", true);

   std::istringstream in(tempString);
   ossim_uint32 numberOfValuesFound = 0;
   geomType = ossimPlanetLandReaderWriterGeomType_NONE;
   while(!in.fail())
   {
      ossimString tempLine;
      
      std::getline(in, tempLine.string());
      tempLine = tempLine.upcase();
	  if (tempLine.begin() == tempLine.end()) continue;

      if(tempLine == "TEXTURE")
      {
         geomType = (ossimPlanetLandReaderWriterGeomType)(geomType | ossimPlanetLandReaderWriterGeomType_TEXTURE);
      }
      else if(tempLine == "GEOM")
      {
         geomType = (ossimPlanetLandReaderWriterGeomType)(geomType | ossimPlanetLandReaderWriterGeomType_GEOM);
      }
      else if((*tempLine.begin()) == 'L')
      {
         tempLine = tempLine.substitute("L", "");
         level = tempLine.toUInt32();
         ++numberOfValuesFound;
      }
      else if((*tempLine.begin()) == 'Y')
      {
         tempLine = tempLine.substitute("Y", "");
         row = tempLine.toUInt32();
         ++numberOfValuesFound;
      }
      else if((*tempLine.begin()) == 'X')
      {
         tempLine = tempLine.substitute("X", "");
         col = tempLine.toUInt32();
         ++numberOfValuesFound;
      }
      else if(((*tempLine.begin()) == 'I')&&
              ((*(tempLine.begin()+1)) == 'D'))
      {
         tempLine = tempLine.substitute("ID", "");
         id = tempLine.toUInt32();
         ++numberOfValuesFound;
      }
   }
   if(!geomType)
   {
      geomType = ossimPlanetLandReaderWriterGeomType_LOD;
   }
   return (numberOfValuesFound == 4);
}

std::string ossimPlanetLandReaderWriter::createDbString(ossim_uint32 level,
                                                        ossim_uint64 row,
                                                        ossim_uint64 col)const
{
   std::stringstream out;
   
   out << "L"<<level<<"_X"<<col<<"_Y"<<row<<"_ID"<<theId;

   return out.str();
}

#if 0
void ossimPlanetLandReaderWriter::setLandType(ossimPlanetLandType landType)
{
   theLandType = landType;
   switch(theLandType)
   {
      case ossimPlanetLandType_NORMALIZED_ELLIPSOID:
      {
         theModel = new ossimPlanetNormalizedEllipsoidModel;
         break;
      }
      case ossimPlanetLandType_ELLIPSOID:
      {
         theModel = new ossimPlanetEllipsoidModel;
         break;
      }
#if 0
      case ossimPlanetLandType_FLAT:
      {
         osg::ref_ptr<ossimPlanetFlatLandModel> theFlatLandModel = new ossimPlanetFlatLandModel;
         theModel = theFlatLandModel.get();
         theFlatLandModel->changeNormalModel(new ossimPlanetNormalizedEllipsoidLandModel);
         
         break;
      }
      case ossimPlanetLandType_ORTHOFLAT:
      {
         setElevationEnabledFlag(false);
         osg::ref_ptr<ossimPlanetFlatLandModel> theFlatLandModel = new ossimPlanetFlatLandModel;
         theModel = theFlatLandModel.get();
         theFlatLandModel->changeNormalModel(new ossimPlanetNormalizedEllipsoidLandModel);
         break;
      }
#endif
      default:
      {
         break;
      }
   }
}
#endif

void ossimPlanetLandReaderWriter::setModel(ossimPlanetGeoRefModel* geoModel)
{
   theModel = geoModel;
}

void ossimPlanetLandReaderWriter::setLandNodeCullCallback(ossimPlanetLandCullCallback* callback)
{
   theCullNodeCallback = callback; 
}

bool ossimPlanetLandReaderWriter::getElevationEnabledFlag()const
{
   return theElevationEnabledFlag;
}

void ossimPlanetLandReaderWriter::setElevationEnabledFlag(bool elevationFlag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theElevationEnabledFlag = elevationFlag;
   
}

ossim_float64 ossimPlanetLandReaderWriter::getHeightExag()const
{
   return theHeightExag;
}

void ossimPlanetLandReaderWriter::setHeightExag(ossim_float64 exag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theHeightExag = exag;
}


ossim_uint32 ossimPlanetLandReaderWriter::getElevationPatchSize()const
{
   return theElevationPatchSize;
}

void ossimPlanetLandReaderWriter::setElevationPatchSize(ossim_uint32 patchSize)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theElevationPatchSize = patchSize;
   if(!(theElevationPatchSize&1))
   {
      theElevationPatchSize += 1;
   }
   
}


ossim_uint32 ossimPlanetLandReaderWriter::getMaxLevelDetail()const
{
   return theMaxLevelDetail;
}

void ossimPlanetLandReaderWriter::setMaxLevelDetail(ossim_uint32 maxLevelDetail)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theMaxLevelDetail = maxLevelDetail;
}

ossimFilename ossimPlanetLandReaderWriter::getElevationCacheDir()const
{
   return theElevationCacheDir;
}


void ossimPlanetLandReaderWriter::setElevationCacheDir(const ossimFilename& cacheDir)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theElevationCacheDir = cacheDir;
}

void ossimPlanetLandReaderWriter::setGridUtility(ossimPlanetGridUtility* grid)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   theGrid = grid; 
}

const ossimPlanetGridUtility* ossimPlanetLandReaderWriter::gridUtility()const
{
   return theGrid.get();
}

void ossimPlanetLandReaderWriter::setReferenceLayer(osg::ref_ptr<ossimPlanetTextureLayerGroup> reference)
{
   theReferenceLayer = reference;
}

void ossimPlanetLandReaderWriter::setOverlayLayers(osg::ref_ptr<ossimPlanetTextureLayerGroup> overlays)
{
   theOverlayLayers = overlays;
}

void ossimPlanetLandReaderWriter::setMipMappingFlag(bool flag)
{
   theMipMappingFlag = flag;
}

bool ossimPlanetLandReaderWriter::getMipMappingFlag()const
{
   return theMipMappingFlag;
}

void ossimPlanetLandReaderWriter::setMultiTextureEnableFlag(bool flag)
{
   theMultiTextureEnableFlag = flag;
}

bool ossimPlanetLandReaderWriter::getMultiTextureEnableFlag()const
{
   return theMultiTextureEnableFlag;
}

void ossimPlanetLandReaderWriter::setLandCache(osg::ref_ptr<ossimPlanetLandCache> landCache)
{
   theLandCache = landCache.get();
}

void ossimPlanetLandReaderWriter::setElevationDatabase(osg::ref_ptr<ossimPlanetElevationDatabaseGroup> databaseGroup)
{
   theElevationDatabase = databaseGroup.get();
}

osg::ref_ptr<ossimPlanetElevationDatabaseGroup> ossimPlanetLandReaderWriter::getElevationGroup()
{
   return theElevationDatabase.get();
}

const osg::ref_ptr<ossimPlanetElevationDatabaseGroup> ossimPlanetLandReaderWriter::getElevationGroup()const
{
   return theElevationDatabase.get();
}


void ossimPlanetLandReaderWriter::newTexture(std::vector<osg::ref_ptr<osg::Texture2D> >& textureList,
                                             ossim_uint32 level,
                                             ossim_uint32 row,
                                             ossim_uint32 col,
                                             ossimPlanetImage::ossimPlanetImageStateType& textureState)const
{
   osg::ref_ptr<ossimPlanetLandCacheNode> cacheNode;
   if(theLandCache.valid())
   {
      cacheNode = theLandCache->getNode(theGrid->getId(level, row, col), true);
   }
   textureList.clear();
   osg::ref_ptr<ossimPlanetTextureLayer> currentLayer = theReferenceLayer.get();
   if(!currentLayer.valid()) return;
   ossim_uint32 size = theOverlayLayers.valid()?theOverlayLayers->numberOfLayers():0;
   if(!theMultiTextureEnableFlag) size = 0;
   ossim_uint32 idx = 0;
//    for(idx = 0; idx < size; ++idx)
   do
   {
      osg::ref_ptr<ossimPlanetImage> planetTexture;
      if(cacheNode.valid()&&cacheNode->getTexture(idx))
      {
//          std::cout << "IN CACHE!" << std::endl;
         planetTexture = cacheNode->getTexture(idx);
      }
      else
      {
//          std::cout << "NOT IN CACHE!" << std::endl;
         planetTexture = currentLayer->getTexture(level, row, col, *theGrid);         
      }
      if(!planetTexture.valid())
      {
         if(!theBlankTexture.valid())
         {
           ossim_uint32 w=256, h=256;
           osg::ref_ptr<ossimPlanetImage> tempImage = new ossimPlanetImage;
           osg::Image::AllocationMode allocMode = osg::Image::USE_NEW_DELETE;
           ossim_uint32 sizeInBytes = w*h*4;
           unsigned char *buf = new unsigned char[sizeInBytes];
           memset(buf, 0, sizeInBytes);
           tempImage->setImage(w, h, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, buf, allocMode);
           theBlankTexture = new ossimPlanetTexture2D;
           theBlankTexture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
           theBlankTexture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);
           theBlankTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
           theBlankTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
           theBlankTexture->setDataVariance(osg::Object::STATIC);
           theBlankTexture->setImage(planetTexture.get());
           theBlankTexture->setUnRefImageDataAfterApply(false);
         }
         textureList.push_back(theBlankTexture.get());
      }
      else if(planetTexture.valid())
//       if(planetTexture.valid())
      {
         //planetTexture->scaleImage(planetTexture->s(),planetTexture->t(),planetTexture->r(),GL_UNSIGNED_SHORT_5_6_5);
         osg::ref_ptr<ossimPlanetTexture2D> tempGlTexture= new ossimPlanetTexture2D;
         if(theMipMappingFlag)
         {
            tempGlTexture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR_MIPMAP_LINEAR);
         }
         else
         {
            tempGlTexture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
         }
         tempGlTexture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
         tempGlTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
         tempGlTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
         tempGlTexture->setDataVariance(osg::Object::STATIC);


         tempGlTexture->setImage(planetTexture.get());
         tempGlTexture->setUnRefImageDataAfterApply(true);
#if defined(OSSIMPLANET_USE_DXT1)
         tempGlTexture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT1_COMPRESSION);
#elif defined(OSSIMPLANET_USE_DXT3)
         tempGlTexture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT3_COMPRESSION);
#elif defined(OSSIMPLANET_USE_DXT5)
         tempGlTexture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT5_COMPRESSION);
#endif
         textureList.push_back(tempGlTexture.get());
      }
      if(idx < size)
      {
         currentLayer = theOverlayLayers->layer(idx);
         ++idx;
      }
      else
      {
         currentLayer = 0;
      }
   }while(currentLayer.valid());
}
   
osg::ref_ptr<osg::MatrixTransform> ossimPlanetLandReaderWriter::newGeometry(ossim_uint32 level,
                                                                            ossim_uint32 row,
                                                                            ossim_uint32 col,
                                                                            std::vector<osg::ref_ptr<osg::Texture2D> >& textures,
                                                                            osg::ref_ptr<ossimPlanetImage> elevationGrid,
                                                                            ossimPlanetBoundingBox& box,
                                                                            bool& useClusterCullingCallback,
                                                                            osg::Vec3d& clusterControlPoint,
                                                                            osg::Vec3d& clusterCenterNormal,
                                                                            double& minDotProduct,
                                                                            double& maxClusterCullingRadius)const
{
//    double minLat;
//    double minLon;
//    double maxLat;
//    double maxLon;
//    theGrid->getLatLonBounds(level,
//                             row, // row 
//                             col, // col
//                             minLat,
//                             minLon,
//                             maxLat,
//                             maxLon);
   osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
   osg::ref_ptr<osg::Vec3Array> norms = new osg::Vec3Array;
   osg::ref_ptr<osg::Vec2Array> tcoords = new osg::Vec2Array;
//    double minLat, minLon;
//    double maxLat, maxLon;
//   unsigned int tempIdx = 0;
//    double x, y, z;
   
   osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
   
   osg::ref_ptr<osg::MatrixTransform> matrixTransform = new osg::MatrixTransform;
   osg::Matrixd localToWorld;
   osg::Matrixd worldToLocal;

   

//    matrixTransform->setMatrix(localToWorld);
//    worldToLocal = matrixTransform->getInverseMatrix();

   createPoints(level,
                row,
                col,
                elevationGrid,
                verts.get(),
                norms.get(),
                tcoords.get(),
                localToWorld,
                box,
                useClusterCullingCallback,
                clusterControlPoint,
                clusterCenterNormal,
                minDotProduct,
                maxClusterCullingRadius);
   
   matrixTransform->setMatrix(localToWorld);
   worldToLocal = matrixTransform->getInverseMatrix();
   
   osg::ref_ptr<osg::UIntArray> vindices = new osg::UIntArray;
   ossim_uint32 numColumns = (theElevationPatchSize);
   ossim_uint32 numRows = numColumns;

   ossim_uint32 c = 0, ei=0;
#if 1
   osg::DrawElementsUShort& drawElements = *(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES));
   osg::Geometry::PrimitiveSetList primSetList;
   ossim_uint32 i = 0;
   ossim_uint32 j = 0;
   for(i = 1; i < numRows;i+=2)
   {
      for(j = 1; j < numColumns;j+=2)
      {
         ossim_uint32 i00 = (i)*numColumns + j;
         drawElements.push_back(i00);
         drawElements.push_back(i00 - (numColumns+1));
         drawElements.push_back(i00 - (numColumns));
         
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 - (numColumns));
         drawElements.push_back(i00 - (numColumns-1));
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 - (numColumns-1));
         drawElements.push_back(i00 + 1);
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 + 1);
         drawElements.push_back(i00 + (numColumns+1));
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 + (numColumns+1));
         drawElements.push_back(i00 + (numColumns));
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 + (numColumns));
         drawElements.push_back(i00 + (numColumns-1));
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 + (numColumns-1));
         drawElements.push_back(i00 - 1);
         
         drawElements.push_back(i00);
         drawElements.push_back(i00 - 1);
         drawElements.push_back(i00 - (numColumns+1));
      }
   }
   primSetList.push_back(&drawElements);
	geom->setPrimitiveSetList(primSetList);
   //smoothGeometry();
#endif
#if 0

   osg::Geometry::PrimitiveSetList primSetList;// ( numRows>>1*numCols>>1 );
   ossim_uint32 i = 0;
   ossim_uint32 j = 0;
   for(i = 1; i < numRows;i+=2)
   {
      for(j = 1; j < numColumns;j+=2)
      {
         ossim_uint32 i00 = (i)*numColumns + j;
         osg::DrawElementsUShort& drawElements = *(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_FAN));
#if 0
         drawElements.push_back(i00);

         drawElements.push_back(i00 - (numColumns+1));
         drawElements.push_back(i00 - (numColumns-1));

         drawElements.push_back(i00 + (numColumns+1));
         drawElements.push_back(i00 + (numColumns-1));

         drawElements.push_back(i00 - (numColumns+1));
#endif
#if 1
         drawElements.push_back(i00);
         drawElements.push_back(i00 - (numColumns+1));
         drawElements.push_back(i00 - (numColumns));
         drawElements.push_back(i00 - (numColumns-1));
         
         drawElements.push_back(i00 + 1);
         
         drawElements.push_back(i00 + (numColumns+1));
         drawElements.push_back(i00 + (numColumns));
         drawElements.push_back(i00 + (numColumns-1));
         
         drawElements.push_back(i00 - 1);
         
         drawElements.push_back(i00 - (numColumns+1));
#endif    
         primSetList.push_back(&drawElements);
      }
   }

	geom->setPrimitiveSetList(primSetList);
#endif
#if 0
	osg::Geometry::PrimitiveSetList primSetList;// ( numColumns - 1 );
	//unsigned int length ( 2 * numRows );
	ossim_uint32 elementIdx = 0;
   unsigned int i,j;
	for (i = 0; i < numRows-1; ++i )
	{
		elementIdx = 0;
		osg::DrawElementsUShort& drawElements = *(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP));
		// Loop through all the columns.
		for (j = 0; j < numColumns; ++j )
		{
         unsigned short i10 = (i+1)*(numColumns)+j;
         unsigned short i00 = (i)*(numColumns)+j;
			drawElements.push_back(i10);
         drawElements.push_back(i00);
		}
		
		// Define the primitive.
		primSetList.push_back(&drawElements);
	}
	geom->setPrimitiveSetList(primSetList);
#endif
   geom->setVertexArray(verts.get());
   geom->setNormalArray(norms.get());
   geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
   osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
   color->push_back(osg::Vec4(1.0,
                              1.0,
                              1.0,
                              1.0));
   geom->setColorBinding(osg::Geometry::BIND_OVERALL);
   geom->setColorArray(color.get());
   
   osgUtil::SmoothingVisitor sv;
   sv.smooth(*geom);  // this will replace the normal vector with a new one
   
   // now we have to reassign the normals back to the orignal pointer.
   norms = dynamic_cast<osg::Vec3Array*>(geom->getNormalArray());
   
   if(theLandType != ossimPlanetLandType_ORTHOFLAT)
   {
      unsigned int numPointsInBody = numColumns*numRows;
      unsigned int numVerticesInSkirt = numColumns*2 + numRows*2;
      
      // top strip
      osg::DrawElementsUShort& drawElementsQuad = *(new osg::DrawElementsUShort(GL_QUAD_STRIP,
                                                                                2*numVerticesInSkirt));
//                                                                          2*numVerticesInSkirt+2));
      geom->addPrimitiveSet(&drawElementsQuad);
      ei=0;
//      int firstSkirtVertexIndex = numPointsInBody;
      osgUtil::Simplifier::IndexList pointsToProtectDuringSimplification;
      
//    bool topPoleFlag    = ((90.0 - std::abs(maxLat) ) <= DBL_EPSILON)&&(level > 4);   
//    bool bottomPoleFlag = ((90.0 - std::abs(minLat) ) <= DBL_EPSILON)&&(level > 4);   
      // do top skirt
      //
      //      for(c=0;c<numColumns-1;++c)
      for(c=0;c<numColumns-1;++c)
      {
         unsigned short i0 = c;
         unsigned short i1 = numPointsInBody + c;
         drawElementsQuad[ei++] = i0;
         drawElementsQuad[ei++] = i1;
//       if(!topPoleFlag)
         {
            pointsToProtectDuringSimplification.push_back(i0);
            pointsToProtectDuringSimplification.push_back(i1);
         }
      }
      // do right skirt
      ossim_uint32 startInteriorIdx = numColumns-1;
      ossim_uint32 startSkirtIdx    = numPointsInBody + numColumns;
      for(c=0;c<numRows;++c)
      {
         unsigned short i0 = startInteriorIdx + numColumns*c;
         unsigned short i1 = startSkirtIdx + c;
         {
            drawElementsQuad[ei++] = i0;
            drawElementsQuad[ei++] = i1;
         }
         pointsToProtectDuringSimplification.push_back(i0);
         pointsToProtectDuringSimplification.push_back(i1);      
      }
      
      startInteriorIdx = (numRows-1)*numColumns + (numColumns-1);
      startSkirtIdx =  numPointsInBody + numColumns + numRows;
      // do bottom skirt
      //
      for(c=0;c<numColumns;++c)
      {
         unsigned short i0 = startInteriorIdx - c;
         unsigned short i1 = startSkirtIdx + c;
         drawElementsQuad[ei++] = i0;
         drawElementsQuad[ei++] = i1;
//       if(!bottomPoleFlag)
         {
            pointsToProtectDuringSimplification.push_back(i0);
            pointsToProtectDuringSimplification.push_back(i1);
         }
      }
      
      // do left skirt
      startInteriorIdx = numRows*(numColumns-1);
      startSkirtIdx    = numPointsInBody + 2*numColumns + numRows;
   
      for(c=0;c<numRows;++c)
      {
         unsigned short i0 = startInteriorIdx - numColumns*c;
         unsigned short i1 = startSkirtIdx + c;
         drawElementsQuad[ei++] = i0;
         drawElementsQuad[ei++] = i1;
         pointsToProtectDuringSimplification.push_back(i0);
         pointsToProtectDuringSimplification.push_back(i1);
      }
   }
   
//    unsigned int numVerticesInBody = numColumns*numRows;
//     unsigned int numVerticesInSkirt = createSkirt ? numColumns*2 + numRows*2 - 4 : 0;
//     unsigned int numVertices = numVerticesInBody+numVerticesInSkirt;
//    unsigned int numVertices = numVerticesInBody;
#if 0
    unsigned int numVerticesInBody = numColumns*numRows;
//     unsigned int numVerticesInSkirt = createSkirt ? numColumns*2 + numRows*2 - 4 : 0;
//     unsigned int numVertices = numVerticesInBody+numVerticesInSkirt;
    unsigned int numVertices = numVerticesInBody;


   if (norms.valid() && norms->size()!=numVertices) norms->resize(numVertices);
#endif
   osg::ref_ptr<osg::StateSet> dstate = geom->getOrCreateStateSet();
   osg::ref_ptr<osg::CullFace> cullFace = new osg::CullFace;


   
   dstate->setAttributeAndModes(cullFace.get(),
                                osg::StateAttribute::ON);

   if(textures.size())
   {
      ossim_uint32 idx = 0;
      for(idx = 0; idx < textures.size(); ++idx)
      {
         if(textures[idx].valid())
         {
            dstate->setTextureAttributeAndModes(idx, textures[idx].get(),
                                                osg::StateAttribute::ON);
         }
      }
   }
//    else if(textures.size())
//    {
//       dstate->setTextureAttributeAndModes(idx, textures[0].get(),
//                                           osg::StateAttribute::ON);
      
//    }
//   geom->setVertexArray(verts.get());
//   geom->setNormalArray(norms.get());
//   geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
   
   
   geom->setTexCoordArray( 0, tcoords.get());   
   geom->setSupportsDisplayList(false);
   
#if 0 // simplifier
//    unsigned int targetMaxNumVertices = numVertices-1;
//    float sample_ratio = (numVertices <= targetMaxNumVertices) ? 1.0f : (float)targetMaxNumVertices/(float)numVertices;
//    if(topPoleFlag||bottomPoleFlag)
   {
      float sample_ratio = .5;
      osgUtil::Simplifier simplifier(sample_ratio,
                                     (geom->getBound().radius());///theModel->getNormalizationScale()));
      
      simplifier.setDoTriStrip(false);
      simplifier.setSmoothing(false);
      
//    simplifier.simplify(*geom, pointsToProtectDuringSimplification);  // this will replace the normal vector with a new one
      simplifier.simplify(*geom,pointsToProtectDuringSimplification);  // this will replace the normal vector with a new one
   }
#endif
//   osgUtil::TriStripVisitor tsv;
//   tsv.setMinStripSize(3);
//   tsv.stripify(*geom);

   osg::ref_ptr<osg::Geode> geode = new osg::Geode;
//    osgUtil::TriStripVisitor v;

//    v.apply(*geode.get());
//    osgUtil::SmoothingVisitor::smooth(*(geom.get()));
   matrixTransform->addChild(geode.get());
   geode->addDrawable(geom.get());
   geom->setUseVertexBufferObjects(true);
   //geom->setFastPathHint(true);
  // geom->setUseDisplayList(false);
   matrixTransform->dirtyBound();
   geom->dirtyBound();
   matrixTransform->getBound();
   geom->getBound();
   
   return matrixTransform;
}
void ossimPlanetLandReaderWriter::createPoints(ossim_uint32 level,
                                               ossim_uint32 row,
                                               ossim_uint32 col,
                                               osg::ref_ptr<ossimPlanetImage> elevationGrid,
                                               osg::Vec3Array *verts,
                                               osg::Vec3Array *norms,
                                               osg::Vec2Array *tcoords,
                                               osg::Matrixd& localToWorld,
                                               ossimPlanetBoundingBox& box,
                                               bool& useClusterCullingCallback,
                                               osg::Vec3d& clusterControlPoint,
                                               osg::Vec3d& clusterCenterNormal,
                                               double& minDotProduct,
                                               double& maxClusterCullingRadius)const
{
   std::vector<ossimPlanetGridUtility::GridPoint> points;
   ossim_uint32 rows = theElevationPatchSize;
   ossim_uint32 cols = rows;

   theGrid->createGridPoints(points,
                             level,
                             row,
                             col,
                             rows,
                             cols);
   
   osg::Vec3d centerPoint;
   osg::Vec3d centerNormal;
   ossim_float32* grid = 0;
   ossim_uint32 idx = 0;
   osg::Vec3d ul;
   osg::Vec3d lr;
   osg::Vec3d ur;
   osg::Vec3d ll;
   osg::Vec3d upAxis;
   osg::Vec3d rightAxis;
   osg::Vec3d norm1, norm2;
   osg::Matrixd inverseLocalToWorld;
   ossim_uint32 numberOfPoints = points.size();
   if(elevationGrid.valid())
   {
      grid = (ossim_float32*)elevationGrid->data();
   }
   
//    if(grid)
//    {
//       for(idx = 0; idx < numberOfPoints; ++idx)
//       {
//          (*grid) *= theHeightExag;
//          ++grid;
//       }
//    }
   if(elevationGrid.valid())
   {
      grid = (ossim_float32*)elevationGrid->data();
   }
   
   ossimPlanetGridUtility::GridPoint centerGridPoint;

   theGrid->getCenterGridPoint(centerGridPoint, level, row, col);
   
   theGrid->getLatLon(centerPoint, centerGridPoint);
   theGrid->getLatLon(ul, points[0]);
   theGrid->getLatLon(ur, points[cols-1]);
   theGrid->getLatLon(lr, points[points.size()-1]);
   theGrid->getLatLon(ll, points[(rows-1)*cols]);
#if 1
   osg::Matrixd lsrMatrix;
   theModel->lsrMatrix(centerPoint,
                       lsrMatrix);
   localToWorld  = lsrMatrix;
   centerNormal = osg::Vec3d(lsrMatrix(2, 0),
                             lsrMatrix(2, 1),
                             lsrMatrix(2, 2));
   rightAxis = osg::Vec3d(lsrMatrix(0, 0),
                          lsrMatrix(0, 1),
                          lsrMatrix(0, 2));
   upAxis    = osg::Vec3d(lsrMatrix(1, 0),
                          lsrMatrix(1, 1),
                          lsrMatrix(1, 2));
#endif
//    theModel->lsrMatrix(centerPoint, localToWorld);
   theModel->forward(osg::Vec3d(centerPoint),
                     centerPoint);
   theModel->forward(osg::Vec3d(ul),
                     ul);
   theModel->forward(osg::Vec3d(ur),
                     ur);
   theModel->forward(osg::Vec3d(lr),
                     lr);
   theModel->forward(osg::Vec3d(ll),
                     ll);
   
//    theModel->computeLocalToWorldTransform(centerPoint, localToWorld);
   inverseLocalToWorld.invert(localToWorld);


   osg::Vec2d normalDistance(0.0,0.0);
   osg::Vec2d rightDistance(0.0,0.0);
   osg::Vec2d upDistance(0.0,0.0);
   
   double h = 0.0;
   osg::Vec3d groundPlaneNormal  = centerNormal;
   osg::Vec3d groundPlaneCenter  = centerPoint;
   osg::Vec3d p1;
   double minDelta = 0.0, maxDelta = 0.0;
   useClusterCullingCallback = level > 0;
   minDotProduct = 1.0;
   double maxClusterCullingHeight = 0.0;      
   maxClusterCullingRadius = 0.0;
   clusterCenterNormal = centerNormal;

   
   osg::Vec3d latLonPoint;
   osg::Vec3d deltaP;
   double upDelta = 0.0;
   double rightDelta = 0.0;
   double normDelta  = 0.0;
   
   for(idx = 0; idx < numberOfPoints; ++idx)
   {
      tcoords->push_back(osg::Vec2d(points[idx].theLocalGridPoint[ossimPlanetGridUtility::GRIDX],
                                    points[idx].theLocalGridPoint[ossimPlanetGridUtility::GRIDY]));
      
      if(grid)
      {
         h = (*grid);//*theHeightExag;
         
         ++grid;
      }
      else
      {
         h = 0.0;
      }

      theGrid->getLatLon(latLonPoint, points[idx]);
      if(h > maxDelta) maxDelta = h;
      if(h < minDelta) minDelta = h;
      latLonPoint[ossimPlanetGridUtility::HGT] = h;
      h /= theModel->getNormalizationScale();
      theModel->forward(latLonPoint, p1);

      deltaP     = (p1-groundPlaneCenter);
      upDelta    = deltaP*upAxis;
      rightDelta = deltaP*rightAxis;
      normDelta  = deltaP*groundPlaneNormal;
      
      if(normDelta < normalDistance[1]) normalDistance[1] = normDelta;
      if(normDelta > normalDistance[0]) normalDistance[0] = normDelta;
      if(upDelta < upDistance[1]) upDistance[1] = upDelta; 
      if(upDelta > upDistance[0]) upDistance[0] = upDelta;
      if(rightDelta < rightDistance[1]) rightDistance[1] = rightDelta; 
      if(rightDelta > rightDistance[0]) rightDistance[0] = rightDelta;
      theModel->normal(p1,
                       norm1);
      
      double product = deltaP*groundPlaneNormal;
      if(product > maxDelta) maxDelta = product;
      if(product < minDelta) minDelta = product;
      
      if (useClusterCullingCallback)
      {
         osg::Vec3 dv = p1 - centerPoint;
         double d = sqrt(dv.x()*dv.x() + dv.y()*dv.y() + dv.z()*dv.z());
         double globeRadius = 1.0;
         double theta = acos( globeRadius/ (globeRadius + fabs(h) ));
         double phi = 2.0 * asin (d*0.5/globeRadius); // d/globeRadius;
         double beta = theta+phi;
         double cutoff = osg::PI_2;
         //my_notify(osg::INFO)<<"theta="<<theta<<"\tphi="<<phi<<" beta "<<beta<<std::endl;
         if (phi<cutoff && beta<cutoff)
         {
            double localDotProduct = -sin(theta + phi);
            double localM = globeRadius*( 1.0/ cos(theta+phi) - 1.0);
            double localRadius = static_cast<float>(globeRadius * tan(beta)); // beta*globeRadius;
            minDotProduct = osg::minimum(minDotProduct, localDotProduct);
            maxClusterCullingHeight = osg::maximum(maxClusterCullingHeight,localM);      
            maxClusterCullingRadius = osg::maximum(maxClusterCullingRadius,localRadius);
         }
         else
         {
            //my_notify(osg::INFO)<<"Turning off cluster culling for wrap around tile."<<std::endl;
            useClusterCullingCallback = false;
         }
      }
      
      p1 = p1*inverseLocalToWorld;
      norm1 = osg::Matrixd::transform3x3(localToWorld, norm1);
      norm1.normalize();
      verts->push_back(p1);
      norms->push_back(norm1);
   }
   clusterControlPoint = centerPoint + centerNormal*maxClusterCullingHeight;

   clusterControlPoint = clusterControlPoint*inverseLocalToWorld;
   clusterCenterNormal = osg::Matrixd::transform3x3(localToWorld,clusterCenterNormal);
   box.extrude(groundPlaneCenter,
               upAxis,
               rightAxis,
               groundPlaneNormal,
               upDistance,
               rightDistance,
               normalDistance);
   box.transform(inverseLocalToWorld);

//#if 0
// ------------------- Now let's do the skirt -------------------------

   ossim_uint32 colIdx = 0;
   ossim_uint32 rowIdx = 0;
   if(theLandType != ossimPlanetLandType_ORTHOFLAT)
   {
      // top skirt points
      double scale = .02;
      double defaultSkirtLength = box.radius()*scale;
      grid = 0;
      if(elevationGrid.valid())
      {
         grid = (ossim_float32*)elevationGrid->data();
      }
      double minHeight = 99999999999.0;
      double maxDeltaHeight = -999999999999.0, minDeltaHeight = 9999999999999.0;
      double deltaHeight=defaultSkirtLength;
      if(grid)
      {
         for(colIdx = 0; colIdx < cols; ++colIdx)
         {
            if(maxDeltaHeight < *grid) maxDeltaHeight = *grid;
            if(minDeltaHeight > *grid) minDeltaHeight = *grid;
//             double value = (*grid -defaultSkirtLength)*theHeightExag;
//             if(value < minHeight) minHeight = value;
            ++grid;
         }
         grid = (ossim_float32*)elevationGrid->data();
         deltaHeight = ossim::max(defaultSkirtLength, ((maxDeltaHeight-minDeltaHeight)));//*theHeightExag));
      }
//       std::cout << "deltaHeight = " << deltaHeight <<  "\n";
//       if(elevationGrid.valid())
//       {
//          grid = (ossim_float32*)elevationGrid->data();
//       }
      for(colIdx = 0; colIdx < cols; ++colIdx)
      {
         tcoords->push_back(osg::Vec2d(points[colIdx].theLocalGridPoint[ossimPlanetGridUtility::GRIDX],
                                       points[colIdx].theLocalGridPoint[ossimPlanetGridUtility::GRIDY]));
      
         theGrid->getLatLon(latLonPoint, points[colIdx]);
         if(grid)
         {
//            latLonPoint[2] = minHeight;
            latLonPoint[2] = *grid - deltaHeight;
            theModel->forward(latLonPoint,
                              p1);
            ++grid;
         }
         else
         {
            latLonPoint[2] = -defaultSkirtLength;
            theModel->forward(latLonPoint,
                              p1);
         }
         theModel->normal(p1,
                          norm1);
         p1 = p1*inverseLocalToWorld;
         
         norm1 = osg::Matrixd::transform3x3(localToWorld, norm1);
         norm1.normalize();
         verts->push_back(p1);
         norms->push_back(norm1);
      }
      // right skirt
      maxDeltaHeight = -999999999999.0, minDeltaHeight = 9999999999999.0;
      if(elevationGrid.valid())
      {
         grid = (((ossim_float32*)elevationGrid->data())+(cols-1));
      }
      minHeight = 99999999999.0;
      ossimPlanetGridUtility::GridPoint* pointsPtr = &points.front() + cols;
      if(grid)
      {
         for(rowIdx = 0; rowIdx < rows; ++rowIdx)
         {
//             double value = (*grid -defaultSkirtLength)*theHeightExag;
            if(maxDeltaHeight < *grid) maxDeltaHeight = *grid;
            if(minDeltaHeight > *grid) minDeltaHeight = *grid;
//             if(value < minHeight) minHeight = value;
            grid+=(cols);
         }
         grid = (((ossim_float32*)elevationGrid->data())+(cols-1));
         deltaHeight = ossim::max(defaultSkirtLength, ((maxDeltaHeight-minDeltaHeight)));//*theHeightExag));
      }

      pointsPtr = &points.front() + cols-1;
      for(rowIdx = 0; rowIdx < rows; ++rowIdx)
      {
         tcoords->push_back(osg::Vec2d(pointsPtr->theLocalGridPoint[ossimPlanetGridUtility::GRIDX],
                                       pointsPtr->theLocalGridPoint[ossimPlanetGridUtility::GRIDY]));
         theGrid->getLatLon(latLonPoint, *pointsPtr);
         if(grid)
         {
//             latLonPoint[2] = minHeight;
            latLonPoint[2] = *grid - deltaHeight;
//            latLonPoint[2] = (*grid -defaultSkirtLength)*theHeightExag;
            theModel->forward(latLonPoint,
                              p1);
            grid+=(cols);
         }
         else
         {
            theModel->forward(osg::Vec3d(latLonPoint[ossimPlanetGridUtility::LAT],
                                         latLonPoint[ossimPlanetGridUtility::LON], -defaultSkirtLength),
                              p1);
         }
         
         theModel->normal(p1,
                          norm1);
         p1 = p1*inverseLocalToWorld;
         
         norm1 = osg::Matrixd::transform3x3(localToWorld, norm1);
         norm1.normalize();
         verts->push_back(p1);
         norms->push_back(norm1);
         pointsPtr += (cols);
      }
      
      pointsPtr = &points.front() + (rows)*(cols) - 1;

      // bottom skirt points
      maxDeltaHeight = -999999999999.0, minDeltaHeight = 9999999999999.0;
      if(elevationGrid.valid())
      {
         grid = (((ossim_float32*)elevationGrid->data())+(((rows)*(cols))-1));
      }
      minHeight = 99999999999.0;
      if(grid)
      {
         for(colIdx = 0; colIdx < cols; ++colIdx)
         {
//             double value = (*grid -defaultSkirtLength)*theHeightExag;
            if(maxDeltaHeight < *grid) maxDeltaHeight = *grid;
            if(minDeltaHeight > *grid) minDeltaHeight = *grid;
//             if(value < minHeight) minHeight = value;
            --grid;
         }
         deltaHeight = ossim::max(defaultSkirtLength, ((maxDeltaHeight-minDeltaHeight)));//*theHeightExag));
         grid = (((ossim_float32*)elevationGrid->data())+(((rows)*(cols))-1));
      }
      for(colIdx = 0; colIdx < cols; ++colIdx)
      {
         tcoords->push_back(osg::Vec2d(pointsPtr->theLocalGridPoint[ossimPlanetGridUtility::GRIDX],
                                       pointsPtr->theLocalGridPoint[ossimPlanetGridUtility::GRIDY]));
         theGrid->getLatLon(latLonPoint, *pointsPtr);
         if(grid)
         {
//             latLonPoint[2] = minHeight;
            latLonPoint[2] = *grid - deltaHeight;
            theModel->forward(latLonPoint,
                              p1);
            --grid;
         }
         else
         {
            theModel->forward(osg::Vec3d(latLonPoint[ossimPlanetGridUtility::LAT],
                                         latLonPoint[ossimPlanetGridUtility::LON],
                                         -defaultSkirtLength),
                              p1);
         }
         
         theModel->normal(p1,
                          norm1);
         p1 = p1*inverseLocalToWorld;
         
         norm1 = osg::Matrixd::transform3x3(localToWorld, norm1);
         norm1.normalize();
         verts->push_back(p1);
         norms->push_back(norm1);
         --pointsPtr;
      }
      

      
      // left skirt
      maxDeltaHeight = -999999999999.0, minDeltaHeight = 9999999999999.0;
      grid = 0;
//       pointsPtr = (&points.front() + ((rows)*(cols+1)));
      pointsPtr = (&points.front() + ((rows-1)*(cols)));
      if(elevationGrid.valid())
      {
         grid = (((ossim_float32*)elevationGrid->data())+((rows-1)*(cols)));
      }
      minHeight = 99999999999.0;
      if(grid)
      {
         for(rowIdx = 0; rowIdx < rows; ++rowIdx)
         {
            if(maxDeltaHeight < *grid) maxDeltaHeight = *grid;
            if(minDeltaHeight > *grid) minDeltaHeight = *grid;
//             double value = (*grid -defaultSkirtLength)*theHeightExag;
//             if(value < minHeight) minHeight = value;
            grid-=(cols);
         }
         deltaHeight = ossim::max(defaultSkirtLength, ((maxDeltaHeight-minDeltaHeight)));//*theHeightExag));
         grid = (((ossim_float32*)elevationGrid->data())+((rows-1)*(cols)));
      }
      for(rowIdx = 0; rowIdx < rows; ++rowIdx)
      {
         tcoords->push_back(osg::Vec2d(pointsPtr->theLocalGridPoint[ossimPlanetGridUtility::GRIDX],
                                       pointsPtr->theLocalGridPoint[ossimPlanetGridUtility::GRIDY]));
         theGrid->getLatLon(latLonPoint, *pointsPtr);
         if(grid)
         {
            latLonPoint[2] = *grid - deltaHeight;
            theModel->forward(latLonPoint,
                              p1);
//             grid-=(cols+1);
         }
         else
         {
            theModel->forward(osg::Vec3d(latLonPoint[0], latLonPoint[1], -defaultSkirtLength),
                              p1);
         }
         
         theModel->normal(p1,
                          norm1);
         p1 = p1*inverseLocalToWorld;
         
         norm1 = osg::Matrixd::transform3x3(localToWorld, norm1);
         norm1.normalize();
         verts->push_back(p1);
         norms->push_back(norm1);
         pointsPtr-=(cols);
      }
   }
//#endif
}

void ossimPlanetLandReaderWriter::initSupportAttributes(ossim_uint32 level,
                                                      ossim_uint32 row,
                                                      ossim_uint32 col,
                                                      osg::Vec3d& centerPoint,
                                                      osg::Vec3d& ulPoint,
                                                      osg::Vec3d& urPoint,
                                                      osg::Vec3d& lrPoint,
                                                      osg::Vec3d& llPoint,
                                                      osg::Vec3d& centerNormal,
                                                      osg::Vec3d& ulNormal,
                                                      osg::Vec3d& urNormal,
                                                      osg::Vec3d& lrNormal,
                                                      osg::Vec3d& llNormal,
                                                      ossimPlanetBoundingBox& box,
                                                      std::vector<ossimPlanetBoundingBox>& childrenBounds,
                                                      const osg::MatrixTransform& transform)const
{
   ossimPlanetGridUtility::GridPoint point;
   osg::Vec3d ul, ur, lr, ll, latLon;
   theGrid->getLatLonCorners(ul, ur, lr, ll, level, row, col);
   
//    double minLat, minLon, maxLat, maxLon;
//    const osg::Matrixd& worldToLocal = transform.getInverseMatrix();
//    theGrid->getLatLonBounds(level,
//                             row,
//                             col,
//                             minLat,
//                             minLon,
//                             maxLat,
//                             maxLon);
   theModel->forward(ul,
                     ulPoint);

//    lod->theUlPoint = lod->theUlPoint*worldToLocal;
   
   theModel->forward(ur,
                     urPoint);

//    lod->theUrPoint = lod->theUrPoint*worldToLocal;

   theModel->forward(lr,
                     lrPoint);
//    lod->theLrPoint = lod->theLrPoint*worldToLocal;

   theModel->forward(ll,
                     llPoint);
//    lod->theLlPoint = lod->theLlPoint*worldToLocal;

   theGrid->getCenterGridPoint(point,
                               level,
                               row,
                               col);
   theGrid->getLatLon(latLon,
                      point);
   theModel->forward(latLon,
                     centerPoint);
//    lod->theCenterPoint = lod->theCenterPoint*worldToLocal;

   theModel->normal(ulPoint,
                    ulNormal);
//    lod->theUlNormal = osg::Matrixd::transform3x3(worldToLocal, lod->theUlNormal);

   theModel->normal(urPoint,
                    urNormal);
//    lod->theUrNormal = osg::Matrixd::transform3x3(worldToLocal, lod->theUrNormal);
   theModel->normal(lrPoint,
                    lrNormal);
//    lod->theLrNormal = osg::Matrixd::transform3x3(worldToLocal, lod->theLrNormal);
   theModel->normal(llPoint,
                    llNormal);
//    lod->theLlNormal = osg::Matrixd::transform3x3(worldToLocal, lod->theLlNormal);
   theModel->normal(centerPoint,
                    centerNormal);
//    lod->theCenterNormal = osg::Matrixd::transform3x3(worldToLocal, lod->theCenterNormal);

   osg::Vec3d center = (ulPoint +
                        urPoint +
                        lrPoint +
                        llPoint)*.25;
   osg::Vec3d delta  = (center-centerPoint);
   double normalizedMax = (6000.0*theHeightExag);///theModel->getNormalizationScale();
   double distance  = delta.length() + normalizedMax;
   double distance2 = -distance;

   switch(theLandType)
   {
      case ossimPlanetLandType_ORTHOFLAT:
      case ossimPlanetLandType_FLAT:
      {
         distance = 0;
         distance2 = 0;
         break;
      }
      default:
      {
         break;
      }
   }
   box.extrude(ulPoint,
               urPoint,
               lrPoint,
               llPoint,
               centerNormal,
               osg::Vec2d(distance, distance2));
   childrenBounds.resize(4);
   ossim_uint32 nextRow = row<<1;
   ossim_uint32 nextCol = col<<1;
   createBounds(level+1,
                nextRow,
                nextCol,
                childrenBounds[0],
                transform);
   createBounds(level+1,
                nextRow,
                nextCol+1,
                childrenBounds[1],
                transform);
   createBounds(level+1,
                nextRow+1,
                nextCol+1,
                childrenBounds[2],
                transform);
   createBounds(level+1,
                nextRow+1,
                nextCol,
                childrenBounds[3],
                transform);
}

void ossimPlanetLandReaderWriter::createBounds(ossim_uint32 level,
                                             ossim_uint32 row,
                                             ossim_uint32 col,
                                             ossimPlanetBoundingBox& bounds,
                                             const osg::MatrixTransform& /* transform */ )const
{
   osg::Vec3d ulPoint, urPoint, lrPoint, llPoint, centerPoint;
   osg::Vec3d centerNormal;
//   double minLat, minLon, maxLat, maxLon;
   osg::Vec3d ul, ur, lr, ll, latLon;
   ossimPlanetGridUtility::GridPoint point;
   theGrid->getLatLonCorners(ul, ur, lr, ll, level, row, col);
//    const osg::Matrixd& worldToLocal = transform.getInverseMatrix();
//    theGrid->getLatLonBounds(level,
//                               row,
//                               col,
//                               minLat,
//                               minLon,
//                               maxLat,
//                               maxLon);
   theModel->forward(ul,
                     ulPoint);

//    lod->theUlPoint = lod->theUlPoint*worldToLocal;
   
   theModel->forward(ur,
                     urPoint);

//    lod->theUrPoint = lod->theUrPoint*worldToLocal;

   theModel->forward(lr,
                     lrPoint);
//    lod->theLrPoint = lod->theLrPoint*worldToLocal;

   theModel->forward(ll,
                     llPoint);
//    lod->theLlPoint = lod->theLlPoint*worldToLocal;

   theGrid->getCenterGridPoint(point,
                               level,
                               row,
                               col);
   theGrid->getLatLon(latLon,
                      point);
   
   theModel->forward(latLon,
                     centerPoint);
//    lod->theCenterPoint = lod->theCenterPoint*worldToLocal;

   theModel->normal(centerPoint,
                    centerNormal);
//    lod->theCenterNormal = osg::Matrixd::transform3x3(worldToLocal, lod->theCenterNormal);

   osg::Vec3d center = (ulPoint +
                        urPoint +
                        lrPoint +
                        llPoint)*.25;
   osg::Vec3d delta  = (center-centerPoint);
   double normalizedMax = (6000.0*theHeightExag);///theModel->getNormalizationScale();
   double distance = delta.length() + normalizedMax;
   double distance2 = distance;

   switch(theLandType)
   {
      case ossimPlanetLandType_ORTHOFLAT:
      case ossimPlanetLandType_FLAT:
      {
         distance = 0;
         distance2 = 0;
         break;
      }
      default:
      {
         break;
      }
   }
   bounds.extrude(ulPoint,
                  urPoint,
                  lrPoint,
                  llPoint,
                  centerNormal,
                  osg::Vec2d(distance, distance2));
}

osg::ref_ptr<ossimPlanetImage> ossimPlanetLandReaderWriter::newElevation(ossim_uint32 level,
                                                                         ossim_uint32 row,
                                                                         ossim_uint32 col)const
{
   osg::ref_ptr<ossimPlanetImage> grid;
   osg::ref_ptr<ossimPlanetLandCacheNode> cacheNode;

   if(theLandCache.valid())
   {
      cacheNode = theLandCache->getNode(theGrid->getId(level, row, col), true);
   }

   if(!theElevationEnabledFlag)
   {
      return grid;
   }

   if(cacheNode.valid())
   {
      grid = cacheNode->getElevation();
   }
   if(grid.valid())
   {
      return grid;
   }
   
   ossim_uint32 rows = theElevationPatchSize;
   ossim_uint32 cols = rows;
   theElevationGrid->setTileWidthHeight(theElevationPatchSize, theElevationPatchSize);

   std::vector<ossimPlanetGridUtility::GridPoint> points;
   theElevationGrid->createGridPoints(points,
                                      level,
                                      row,
                                      col,
                                      rows,
                                      cols);
   grid = theElevationDatabase->getTexture(level, row, col, *theElevationGrid);
   if(grid.valid())
   {
      ossim_float32* gridPtr = (ossim_float32*)grid->data();

      ossim_uint32 idx = 0;
      ossim_uint32 area = grid->s()*grid->t();
      osg::Vec3d latLonPoint;
      for(;idx < area; ++idx)
      {
         theElevationGrid->getLatLon(latLonPoint, points[idx]);
         double offset = theModel->getGeoidOffset(latLonPoint[0],
                                                  latLonPoint[1]);
         if(*gridPtr == OSSIMPLANET_NULL_HEIGHT)
         {
            *gridPtr = offset;
         }
         else
         {
            *gridPtr = (*gridPtr)*theHeightExag + offset;
         }
         ++gridPtr;
      }
   }

   return grid;

}

osg::ref_ptr<ossimPlanetImage> ossimPlanetLandReaderWriter::getCachedElevation(ossim_uint32 level,
                                                                               ossim_uint32 row,
                                                                               ossim_uint32 col)const
{
   osg::ref_ptr<ossimPlanetImage> result;
   std::ostringstream tempStream;

   if(!theElevationEnabledFlag) return result;
   if(theElevationCacheDir!= "")
   {
      tempStream << "L" << level<<"_X"<<col<<"_Y"<<row<<".elev";
      ossimFilename filename;
      
      filename = theElevationCacheDir.dirCat(tempStream.str().c_str());
      if(filename.exists())
      {
         ossimEndian endian;
		 std::ifstream inFile(filename.c_str(),std::ios::in|std::ios::binary);
         ossim_uint8 endianFlag;
         ossim_uint32 w;
         ossim_uint32 h;
         ossimByteOrder byteOrder;
         if(inFile.good())
         {
            inFile.read((char*)(&endianFlag), 1);
            inFile.read((char*)(&w), 4);
            inFile.read((char*)(&h), 4);
            
            byteOrder = (ossimByteOrder)endianFlag;
            if(endian.getSystemEndianType() != byteOrder)
            {
               endian.swap(&w, 4);
               endian.swap(&h, 4);
            }
            
            result = new ossimPlanetImage;
            result->allocateImage(w, h, 1, GL_LUMINANCE, GL_FLOAT);//new ossimPlanetElevationGrid(w, h);
            ossim_float32* data = (ossim_float32*)result->data();
            
            inFile.read((char*)data, w*h*sizeof(ossim_float32));
            
            if(endian.getSystemEndianType() != byteOrder)
            {
               endian.swap(data, w*h);
            }
         }
      }
   }

   return result;
}

void ossimPlanetLandReaderWriter::writeElevationToCache(ossim_uint32 level,
                                                      ossim_uint32 row,
                                                      ossim_uint32 col,
                                                      osg::ref_ptr<ossimPlanetImage> elevation)const
{
   if(theElevationCacheDir!= "")
   {
      if(theElevationCacheDir.exists())
      {
         std::ostringstream tempStream;
         ossimEndian endian;
         tempStream << "L" << level<<"_X"<<col<<"_Y"<<row<<".elev";
         ossimFilename filename;
         filename = theElevationCacheDir.dirCat(tempStream.str().c_str());
         
         std::ofstream oFile(filename.c_str(),
			 std::ios::out|std::ios::binary);
         ossim_uint8 endianType = (ossim_uint8)endian.getSystemEndianType();
         ossim_uint32 w = elevation->getWidth();
         ossim_uint32 h = elevation->getHeight();
         ossim_float32* data = (ossim_float32*)elevation->data();
         if(oFile.good())
         {
            oFile.write((char*)&endianType, 1);
            oFile.write((char*)&w, 4);
            oFile.write((char*)&h, 4);
            oFile.write((char*)data, sizeof(ossim_float32)*w*h);
         }
      }
   }
}

