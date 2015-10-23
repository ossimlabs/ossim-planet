#ifndef ACTIONROUTER_H
#define ACTIONROUTER_H

// central router for routing Actions to all registered ActionRecievers

#include <assert.h>
#include <vector>
#include <map>
#include <queue>
#include "ossimPlanetActionReceiver.h"
#include "ossimPlanetNetworkConnection.h"
#include "ossimPlanetExport.h"
#include "ossimPlanetRefBlock.h"
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <ossimPlanet/ossimPlanetReentrantMutex.h>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Block>

class OSSIMPLANET_DLL ossimPlanetActionRouterThreadQueue : virtual public osg::Referenced,
                                                           public OpenThreads::Thread
{
public:
   ossimPlanetActionRouterThreadQueue()
      :theDoneFlag(false),
      theBlock(new ossimPlanetRefBlock)
   {
      
   }
   virtual ~ossimPlanetActionRouterThreadQueue()
   {
      if(isRunning())
      {
         cancel();
      }
   }
   
   virtual void run()
   {
      while(!theDoneFlag)
      {
         theBlock->block();
         /**
			* if we were released from the cancel then return immediately
          */ 
         if(theDoneFlag) return;
         osg::ref_ptr<ossimPlanetAction> a;
         theActionQueueMutex.lock();
         if(!theActionQueue.empty())
         {
            a = theActionQueue.front();
            theActionQueue.pop();
         }
			theActionQueueMutex.unlock();  
			if(a.valid())
			{
				a->execute();
			}
         updateThreadBlock();
         OpenThreads::Thread::YieldCurrentThread();
      }
   }
   virtual int cancel()
   {
      int result = 0;
      
      if( isRunning() )
      {
         theDoneFlag = true;
         theBlock->release();
         
         while(isRunning())
         {
            OpenThreads::Thread::YieldCurrentThread();
         }
      }
      return result;
   }
   void updateThreadBlock()
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theActionQueueMutex);
      theBlock->set(!theActionQueue.empty());
   }
   void setDoneFlag(bool flag)
   {
      theDoneFlag = flag;
   }
   void addAction(const ossimPlanetAction& action)
   {
      theActionQueueMutex.lock();
      theActionQueue.push(action.clone());
      theActionQueueMutex.unlock();
      updateThreadBlock();
   }
protected:
   bool     theDoneFlag;
   osg::ref_ptr<ossimPlanetRefBlock> theBlock;
   mutable ossimPlanetReentrantMutex    theActionQueueMutex;
   std::queue<osg::ref_ptr<ossimPlanetAction> > theActionQueue;
};

class OSSIMPLANET_DLL ossimPlanetActionRouter : public ossimPlanetActionReceiver
{
public:
   typedef std::map<std::string, ossimPlanetActionReceiver*> MapType;
    // initialize/shutdown
    
	static ossimPlanetActionRouter* instance();
	    // pointer to the lazy initialized ActionRouter singleton
	    // assert(instance() != NULL)
	
	static void shutdown();
	    // clean up the singleton

        // network
        
        void addNetworkConnection(ossimPlanetNetworkConnection* commLink);
            // add an external communication interface to be used
            // require(commLink != NULL && commLink->error().empty())
        
        void removeNetworkConnection(const ossimString& name);
            // remove the named external communication interface from use
            //    NB: this routine will not call delete on the removed connection!
        
        ossimPlanetNetworkConnection* networkConnection(const ossimString& name) const;
            // return the NetworkConnection with the given name or NULL if nonexistent
        
        // federate name
        
        const ossimString& federateName() const
            // the name of this federate as seen from the network
            { return federateName_; }
        
        void setFederateName(const ossimString& newName);
            // set the network name of this federate
            // require(!newName.empty())
            // require(newName.find(' ', 0) <= 0)
            // ensure(newName == federateName())

        /**
         * This will post to the thread queue the action to route and route it later
         */ 
        void post(const ossimPlanetAction& a);
        
        /**
         * Routes the Action immediately.
         */ 
	void route(const ossimPlanetAction& a);
	    // cause the action's target to execute() the Action on the local machine 
    
	void allRoute(const ossimPlanetAction& a);
	    // cause the action's target to execute() the Action on
	    //    the local machine as well as all machines in the
	    //    distributed sim.
	
	void tellRoute(const ossimPlanetAction& a, const ossimString& destination);
	    // cause the action's target to execute() the Action on
	    //    the the destination machine
	    // require(!destination.empty())
	
	void executeFile(const ossimString& filename, const ossimString& origin = ossimPlanetActionRouter::instance()->federateName());
	    // sequentially call execute() on each Action in the file,
	    //    with all Actions having origin as their origin.
	
	void executeNetworkActions();
	    // call execute() on pending Actions on all NetworkConnections
        
        // ActionReceiver management
    
	void registerReceiver(ossimPlanetActionReceiver* r);
	    // r should receive actions
	    // assert(r != NULL)
	    // assert(receiver(r->pathName()) == r)
	
	void unregisterReceiver(ossimPlanetActionReceiver* r);
	    // r should no longer receive actions
	    // assert(r != NULL)
	    // assert(receiver(r->pathname()) != r)
	
	ossimPlanetActionReceiver* receiver(const ossimString& receiverPathname) const;
	    // the object with the specified pathname
	    // assert(result == NULL || result->pathName() == receiverPathname)
	
	void printReceivers();
	    // print the list of receivers

    // ActionReceiver features

	void execute(const ossimPlanetAction& a);
	    // execute the given action 
	
protected:
    
    // hide these to enforce singleton
    ossimPlanetActionRouter();
    ~ossimPlanetActionRouter();
    ossimPlanetActionRouter(const ossimPlanetActionRouter &other):ossimPlanetActionReceiver(other) { assert(false);}
    ossimPlanetActionRouter& operator=(const ossimPlanetActionRouter &) { assert(false); return *this;}
    
    static ossimPlanetActionRouter* instance_;
    osg::ref_ptr<ossimPlanetActionRouterThreadQueue> theThreadQueue;
    // our singleton instance
    mutable ossimPlanetReentrantMutex theReceiverMutex;
   MapType receivers_;
    std::vector<ossimPlanetNetworkConnection*> network_;
	// receiver name/object pairs
    
    ossimString federateName_;
        // this federate's name
    
    void remoteRouteImplementation(const ossimPlanetAction& a, const ossimString& destination);
	// implementation for allRoute() and tellRoute()

    // STL equality predicate used to find a NetworkConnection by name
    class NCEqualPred
    {
    public:
       NCEqualPred(const ossimString& n) : targetName_(n) {}
          const ossimString& targetName_;
	bool operator()(const ossimPlanetNetworkConnection* i) const
        { return i->name() == targetName_.string(); }
    };
};

inline ossimPlanetActionRouter* ossimPlanetActionRouter::instance()
{
    if (instance_ == 0)
    {
	    instance_ = new ossimPlanetActionRouter;
        instance_->theThreadQueue = new ossimPlanetActionRouterThreadQueue;
        instance_->theThreadQueue->setDoneFlag(false);
        instance_->theThreadQueue->start();
    }
/*     assert(instance_ != NULL); */
    return instance_;
}

inline void ossimPlanetActionRouter::shutdown()
{
   if(instance_)
   {
      delete instance_;
      instance_ = 0;
   }
}

#endif
