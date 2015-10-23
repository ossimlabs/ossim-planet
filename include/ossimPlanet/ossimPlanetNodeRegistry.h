#ifndef ossimPlanetNodeRegistry_HEADER
#define ossimPlanetNodeRegistry_HEADER
#include <ossim/base/ossimConstants.h>
#include <ossimPlanet/ossimPlanetNodeFactoryBase.h>
#include <OpenThreads/ReadWriteMutex>
#include <OpenThreads/ScopedLock>

class ossimPlanetNodeRegistry  
{
public:
   typedef std::vector<osg::ref_ptr<ossimPlanetNodeFactoryBase> > FactoryListType;
   ossimPlanetNodeRegistry();
   ~ossimPlanetNodeRegistry();
	static ossimPlanetNodeRegistry* instance();
   void finalize();
   ossimPlanetNode* create(const ossimString& type)const;
	void registerFactory(ossimPlanetNodeFactoryBase* factory,
								bool insertFrontFlag=false);
   void unregisterFactory(const ossimPlanetNodeFactoryBase* factory);
	
protected:
	bool hasFactory(const ossimPlanetNodeFactoryBase* factory)const;
	
	static ossimPlanetNodeRegistry* theInstance;
	static ossim_uint32 theInitializeCount;
	mutable OpenThreads::ReadWriteMutex theFactoryListMutex;
	FactoryListType theFactoryList;
};

#endif
