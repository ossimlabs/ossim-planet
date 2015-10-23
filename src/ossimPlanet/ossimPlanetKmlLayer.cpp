#include <ossimPlanet/ossimPlanetKmlLayer.h>
#include <osgDB/Registry>
#include <osg/Geode>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/Optimizer>
#include <osg/Group>
#include <osg/Texture2D>
#include <osg/CoordinateSystemNode>
#include <osg/Billboard>
#include <osg/DrawPixels>
#include <osg/MatrixTransform>
#include <osgUtil/CullVisitor>
#include <ossim/imaging/ossimMemoryImageSource.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetLand.h>
#include <ossimPlanet/ossimPlanetOssimImage.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <ossimPlanet/mkUtils.h>
#include <osg/PolygonOffset>
#include <osg/PolygonMode>
#include <osg/TextureRectangle>
#include <osgDB/ReadFile>
#include <osg/CoordinateSystemNode>
#include <osgText/Text>
#include <time.h>
#include <ossimPlanet/ossimPlanetLabelGeom.h>
#include <ossimPlanet/ossimPlanetBillboardIcon.h>
#include <ossimPlanet/ossimPlanetKmlPlacemarkNode.h>
#include <ossimPlanet/ossimPlanetKmlScreenOverlayNode.h>
#include <ossimPlanet/ossimPlanetKml.h>
#include <ossimPlanet/ossimPlanetKmlNetworkLinkNode.h>
#include <osgGA/GUIActionAdapter>
#include <osgGA/EventVisitor>
#include <wms/wmsCurlMemoryStream.h>

class ossimPlanetStageKmlOperation : public ossimPlanetOperation
{
public:
   ossimPlanetStageKmlOperation(ossimPlanetKmlLayer* layer)
   :theKmlLayer(layer),
   theParent(layer)
   {
      
   }
   virtual void run()
   {
     if(!theKmlObject.valid())
      {
         if(theKmlFile.exists())
         {
            osg::ref_ptr<ossimPlanetKml> kmlObject;
            if(theKmlFile.ext() == "kmz")
            {
               kmlObject = new ossimPlanetKmz;
            }
            else if(theKmlFile.ext() == "kml")
            {

               kmlObject = new ossimPlanetKml;
            }
            if(kmlObject.valid()&&!kmlObject->parse(theKmlFile))
            {
               return;
            }
            theKmlObject = kmlObject.get();
         }
      }
      if(theKmlObject.valid())
      {
         osg::ref_ptr<ossimXmlNode> xml = new ossimXmlNode;
         theKmlObject->write(xml.get());
         
         osg::ref_ptr<ossimPlanetKmlLayerNode> layerNode = 0;
         if(theKmlObject->getObjectList().size() > 0)
         {
            layerNode = setupContainer(theKmlObject.get()).get();
         }
         else
         {
            layerNode = setupFeature(theKmlObject.get()).get();
         }
         if(layerNode.valid())
         {
            layerNode->setId(theKmlObject->id());
         }
         if(theKmlLayer)
         {
            theKmlLayer->readyToAddNode(theParent, layerNode.get());
         }
      }
   }
   void setParent(osg::Group* parent)
   {
      theParent = parent;
   }
   void setKmlObject(osg::ref_ptr<ossimPlanetKmlObject> kml)
   {
      theKmlObject = kml;
   }
   void setFile(const ossimFilename& file)
   {
      theKmlFile = file;
   }
protected:
   osg::ref_ptr<ossimPlanetKmlLayerNode> setupContainer(osg::ref_ptr<ossimPlanetKmlObject> container)const
   {
      const ossimPlanetKmlObject::ObjectList& objList = container->getObjectList();
      ossim_uint32 idx = 0;
      osg::ref_ptr<ossimPlanetKmlLayerNode> group = new ossimPlanetKmlLayerNode;
      group->setKmlObject(container.get());
      for(idx = 0; idx < objList.size();++idx)
      {
         osg::ref_ptr<ossimPlanetKmlLayerNode> node = 0;
         if(objList[idx]->getObjectList().size())
         {
            node = setupContainer(objList[idx].get());
         }
         else
         {
            node = setupFeature(objList[idx].get());
         }
         if(node.valid())
         {
            node->setId(objList[idx]->id());
            group->addChild(node.get());
         }
      }
      return group.get();
   }
   
   osg::ref_ptr<ossimPlanetKmlLayerNode> setupFeature(osg::ref_ptr<ossimPlanetKmlObject> featureObject)const
   {
      ossimPlanetKmlFeature* feature = dynamic_cast<ossimPlanetKmlFeature*>(featureObject.get());
      if(feature)
      {
         if(dynamic_cast<ossimPlanetKmlPlacemark*>(feature))
         {
            osg::ref_ptr<ossimPlanetKmlPlacemarkNode> result = new ossimPlanetKmlPlacemarkNode(theKmlLayer, featureObject.get());
            
            if(result->init())
            {
               return result.get();
            }
         }
         else if(dynamic_cast<ossimPlanetKmlScreenOverlay*>(feature))
         {
            osg::ref_ptr<ossimPlanetKmlScreenOverlayNode> result = new ossimPlanetKmlScreenOverlayNode(theKmlLayer,
                                                                                                       featureObject.get());
            if(result->init())
            {
               return result.get();
            }
         }
         else if(dynamic_cast<ossimPlanetKmlNetworkLink*>(feature))
         {
            ossimPlanetKmlNetworkLink* link = dynamic_cast<ossimPlanetKmlNetworkLink*>(feature);
            if(link->link())
            {
               if(!link->link()->href().empty())
               {
                  ossimPlanetKmlNetworkLinkNode* node = new ossimPlanetKmlNetworkLinkNode(theKmlLayer,
                                                                                          featureObject.get());
                  node->init();
                  
                  return node;
              }
            }
         }
     }

      return 0;
   }
   osg::ref_ptr<ossimPlanetKmlObject> theKmlObject;
   ossimFilename theKmlFile;
   ossimPlanetKmlLayer* theKmlLayer;
   osg::Group* theParent;
};


osgDB::ReaderWriter::ReadResult ossimPlanetKmlLayerReaderWriter::readNode(
   const std::string& /* fileName */, const Options*)const
{
#if 0
   osg::ref_ptr<osg::Node> node;
   if(fileName == "PLW_STAGE_OBJECTS")
   {
      std::vector<char> buf(1024);
      ossimFilename file = theLayer->nextCityDatabase();
      const ossimPlanetGeoRefModel* model = theLayer->landModel();
      if(!model) return 0;
      if(file.isDir())
      {
         file = file.dirCat("origins.txt");
      }
      if(file.exists())
      {
         osg::Vec3d localOrigin;
         osg::Vec3d origin;
         double x,y,z,lat,lon,height;
         ossimFilename path = file.path();
         theCurrentPlwLocation = path;
         std::ifstream in(file.c_str(), std::ios::in);
         if(in.getline(&buf.front(), buf.size()))
         {
            std::istringstream header(&buf.front());
            ossimString field1;
            header >> field1;
            if(field1.downcase().contains("origin_wgs84"))
            {
               osg::MatrixTransform* modelScaleTransform      = new osg::MatrixTransform;
               node = modelScaleTransform;
               header >>x>>y>>z>>lat>>lon>>height;
               double ellipsoidalHeight = model->getHeightAboveEllipsoid(lat, lon);

               modelScaleTransform->setMatrix(osg::Matrixd::scale(1.0/model->getNormalizationScale(),
                                                                  1.0/model->getNormalizationScale(),
                                                                  1.0/model->getNormalizationScale()));
               while(in.getline(&buf.front(), buf.size()))
               {
                  std::istringstream files(&buf.front());
                  files >>field1>>x>>y>>z>>lat>>lon>>height;
                  osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(std::string(path.dirCat(field1).c_str()));
                  if(loadedModel.get())
                  {
                     osg::MatrixTransform* modelTransform = new osg::MatrixTransform;
                     osg::Matrixd lsrMatrix;
                     ellipsoidalHeight = model->getHeightAboveEllipsoid(lat, lon);
                     model->lsrMatrix(osg::Vec3d(lat, lon, ellipsoidalHeight),
                                                   lsrMatrix);
                     lsrMatrix(3, 0) *= model->getNormalizationScale();
                     lsrMatrix(3, 1) *= model->getNormalizationScale();
                     lsrMatrix(3, 2) *= model->getNormalizationScale();
                     modelTransform->setMatrix(lsrMatrix);
                     modelTransform->addChild(loadedModel.get());
                     modelScaleTransform->addChild(modelTransform);
                  }
               }
            }
         }
         theCurrentPlwLocation = "";
      }
   }
   return node.get();
#endif
   return 0;
}

osgDB::ReaderWriter::ReadResult ossimPlanetKmlLayerReaderWriter::readImage(const std::string& fileName, const Options*) const
{
   if(theRecurseFlag) return 0;
   if(ossimFilename(fileName).exists())
   {
      theRecurseFlag = true;
      osgDB::ReaderWriter::ReadResult i = osgDB::Registry::instance()->readImage(fileName, 0);
      theRecurseFlag = false;
      if(i.getImage()) return i;
     
      ossimPlanetOssimImage loader;
      ossimPlanetImage img;
      if(loader.loadFile(fileName, img))
      {
         osg::Image* i= new osg::Image(img);
         i->flipVertical();
         return i;
      }
   }
   if(theCurrentPlwLocation.exists())
   {
      ossimFilename tempInputFile(fileName);
      tempInputFile.convertBackToForwardSlashes();
      ossimFilename file = theCurrentPlwLocation.dirCat(tempInputFile);
      if(file.exists())
      {
         theRecurseFlag = true;
         osgDB::ReaderWriter::ReadResult i = osgDB::Registry::instance()->readImage(file.c_str(), 0);
         theRecurseFlag = false;
         return i;
      }
   }
   return ReadResult(ReadResult::FILE_NOT_HANDLED);
}

ossimPlanetKmlLayer::ossimPlanetKmlLayer()
{
   theFudgeStamp = new osg::FrameStamp;
   theReaderWriter = new ossimPlanetKmlLayerReaderWriter(this);
   osgDB::Registry::instance()->addReaderWriter(theReaderWriter.get());
   theKmlCacheLocation = ossimEnvironmentUtility::instance()->getUserOssimSupportDir();
   theKmlCacheLocation = theKmlCacheLocation.dirCat("kml");
   theReaderWriter->setKmlCacheLocation(theKmlCacheLocation);
   getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
   initializePalettes();
   theOperationQueue = new ossimPlanetOperationThreadQueue;
}

void ossimPlanetKmlLayer::traverse(osg::NodeVisitor& nv)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theGraphMutex);
   
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if(!thePlanet)
         {
            thePlanet = ossimPlanet::findPlanet(this);
         }
         {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theNodesToAddListMutex);
            if(theNodesToAddList.size()>0)
            {
               ossim_uint32 idx = 0;
               for(idx=0;idx<theNodesToAddList.size();++idx)
               {
                  theNodesToAddList[idx].theParent->addChild(theNodesToAddList[idx].theNode.get());
               }
               theNodesToAddList.clear();
            }

         }
         break;
      }
      case osg::NodeVisitor::CULL_VISITOR:
      {
         if((theReadyToProcessKmlFileList.size() > 0) ||
            (theReadyToProcessKmlList.size()>0) )
         {
            nv.getDatabaseRequestHandler()->requestNodeFile("KML_STAGE_OBJECTS",
                                                            nv.getNodePath(),
                                                            9999999,
                                                            theFudgeStamp.get(),
                                                            theRequestRef, 
                                                            0);
         }
        break;
      }
      case osg::NodeVisitor::EVENT_VISITOR:
      {
         break;
      }
      default:
      {
         break;
      }
   }
   
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theNodeList.size(); ++idx)
   {
      theNodeList[idx]->accept(nv);
   }
   ossimPlanetLayer::traverse(nv);
}

void ossimPlanetKmlLayer::addKml(osg::ref_ptr<ossimPlanetKmlObject> kml)
{
   addKml(this, kml.get());
}

void ossimPlanetKmlLayer::addKml(osg::ref_ptr<osg::Group> parent,
                                 osg::ref_ptr<ossimPlanetKmlObject> kml)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReadyToProcessKmlListMutex);
   ossimPlanetStageKmlOperation* operation = new ossimPlanetStageKmlOperation(this);  
   operation->setKmlObject(kml);
   operation->setParent(parent.get());
   theOperationQueue->add(operation);
}

void ossimPlanetKmlLayer::addKml(const ossimFilename& kmlFile)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theReadyToProcessKmlListMutex);
   ossimPlanetStageKmlOperation* operation = new ossimPlanetStageKmlOperation(this);  
   operation->setFile(kmlFile);
   theOperationQueue->add(operation);
   //theReadyToProcessKmlFileList.push(kmlFile);
//   setRedrawFlag(true);
}

bool ossimPlanetKmlLayer::addChild( Node *child )
{
   return ossimPlanetLayer::addChild(child);
}


osg::ref_ptr<ossimPlanetIconGeom> ossimPlanetKmlLayer::getOrCreateIconEntry(const ossimString& src)
{
   osg::ref_ptr<ossimPlanetIconGeom> result = getIconEntry(src);
   if(result.valid())
   {
      return result.get();
   }

   if(ossimFilename(src).exists())
   {
      osg::ref_ptr<osg::Image> img = ossimPlanetImage::readNewOsgImage(ossimFilename(src));
      osg::ref_ptr<ossimPlanetIconGeom> geom = new ossimPlanetIconGeom;
      geom->setTexture(new osg::Texture2D(img.get()));
      theIconMap.insert(std::make_pair(src.c_str(),geom.get()));
   }
   
   return getIconEntry(src);
}


osg::ref_ptr<ossimPlanetIconGeom> ossimPlanetKmlLayer::getIconEntry(const ossimString& src)
{
   ossimPlanetKmlLayer::IconMap::iterator iter = theIconMap.find(src);

   if(iter!=theIconMap.end())
   {
      return iter->second.get();
   }

   return 0;
}


void ossimPlanetKmlLayer::initializePalettes()
{
//    ossimFilename paletteDir = ossimEnvironmentUtility::instance()->getUserOssimSupportDir();
//    paletteDir = paletteDir.dirCat("images");
//    paletteDir = paletteDir.dirCat("icons");
//    if(!paletteDir.exists())
//    {
//       paletteDir = ossimEnvironmentUtility::instance()->getInstalledOssimSupportDir();
//       paletteDir = paletteDir.dirCat("images");
//       paletteDir = paletteDir.dirCat("icons");      
//    }


//    if(paletteDir.exists())
//    {
//       theIconMap.clear();
//       ossim_uint32 idx = 2;
//       for(idx = 1; idx <=5; ++idx)
//       {
//          ossimFilename tempFile = paletteDir;
//          tempFile = tempFile.dirCat(ossimString("palette-") +
//                                     ossimString::toString(idx)+".png");
         
//          osg::ref_ptr<osg::Image> img = ossimPlanetImage::readNewOsgImage(tempFile);
//          if(img.valid())
//          {
//             std::cout << "Adding icon " << tempFile << std::endl;
//             theIconMap.insert(std::make_pair(tempFile.fileNoExtension(), new osg::Texture2D(img.get())));
//          }
//       }
//    }
}

void ossimPlanetKmlLayer::deleteNode(const ossimString& id)
{
   ossimPlanetKmlLayer::FindNodeVisitor nv(id);

   this->accept(nv);
   if(nv.layerList().size())
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theGraphMutex);
      FindNodeVisitor::LayerNodeList&  layerList = nv.layerList();
      ossim_uint32 idx = 0;
      for(idx = 0; idx < layerList.size(); ++idx)
      {
         if(layerList[idx]->getParent(0))
         {
            layerList[idx]->getParent(0)->removeChild(layerList[idx].get());
            notifyRemoveChild(layerList[idx].get());
         }
      }
   }
}

void ossimPlanetKmlLayer::readyToAddNode(osg::Group* parent, 
                                         osg::Node* node)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theNodesToAddListMutex);

   theNodesToAddList.push_back(NodeToAddInfo(parent, node));
   setRedrawFlag(true);
}


const ossimPlanetGeoRefModel* ossimPlanetKmlLayer::landModel()const
{
   if(thePlanet)
   {
      return thePlanet->model().get();
   }
   return 0;
}

ossimPlanetGeoRefModel* ossimPlanetKmlLayer::landModel()
{
   if(thePlanet)
   {
      return thePlanet->model().get();
   }
   return 0;
}


