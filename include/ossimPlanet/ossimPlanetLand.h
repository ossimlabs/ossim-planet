#ifndef ossimPlanetLand_HEADER
#define ossimPlanetLand_HEADER
#include <osg/Group>
#include <osg/Timer>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <osgDB/DatabasePager>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetLayer.h>
#include <ossimPlanet/ossimPlanetConstants.h>
#include <ossimPlanet/ossimPlanetLandNormalType.h>
#include <ossimPlanet/ossimPlanetTextureLayer.h>
#include <ossimPlanet/ossimPlanetExtents.h>
#include <ossimPlanet/ossimPlanetShaderProgramSetup.h>
#include <ossimPlanet/ossimPlanetConstants.h>
#include <ossim/base/ossimFilename.h>
#include <ossimPlanet/ossimPlanetLandCache.h>
#include <ossimPlanet/ossimPlanetElevationDatabaseGroup.h>
#include <ossimPlanet/ossimPlanetElevationDatabase.h>
#include <ossimPlanet/ossimPlanetCallback.h>
#include <ossimPlanet/ossimPlanetLandCullCallback.h>
#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetXmlAction.h>
#include <osg/GL2Extensions>

class ossimPlanetGeoRefModel;
class ossimPlanetLandReaderWriter;

class OSSIMPLANET_DLL ossimPlanetLand : public ossimPlanetLayer 
{
public:
   friend class ossimPlanetLandUpdateCallback;
   friend class ossimPlanetLandRefreshVisitor;
   ossimPlanetLand();

   virtual osg::Object* cloneType() const { return new ossimPlanetLand(); }
   virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const ossimPlanetLand *>(obj)!=NULL; }
   virtual const char* className() const { return "ossimPlanetLand"; } 
   virtual const char* libraryName() const { return "ossimPlanet"; }
   virtual void accept(osg::NodeVisitor& nv);
   virtual void traverse(osg::NodeVisitor& nv);

   virtual void setModel(ossimPlanetGeoRefModel* model);
   void setLineOfSiteIntersection(const osg::Vec3d& pt);
   
   void setElevationEnabledFlag(bool elevationEnabledFlag);
   bool getElevationEnabledFlag()const;
   
   ossim_float64 getHeightExag()const;
   void setHeightExag(ossim_float64 heightExag);
   ossim_uint32 getElevationPatchSize()const;
   void setElevationPatchSize(ossim_uint32 patchSize);
   void setReaderWriter(ossimPlanetLandReaderWriter* readerWriter);
 	ossimPlanetLandReaderWriter* getReaderWriter();
	const ossimPlanetLandReaderWriter* getReaderWriter()const;
	ossim_uint32 getMaxLevelDetail()const;
   void setMaxLevelDetail(ossim_uint32 maxLevelDetail);
   ossimFilename getElevationCacheDir()const;
   void setElevationCacheDir(const ossimFilename& cacheDir);
   void resetGraph(osg::ref_ptr<ossimPlanetExtents> extents=0,
                   ossimPlanetLandRefreshType refreshType=ossimPlanetLandRefreshType_PRUNE);
   virtual bool addChild( Node *child );
   const osg::ref_ptr<ossimPlanetGeoRefModel> model()const;

   ossim_uint32 getNumberOfOverlayLayers()const;
   void setReferenceLayer(osg::ref_ptr<ossimPlanetTextureLayerGroup> reference);
   osg::ref_ptr<ossimPlanetTextureLayerGroup> referenceLayer();
   const osg::ref_ptr<ossimPlanetTextureLayerGroup> referenceLayer()const;
   osg::ref_ptr<ossimPlanetTextureLayerGroup> overlayLayers();
   const osg::ref_ptr<ossimPlanetTextureLayerGroup> overlayLayers()const;
   osg::ref_ptr<ossimPlanetTextureLayer> overlayLayer(ossim_uint32 layerIdx);
   const osg::ref_ptr<ossimPlanetTextureLayer> overlayLayer(ossim_uint32 layerIdx)const;
   osg::ref_ptr<ossimPlanetTextureLayer> removeOverlayLayer(ossim_uint32 layerIdx);
   void addOverlayLayer(osg::ref_ptr<ossimPlanetTextureLayerGroup> overlay);
   void setOverlayLayer(ossim_uint32 idx,
                        osg::ref_ptr<ossimPlanetTextureLayerGroup> overlay);
   osg::ref_ptr<ossimPlanetShaderProgramSetup> getCurrentShader();
   void setCurrentFragmentShaderType(ossimPlanetShaderProgramSetup::ossimPlanetFragmentShaderType fragType);
   /**
    * Setting to 0 will disable caching.  The second argument dictates how far to shrink the cache when it exceeds
    * the maxSize allowed.  Maybe a good value is .85*maxSize.  This will allow about a 15% reduction in cache size
    */ 
   void setCacheSize(ossim_uint64 maxSize, ossim_uint64 minSize);
   ossim_uint64 getCacheSize()const;

   /**
    * This is a ratio distance from the eye versus to radial bounds of
    * a patch.  By default this is set to 3.  So this means that once the eye
    * is within 3 times the radial distance of the patch the patch will split.  So the affect is
    * increasing this number will cause the land to tesselate faster as the eye approaches.
    */ 
   void setSplitMetricRatio(double ratio);
   double getSplitMetricRatio()const;

   void setSplitPriorityType(ossimPlanetPriorityType priorityType);
   ossimPlanetPriorityType getSplitPrioirtyType()const;

   void setCullingFlag(bool flag);
   bool getCullingFlag()const;

   void setFreezeRequestFlag(bool flag);
   bool getFreezRequestFlag()const;

   bool getMipMappingFlag()const;
   void setMipMappingFlag(bool flag);
   
   void addElevationDirectory(const ossimFilename& file, bool sortFlag=false);
   void addElevation(osg::ref_ptr<ossimPlanetElevationDatabase> database, bool sortFlag=false);
   bool shadersInitialized()const;
   
   void clearElevation();
   
   
   /**
    * Will add callbacks to the textures layers and automatically refresh the scene graph as needed if the passed in
    * flag is true.  If you want to control refreshing then set the flag to false.
    */ 
/*    void setAutoRefreshFlag(bool flag); */
/*    bool getAutoRefreshFlag(bool flag)const; */

   void pagedLodAdded(osg::Node* parent, osg::Node* child);
   void pagedLodRemoved(osg::Node* node);
	void pagedLodModified(osg::Node* node);

   /**
    * Executes receiver command actions:
    * <pre>
    *
    * </pre>
    */ 
   virtual void execute(const ossimPlanetAction &a);
   
protected:
   struct refreshInfo : public osg::Referenced
   {
      osg::ref_ptr<ossimPlanetExtents> theExtents;
      ossimPlanetLandRefreshType theRefreshType;
   };
   virtual ~ossimPlanetLand();
	void xmlExecute(const ossimPlanetXmlAction& a);
   void resetGraphLocal();
   void initShaders();
   void initElevation();
   
   void add(const ossimPlanetAction& a);
   void remove(const ossimPlanetAction& a);
   void resetGraph(const ossimPlanetAction& a);
   void addImageObject(const ossimString& objectName, const ossimString& objectArg);
   
   osg::ref_ptr<ossimPlanetLandReaderWriter>    theReaderWriter;
   static ossim_uint32                          theLandReaderWriterId;
   mutable ossimPlanetReentrantMutex                   theMutex;
   mutable ossimPlanetReentrantMutex                   theRefreshMutex;
   osg::ref_ptr<ossimPlanetTextureLayerGroup>   theReferenceLayer;
   osg::ref_ptr<ossimPlanetTextureLayerGroup>   theOverlayLayers;
   mutable std::vector<osg::ref_ptr<refreshInfo> > theExtentRefreshList;
   osg::ref_ptr<ossimPlanetShaderProgramSetup>  theCurrentShaderProgram;
   osg::ref_ptr<osg::Uniform>                   theReferenceTexture;
   osg::ref_ptr<osg::Uniform>                   theTopTexture;
   osg::ref_ptr<osg::Program>                   theLandShaderProgram;
   osg::ref_ptr<osg::Program>                   theNoShaderProgram;
   osg::ref_ptr<osg::Shader>                    theVertexShader;
   osg::ref_ptr<osg::Shader>                    theFragShader;
   std::string                                  theTopSource;
   std::string                                  theReferenceSource;
   std::string                                  theOpacitySource;
   std::string                                  theSwipeSource;
   std::string                                  theFalseColorReplacementSource;
   std::string                                  theAbsoluteDifferenceSource;
   bool                                         theAutoRefreshFlag;
   osg::ref_ptr<osg::StateSet>                  theStateSet;
   osg::ref_ptr<osg::GL2Extensions>             theGL2Extensions;
   osg::ref_ptr<ossimPlanetLandCache>           theLandCache;
   osg::ref_ptr<ossimPlanetElevationDatabaseGroup> theElevationDatabase;
   osg::ref_ptr<ossimPlanetLandCullCallback>    theCullCallback;
   osg::ref_ptr<ossimPlanetTextureLayerCallback> theTextureLayerCallback;   
   osg::ref_ptr<ossimPlanetTextureLayerCallback> theElevationLayerCallback;   
   
   bool theShadersInitializedFlag;
};

#endif
