#ifndef ossimPlanetNodeFactory_HEADER
#define ossimPlanetNodeFactory_HEADER
#include "ossimPlanetNodeFactoryBase.h"

class OSSIMPLANET_DLL ossimPlanetNodeFactory : public ossimPlanetNodeFactoryBase
{
public:
	ossimPlanetNodeFactory();
	virtual ~ossimPlanetNodeFactory();
	static ossimPlanetNodeFactory* instance();
   virtual ossimPlanetNode* create(const ossimString& type)const;
	
protected:
		static ossimPlanetNodeFactory* theInstance;
};

#endif
