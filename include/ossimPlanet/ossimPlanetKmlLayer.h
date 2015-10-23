#ifndef ossimPlanetKmlLayer_HEADER
#define ossimPlanetKmlLayer_HEADER
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetKmlLayerNode.h>
#include <ossimPlanet/ossimPlanetKml.h>
#include <OpenThreads/Mutex>
#include <osgDB/ReaderWriter>
#include <osg/Texture2D>
#include <osg/FrameStamp>
#include <osgUtil/IntersectVisitor>
#include <queue>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <time.h>
#include <ossimPlanet/ossimPlanetIconGeom.h>
#include <ossimPlanet/ossimPlanetDatabasePager.h>
#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetOperation.h>

class ossimPlanet;
class OSSIMPLANET_DLL ossimPlanetKmlLayerReaderWriter : public osgDB::ReaderWriter
{
public:
   ossimPlanetKmlLayerReaderWriter(ossimPlanetKmlLayer* layer=0)
      :theLayer(layer)
      {
         theRecurseFlag = false;
         srand(time(0));
      }
   void setLayer(ossimPlanetKmlLayer* layer)
      {
         theLayer = layer;
      }
   virtual ReadResult readNode(const std::string& fileName, const Options*)const;
   void setKmlCacheLocation(const ossimFilename& location)
   {
      theKmlCacheLocation = location;
   }
   virtual ReadResult readImage(const std::string& /*fileName*/,const Options* =NULL) const;
   void setId(const ossimString& id)
   {
	theIdString = id;
   }
protected:
   
   mutable ossimPlanetKmlLayer*                  theLayer; 
   ossimFilename                                 theKmlCacheLocation;
   mutable ossimFilename                         theCurrentPlwLocation;
   mutable bool                                  theRecurseFlag;
   ossimString                                   theIdString;
};



// theis is teporary testing for ground annotations.  We will initially be testing
// grouping of Placemark Icons into common/shared draws
//
class ossimPlanetKmlLayerTile : public osg::Referenced
{
public:

protected:
   // will hold the tile location
   ossim_uint32 theRow;
   ossim_uint32 theCol;
   ossim_uint32 theLevel;
   ossim_uint32 theFace;

   // will hold the local space transform for this cell.
   // used to convert common points to the local space.
   // Entities will be added to this graph after transformation
   //
   osg::ref_ptr<osg::MatrixTransform> theLocalToWorldTransform;
   
};

class OSSIMPLANET_DLL ossimPlanetKmlLayer : public ossimPlanetLayer
{
   friend class ossimPlanetKmlLayerReaderWriter;
public:
   class FindNodeVisitor : public osg::NodeVisitor
   {
   public:
      typedef std::vector<osg::ref_ptr<ossimPlanetKmlLayerNode> > LayerNodeList;
      FindNodeVisitor(const ossimString& id)
         :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
         theId(id)
         {
         }
      virtual void apply(osg::Node& node)
      {
         ossimPlanetKmlLayerNode* n = dynamic_cast<ossimPlanetKmlLayerNode*>(&node);
         if(n)
         {
            if(n->id() == theId)
            {
               theLayerNodeList.push_back(n);
            }
         }
         
         traverse(node);
      }
      FindNodeVisitor::LayerNodeList& layerList()
      {
         return theLayerNodeList;
      }
      const FindNodeVisitor::LayerNodeList& layerList()const
      {
         return theLayerNodeList;
      }
      
   protected:
      FindNodeVisitor::LayerNodeList theLayerNodeList;
      ossimString theId;
   };
   typedef std::map<std::string, osg::ref_ptr<ossimPlanetIconGeom> > IconMap;
   class OSSIMPLANET_DLL ossimPlanetKmlLayerCallback : public osg::Referenced
   {
   public:
      ossimPlanetKmlLayerCallback()
         :osg::Referenced(),
         theEnableFlag(true)
      {
      }
      virtual void nodeAdded(osg::Node* /*node*/){}
      virtual void nodeDeleted(osg::Node* /*node*/){}
      bool enableFlag()const
      {
         return theEnableFlag;
      }
      void setEnableFlag(bool flag)
      {
         theEnableFlag = flag;
      }
   protected:
      bool theEnableFlag;
   };
   ossimPlanetKmlLayer();
   virtual osg::Object* cloneType() const { return new ossimPlanetKmlLayer(); }
   virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const ossimPlanetKmlLayer *>(obj)!=0; }
   virtual const char* className() const { return "ossimPlanetKmlLayer"; } 
   virtual const char* libraryName() const { return "ossimPlanet"; }
   virtual void traverse(osg::NodeVisitor& nv);
   virtual void addKml(const ossimFilename& kmlFile);
   virtual void addKml(osg::ref_ptr<ossimPlanetKmlObject> kml);
   virtual void addKml(osg::ref_ptr<osg::Group> parent,
                       osg::ref_ptr<ossimPlanetKmlObject> kml);

   virtual bool addChild( Node *child );

   osg::ref_ptr<ossimPlanetIconGeom> getOrCreateIconEntry(const ossimString& src);
   osg::ref_ptr<ossimPlanetIconGeom> getIconEntry(const ossimString& src);
   void deleteNode(const ossimString& id);
   
   const ossimPlanet* planet()const;
   const ossimPlanetGeoRefModel* landModel()const;
   ossimPlanet* planet();
   ossimPlanetGeoRefModel* landModel();
   
   void readyToAddNode(osg::Group* parent,
                       osg::Node* node);
   
protected:
   
   class NodeToAddInfo
   {
   public:
      NodeToAddInfo(osg::Group* parent,
                     osg::Node* node)
      :theParent(parent),
      theNode(node)
      {
      }
      osg::ref_ptr<osg::Group> theParent;
      osg::ref_ptr<osg::Node> theNode;
   };
   bool hasCallback(const ossimPlanetKmlLayerCallback* callback)const;
   void initializePalettes();
   mutable ossimPlanetReentrantMutex theGraphMutex;                                           
   mutable ossimPlanetReentrantMutex theReadyToAddListMutex;                                           
   mutable ossimPlanetReentrantMutex theReadyToProcessKmlListMutex;
   std::vector<osg::ref_ptr<ossimPlanetKmlLayerNode> > theNodeList;


   std::queue<ossimFilename>                        theReadyToProcessKmlFileList;
   std::queue<osg::ref_ptr<ossimPlanetKmlObject> >  theReadyToProcessKmlList;
   osg::ref_ptr<ossimPlanetKmlLayerReaderWriter>    theReaderWriter;
   ossimFilename                                    theKmlCacheLocation;
   ossimPlanetKmlLayer::IconMap                     theIconMap;
   osg::ref_ptr<osg::FrameStamp>                    theFudgeStamp;
//   osg::ref_ptr<ossimPlanetDatabasePager>           theDatabasePager;
   bool theNodesAddedFlag;
   osg::ref_ptr<ossimPlanetOperationThreadQueue> theOperationQueue;
//   osg::ref_ptr<ossimPlanetKmlLayer::PagerCallback> thePagerCallback; 
   
   OpenThreads::Mutex theNodesToAddListMutex;
   std::vector<NodeToAddInfo> theNodesToAddList;
   osg::ref_ptr<osg::Referenced> theRequestRef;
};

#endif
