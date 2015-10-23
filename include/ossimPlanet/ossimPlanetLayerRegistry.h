#ifndef ossimPlanetLayerRegistry_HEADER
#define ossimPlanetLayerRegistry_HEADER
#include "ossimPlanetExport.h"
#include <OpenThreads/ReadWriteMutex>
#include <OpenThreads/ScopedLock>
#include <vector>
#include <ossim/base/ossimString.h>
#include "ossimPlanetLayerFactoryBase.h"
#include <osg/ref_ptr>

class ossimPlanetLayer;
class ossimPlanetLayerRegistry
{
public:
   typedef std::vector<osg::ref_ptr<ossimPlanetLayerFactoryBase> > FactoryListType;
   
   ossimPlanetLayerRegistry();
   ~ossimPlanetLayerRegistry();
   static ossimPlanetLayerRegistry* instance();
   void finalize();
	
   /**
	 * @param type is the type name of the layer to create
    */
   ossimPlanetLayer* create(const ossimString& type)const;
   
   void registerFactory(ossimPlanetLayerFactoryBase* factory,
								bool insertFrontFlag=false);
   void unregisterFactory(const ossimPlanetLayerFactoryBase* factory);
   
protected:
   bool hasFactory(const ossimPlanetLayerFactoryBase* factory)const;
   
   mutable OpenThreads::ReadWriteMutex       theFactoryListMutex;
   static ossimPlanetLayerRegistry* theInstance;
   FactoryListType                  theFactoryList;
};

#endif
