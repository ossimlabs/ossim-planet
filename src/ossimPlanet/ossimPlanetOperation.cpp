#include <ossimPlanet/ossimPlanetOperation.h>
#include <iostream>
#include <algorithm>
const ossimPlanetOperation::Priority ossimPlanetOperation::PRIORITY_HIGHEST = 1.0/DBL_EPSILON;
const ossimPlanetOperation::Priority ossimPlanetOperation::PRIORITY_LOWEST = 0.0;

osg::ref_ptr<ossimPlanetOperation> ossimPlanetOperation::nextDependency(bool recurseFlag)
{
   osg::ref_ptr<ossimPlanetOperation> result;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theDependencyListMutex);
   if(!recurseFlag)
   {
      if(theDependencyList.size()>0)
      {
         result = *theDependencyList.begin();
         theDependencyList.erase(theDependencyList.begin());
      }
   }
   else if(theDependencyList.size()>0)

   {
      
      osg::ref_ptr<ossimPlanetOperation> prevOperation = (*theDependencyList.begin()).get();
      osg::ref_ptr<ossimPlanetOperation> currentOperation = prevOperation;
      
      if(currentOperation.valid())
      {
         while(currentOperation->hasDependency())
         {
            prevOperation = currentOperation;
            currentOperation = (*currentOperation->dependencyList().begin()).get();
         }
         result = currentOperation;
         prevOperation->dependencyList().erase(prevOperation->dependencyList().begin());
      }
   }
   
   return result;
}

void ossimPlanetOperation::notifyReady()
{	
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx;
   for(idx =0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->ready(this);
      }
   }
}
void ossimPlanetOperation::notifyStarted()
{	
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx;
   for(idx =0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->started(this);
      }
   }
}

void ossimPlanetOperation::notifyFinished()
{	
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx;
   for(idx =0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->finished(this);
      }
   }
}


void ossimPlanetOperation::notifyCanceled()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx;
   for(idx =0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->canceled(this);
      }
   }
}

void ossimPlanetOperation::notifyPriorityChanged()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx;
   for(idx =0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->priorityChanged(this);
      }
   }
}

void ossimPlanetOperation::notifyPropertyChanged(const ossimString& name)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theCallbackListMutex);
   if(theBlockCallbacksFlag) return;
   ossim_uint32 idx;
   for(idx =0; idx < theCallbackList.size(); ++idx)
   {
      if(theCallbackList[idx]->enableFlag())
      {
         theCallbackList[idx]->propertyChanged(name, this);
      }
   }
}

ossimPlanetOperationQueue::ossimPlanetOperationQueue()
{
   setThreadSafeRefUnref(true);
   theBlock = new ossimPlanetRefBlock;
}

ossimPlanetOperationQueue::~ossimPlanetOperationQueue()
{
}

void ossimPlanetOperationQueue::add(ossimPlanetOperation* operation, bool guaranteeUniqueFlag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
   
   if(guaranteeUniqueFlag)
   {
      if(findByPointer(operation) != theOperationQueue.end())
      {
         theBlock->set(true);
         return;
      }
   }
   theOperationQueue.push_back(operation);
   
   theBlock->set(true);
}

void ossimPlanetOperationQueue::removeStoppedOperations()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
   ossimPlanetOperation::List::iterator iter = theOperationQueue.begin();
   while(iter!=theOperationQueue.end())
   {
      if((*iter)->isStopped())
      {
         iter = theOperationQueue.erase(iter);
      }
      else
      {
         ++iter;
      }
   }
}

osg::ref_ptr<ossimPlanetOperation> ossimPlanetOperationQueue::removeById(const ossimString& id)
{   
   osg::ref_ptr<ossimPlanetOperation> result;
   if(id.empty()) return result;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
   ossimPlanetOperation::List::iterator iter = findById(id);
   if(iter!=theOperationQueue.end())
   {
      result = *iter;
      theOperationQueue.erase(iter);
   }
   
   theBlock->set(!theOperationQueue.empty());
   
   return result;
}

osg::ref_ptr<ossimPlanetOperation> ossimPlanetOperationQueue::removeByName(const ossimString& name)
{   
   osg::ref_ptr<ossimPlanetOperation> result;
   if(name.empty()) return result;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
   ossimPlanetOperation::List::iterator iter = findByName(name);
   if(iter!=theOperationQueue.end())
   {
      result = *iter;
      theOperationQueue.erase(iter);
   }
   
   theBlock->set(!theOperationQueue.empty());
   
   return result;
}

void ossimPlanetOperationQueue::remove(const ossimPlanetOperation* operation)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
   ossimPlanetOperation::List::iterator iter = findByPointer(operation);
   if(iter!=theOperationQueue.end())
   {
      theOperationQueue.erase(iter);
   }
   
   theBlock->set(!theOperationQueue.empty());
}

void ossimPlanetOperationQueue::removeAllOperations()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
   theOperationQueue.clear();
   theBlock->set(false);
}
osg::ref_ptr<ossimPlanetOperation> ossimPlanetOperationQueue::nextOperation(bool blockIfEmptyFlag)
{
	theOperationQueueMutex.lock();
	bool emptyFlag = theOperationQueue.empty();
	theOperationQueueMutex.unlock();
   if (blockIfEmptyFlag && emptyFlag)
   {
      theBlock->block();
   }
   osg::ref_ptr<ossimPlanetOperation> result;
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
   
   if (theOperationQueue.empty())
   {
      theBlock->set(false);
      return result;
   }
   
   ossimPlanetOperation::List::iterator iter= theOperationQueue.begin();
   while((iter != theOperationQueue.end())&&
         (((*iter)->isStopped())))
   {
      iter = theOperationQueue.erase(iter);
   }
   if(iter != theOperationQueue.end())
   {
      result = *iter;
      theOperationQueue.erase(iter);
   }
   theBlock->set(!theOperationQueue.empty());
   return result;
}

ossimPlanetOperation::List::iterator ossimPlanetOperationQueue::findById(const ossimString& id)
{
   if(id.empty()) return theOperationQueue.end();
   ossimPlanetOperation::List::iterator iter = theOperationQueue.begin();
   while(iter != theOperationQueue.end())
   {
      if(id == (*iter)->id())
      {
         return iter;
      }
      ++iter;
   }  
   return theOperationQueue.end();
}

ossimPlanetOperation::List::iterator ossimPlanetOperationQueue::findByName(const ossimString& name)
{
   if(name.empty()) return theOperationQueue.end();
   ossimPlanetOperation::List::iterator iter = theOperationQueue.begin();
   while(iter != theOperationQueue.end())
   {
      if(name == (*iter)->name())
      {
         return iter;
      }
      ++iter;
   }  
   return theOperationQueue.end();
}

ossimPlanetOperation::List::iterator ossimPlanetOperationQueue::findByPointer(const ossimPlanetOperation* operation)
{
   return std::find(theOperationQueue.begin(),
                    theOperationQueue.end(),
                    operation);
}

ossimPlanetOperation::List::iterator ossimPlanetOperationQueue::findByNameOrPointer(const ossimPlanetOperation* operation)
{
   ossimString n = operation->name();
   ossimPlanetOperation::List::iterator iter = theOperationQueue.begin();
   while(iter != theOperationQueue.end())
   {
      if((*iter).get() == operation)
      {
         return iter;
      }
      else if((!n.empty())&&
              (operation->name() == (*iter)->name()))
      {
         return iter;
      }
      ++iter;
   }  
   
   return theOperationQueue.end();
}

bool ossimPlanetOperationQueue::hasOperation(ossimPlanetOperation* operation)
{
   ossimPlanetOperation::List::const_iterator iter = theOperationQueue.begin();
   while(iter != theOperationQueue.end())
   {
      if(operation == (*iter).get())
      {
         return true;
      }
      ++iter;
   }

   return false;
}


void ossimPlanetOperationQueue::releaseOperationsBlock()
{
   theBlock->release();
}

bool ossimPlanetOperationQueue::empty()const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
   return theOperationQueue.empty();
}


struct SortRequestFunctor
{
   bool operator() (const osg::ref_ptr<ossimPlanetOperation>& lhs,
                    const osg::ref_ptr<ossimPlanetOperation>& rhs) const
   {
      return (lhs->priority()>rhs->priority());
   }
};

ossimPlanetOperationPriorityQueue::ossimPlanetOperationPriorityQueue()
{
}

ossimPlanetOperationPriorityQueue::~ossimPlanetOperationPriorityQueue()
{
   
}

osg::ref_ptr<ossimPlanetOperation> ossimPlanetOperationPriorityQueue::nextOperation(bool blockIfEmptyFlag)
{
   // we will sort the queue and let the base pop the first element and process
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theOperationQueueMutex);
      
      theOperationQueue.sort(SortRequestFunctor());
   }
   
   return ossimPlanetOperationQueue::nextOperation(blockIfEmptyFlag);
}

ossimPlanetOperationThreadQueue::ossimPlanetOperationThreadQueue(ossimPlanetOperationQueue* opq)
: osg::Referenced(true),
   theDoneFlag(false),
   theOperationQueue(opq)
{
   if(!theOperationQueue.valid())
   {
      theOperationQueue = new ossimPlanetOperationQueue();
   }
}

ossimPlanetOperationThreadQueue::~ossimPlanetOperationThreadQueue()
{
   cancel();
}

void ossimPlanetOperationThreadQueue::setOperationQueue(ossimPlanetOperationQueue* opq)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
   
   if (theOperationQueue == opq) return;
   
   theOperationQueue = opq;
}

void ossimPlanetOperationThreadQueue::setDone(bool done)
{
   if (theDoneFlag==done) return;
   
   theDoneFlag = done;
   
   if(theDoneFlag)
   {
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
         if (theCurrentOperation.valid())
         {
            theCurrentOperation->release();
         }
      }
      
      if (theOperationQueue.valid()) theOperationQueue->releaseOperationsBlock();
   }
}


int ossimPlanetOperationThreadQueue::cancel()
{
   int result = 0;
   
   removeAllOperations();

   if( isRunning() )
   {
      
      theDoneFlag = true;
      
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
			if (theCurrentOperation.valid())
			{
				theCurrentOperation->cancel();
			}
         
         if (theOperationQueue.valid()) 
         {
            theOperationQueue->releaseOperationsBlock();
         }
         
         if (theOperationQueue.valid()) theOperationQueue->releaseOperationsBlock();
      }
      
      // then wait for the the thread to stop running.
      while(isRunning())
      {
         
#if 1
         {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
            
            if (theOperationQueue.valid()) 
            {
               theOperationQueue->releaseOperationsBlock();
               // _operationQueue->releaseAllOperations();
            }
            
            if (theOperationQueue.valid()) theOperationQueue->releaseOperationsBlock();
         }
#endif
         OpenThreads::Thread::YieldCurrentThread();
      }
   }
   
   return result;
}

void ossimPlanetOperationThreadQueue::add(ossimPlanetOperation* operation, bool guaranteeUniqueFlag)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
   osg::ref_ptr<ossimPlanetOperation> op = operation;
   if (!theOperationQueue) return;
   theOperationQueue->add(op.get(), guaranteeUniqueFlag);
   if(!isRunning())
   {
      start();
      while(!isRunning())
      {
         OpenThreads::Thread::YieldCurrentThread();
      }
   }
}

void ossimPlanetOperationThreadQueue::remove(ossimPlanetOperation* operation)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
   if (theOperationQueue.valid()) theOperationQueue->remove(operation);
}

osg::ref_ptr<ossimPlanetOperation> ossimPlanetOperationThreadQueue::removeByName(const ossimString& name)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
   if (theOperationQueue.valid()) return theOperationQueue->removeByName(name);
   
   return 0;
}

osg::ref_ptr<ossimPlanetOperation> ossimPlanetOperationThreadQueue::removeById(const ossimString& id)
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
   if (theOperationQueue.valid()) return theOperationQueue->removeById(id);
   
   return 0;
}

void ossimPlanetOperationThreadQueue::removeAllOperations()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
   if (theOperationQueue.valid()) 
	{
		theOperationQueue->removeAllOperations();
	}
}

void ossimPlanetOperationThreadQueue::cancelCurrentOperation()
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
	if(theCurrentOperation.valid())
	{
		theCurrentOperation->cancel();
	}
}

void ossimPlanetOperationThreadQueue::run()
{
   bool firstTime = true;
   
   do
   {
      // osg::notify(osg::NOTICE)<<"In thread loop "<<this<<std::endl;
      osg::ref_ptr<ossimPlanetOperation> operation;
      osg::ref_ptr<ossimPlanetOperationQueue> queue;
      
      {
         OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
         queue = theOperationQueue;
      }
      
      operation = nextOperation();
      
      if (theDoneFlag) break;
      
      if (operation.valid())
      {
         {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
            theCurrentOperation = operation;
         }
         
         while(operation->hasDependency())
         {
            osg::ref_ptr<ossimPlanetOperation> dependency = operation->nextDependency();
				if(!operation->isStopped())
				{
					dependency->start();
				}
         }
			if(!operation->isStopped())
			{
				operation->start();
			}
         {            
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
            theCurrentOperation = 0;
         }
      }
      
      if (firstTime)
      {
         // do a yield to get round a peculiar thread hang when testCancel() is called 
         // in certain cirumstances - of which there is no particular pattern.
         YieldCurrentThread();
         firstTime = false;
      }
   } while (!testCancel() && !theDoneFlag);
}

osg::ref_ptr<ossimPlanetOperation> ossimPlanetOperationThreadQueue::nextOperation()
{
   return theOperationQueue->nextOperation(true);
}

ossimPlanetOperationMultiThreadQueue::ossimPlanetOperationMultiThreadQueue(ossim_uint32 numberOfThreads)
:theOperationQueue(new ossimPlanetOperationQueue)
{
	ossim_uint32 idx = 0;
	for(idx = 0; idx < numberOfThreads;++idx)
	{
		ossimPlanetOperationThreadQueue* threadQueue = new ossimPlanetOperationThreadQueue(theOperationQueue.get());
		threadQueue->start();
		theThreadQueueList.push_back(threadQueue);
	}
}

ossimPlanetOperationMultiThreadQueue::~ossimPlanetOperationMultiThreadQueue()
{
	removeAllOperations();
	cancelCurrentOperation();
	ossim_uint32 idx = 0;
	for(idx = 0; idx < theThreadQueueList.size();++idx)
	{
		theThreadQueueList[idx]->cancel();
	}
	theThreadQueueList.clear();
}

ossimPlanetOperationQueue* ossimPlanetOperationMultiThreadQueue::operationQueue() 
{ 
	return theOperationQueue.get(); 
}

/** Get the const OperationQueue. */
const ossimPlanetOperationQueue* ossimPlanetOperationMultiThreadQueue::operationQueue() const 
{ 
	return theOperationQueue.get(); 
}

void ossimPlanetOperationMultiThreadQueue::add(ossimPlanetOperation* operation, bool guaranteeUniqueFlag)
{
	theOperationQueue->add(operation, guaranteeUniqueFlag);
}

/** Remove operation from OperationQueue.*/
void ossimPlanetOperationMultiThreadQueue::remove(ossimPlanetOperation* operation)
{
	theOperationQueue->remove(operation);
}

/** Remove named operation from OperationQueue.*/
osg::ref_ptr<ossimPlanetOperation> ossimPlanetOperationMultiThreadQueue::removeByName(const ossimString& name)
{
	return theOperationQueue->removeByName(name);
}
/** Remove named operation from OperationQueue.*/
osg::ref_ptr<ossimPlanetOperation> ossimPlanetOperationMultiThreadQueue::removeById(const ossimString& id)
{
	return theOperationQueue->removeById(id);
}

/** Remove all operations from OperationQueue.*/
void ossimPlanetOperationMultiThreadQueue::removeAllOperations()
{
	theOperationQueue->removeAllOperations();
}

void ossimPlanetOperationMultiThreadQueue::cancelCurrentOperation()
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theThreadMutex);
	ossim_uint32 idx = 0;
	for(idx = 0; idx < theThreadQueueList.size();++idx)
	{
		theThreadQueueList[idx]->cancelCurrentOperation();
	}
}
