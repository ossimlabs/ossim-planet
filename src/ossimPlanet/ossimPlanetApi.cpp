#include <ossimPlanet/ossimPlanetApi.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetLand.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetTextureLayerRegistry.h>
#include <ossimPlanet/ossimPlanetLayerRegistry.h>
#include <ossim/base/ossimMatrix4x4.h>
#include <ossim/base/ossimTraceManager.h>
#include <ossim/base/ossimDirectory.h>
#include <ossim/plugin/ossimSharedPluginRegistry.h>
#include <ossim/base/ossimGeoidManager.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimGeoidEgm96.h>
#include <ossim/base/ossimGeoidNgs.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/init/ossimInit.h>
#include <osgUtil/SceneView>
#include <osgViewer/Viewer>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <ossimPlanet/ossimPlanetManipulator.h>
#include <osgGA/TerrainManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgDB/Registry>
#include <ossimPlanet/ul.h>
#include <fstream>
#include <ossimPlanet/ossimPlanetXmlAction.h>
#include <ossimPlanet/ossimPlanetViewer.h>
#include <ossimPlanet/ossimPlanetShaderProgramSetup.h>

static OpenThreads::Mutex ossimPlanet_LayerListMutex;
static ossim_uint64 ossimPlanet_initializationCount = 0;
static ossimTrace traceDebug("ossimPlanetApi:debug");

class ossimPlanetStateBase : public ossimPlanetActionReceiver
   {
   public:
      ossimPlanetStateBase(ossimPlanet_ContextType type)
      :theContextType(type),
      thePlanet(new ossimPlanet)
      {
      }
      virtual ~ossimPlanetStateBase()
      {
      }
      ossimPlanet_ContextType contextType()const
      {
         return theContextType;
      }
      const osg::ref_ptr<ossimPlanetGeoRefModel> landModel()const
      {
         return thePlanet->model();
      }
      void setPlanet(ossimPlanet* planet)
      {
         thePlanet = planet;
      }
      osg::ref_ptr<ossimPlanet> planet()
      {
         return thePlanet;
      }
      const osg::ref_ptr<ossimPlanet> planet()const
      {
         return thePlanet;
      }
      virtual void setProjectionMatrix(const osg::Matrixd& m)=0;
      virtual void setViewport(int x, int y, int w, int h)=0;
      virtual void setViewMatrix(const osg::Matrixd& m) = 0;
      virtual void setViewportClearColor(const osg::Vec4& color)=0;
      virtual bool needsRendering()
      {
         if(thePlanet.valid())
         {
            return thePlanet->redrawFlag();
         }
         return false;
      }
      virtual bool frame()=0;
      virtual void setSceneData(){}
      virtual void setCameraManipulator(osg::ref_ptr<osgGA::CameraManipulator> m){}
      
      virtual void execute(const ossimPlanetAction& action)
      {
         ossimString command;
         action.command(command);
      }
   protected:
      ossimPlanet_ContextType theContextType;
      osg::ref_ptr<ossimPlanet> thePlanet;
   };

#define ossimPlanetStateBaseCast(X) static_cast<ossimPlanetStateBase*>(X)
#define ossimPlanetLayerCast(X) static_cast<ossimPlanetLayer*>(X)
#define ossimPlanetCast(X) static_cast<ossimPlanet*>(X)

class ossimPlanetStatePlanetOnly : public ossimPlanetStateBase
{
public:
   ossimPlanetStatePlanetOnly()
   :ossimPlanetStateBase(ossimPlanet_PLANET_ONLY_CONTEXT)
   {
   }
   virtual void setProjectionMatrix(const osg::Matrixd& m)
   {
   }
   virtual void setViewport(int x, int y, int w, int h)
   {
   }
   virtual void setViewMatrix(const osg::Matrixd& m)
   {
   }
   virtual void setViewportClearColor(const osg::Vec4& color)
   {
   }
   virtual bool needsRendering()
   {
      return ossimPlanetStateBase::needsRendering();
   }
   virtual bool frame()
   {
      return false;
   }   
};

class ossimPlanetStateSceneView : public ossimPlanetStateBase
   {
   public:
      ossimPlanetStateSceneView()
      :ossimPlanetStateBase(ossimPlanet_NOVIEWER_CONTEXT),
      theSceneView(new osgUtil::SceneView),
      theNeedsRedrawFlag(true)
      {
      }
      virtual ~ossimPlanetStateSceneView()
      {
      }
      virtual void setProjectionMatrix(const osg::Matrixd& m)
      {
         theNeedsRedrawFlag = true;
         theSceneView->setProjectionMatrix(m);
      }
      virtual void setViewport(int x, int y, int w, int h)
      {
         theNeedsRedrawFlag = true;
         theSceneView->setViewport(x, y, w, h);
      }
      virtual void setViewMatrix(const osg::Matrixd& m)
      {
         theNeedsRedrawFlag = true;
         theSceneView->setViewMatrix(m);
      }
      virtual void setViewportClearColor(const osg::Vec4& color)
      {
         theNeedsRedrawFlag = true;
         theSceneView->setClearColor(color);
      }
      virtual bool needsRendering()
      {
         return (theNeedsRedrawFlag||ossimPlanetStateBase::needsRendering());
      }
      virtual void setSceneData()
      {
         theNeedsRedrawFlag = true;
         theSceneView->setSceneData(thePlanet.get());         
      }
      virtual bool frame()
      {
         theNeedsRedrawFlag = false;
         theSceneView->update();
         theSceneView->cull();
         theSceneView->draw();
         return true;
      }
      osg::ref_ptr<osgUtil::SceneView> sceneView()
      {
         return theSceneView.get();
      }
   protected:
      osg::ref_ptr<osgUtil::SceneView> theSceneView;
      bool theNeedsRedrawFlag;
   };

class ossimPlanetStateViewer : public ossimPlanetStateBase
   {
   public:
      ossimPlanetStateViewer()
         :ossimPlanetStateBase(ossimPlanet_VIEWER_CONTEXT),
            theViewer(new ossimPlanetViewer)
      {
         theViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
      }
      virtual ~ossimPlanetStateViewer()
      {
      }
      virtual void setProjectionMatrix(const osg::Matrixd& m)
      {
         theViewer->getCamera()->setProjectionMatrix(m);
      }
      virtual void setViewport(int x, int y, int w, int h)
      {
         theViewer->getCamera()->setViewport(x, y, w, h);
      }
      virtual void setViewMatrix(const osg::Matrixd& m)
      {
         theViewer->getCamera()->setViewMatrix(m);
      }
      virtual void setViewportClearColor(const osg::Vec4& color)
      {
         theViewer->getCamera()->setClearColor(color);
      }
      virtual bool needsRendering()
      {
         if(theManipulator.valid())
         {
            if(theManipulator->navigator()->needsContinuousUpdate())
            {
               return true;
            }
         }
         return ossimPlanetStateBase::needsRendering();
      }
      virtual void setSceneData()
      {
         theViewer->setSceneData(thePlanet.get());
      }
      virtual bool frame()
      {
         
         if (!theViewer->isRealized())
         {
            theViewer->realize();
         }
         
         theViewer->frame();
         
         return !theViewer->done();
      }
      osg::ref_ptr<osgViewer::Viewer> viewer()
      {
         return theViewer;
      }
      virtual void setCameraManipulator(osg::ref_ptr<osgGA::CameraManipulator> m)
      {
         theManipulator = dynamic_cast<ossimPlanetManipulator*>(m.get());
         theViewer->setCameraManipulator(m.get());
      }
   protected:
      osg::ref_ptr<ossimPlanetManipulator> theManipulator;
      osg::ref_ptr<osgViewer::Viewer> theViewer;
   };

// private utility functions
static void ossimPlanetPrvt_fixReceiverPath(ossimString& pathName)
{
   if(!pathName.empty())
   {
      if(pathName[(ossimString::size_type)0] != ':')
      {
         pathName = ":" + pathName;
      }
   }
}
static void ossimPlanetPrvt_extractSeaparatedActions(std::vector<ossimString>& result,
                                                     ossimPlanet_ConstStringType actions,
                                                     ossimPlanet_ConstStringType separator)
                                                   
{
   ossimString(actions).split(result, ossimString(separator));
}

// API definitions
//
void ossimPlanet_addOpenSceneGraphLibraryPath(ossimPlanet_ConstStringType path,
                                              ossimPlanet_BOOL insertFrontFlag)
{
   if(!path) return;
   if(insertFrontFlag != ossimPlanet_TRUE)
   {
      osgDB::Registry::instance()->getLibraryFilePathList().push_back(path);      
   }
   else
   {
      osgDB::Registry::instance()->getLibraryFilePathList().insert(osgDB::Registry::instance()->getLibraryFilePathList().begin(),
                                                                   path);            
   }
}

void ossimPlanet_addOpenSceneGraphDataPath(ossimPlanet_ConstStringType path,
                                           ossimPlanet_BOOL insertFrontFlag)
{
   if(!path) return;
   if(insertFrontFlag != ossimPlanet_TRUE)
   {
      osgDB::Registry::instance()->getDataFilePathList().push_back(path);      
   }
   else
   {
      osgDB::Registry::instance()->getDataFilePathList().insert(osgDB::Registry::instance()->getLibraryFilePathList().begin(),
                                                                   path);            
   }
}

void ossimPlanet_loadOssimPreferenceFile(ossimPlanet_ConstStringType preferenceFile)
{
   ossimPreferences::instance()->loadPreferences(ossimFilename(preferenceFile));
}

void ossimPlanet_setOssimPreferenceNameValue(ossimPlanet_ConstStringType name,
                                             ossimPlanet_ConstStringType value)
{
   ossimPreferences::instance()->addPreference(name, value);   
}

void ossimPlanet_addOssimElevation(ossimPlanet_ConstStringType path)
{
   if(path&&ossimFilename(path).isDir())
   {
      ossimElevManager::instance()->loadElevationPath(path);
   }
}

void ossimPlanet_addGeoid(ossimPlanet_ConstStringType path,
                          ossimByteOrder byteOrder,
                          ossimPlanet_BOOL insertFrontFlag)
{
   ossimFilename geoidFile = path;
   if(path)
   {
      ossimRefPtr<ossimGeoid> geoid;
      if(geoidFile.exists())
      {
         geoid = new ossimGeoidEgm96;
         if(geoid->open(path, byteOrder))
         {
            ossimGeoidManager::instance()->addGeoid(geoid, insertFrontFlag==ossimPlanet_TRUE);
         }
         else
         {
            geoid = new ossimGeoidNgs;
            if(geoid->open(path))
            {
               ossimGeoidManager::instance()->addGeoid(geoid, insertFrontFlag==ossimPlanet_TRUE);
            }
            else
            {
               if(traceDebug())
               {
                  ossimNotify(ossimNotifyLevel_WARN) << "No grid handler found for path = " << ossimString(path) << "\n";
               }
            }
         }
      }
   }
}

void ossimPlanet_addOssimPlugin(ossimPlanet_ConstStringType path,
                                ossimPlanet_BOOL insertFrontFlag)
{
   ossimFilename plugin(path);
   if(plugin.exists())
   {
      if(plugin.isDir())
      {
         ossimDirectory dir;
         if(dir.open(plugin))
         {
            ossimFilename file;
            bool loadedPluginFlag = false;
            if(dir.getFirst(file,
                            ossimDirectory::OSSIM_DIR_FILES))
            {
               do
               { 
                  if(ossimSharedPluginRegistry::instance()->registerPlugin(file, insertFrontFlag==ossimPlanet_TRUE))
                  {
                     loadedPluginFlag=true;
                  }
               }
               while(dir.getNext(file));
            }
            if(!loadedPluginFlag)
            {
               if(traceDebug())
               {
                  ossimNotify(ossimNotifyLevel_WARN) << "Unable find a plugin in directory " << file << "\n";
               }               
            }
         }
      }
      else
      {
         if(!ossimSharedPluginRegistry::instance()->registerPlugin(plugin, insertFrontFlag==ossimPlanet_TRUE))
         {
            if(traceDebug())
            {
               ossimNotify(ossimNotifyLevel_WARN) << "Unable to load plugin " << plugin << "\n";
            }
         }
      }
   }
   
}

void ossimPlanet_setTracePattern(ossimPlanet_ConstStringType pattern)
{
   ossimTraceManager::instance()->setTracePattern(ossimString(pattern));
}

void ossimPlanet_microSecondSleep(ossimPlanet_SizeType microSeconds)
{
   ulMicroSecondSleep((int)microSeconds);
   
}

void ossimPlanet_milliSecondSleep(ossimPlanet_SizeType milliSeconds)
{
   ulMilliSecondSleep((int)milliSeconds);
}

void ossimPlanet_secondSleep(ossimPlanet_SizeType seconds)
{
   ulSleep((int)seconds);
}

ossimPlanet_StatePtr ossimPlanet_newState(ossimPlanet_ContextType type)
{
   switch(type)
   {
      case  ossimPlanet_PLANET_ONLY_CONTEXT:
      {
         return static_cast<ossimPlanet_StatePtr>(new ossimPlanetStatePlanetOnly());
      }
      case ossimPlanet_NOVIEWER_CONTEXT:
      {
         return static_cast<ossimPlanet_StatePtr>(new ossimPlanetStateSceneView());
      }
      case ossimPlanet_VIEWER_CONTEXT:
      {
         return static_cast<ossimPlanet_StatePtr>(new ossimPlanetStateViewer());         
      }
   }
   // should never get to here
   return static_cast<ossimPlanet_StatePtr>(0);
}

void ossimPlanet_setStateReceiverPathName(ossimPlanet_StatePtr state,
                                          ossimPlanet_ConstStringType path)
{
   if(state)
   {
      ossimString pathName = path;
      
      ossimPlanetPrvt_fixReceiverPath(pathName);
      ossimPlanetStateBaseCast(state)->setPathnameAndRegister(pathName);
   }
}

void ossimPlanet_setPlanetReceiverPathName(ossimPlanet_StatePtr state,
										   ossimPlanet_ConstStringType path)
{
	if(state)
	{
		ossimString pathName = path;
		
		ossimPlanetPrvt_fixReceiverPath(pathName);
		ossimPlanetStateBaseCast(state)->planet()->setPathnameAndRegister(pathName);
	}
}

void ossimPlanet_deleteState(ossimPlanet_StatePtr state)
{
   if(state)
   {
      delete ossimPlanetStateBaseCast(state);
   }
}

void ossimPlanet_init()
{
   if(ossimPlanet_initializationCount == 0)
   {
      ossimInit::instance()->initialize();            
   }
   ++ossimPlanet_initializationCount;
}

void ossimPlanet_initWithArgs(int* argc, char** argv[])
{
   if(ossimPlanet_initializationCount == 0)
   {
      ossimInit::instance()->initialize(*argc, *argv);            
   }
   ++ossimPlanet_initializationCount;
}

void ossimPlanet_finalize()
{
   if(ossimPlanet_initializationCount > 0)
   {
      --ossimPlanet_initializationCount;
   }
   
   // do cleanup if all users have finalized
   //
   if(ossimPlanet_initializationCount == 0)
   {
      ossimInit::instance()->finalize();            
   }
}

ossimPlanet_PlanetPtr ossimPlanet_getNativePlanetPointer(ossimPlanet_StatePtr state)
{
   if(state)
   {
      return static_cast<ossimPlanet_PlanetPtr>(ossimPlanetStateBaseCast(state)->planet().get());
   }
   return static_cast<ossimPlanet_PlanetPtr>(0);
}

void ossimPlanet_setViewManipulator(ossimPlanet_StatePtr state,
                                    ossimPlanet_ConstStringType typeName,
                                    ossimPlanet_ConstStringType receiverPathName)
{
   ossimPlanetStateBase* base = ossimPlanetStateBaseCast(state);
   if(base)
   {
      if(ossimString(typeName) == "ossimPlanetManipulator")
      {
         ossimPlanetManipulator* manipulator = new ossimPlanetManipulator();
         
         base->setCameraManipulator( manipulator );
      }
   }
}

void ossimPlanet_setLandFragShaderType(ossimPlanet_StatePtr state, ossimPlanetFragShaderType type)
{
#if 0
   osg::ref_ptr<ossimPlanet> ptr = ossimPlanetStateBaseCast(state)->planet();
   if(ptr.valid())
   {
      if(ptr->land().valid())
      {
         ossimPlanetShaderProgramSetup::ossimPlanetFragmentShaderType shaderType = ossimPlanetShaderProgramSetup::NO_SHADER;
         switch(type)
         {    
            case ossimPlanet_NO_SHADER:
            {
               shaderType = ossimPlanetShaderProgramSetup::NO_SHADER;
               break;
            }
            case ossimPlanet_TOP:
            {
               shaderType = ossimPlanetShaderProgramSetup::TOP;
               break;
            }
            case ossimPlanet_REFERENCE:
            {
               shaderType = ossimPlanetShaderProgramSetup::REFERENCE;
               break;
            }
            case ossimPlanet_OPACITY:
            {
               shaderType = ossimPlanetShaderProgramSetup::OPACITY;
               break;
            }
            case ossimPlanet_HORIZONTAL_SWIPE:
            {
               shaderType = ossimPlanetShaderProgramSetup::HORIZONTAL_SWIPE;
               break;
            }
            case ossimPlanet_VERTICAL_SWIPE:
            {
               shaderType = ossimPlanetShaderProgramSetup::VERTICAL_SWIPE;
               break;
            }
            case ossimPlanet_BOX_SWIPE:
            {
               shaderType = ossimPlanetShaderProgramSetup::BOX_SWIPE;
               break;
            }
            case ossimPlanet_CIRCLE_SWIPE:
            {
               shaderType = ossimPlanetShaderProgramSetup::TOP;
               break;
            }
            case ossimPlanet_ABSOLUTE_DIFFERENCE:
            {
               shaderType = ossimPlanetShaderProgramSetup::CIRCLE_SWIPE;
               break;
            }
            case ossimPlanet_FALSE_COLOR_REPLACEMENT:
            {
               shaderType = ossimPlanetShaderProgramSetup::FALSE_COLOR_REPLACEMENT;
               break;
            }
         }
         ptr->land()->setCurrentFragmentShaderType(shaderType);
      }
   }
#endif
}


void ossimPlanet_setSceneDataToView(ossimPlanet_StatePtr state)
{
   if(state)
   {
      ossimPlanetStateBaseCast(state)->setSceneData();
   }
}

void setViewTransformationsToCurrentGlSettings(ossimPlanet_StatePtr state)
{
   if(state)
   {
      GLint viewParams[4]; 
      GLdouble glMat[16]; 
      osg::Matrix m;
      
      glGetIntegerv(GL_VIEWPORT, viewParams); 
      ossimPlanetStateBaseCast(state)->setViewport(viewParams[0], 
                                                   viewParams[1], 
                                                   viewParams[2], 
                                                   viewParams[3]); 
      
      glGetDoublev(GL_PROJECTION_MATRIX, glMat); 
      m.set( glMat ); 
      ossimPlanetStateBaseCast(state)->setProjectionMatrix( m );
      //    double fov, aspect, znear, zfar;
      //    globalSettings.theSceneView3d->getProjectionMatrixAsPerspective(fov, aspect, znear, zfar);
      //    std::cout << "fov = " << fov << " aspect = " << aspect << " zn = " << znear << " zf = " << zfar << std::endl;
      
      glGetDoublev( GL_MODELVIEW_MATRIX, glMat ); 
      m.set( glMat ); 
      ossimPlanetStateBaseCast(state)->setViewMatrix( m );
   }
}

void ossimPlanet_setProjectionMatrixAsPerspective(ossimPlanet_StatePtr state,
                                                  double fovValue,
                                                  double aspectRatioValue,
                                                  double nearValue, 
                                                  double farValue)
{
   if(state)
   {
      osg::Matrixd m;
	  m.makePerspective(fovValue, aspectRatioValue, nearValue, farValue);
      //m.makePerspective(fov, aspectRatio, near, far);
      ossimPlanetStateBaseCast(state)->setProjectionMatrix(m);
   }
}

void ossimPlanet_setProjectionMatrixAsFrustum(ossimPlanet_StatePtr state,
                                              double left, double right,
                                              double bottom, double top,
                                              double zNear, double zFar)
{
   if(state)
   {
      osg::Matrixd m;
      m.makeFrustum(left, right, bottom, top, zNear, zFar);
      ossimPlanetStateBaseCast(state)->setProjectionMatrix(m);
   }
}

void ossimPlanet_setProjectionMatrixAsOrtho(ossimPlanet_StatePtr state,
                                            double left, double right,
                                            double bottom, double top,
                                            double zNear, double zFar)
{
   if(state)
   {
      osg::Matrixd m;
      m.makeOrtho(left, right, bottom, top, zNear, zFar);
      ossimPlanetStateBaseCast(state)->setProjectionMatrix(m);
   }   
}

void ossimPlanet_setProjectionMatrixAsOrtho2D(ossimPlanet_StatePtr state,
                                              double left, double right,
                                              double bottom, double top)
{
   if(state)
   {
      osg::Matrixd m;
      m.makeOrtho2D(left, right, bottom, top);
      ossimPlanetStateBaseCast(state)->setProjectionMatrix(m);      
   }
}

void ossimPlanet_setProjectionMatrixAsRowOrderedArray(ossimPlanet_StatePtr state,
                                                      double* m)
{
   if(state&&m)
   {
      osg::Matrixd mResult;
      mResult.set(m);
      ossimPlanetStateBaseCast(state)->setProjectionMatrix(mResult);
   }
}

void ossimPlanet_setProjectionMatrix(ossimPlanet_StatePtr state,
                                     double m00, double m01, double m02, double m03,
                                     double m10, double m11, double m12, double m13,
                                     double m20, double m21, double m22, double m23,
                                     double m30, double m31, double m32, double m33)
{
   if(state)
   {
      ossimPlanetStateBaseCast(state)->setProjectionMatrix(osg::Matrixd(m00, m01, m02, m03,
                                                                        m10, m11, m12, m13,
                                                                        m20, m21, m22, m23,
                                                                        m30, m31, m32, m33));      
   }
}
void ossimPlanet_setViewMatrixAsLlhHprRelativeTangent(ossimPlanet_StatePtr state,
                                                      double lat,
                                                      double lon,
                                                      double height,
                                                      double heading,
                                                      double pitch,
                                                      double roll)
{
   if(state)
   {
      const osg::ref_ptr<ossimPlanetGeoRefModel> model = ossimPlanetStateBaseCast(state)->landModel();
      if(!model.valid()) return;
      osg::Matrixd output;
      osg::Vec3d xyz;
      model->forward(osg::Vec3d(lat, lon, height),
                     xyz);

      model->lsrMatrix(osg::Vec3d(lat, lon, height),
                       output,
                       heading);
      output(3,0) = 0.0;
      output(3,1) = 0.0;
      output(3,2) = 0.0;
      
      
      NEWMAT::Matrix orien = (ossimMatrix4x4::createRotationXMatrix(pitch, OSSIM_LEFT_HANDED)*
                              ossimMatrix4x4::createRotationYMatrix(roll, OSSIM_LEFT_HANDED));
      // now transpose since ossim matrix is column oriented and OSG is row oriented
      osg::Matrixd tempM(orien[0][0], orien[1][0], orien[2][0], 0.0,
                         orien[0][1], orien[1][1], orien[2][1], 0.0,
                         orien[0][2], orien[1][2], orien[2][2], 0.0,
                         0.0, 0.0, 0.0, 1.0);
      
      ossimPlanetStateBaseCast(state)->setViewMatrix(osg::Matrix::inverse(tempM*output*osg::Matrixd::translate(xyz[0], xyz[1], xyz[2])));
   }
}

void ossimPlanet_setViewMatrixAsRowOrderedArray(ossimPlanet_StatePtr state,
                                                const double *m)
{
   if(state)
   {
      osg::Matrixd mResult;
      mResult.set(m);
      ossimPlanetStateBaseCast(state)->setViewMatrix(mResult);
   }
}

void ossimPlanet_setViewMatrix(ossimPlanet_StatePtr state,
                               double m00, double m01, double m02, double m03,
                               double m10, double m11, double m12, double m13,
                               double m20, double m21, double m22, double m23,
                               double m30, double m31, double m32, double m33)
{
   if(state)
   {
      ossimPlanetStateBaseCast(state)->setViewMatrix(osg::Matrixd(m00, m01, m02, m03,
                                                                        m10, m11, m12, m13,
                                                                        m20, m21, m22, m23,
                                                                        m30, m31, m32, m33));      
   }   
}

void ossimPlanet_setViewport(ossimPlanet_StatePtr state,
                             int x, 
                             int y, 
                             int w, 
                             int h)
{
   if(state)
   {
      ossimPlanetStateBaseCast(state)->setViewport(x, y, w, h);
   }
}

void ossimPlanet_setViewportClearColor(ossimPlanet_StatePtr state,
                                       float red,
                                       float green,
                                       float blue,
                                       float alpha)
{
   if(state)
   {
      ossimPlanetStateBaseCast(state)->setViewportClearColor(osg::Vec4(red, green, blue, alpha));
   }
}

ossimPlanet_BOOL ossimPlanet_renderFrame(ossimPlanet_StatePtr state)
{
   if(state)
   {
      ossimPlanetStateBaseCast(state)->planet()->resetAllRedrawFlags();
      return ossimPlanetStateBaseCast(state)->frame()?ossimPlanet_TRUE:ossimPlanet_FALSE;
   }
   return ossimPlanet_FALSE;
}

ossimPlanet_BOOL ossimPlanet_renderFramePreserveState(ossimPlanet_StatePtr state)
{
   ossimPlanet_BOOL result = ossimPlanet_TRUE;
	
	ossimPlanet_pushState();
	result = ossimPlanet_renderFrame(state);
	ossimPlanet_popState();

   return result;
}

ossimPlanet_BOOL ossimPlanet_needsRendering(ossimPlanet_StatePtr state)
{
   if(state)
   {
      return ossimPlanetStateBaseCast(state)->needsRendering()?ossimPlanet_TRUE:ossimPlanet_FALSE;
   }   
   
   return ossimPlanet_FALSE;
}

void ossimPlanet_pushState()
{
   glPushAttrib(GL_ALL_ATTRIB_BITS);
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glMatrixMode(GL_TEXTURE);
   glPushMatrix();
}

void ossimPlanet_popState()
{
   glMatrixMode(GL_TEXTURE);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glPopAttrib();   
}

ossimPlanet_LayerPtr ossimPlanet_addLayer(ossimPlanet_StatePtr state,
                                          ossimPlanet_ConstStringType layerType,
                                          ossimPlanet_ConstStringType name,
                                          ossimPlanet_ConstStringType id,
                                          ossimPlanet_ConstStringType description,
                                          ossimPlanet_ConstStringType receiverPathName)
{
   ossimPlanet_LayerPtr result = 0;
   if(state)
   {
      osg::ref_ptr<ossimPlanetLayer> layer = ossimPlanetLayerRegistry::instance()->create(ossimString(layerType));
      if(layer.valid())
      {
         ossimPlanet_setLayerName(static_cast<ossimPlanet_LayerPtr>(layer.get()), name);
         ossimPlanet_setLayerId(static_cast<ossimPlanet_LayerPtr>(layer.get()), id);
         ossimPlanet_setLayerDescription(static_cast<ossimPlanet_LayerPtr>(layer.get()), description);
         ossimPlanet_setLayerReceiverPathName(static_cast<ossimPlanet_LayerPtr>(layer.get()), receiverPathName);
         result = static_cast<ossimPlanet_LayerPtr>(layer.get());
         ossimPlanet_LayerListMutex.lock();
         ossimPlanetStateBaseCast(state)->planet()->addChild(layer.get());
         ossimPlanet_LayerListMutex.unlock();
      }
   }
   
   return result;
}

OSSIMPLANET_DLL void ossimPlanet_removeLayerGivenPtr(ossimPlanet_StatePtr state,
                                                     ossimPlanet_LayerPtr layerPtr)
{
   ossimPlanet_IndexType idx = ossimPlanet_getIndexOfLayerGivenPtr(state, layerPtr);
   if(idx != ossimPlanet_INVALID_INDEX)
   {
      ossimPlanet_LayerListMutex.lock();
      ossimPlanetStateBaseCast(state)->planet()->removeChildren(idx, 1);
      ossimPlanet_LayerListMutex.unlock();
   }
}

ossimPlanet_SizeType ossimPlanet_getNumberOfLayers(ossimPlanet_StatePtr state)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ossimPlanet_LayerListMutex);
   if(state)
   {
      return static_cast<ossimPlanet_SizeType>(ossimPlanetStateBaseCast(state)->planet()->getNumChildren());
   }
   
   return 0;
}

ossimPlanet_IndexType ossimPlanet_getIndexOfLayerGivenPtr(ossimPlanet_StatePtr state,
                                                          ossimPlanet_LayerPtr layerPtr)
{
   ossimPlanet_IndexType result = ossimPlanet_INVALID_INDEX;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ossimPlanet_LayerListMutex);
   
   if(state&&layerPtr)
   {
      ossimPlanet* planet         = ossimPlanetStateBaseCast(state)->planet().get();
      ossimPlanetLayer* layerTest = ossimPlanetLayerCast(layerPtr);
      if(!planet) return result;
      ossim_uint32 idx    = 0;
      ossim_uint32 upperIndex = planet->getNumChildren();
      for(; ((idx < upperIndex)&&(result == ossimPlanet_INVALID_INDEX));++idx)
      {
         ossimPlanetLayer* l = dynamic_cast<ossimPlanetLayer*>(planet->getChild(idx));
         if(l == layerTest)
         {
            result = idx;
         }
      }
   }
   
   return result;
}

ossimPlanet_LayerPtr ossimPlanet_getLayerGivenIndex(ossimPlanet_StatePtr state,
                                                    ossimPlanet_IndexType idx)
{
   ossimPlanet_LayerPtr result = 0;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ossimPlanet_LayerListMutex);
  
   if(state&&(idx>=0))
   {
      ossimPlanet* planet         = ossimPlanetStateBaseCast(state)->planet().get();
      if(!planet) return result;
      if(idx < static_cast<ossimPlanet_IndexType>(planet->getNumChildren()))
      {
         result = dynamic_cast<ossimPlanetLayer*>(planet->getChild(idx));
      }
   }   
   
   return result;
}

ossimPlanet_LayerPtr ossimPlanet_getLayerGivenId(ossimPlanet_StatePtr state,
                                                 ossimPlanet_ConstStringType id)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ossimPlanet_LayerListMutex);
   ossimPlanet_LayerPtr result = 0;
   ossimString idString(id);
   
   if(state&&!idString.empty())
   {
      ossimPlanet* planet         = ossimPlanetStateBaseCast(state)->planet().get();
      if(!planet) return result;
      ossim_uint32 idx    = 0;
      ossim_uint32 upperIndex = planet->getNumChildren();
      for(; ((idx < upperIndex)&&(result==0));++idx)
      {
         ossimPlanetLayer* l = dynamic_cast<ossimPlanetLayer*>(planet->getChild(idx));
         if(l&&l->id()==idString)
         {
            result = l;
         }
      }      
   }
   
   return result;
}

void ossimPlanet_setLayerId(ossimPlanet_LayerPtr layer,
                            ossimPlanet_ConstStringType id)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ossimPlanet_LayerListMutex);
   if(layer)
   {
      ossimPlanetLayerCast(layer)->setId(ossimString(id));
   }
}

void ossimPlanet_setLayerName(ossimPlanet_LayerPtr layer,
                              ossimPlanet_ConstStringType name)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ossimPlanet_LayerListMutex);
   if(layer)
   {
      ossimPlanetLayerCast(layer)->setName(ossimString(name));
   }   
}

void ossimPlanet_setLayerDescription(ossimPlanet_LayerPtr layer,
                                     ossimPlanet_ConstStringType description)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ossimPlanet_LayerListMutex);
   if(layer)
   {
      ossimPlanetLayerCast(layer)->setDescription(ossimString(description));
   }   
}


void ossimPlanet_setLayerReceiverPathName(ossimPlanet_LayerPtr layer,
                                          ossimPlanet_ConstStringType receiverPathName)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ossimPlanet_LayerListMutex);
   if(layer)
   {
      ossimString pathName = receiverPathName;
      ossimPlanetPrvt_fixReceiverPath(pathName);
      if(!pathName.empty())
      {
         ossimPlanetLayerCast(layer)->setPathnameAndRegister(pathName);
      }
   }      
}

ossimPlanet_BOOL ossimPlanet_getLayerEnableFlag(ossimPlanet_LayerPtr layer)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ossimPlanet_LayerListMutex);
   if(layer)
   {
     bool flag = ossimPlanetLayerCast(layer)->enableFlag();
		
		return flag?ossimPlanet_TRUE:ossimPlanet_FALSE;
   }      
	return ossimPlanet_FALSE;
}

void ossimPlanet_setLayerEnableFlag(ossimPlanet_LayerPtr layer, 
												ossimPlanet_BOOL flag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(ossimPlanet_LayerListMutex);
   if(layer)
   {
		ossimPlanetLayerCast(layer)->setEnableFlag(flag==ossimPlanet_TRUE);
	}
}

void  ossimPlanet_addImageToReceiver(ossimPlanet_ConstStringType location,
                                     ossimPlanet_ConstStringType receiverName,
												 ossimPlanet_BOOL addInTheBackgroundFlag)
{
	ossimFilename loc(location);
	if(!loc.exists()) return;
	
	if(loc.isDir())
	{
		ossimFilename file;
		ossimDirectory dir(location);
		if(dir.getFirst(file, ossimDirectory::OSSIM_DIR_FILES))
		{
			do
			{
				if((file.ext() != "his")&&
					(file.ext() != "ovr"))
				{
               ossimString action;
               
               action += "<Add target=\""+ossimString(receiverName)+"\">";
               action += "<Image>";
               action += ossimString("<id>")+ossimString(file)+ossimString("</id>");
               action += ossimString("<name>")+ossimString(file)+ossimString("</name>");
               action += ossimString("<filename>")+ossimString(file)+ossimString("</filename>");
               action += "</Image>";
               action += "</Add>";
					if(addInTheBackgroundFlag == ossimPlanet_TRUE)
					{
						ossimPlanet_postXmlAction(action.c_str());
					}
					else
					{
						ossimPlanet_executeXmlAction(action.c_str());
					}
				}
			}while(dir.getNext(file));
		}
	}
	else
	{
      ossimString action;
      
      action += "<Add target=\""+ossimString(receiverName)+"\">";
      action += "<Image>";
      action += ossimString("<id>")+ossimString(location)+ossimString("</id>");
      action += ossimString("<name>")+ossimString(location)+ossimString("</name>");
      action += ossimString("<filename>")+ossimString(location)+ossimString("</filename>");
      action += "</Image>";
      action += "</Add>";
		
      if(addInTheBackgroundFlag == ossimPlanet_TRUE)
      {
         ossimPlanet_postXmlAction(action.c_str());
      }
      else
      {
         ossimPlanet_executeXmlAction(action.c_str());
      }
	}
}

OSSIMPLANET_DLL void ossimPlanet_addTextureLayersFromKwlFile(ossimPlanet_StatePtr state,
                                                             const char* kwlFile)
{
#if 0
   ossimKeywordlist kwl;
   kwl.addFile(kwlFile);
   
   osg::ref_ptr<ossimPlanetTextureLayer> layer = ossimPlanetTextureLayerRegistry::instance()->createLayer(kwl.toString());
   if(layer.valid())
   {
      ossimPlanetStateBaseCast(state)->planet()->land()->referenceLayer()->addTop(layer.get());
   }
#endif
}


void ossimPlanet_executeXmlAction(ossimPlanet_ConstStringType completeAction)
{
   ossimPlanetXmlAction(completeAction).execute();
}

void ossimPlanet_postXmlAction(ossimPlanet_ConstStringType completeAction)
{
   ossimPlanetXmlAction(completeAction).post();
}

