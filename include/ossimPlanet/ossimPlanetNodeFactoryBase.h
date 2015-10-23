#ifndef ossimPlanetNodeFactoryBase_HEADER
#define ossimPlanetNodeFactoryBase_HEADER
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/ossimPlanetExport.h>
class ossimPlanetNode;
class OSSIMPLANET_DLL ossimPlanetNodeFactoryBase : public osg::Referenced
{
public:
   ossimPlanetNodeFactoryBase(){}
   virtual ~ossimPlanetNodeFactoryBase()
	{
	}
   virtual ossimPlanetNode* create(const ossimString& type)const=0;
};

#endif
