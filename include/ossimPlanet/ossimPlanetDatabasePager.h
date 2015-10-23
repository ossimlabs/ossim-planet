#ifndef ossimPlanetDatabasePager_HEADER
#define ossimPlanetDatabasePager_HEADER
#include <osgDB/DatabasePager>
#include <ossimPlanet/ossimPlanetExport.h>
//#include <ossimPlanet/ossimPlanetCallback.h>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
class OSSIMPLANET_DLL ossimPlanetDatabasePager : public osgDB::DatabasePager
{
public:
   ossimPlanetDatabasePager();
   virtual DatabasePager* clone() const { return new ossimPlanetDatabasePager(*this); }
   #if 0
   virtual bool isRunning() const;
   
   virtual void signalBeginFrame(const osg::FrameStamp* framestamp);
   virtual void signalEndFrame();
   virtual void updateSceneGraph(const osg::FrameStamp& frameStamp);
   virtual void compileGLObjects(osg::State& state,double& availableTime);
   virtual void compileAllGLObjects(osg::State& state);
   
   #endif
#if 0
   class Callback : public osg::Referenced
   {
     public:
          Callback(){}
          virtual void updateSceneGraph(){}
          virtual void noMoreWork(){}
          virtual void doingWork(){}
   }; 
   typedef std::vector<osg::ref_ptr<ossimPlanetDatabasePager::Callback> > CallbackListType;
   ossimPlanetDatabasePager();
   virtual DatabasePager* clone() const { return new ossimPlanetDatabasePager(*this); }
   virtual ~ossimPlanetDatabasePager();
   virtual void run();
   virtual int cancel();
   virtual void invalidateRequest(const std::string& requestString);
   virtual void requestNodeFile(const std::string& fileName,osg::Group* group, float priority, const osg::FrameStamp* framestamp);
   bool listsAreEmpty()const;
        
   void addCallback(osg::ref_ptr<ossimPlanetDatabasePager::Callback> callback);
   void removeCallback(osg::ref_ptr<ossimPlanetDatabasePager::Callback> callback);
   void removeRequest(osg::Group* group);
   void addToDeleteList(osg::Object* obj);
   void clearRequests();
   virtual void updateSceneGraph(double currentFrameTime);
   
   class  MyFindCompileableGLObjectsVisitor;
   struct MySortFileRequestFunctor;
   struct MyCompileOperation;
   friend struct MyCompileOperation;
   friend struct MySortFileRequestFunctor;
   friend class MyFindCompileableGLObjectsVisitor;
#endif

protected:
#if 0
   void notifyDoingWork();
   void notifyNoMoreWork();
   void notifyUpdateSceneGraph();
   ossimPlanetReentrantMutex theCallbackListMutex;
   ossimPlanetDatabasePager::CallbackListType theCallbackList;
//   SceneNotificationCallbackListType theCallbackList;
#endif
};

#endif
