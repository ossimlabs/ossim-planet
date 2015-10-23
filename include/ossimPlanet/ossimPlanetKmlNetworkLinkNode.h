#ifndef ossimPlanetKmlNetworkLinkNode_HEADER
#define ossimPlanetKmlNetworkLinkNode_HEADER
#include <ossimPlanet/ossimPlanetKmlLayerNode.h>

class OSSIMPLANET_DLL ossimPlanetKmlNetworkLinkNode : public ossimPlanetKmlLayerNode
{
public:
   ossimPlanetKmlNetworkLinkNode(ossimPlanetKmlLayer* layer = 0,
                                 ossimPlanetKmlObject* obj  = 0);
   virtual void traverse(osg::NodeVisitor& nv);
   virtual bool init();
   
protected:
   osg::ref_ptr<ossimPlanetKml> theKmlData;
   bool theScheduledFlag;

};
#endif
