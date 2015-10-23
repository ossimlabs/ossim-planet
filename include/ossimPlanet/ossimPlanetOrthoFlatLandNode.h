#ifndef ossimPlanetOrthoFlatLandNode_HEADER
#define ossimPlanetOrthoFlatLandNode_HEADER
#include "ossimPlanetFlatLandNode.h"
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetOrthoFlatLandNode : public ossimPlanetFlatLandNode
{
public:
   ossimPlanetOrthoFlatLandNode(ossim_uint32 level = 0,
                     ossim_uint32 row   = 0,
                     ossim_uint32 col   = 0);
   ossimPlanetOrthoFlatLandNode(const ossimPlanetFlatLandNode& plod,const osg::CopyOp& copyop);
   virtual ~ossimPlanetOrthoFlatLandNode();
   virtual osg::Object* cloneType() const { return new ossimPlanetOrthoFlatLandNode(); }
   virtual bool isSameKindAs(const osg::Object* obj) const
   {
      return dynamic_cast<const ossimPlanetOrthoFlatLandNode *>(obj)!=NULL;
   }
   virtual const char* className() const { return "OrthoFlatLandNode"; } 
   virtual const char* libraryName() const { return ""; }
   virtual void handleCullTraversal(osg::NodeVisitor& nv);

   
};

#endif
