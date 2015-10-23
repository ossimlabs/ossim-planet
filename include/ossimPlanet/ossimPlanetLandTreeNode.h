#ifndef ossimPlanetLandTreeNode_HEADER
#define ossimPlanetLandTreeNode_HEADER
#include <osg/Node>

class ossimPlanetLandTreeNode : public osg::Node
{
public:
   ossimPlanetLandTreeNode(ossim_uint32 level,
                           ossim_uint64 row,
                           ossim_uint64 col);

protected:
   ossim_uint32 theLevel;
   ossim_uint32 theRow;
   ossim_uint32 theCol;
   ossimPlanetLandTreeNode* theParent;
   ossimPlanetLandTreeNode* theRightNeighbor;
   ossimPlanetLandTreeNode* theLeftNeighbor;
   ossimPlanetLandTreeNode* theTopNeighbor;
   ossimPlanetLandTreeNode* theBottomNeighbor;
   ossimRefPtr<ossimPlanetLandTreeNode> theChildren[4];
   
   
};

#endif
