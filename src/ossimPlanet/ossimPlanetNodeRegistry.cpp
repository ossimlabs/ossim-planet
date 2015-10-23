#include <ossimPlanet/ossimPlanetNodeRegistry.h>
#include <ossimPlanet/ossimPlanetNodeFactory.h>
#include <algorithm>
ossimPlanetNodeRegistry* ossimPlanetNodeRegistry::theInstance = 0;
ossim_uint32 ossimPlanetNodeRegistry::theInitializeCount           = 0;


ossimPlanetNodeRegistry::ossimPlanetNodeRegistry()
{
	theInstance = this;
}
ossimPlanetNodeRegistry::~ossimPlanetNodeRegistry()
{
	theInstance = 0;
}

ossimPlanetNodeRegistry* ossimPlanetNodeRegistry::instance()
{
	if(!theInstance)
	{
		theInstance = new ossimPlanetNodeRegistry;
		theInstance->registerFactory(ossimPlanetNodeFactory::instance());
	}
	
	return theInstance;
}

void ossimPlanetNodeRegistry::finalize()
{	
}

ossimPlanetNode* ossimPlanetNodeRegistry::create(const ossimString& type)const
{
	ossimPlanetNode* layerNode = 0;
   theFactoryListMutex.readLock();
	FactoryListType::const_iterator iter = theFactoryList.begin();
	while((iter != theFactoryList.end())&&(!layerNode))
	{
		layerNode = (*iter)->create(type);
		++iter;
	}
   theFactoryListMutex.readUnlock();
	return layerNode;
}

bool ossimPlanetNodeRegistry::hasFactory(const ossimPlanetNodeFactoryBase* factory)const
{
	FactoryListType::const_iterator iter = std::find(theFactoryList.begin(),
																	 theFactoryList.end(), 
																	 factory);
	return (iter != theFactoryList.end());
}

void ossimPlanetNodeRegistry::registerFactory(ossimPlanetNodeFactoryBase* factory,
																	bool insertFrontFlag)
{
	if(!hasFactory(factory))
	{
      theFactoryListMutex.writeLock();
		if(!insertFrontFlag)
		{
			theFactoryList.push_back(factory);
		}
		else
		{
			theFactoryList.insert(theFactoryList.begin(),
										 factory);
		}
      theFactoryListMutex.writeUnlock();
	}
}

void ossimPlanetNodeRegistry::unregisterFactory(const ossimPlanetNodeFactoryBase* factory)
{
   theFactoryListMutex.writeLock();
	FactoryListType::iterator iter = std::find(theFactoryList.begin(),
															 theFactoryList.end(), 
															 factory);
	if(iter!=theFactoryList.end())
	{
		theFactoryList.erase(iter);
	}
   theFactoryListMutex.writeUnlock();
}
