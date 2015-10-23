#ifndef ossimPlanetTileRequest_HEADER
#define ossimPlanetTileRequest_HEADER
#include <ossimPlanet/ossimPlanetOperation.h>
#include <queue>
#include <osg/observer_ptr>
#include <ossimPlanet/ossimPlanetTerrainTechnique.h>
#include <ossimPlanet/ossimPlanetGrid.h>
#include <ossimPlanet/ossimPlanetGridUtility.h>
#include <ossimPlanet/ossimPlanetGrid.h>
#include <osgUtil/IncrementalCompileOperation>
class ossimPlanetTerrainTile;
class ossimPlanetTerrain;

class OSSIMPLANET_DLL ossimPlanetTileRequest : public ossimPlanetOperation
{
public:
   struct SortFunctor
   {
      bool operator() (const osg::ref_ptr<ossimPlanetOperation>& lhs,
                       const osg::ref_ptr<ossimPlanetOperation>& rhs) const
      {
         return (lhs->priority()>rhs->priority());
      }
      bool operator() (const osg::ref_ptr<ossimPlanetTileRequest>& lhs,
                       const osg::ref_ptr<ossimPlanetTileRequest>& rhs) const
      {
         return (lhs->priority()>rhs->priority());
      }
   };
   typedef std::queue<osg::ref_ptr<ossimPlanetTileRequest> > Queue;
   typedef std::list<osg::ref_ptr<ossimPlanetTileRequest> > List;
   typedef std::vector<osg::ref_ptr<ossimPlanetTileRequest> > Vector;
   typedef std::set< osg::ref_ptr<osg::Texture> >               TextureSetList;
   typedef std::set< osg::ref_ptr<osg::Drawable> >              DrawableList;
   typedef std::set< osg::ref_ptr<osg::VertexBufferObject> >    VertexBufferList;
   struct DataToCompile
   {
      TextureSetList   textures;
      VertexBufferList vbos;
   };
   
   class FindCompileableGLObjectsVisitor : public osg::NodeVisitor
   {
   public:
      FindCompileableGLObjectsVisitor(DataToCompile& dataToCompile, 
                                      ossimPlanetTerrain* terrain,
                                      ossim_uint32 context):
      osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
      theDataToCompile(dataToCompile),
      theTerrain(terrain),
      theContextId(context)
      {
      }
      virtual void apply(osg::Node& node);
      virtual void apply(osg::Geode& geode);
      inline void apply(osg::StateSet* stateset);
      inline void apply(osg::Drawable* drawable);
      
   protected:
      ossimPlanetTileRequest::DataToCompile&      theDataToCompile;
      const ossimPlanetTerrain*               theTerrain;
      std::set<osg::ref_ptr<osg::Texture> >   theTextureSet;
      std::set<osg::ref_ptr<osg::Drawable> >  theDrawableSet;
      ossim_uint32                            theContextId;
   };

   ossimPlanetTileRequest()
   {
      setThreadSafeRefUnref(true);
   }
   ossimPlanetTileRequest(ossimPlanetTerrainTile* tile);
   virtual void setTile(ossimPlanetTerrainTile* tile);
   ossimPlanetTerrainTile* tile();
   const ossimPlanetTerrainTile* tile()const;
   bool needsToCompile()const
   {
      return true;
   }
   ossim_int32 frameNumberOfLastRequest()const{return theFrameNumberLastRequest;}
   void setFrameNumberOfLastRequest(ossim_int32 num)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theFrameNumberLastRequest = num;
   }
   bool isRequestCurrent(ossim_int32 frameNumber)const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return ((frameNumber-theFrameNumberLastRequest)<=1);
   }
   void setTimestampFirstRequest(double timeStamp)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theTimestampFirstRequest = timeStamp;
      theTimestampLastRequest  = timeStamp;
   }
   double timeStampFirstRequest()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theTimestampFirstRequest;
   }
   void setTimestampLastRequest(double timeStamp)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      theTimestampLastRequest = timeStamp;
   }
   double timestampLastRequest()const
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(thePropertyMutex);
      return theTimestampLastRequest;
   }
   virtual void applyToGraph()
   {
      
   }
   virtual bool compileObjects(osg::RenderInfo& renderInfo,//osg::State* s, 
                               double availableTimeInSeconds=9999999);
   virtual bool populateCompileSet(osgUtil::IncrementalCompileOperation::ContextSet& contexts, 
                                   osgUtil::IncrementalCompileOperation::CompileSet& compileSet);
protected:
   mutable OpenThreads::Mutex thePropertyMutex;
   ossim_int32                theFrameNumberFirstRequest;
   double                     theTimestampFirstRequest;
   ossim_int32                theFrameNumberLastRequest;
   ossim_float64              theTimestampLastRequest;
   ossimPlanetTerrainTileId                  theTileId;
   osg::observer_ptr<ossimPlanetTerrainTile> theTile;
   osg::observer_ptr<ossimPlanetTerrain>     theTerrain;
   DataToCompile theDataToCompile;
};

class OSSIMPLANET_DLL ossimPlanetSplitRequest : public ossimPlanetTileRequest
{
public:
   ossimPlanetSplitRequest(){}
   ossimPlanetSplitRequest(ossimPlanetTerrainTile* tile);
   
   virtual void run();
   virtual void applyToGraph();
   virtual bool compileObjects(osg::RenderInfo& renderInfo,//osg::State* s, 
                               double availableTimeInSeconds=9999999);
   virtual bool populateCompileSet(osgUtil::IncrementalCompileOperation::ContextSet& contexts, 
                                   osgUtil::IncrementalCompileOperation::CompileSet& compileSet);
   virtual void setTile(ossimPlanetTerrainTile* tile);
protected:
   ossimPlanetTerrainTechnique::TileIdList theNeededChildrenList;
   std::vector<osg::ref_ptr<ossimPlanetTerrainTile> > theNewTiles;
};

class OSSIMPLANET_DLL ossimPlanetTextureRequest : public ossimPlanetTileRequest
{
public:
   ossimPlanetTextureRequest();
   ossimPlanetTextureRequest(ossimPlanetTerrainTile* tile, 
                             ossim_uint32 imageLayerIdx);
   virtual void run();
   virtual void applyToGraph();
   void setImageLayerIdx(ossim_uint32 idx)
   {
      std::vector<ossim_uint32> idxValues;
      idxValues.push_back(idx);
      setTextureLayerIndices(idxValues);
   }
   void setTextureLayerIndices(const std::vector<ossim_uint32>& values);
   
   virtual bool compileObjects(osg::RenderInfo& renderInfo,//osg::State* s, 
                               double availableTimeInSeconds=9999999);
   virtual bool populateCompileSet(osgUtil::IncrementalCompileOperation::ContextSet& contexts, 
                                   osgUtil::IncrementalCompileOperation::CompileSet& compileSet);
protected:
   class Result
   {
   public:
      osg::ref_ptr<ossimPlanetTexture2D> theTexture;
      osg::ref_ptr<ossimPlanetImage> theImage;
   };
   typedef std::map<ossim_uint32, Result>  TextureResultMap;
   TextureResultMap theResultList;
};

class OSSIMPLANET_DLL ossimPlanetElevationRequest : public ossimPlanetTileRequest
{
public:
   ossimPlanetElevationRequest();
   ossimPlanetElevationRequest(ossimPlanetTerrainTile* tile,
                               ossim_uint32 width,
                               ossim_uint32 height);
   virtual void run();
   virtual bool compileObjects(osg::RenderInfo& renderInfo,//osg::State* s, 
                               double availableTimeInSeconds=9999999);
    virtual bool populateCompileSet(osgUtil::IncrementalCompileOperation::ContextSet& contexts, 
                                   osgUtil::IncrementalCompileOperation::CompileSet& compileSet);
  virtual void applyToGraph();
protected:
   osg::ref_ptr<ossimPlanetTerrainTile> theNewMesh;
   osg::ref_ptr<ossimPlanetImage> theImage;
};

class OSSIMPLANET_DLL ossimPlanetTileRequestQueue : public ossimPlanetOperationQueue
{
public:
   ossimPlanetTileRequestQueue(bool sortFlag=true);
   void sort();
   virtual void add(ossimPlanetTileRequest* request);
   virtual osg::ref_ptr<ossimPlanetOperation> nextOperation(bool blockIfEmptyFlag=true);
   void setCurrentFrameNumber(ossim_int32 num)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theRequestQueueMutex);
      theCurrentFrameNumber = num;
   }
   void setSortFlag(bool flag)
   {
      theSortFlag = flag;
   }
   bool sortFlag()const
   {
      return theSortFlag;
   }
protected:
   OpenThreads::Mutex theRequestQueueMutex;
   ossim_int32 theCurrentFrameNumber;
   bool theSortFlag;
};

class OSSIMPLANET_DLL ossimPlanetTileRequestThreadQueue : public ossimPlanetOperationThreadQueue
{
public:
   ossimPlanetTileRequestThreadQueue ()
   :ossimPlanetOperationThreadQueue()
   {
      setOperationQueue(new ossimPlanetTileRequestQueue());
   }
   virtual ~ossimPlanetTileRequestThreadQueue(){}
   virtual void add(ossimPlanetTileRequest* request);
   virtual void run();
   void setCurrentFrameNumber(ossim_int32 num)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theRequestThreadQueueMutex);
      theCurrentFrameNumber = num;
      ossimPlanetTileRequestQueue* tileQueue = dynamic_cast<ossimPlanetTileRequestQueue*>(operationQueue());
      if(tileQueue)
      {
         tileQueue->setCurrentFrameNumber(num);
      }
   }
   ossim_int32 currentFrameNumber()const
   {
      return theCurrentFrameNumber;
   }
   //void applyToGraph(double availableTime=2.5);
protected:
   OpenThreads::Mutex theRequestThreadQueueMutex;
   ossim_int32 theCurrentFrameNumber;
};


#endif
