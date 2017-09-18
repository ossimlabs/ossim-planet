#ifndef ossimPlanetOperation_HEADER
#define ossimPlanetOperation_HEADER
#include <OpenThreads/Thread>
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <vector>
#include <list>
#include <algorithm>
#include <ossimPlanet/ossimPlanetCallback.h>
#include <ossimPlanet/ossimPlanetOperation.h>
#include <ossimPlanet/ossimPlanetRefBlock.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossim/base/ossimString.h>
#include <iostream>
#include <mutex>

class ossimPlanetOperation;
class ossimPlanetOperationThreadQueue;
class ossimPlanetOperationMultiThreadQueue;
class ossimPlanetOperationQueue;
class OSSIMPLANET_DLL ossimPlanetOperationCallback : public ossimPlanetCallback
{
public:
	ossimPlanetOperationCallback(){}
	virtual void ready(ossimPlanetOperation* /*operation*/){}
	virtual void started(ossimPlanetOperation* /*operation*/){}
	virtual void finished(ossimPlanetOperation* /*operation*/){}
	virtual void canceled(ossimPlanetOperation* /*operation*/){}
   virtual void priorityChanged(ossimPlanetOperation* /*operation*/){}
   virtual void propertyChanged(const ossimString& /*name*/,
                                ossimPlanetOperation* /*operation*/){}
	
};


class OSSIMPLANET_DLL ossimPlanetOperation : public osg::Referenced,
                                             public ossimPlanetCallbackListInterface<ossimPlanetOperationCallback>
{
public:
	friend class ossimPlanetOperationThreadQueue;
	friend class ossimPlanetOperationMultiThreadQueue;
	friend class ossimPlanetOperationQueue;
   typedef double Priority;

   enum StateType
	{
		READY_STATE = 0, // Specifies that the operation is ready to run
		RUN_STATE = 1,   // Specifies that the operation is currently running
		CANCELED_STATE = 4, // Specifies that the operation was canceled
		FINISHED_STATE = 8 // The operation ran through to completion
	};
   static const ossimPlanetOperation::Priority PRIORITY_HIGHEST;
   static const ossimPlanetOperation::Priority PRIORITY_LOWEST;
   typedef std::list<osg::ref_ptr<ossimPlanetOperation> > List;
   ossimPlanetOperation(const ossimString& name="", const ossimString id="", ossimPlanetOperation::Priority queuePriority=0)
   :theName(name),
   theId(id),
	thePriority(queuePriority),
	theState(READY_STATE)
   {
      setThreadSafeRefUnref(true);
   }
   virtual ~ossimPlanetOperation()
   {
#if 0
		if((state() == RUN_STATE)||
         (state() == READY_STATE))
		{
			cancel();
		}
#endif
   }
   bool operator < (const ossimPlanetOperation& rhs)const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return (thePriority < rhs.thePriority);
   }
   bool operator < (const osg::ref_ptr<ossimPlanetOperation> rhs)const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return (thePriority < rhs->thePriority);
   }
   bool operator < (const ossimPlanetOperation* rhs)const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return (thePriority < rhs->thePriority);
   }
   bool operator > (const ossimPlanetOperation& rhs)const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return (thePriority > rhs.thePriority);
   }
   bool operator > (const ossimPlanetOperation* rhs)const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return (thePriority > rhs->thePriority);
   }
   bool operator > (const osg::ref_ptr<ossimPlanetOperation> rhs)const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return (thePriority > rhs->thePriority);
   }
   void setName(const ossimString& name)
   {
      {
         std::lock_guard<std::mutex> lock(thePropertyMutex);
         theName = name;
      }
      notifyPropertyChanged("name");
   }
	
   const ossimString& name()const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return theName;
   }
   void setId(const ossimString& id)
   {
      {
         std::lock_guard<std::mutex> lock(thePropertyMutex);
         theId = id;
      }
      notifyPropertyChanged("id");
   }
	const ossimString& id()const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return theId;
   }
	void setState(StateType state)
	{
		{
			std::lock_guard<std::mutex> lock(thePropertyMutex);
			theState = state;
		}
		switch(state)
		{
			case READY_STATE:
			{
            notifyReady();
				break;
			}
			case RUN_STATE:
			{
				notifyStarted();
				break;
			}
			case CANCELED_STATE:
			{
				notifyCanceled();
				break;
			}
			case FINISHED_STATE:
			{
				notifyFinished();
				break;
			}
		}
	}
   bool isStopped()const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return ((theState == CANCELED_STATE)||
              (theState == FINISHED_STATE));
   }
	virtual StateType state()const
	{
      std::lock_guard<std::mutex> lock(thePropertyMutex);
		return theState;
	}
   virtual void release(){}
   void addDependency(ossimPlanetOperation* operation)
   {
      std::lock_guard<std::mutex> lock(theDependencyListMutex);
      List::iterator iter = std::find(theDependencyList.begin(),
                                      theDependencyList.end(),
                                      operation);
      if(iter==theDependencyList.end())
      {
         theDependencyList.push_back(operation);
      }
   }
   void removeDependency(ossimPlanetOperation* operation)
   {
      std::lock_guard<std::mutex> lock(theDependencyListMutex);
      List::iterator iter = std::find(theDependencyList.begin(),
                                      theDependencyList.end(),
                                      operation);
      if(iter!=theDependencyList.end())
      {
         theDependencyList.erase(iter);
      }
   }
   bool hasDependency()const
   {
      std::lock_guard<std::mutex> lock(theDependencyListMutex);
      return (theDependencyList.size() > 0);
   }
   ossimPlanetOperation::List& dependencyList()
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return theDependencyList;
   }
   const ossimPlanetOperation::List& dependencyList()const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return theDependencyList;
   }
   void setPriority(ossimPlanetOperation::Priority priority)
   {
      {
         std::lock_guard<std::mutex> lock(thePropertyMutex);
         thePriority = priority;
      }
      notifyPriorityChanged();
   }
   virtual ossimPlanetOperation::Priority priority()const
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      return thePriority;
   }
   virtual void status(ossimString& result)
   {
      std::lock_guard<std::mutex> lock(thePropertyMutex);
      result = theStatus;
   }
   virtual void setStatus(const ossimString& value)
   {
      {
         std::lock_guard<std::mutex> lock(thePropertyMutex);
         theStatus = value;
      }
      notifyPropertyChanged("status");
   }
   /**
    * Since you can have nested dependencies it will go until no more dependencies depend
    * on other dependencies if the recures flag is true.  If false it will just return the 
    * first one in the list.
    */
   osg::ref_ptr<ossimPlanetOperation> nextDependency(bool recurseFlag=true);
	virtual void cancel()
	{
		setState(CANCELED_STATE);
	}
   std::mutex& runMutex(){return theRunMutex;}
   virtual void reset()
   {
		setState(READY_STATE);
   }
	virtual void start()
	{
      std::lock_guard<std::mutex> lock(theRunMutex);
		setState(RUN_STATE);
		run();
		if(state() == RUN_STATE) // if it wasn't canceled or set to some other state then say it has finished
		{
			setState(FINISHED_STATE);
		}
	}
protected:
	void notifyReady();
	void notifyStarted();
	void notifyFinished();
	void notifyCanceled();
	void notifyPriorityChanged();
	void notifyPropertyChanged(const ossimString& name);
   virtual void run()=0;
	
   mutable std::mutex theRunMutex;
   mutable std::mutex thePropertyMutex;
   bool theFinishedFlag;
   bool theCanceledFlag;
   mutable std::mutex theDependencyListMutex;
   ossimPlanetOperation::List theDependencyList;
   ossimString theName;
   ossimString theId;
   ossimString theStatus;
   ossimPlanetOperation::Priority thePriority;
	StateType theState;
};

                                        
class OSSIMPLANET_DLL ossimPlanetOperationQueue : public osg::Referenced
{
public:
   ossimPlanetOperationQueue();
   virtual ~ossimPlanetOperationQueue();
   virtual void add(ossimPlanetOperation* operation, bool guaranteeUniqueFlag=true);
   osg::ref_ptr<ossimPlanetOperation> removeByName(const ossimString& name);
   osg::ref_ptr<ossimPlanetOperation> removeById(const ossimString& id);
   void remove(const ossimPlanetOperation* operation);
   void removeStoppedOperations();
   void removeAllOperations();
   
   virtual osg::ref_ptr<ossimPlanetOperation> nextOperation(bool blockIfEmptyFlag=true);
   void releaseOperationsBlock();
   bool empty()const;
   ossim_uint32 size()
   {
      std::lock_guard<std::mutex> lock(theOperationQueueMutex);
      return theOperationQueue.size();
   }
protected:
   ossimPlanetOperation::List::iterator findById(const ossimString& name);
   ossimPlanetOperation::List::iterator findByName(const ossimString& name);
   ossimPlanetOperation::List::iterator findByPointer(const ossimPlanetOperation* operation);
   ossimPlanetOperation::List::iterator findByNameOrPointer(const ossimPlanetOperation* operation);
   bool hasOperation(ossimPlanetOperation* operation);
   osg::ref_ptr<ossimPlanetRefBlock> theBlock;
  
   mutable std::mutex theOperationQueueMutex;
   ossimPlanetOperation::List theOperationQueue;
};

class OSSIMPLANET_DLL ossimPlanetOperationPriorityQueue : public ossimPlanetOperationQueue
{
public:
   ossimPlanetOperationPriorityQueue();
   virtual ~ossimPlanetOperationPriorityQueue();
   
   virtual osg::ref_ptr<ossimPlanetOperation> nextOperation(bool blockIfEmptyFlag=true);
protected:
   
};

class OSSIMPLANET_DLL ossimPlanetOperationThreadQueue : public osg::Referenced, 
                                                        public OpenThreads::Thread
{
public:
   ossimPlanetOperationThreadQueue(ossimPlanetOperationQueue* opq=0);
   
   /** Set the OperationQueue. */
   void setOperationQueue(ossimPlanetOperationQueue* opq);
   
   /** Get the OperationQueue. */
   ossimPlanetOperationQueue* operationQueue() { return theOperationQueue.get(); }
   
   /** Get the const OperationQueue. */
   const ossimPlanetOperationQueue* operationQueue() const { return theOperationQueue.get(); }
   
   
   /** Add operation to end of OperationQueue, this will be 
    * executed by the graphics thread once this operation gets to the head of the queue.*/
   void add(ossimPlanetOperation* operation, bool guaranteeUniqueFlag=true);
   
   /** Remove operation from OperationQueue.*/
   void remove(ossimPlanetOperation* operation);
   
   /** Remove named operation from OperationQueue.*/
   osg::ref_ptr<ossimPlanetOperation> removeByName(const ossimString& name);
   
   /** Remove id operation from OperationQueue.*/
   osg::ref_ptr<ossimPlanetOperation> removeById(const ossimString& id);
   
   /** Remove all operations from OperationQueue.*/
   void removeAllOperations();
   
   
   /** Get the operation currently being run.*/
   osg::ref_ptr<ossimPlanetOperation> currentOperation() { return theCurrentOperation; }
   
	void cancelCurrentOperation();
   /** Run does the opertion thread run loop.*/
   virtual void run();
   
   void setDone(bool done);
   
   bool done() const { return theDoneFlag; }
   
   /** Cancel this graphics thread.*/        
   virtual int cancel();
   bool empty()const
   {
      std::lock_guard<std::recursive_mutex> lock(theThreadMutex);
      return theOperationQueue->empty();
   }
protected:
   virtual ~ossimPlanetOperationThreadQueue();
   virtual osg::ref_ptr<ossimPlanetOperation> nextOperation();
   
   bool                                    theDoneFlag;
   mutable std::recursive_mutex            theThreadMutex;
   osg::ref_ptr<ossimPlanetOperationQueue> theOperationQueue;
   osg::ref_ptr<ossimPlanetOperation>      theCurrentOperation;
   
};

class OSSIMPLANET_DLL ossimPlanetOperationMultiThreadQueue : public osg::Referenced
{
public:
	typedef std::vector<osg::ref_ptr<ossimPlanetOperationThreadQueue> > ThreadQueueList;
	
	ossimPlanetOperationMultiThreadQueue(ossim_uint32 numberOfThreads=1);
	virtual ~ossimPlanetOperationMultiThreadQueue();
	/** Get the OperationQueue. */
   ossimPlanetOperationQueue* operationQueue();
   
   /** Get the const OperationQueue. */
   const ossimPlanetOperationQueue* operationQueue() const;	
   void add(ossimPlanetOperation* operation, bool guaranteeUniqueFlag=true);
   
   /** Remove operation from OperationQueue.*/
   void remove(ossimPlanetOperation* operation);
   
   /** Remove named operation from OperationQueue.*/
   osg::ref_ptr<ossimPlanetOperation> removeByName(const ossimString& name);
   osg::ref_ptr<ossimPlanetOperation> removeById(const ossimString& id);
   
   /** Remove all operations from OperationQueue.*/
   void removeAllOperations();
	void cancelCurrentOperation();
protected:
   std::recursive_mutex                    theThreadMutex;
   osg::ref_ptr<ossimPlanetOperationQueue> theOperationQueue;
	ThreadQueueList theThreadQueueList;
};
#endif
