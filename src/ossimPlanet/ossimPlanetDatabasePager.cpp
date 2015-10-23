#include <ossimPlanet/ossimPlanetDatabasePager.h>



ossimPlanetDatabasePager::ossimPlanetDatabasePager()
:DatabasePager()
{
}
#if 0
bool ossimPlanetDatabasePager::isRunning() const
{
   return DatabasePager::isRunning();
}

void ossimPlanetDatabasePager::signalBeginFrame(const osg::FrameStamp* framestamp)
{
   DatabasePager::signalBeginFrame(framestamp);
}

void ossimPlanetDatabasePager::signalEndFrame()
{
   DatabasePager::signalEndFrame();
}

void ossimPlanetDatabasePager::updateSceneGraph(const osg::FrameStamp& frameStamp)
{
   DatabasePager::updateSceneGraph(frameStamp);
}

void ossimPlanetDatabasePager::compileGLObjects(osg::State& state,double& availableTime)
{
   DatabasePager::compileGLObjects(state, availableTime);
}

void ossimPlanetDatabasePager::compileAllGLObjects(osg::State& state)
{
   DatabasePager::compileAllGLObjects(state);
}
#endif
#if 0
#ifdef OSSIMPLANET_USE_PAGER

#include <ossimPlanet/ossimPlanetDatabasePager.h>
#include <ossimPlanet/ossimPlanetPagedLandLod.h>
#include <osg/Notify>
#include <iostream>
#include <osg/Texture>
#include <osgDB/ReadFile>
#include <algorithm>
#include <osg/GraphicsThread>

#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
static int allocationCount = 0;
#endif
struct ossimPlanetDatabasePager::MyCompileOperation : public osg::GraphicsOperation
{
   MyCompileOperation(DatabasePager* databasePager);
   virtual ~MyCompileOperation(){}
   virtual void operator () (osg::GraphicsContext* context);
   
   osg::observer_ptr<DatabasePager> _databasePager;
};

ossimPlanetDatabasePager::MyCompileOperation::MyCompileOperation(osgDB::DatabasePager* databasePager):
    osg::GraphicsOperation("DatabasePager::MyCompileOperation",false),
    _databasePager(databasePager)
{
}

void ossimPlanetDatabasePager::MyCompileOperation::operator () (osg::GraphicsContext* context)
{
    // osg::notify(osg::NOTICE)<<"Background thread compiling"<<std::endl;
    if (_databasePager.valid()) _databasePager->compileAllGLObjects(*(context->getState())); 
}

struct ossimPlanetDatabasePager::MySortFileRequestFunctor
{
    bool operator() (const osg::ref_ptr<ossimPlanetDatabasePager::DatabaseRequest>& lhs,const osg::ref_ptr<ossimPlanetDatabasePager::DatabaseRequest>& rhs) const
    {
        if (lhs->_timestampLastRequest>rhs->_timestampLastRequest) return true;
        else if (lhs->_timestampLastRequest<rhs->_timestampLastRequest) return false;
        else return (lhs->_priorityLastRequest>rhs->_priorityLastRequest);
    }
};

class ossimPlanetDatabasePager::MyFindCompileableGLObjectsVisitor : public osg::NodeVisitor
{
public:
   MyFindCompileableGLObjectsVisitor(ossimPlanetDatabasePager::DataToCompile& dataToCompile, 
                                     bool changeAutoUnRef, bool valueAutoUnRef,
                                     bool changeAnisotropy, float valueAnisotropy,
                                     ossimPlanetDatabasePager::DrawablePolicy drawablePolicy):
            osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
            _dataToCompile(dataToCompile),
            _changeAutoUnRef(changeAutoUnRef), _valueAutoUnRef(valueAutoUnRef),
            _changeAnisotropy(changeAnisotropy), _valueAnisotropy(valueAnisotropy),
            _drawablePolicy(drawablePolicy)
    {
    }
    
    virtual void apply(osg::Node& node)
    {
        apply(node.getStateSet());

        traverse(node);
    }
    
    virtual void apply(osg::Geode& geode)
    {
        apply(geode.getStateSet());
    
        for(unsigned int i=0;i<geode.getNumDrawables();++i)
        {
            apply(geode.getDrawable(i));
        }

        traverse(geode);
    }
    
    inline void apply(osg::StateSet* stateset)
    {
        if (stateset)
        {
            // search for the existance of any texture object attributes
            bool foundTextureState = false;
            for(unsigned int i=0;i<stateset->getTextureAttributeList().size();++i)
            {
                osg::Texture* texture = dynamic_cast<osg::Texture*>(stateset->getTextureAttribute(i,osg::StateAttribute::TEXTURE));
                if (texture)
                {
                    if (_changeAutoUnRef) texture->setUnRefImageDataAfterApply(_valueAutoUnRef);
                    if (_changeAnisotropy) texture->setMaxAnisotropy(_valueAnisotropy);
                    foundTextureState = true;
                }
            }

            // if texture object attributes exist add the state to the list for later compilation.
            if (foundTextureState)
            {
                //osg::notify(osg::DEBUG_INFO)<<"Found compilable texture state"<<std::endl;
                _dataToCompile.first.insert(stateset);
            }
        }
    }
    
    inline void apply(osg::Drawable* drawable)
    {
        apply(drawable->getStateSet());
        
        switch(_drawablePolicy)
        {
        case ossimPlanetDatabasePager::DO_NOT_MODIFY_DRAWABLE_SETTINGS: 
             // do nothing, leave settings as they came in from loaded database.
             // osg::notify(osg::NOTICE)<<"DO_NOT_MODIFY_DRAWABLE_SETTINGS"<<std::endl;
             break;
        case ossimPlanetDatabasePager::USE_DISPLAY_LISTS: 
             drawable->setUseDisplayList(true);
             drawable->setUseVertexBufferObjects(false);
             break;
        case ossimPlanetDatabasePager::USE_VERTEX_BUFFER_OBJECTS:
             drawable->setUseDisplayList(true);
             drawable->setUseVertexBufferObjects(true);
             // osg::notify(osg::NOTICE)<<"USE_VERTEX_BUFFER_OBJECTS"<<std::endl;
             break;
        case ossimPlanetDatabasePager::USE_VERTEX_ARRAYS:
             drawable->setUseDisplayList(false);
             drawable->setUseVertexBufferObjects(false);
             // osg::notify(osg::NOTICE)<<"USE_VERTEX_ARRAYS"<<std::endl;
             break;
        }
        
        if (drawable->getUseDisplayList() || drawable->getUseVertexBufferObjects())
        {
            // osg::notify(osg::NOTICE)<<"  Found compilable drawable"<<std::endl;
            _dataToCompile.second.push_back(drawable);
        }
    }
    
    ossimPlanetDatabasePager::DataToCompile&   _dataToCompile;
    bool                            _changeAutoUnRef;
    bool                            _valueAutoUnRef;
    bool                            _changeAnisotropy;
    float                           _valueAnisotropy;
    ossimPlanetDatabasePager::DrawablePolicy   _drawablePolicy;
};

ossimPlanetDatabasePager::ossimPlanetDatabasePager()
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   ++allocationCount;
   std::cout << "ossimPlanetDatabasePager Count: " << allocationCount << std::endl;
#endif
}
ossimPlanetDatabasePager::~ossimPlanetDatabasePager()
{
#ifdef OSGPLANET_ENABLE_ALLOCATION_COUNT
   --allocationCount;
   std::cout << "ossimPlanetDatabasePager Count: " << allocationCount << std::endl;
#endif
}

int ossimPlanetDatabasePager::cancel()
{
   return osgDB::DatabasePager::cancel();
}

void ossimPlanetDatabasePager::invalidateRequest(const std::string& requestString)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_fileRequestListMutex);
   bool foundEntry = false;
   for(DatabaseRequestList::iterator ritr = _fileRequestList.begin();
       ritr != _fileRequestList.end() && !foundEntry;
       ++ritr)
   {
      if ((*ritr)->_fileName==requestString)
      {
         (*ritr)->_frameNumberLastRequest = _frameNumber - 100000;
         foundEntry = true;
      }
   }
}

void ossimPlanetDatabasePager::removeRequest(osg::Group* group)
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_fileRequestListMutex);
      for(DatabaseRequestList::iterator ritr = _fileRequestList.begin();
          ritr != _fileRequestList.end();
          ++ritr)
      {
         if ((*ritr)->_groupForAddingLoadedSubgraph==group)
         {
            _fileRequestList.erase(ritr);
            break;
         }
      }
   }
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_dataToCompileListMutex);
      for(DatabaseRequestList::iterator ritr = _dataToCompileList.begin();
          ritr != _dataToCompileList.end();
          ++ritr)
      {
         if ((*ritr)->_groupForAddingLoadedSubgraph==group)
         {
            _dataToCompileList.erase(ritr);
            break;
         }
      }
   }
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_dataToMergeListMutex);
      for(DatabaseRequestList::iterator ritr = _dataToMergeList.begin();
          ritr != _dataToMergeList.end();
          ++ritr)
      {
         if ((*ritr)->_groupForAddingLoadedSubgraph==group)
         {
            _dataToMergeList.erase(ritr);
            break;
         }
      }
   }
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock1(_fileRequestListMutex);
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(_dataToCompileListMutex);
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock3(_dataToMergeListMutex);
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock4(_childrenToDeleteListMutex);
      updateDatabasePagerThreadBlock();
      if(_dataToMergeList.empty()&&
         _dataToCompileList.empty()&&
         _fileRequestList.empty()&&
         _childrenToDeleteList.empty())
      {
         notifyNoMoreWork();
      }
   }
}

void ossimPlanetDatabasePager::addToDeleteList(osg::Object* obj)
{
   if (_deleteRemovedSubgraphsInDatabaseThread)
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_childrenToDeleteListMutex);
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock1(_fileRequestListMutex);
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(_dataToCompileListMutex);
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock3(_dataToMergeListMutex);
      _childrenToDeleteList.push_back(obj);
      notifyDoingWork();
      updateDatabasePagerThreadBlock();
   }
}

void ossimPlanetDatabasePager::requestNodeFile(const std::string& fileName,
                                             osg::Group* group,
                                             float priority,
                                             const osg::FrameStamp* framestamp)
{
   DatabasePager::requestNodeFile(fileName,
                                  group,
                                  priority,
                                  framestamp);
   notifyDoingWork();
}

void ossimPlanetDatabasePager::run()
{
   osg::notify(osg::INFO)<<"DatabasePager::run()"<<std::endl;
   
   // need to set the texture object manager to be able to reuse textures
   osg::Texture::setMinimumNumberOfTextureObjectsToRetainInCache(100);
   
   // need to set the texture object manager to be able to reuse textures
   osg::Drawable::setMinimumNumberOfDisplayListsToRetainInCache(500);
   
   bool firstTime = true;
   
   do
   {
     _databasePagerThreadBlock->block();
      //
      // delete any children if required.
      //
//       if (_deleteRemovedSubgraphsInDatabaseThread)
      {
         osg::ref_ptr<osg::Object> obj = 0;
         {
            int deleteCount = 0;
            int deleteCountMax = 20;
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_childrenToDeleteListMutex);
            while (!_childrenToDeleteList.empty()&&(deleteCount < deleteCountMax))
            {
               //osg::notify(osg::NOTICE)<<"In DatabasePager thread deleting "<<_childrenToDeleteList.size()<<" objects"<<std::endl;
               //osg::Timer_t before = osg::Timer::instance()->tick();
               obj = _childrenToDeleteList.back();
               _childrenToDeleteList.pop_back();
               //osg::notify(osg::NOTICE)<<"Done DatabasePager thread deleted in "<<osg::Timer::instance()->delta_m(before,osg::Timer::instance()->tick())<<" ms"<<" objects"<<std::endl;
               ++deleteCount;
            }
            updateDatabasePagerThreadBlock();
         }
      }
      
      //
      // load any subgraphs that are required.
      //
      osg::ref_ptr<DatabaseRequest> databaseRequest;
      
      // get the front of the file request list.
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_fileRequestListMutex);
         if (!_fileRequestList.empty())
         {
            std::sort(_fileRequestList.begin(),_fileRequestList.end(),MySortFileRequestFunctor());
            databaseRequest = _fileRequestList.front();
         }
      }
      
      if (databaseRequest.valid())
      {
         // check if databaseRequest is still relevant
         if (_frameNumber-databaseRequest->_frameNumberLastRequest<=1)
         {
            
            // assume that we only have one DatabasePager, or that readNodeFile is thread safe...
            databaseRequest->_loadedModel = osgDB::readNodeFile(databaseRequest->_fileName,
                                                                databaseRequest->_loadOptions.get());
            
            //osg::notify(osg::NOTICE)<<"     node read in "<<osg::Timer::instance()->delta_m(before,osg::Timer::instance()->tick())<<" ms"<<std::endl;
            
            bool loadedObjectsNeedToBeCompiled = false;
            
            if (_doPreCompile && databaseRequest->_loadedModel.valid() && !_activeGraphicsContexts.empty())
            {
               // force a compute of the loaded model's bounding volume, so that when the subgraph
               // merged with the main scene graph and large computeBound() isn't incurred.
               databaseRequest->_loadedModel->getBound();
               
               
               ActiveGraphicsContexts::iterator itr = _activeGraphicsContexts.begin();
               
               DataToCompile& dtc = databaseRequest->_dataToCompileMap[*itr];
               ++itr;                
               
               // find all the compileable rendering objects
               MyFindCompileableGLObjectsVisitor frov(dtc, 
                                                      _changeAutoUnRef, _valueAutoUnRef,
                                                      _changeAnisotropy, _valueAnisotropy,
                                                      _drawablePolicy);
               
               databaseRequest->_loadedModel->accept(frov);
               
               if (!dtc.first.empty() || !dtc.second.empty())
               {
                  loadedObjectsNeedToBeCompiled = true;                
                  
                  // copy the objects from the compile list to the other graphics context list.
                  for(;
                      itr != _activeGraphicsContexts.end();
                      ++itr)
                  {
                     databaseRequest->_dataToCompileMap[*itr] = dtc;
                  }
               }
            }            
            
            // move the databaseRequest from the front of the fileRequest to the end of
            // dataToCompile or dataToMerge lists.
            {
               OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_fileRequestListMutex);
               
               DatabaseRequestList::iterator itr = std::find(_fileRequestList.begin(),_fileRequestList.end(),databaseRequest);
               if (itr != _fileRequestList.end()) 
               {
                  if (databaseRequest->_loadedModel.valid())
                  {
                     if (loadedObjectsNeedToBeCompiled)
                     {
                        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_dataToCompileListMutex);
			_dataToCompileList.push_back(databaseRequest);
                        notifyUpdateSceneGraph();
                     }
                     else
                     {
                        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_dataToMergeListMutex);
                        _dataToMergeList.push_back(databaseRequest);
			notifyUpdateSceneGraph();
                     }
                  }        
                  _fileRequestList.erase(itr);
               }
               
               updateDatabasePagerThreadBlock();
            }
            
            if (loadedObjectsNeedToBeCompiled)
            {
               for(ActiveGraphicsContexts::iterator itr = _activeGraphicsContexts.begin();
                   itr != _activeGraphicsContexts.end();
                   ++itr)
               {
                  osg::GraphicsContext* gc = osg::GraphicsContext::getCompileContext(*itr);
                  if (gc)
                  {   
                     osg::GraphicsThread* gt = gc->getGraphicsThread();
                     if (gt)
                     {
                        gt->add(new ossimPlanetDatabasePager::MyCompileOperation(this));
                     }
                     else
                     {
                        gc->makeCurrent();
                        
                        compileAllGLObjects(*(gc->getState()));
                        
                        gc->releaseContext();
                     }
                  }
               }
               
               // osg::notify(osg::NOTICE)<<"Done compiling in paging thread"<<std::endl;                   
            }
            
         }
         else
         {
            //std::cout<<"frame number delta for "<<databaseRequest->_fileName<<" "<<_frameNumber-databaseRequest->_frameNumberLastRequest<<std::endl;
            // remove the databaseRequest from the front of the fileRequest to the end of
            // dataLoad list as its is no longer relevant
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_fileRequestListMutex);
            
            if (!_fileRequestList.empty()) _fileRequestList.erase(_fileRequestList.begin());
            
            updateDatabasePagerThreadBlock();
            
         }
      }
      else
      {
      }
      // go to sleep till our the next time our thread gets scheduled.
      
      if (firstTime)
      {
         // do a yield to get round a peculiar thread hang when testCancel() is called 
         // in certain cirumstances - of which there is no particular pattern.
         YieldCurrentThread();
         firstTime = false;
      }
      
   } while (!testCancel() && !_done);
}

void ossimPlanetDatabasePager::clearRequests()
{
   DatabasePager::clear();
}

void ossimPlanetDatabasePager::updateSceneGraph(double currentFrameTime)
{
   osgDB::DatabasePager::updateSceneGraph(currentFrameTime);
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock1(_fileRequestListMutex);
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(_dataToCompileListMutex);
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock3(_dataToMergeListMutex);
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock4(_childrenToDeleteListMutex);
   if(_dataToMergeList.empty()&&
      _dataToCompileList.empty()&&
      _fileRequestList.empty()&&
      _childrenToDeleteList.empty())
   {
      notifyNoMoreWork();
   }
   updateDatabasePagerThreadBlock();
}

bool ossimPlanetDatabasePager::listsAreEmpty()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock1(_fileRequestListMutex);
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(_dataToCompileListMutex);
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock3(_dataToMergeListMutex);
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock4(_childrenToDeleteListMutex);

   return (_dataToMergeList.empty()&&
           _dataToCompileList.empty()&&
           _fileRequestList.empty()&&
           _childrenToDeleteList.empty());
}

void ossimPlanetDatabasePager::addCallback(osg::ref_ptr<ossimPlanetDatabasePager::Callback> callback)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   ossimPlanetDatabasePager::CallbackListType::iterator i = std::find(theCallbackList.begin(),
                                                             theCallbackList.end(),
                                                             callback);
   if(i == theCallbackList.end())
   {
      theCallbackList.push_back(callback);
   }
}

void ossimPlanetDatabasePager::removeCallback(osg::ref_ptr<ossimPlanetDatabasePager::Callback> callback)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   ossimPlanetDatabasePager::CallbackListType::iterator i = std::find(theCallbackList.begin(),
                                                             theCallbackList.end(),
                                                             callback);
   if(i != theCallbackList.end())
   {
      theCallbackList.erase(i);
   }
}

void ossimPlanetDatabasePager::notifyDoingWork()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theCallbackList.size(); ++idx)
   {
      theCallbackList[idx]->doingWork();
   }
}

void ossimPlanetDatabasePager::notifyNoMoreWork()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theCallbackList.size(); ++idx)
   {
      theCallbackList[idx]->noMoreWork();
   }
}

void ossimPlanetDatabasePager::notifyUpdateSceneGraph()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theCallbackList.size(); ++idx)
   {
      theCallbackList[idx]->updateSceneGraph();
   }
}
#endif
#endif