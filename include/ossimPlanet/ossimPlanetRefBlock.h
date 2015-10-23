#ifndef ossimPlanetRefBlock_HEADER
#define ossimPlanetRefBlock_HEADER
#include <osg/Referenced>
#include <OpenThreads/Block>
#include "ossimPlanetExport.h"

class OSSIMPLANET_DLL ossimPlanetRefBlock : virtual public osg::Referenced,
                                            public OpenThreads::Block
{
public:
   ossimPlanetRefBlock()
   {}
};

#endif
