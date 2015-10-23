#ifndef ossimPlanetLayer_HEADER
#define ossimPlanetLayer_HEADER
#include <osg/Group>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/ossimPlanetCallback.h>
#include <ossimPlanet/ossimPlanetActionReceiver.h>
#include <ossimPlanet/ossimPlanetNode.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>

class ossimPlanet;

class OSSIMPLANET_DLL ossimPlanetLayer :public ossimPlanetNode
{
public:
   friend class ossimPlanetLayerUpdateCallback;
   
   ossimPlanetLayer();
   virtual ~ossimPlanetLayer();   
   
   virtual void execute(const ossimPlanetAction& action);
   
   virtual void setModel(ossimPlanetGeoRefModel* model)
   {
      theModel = model;
   }
   const ossimPlanetGeoRefModel* model()const
   {
      return theModel.get();
   }
   ossimPlanetGeoRefModel* model()
   {
      return theModel.get();
   }
   virtual void setPlanet(ossimPlanet* planet)
   {
      thePlanet = planet;
   }
   ossimPlanet* planet()
   {
      return thePlanet;
   }
   const ossimPlanet* planet()const
   {
      return thePlanet;
   }
	virtual void needsRemoving(osg::Node* /*node*/){}
   static ossimPlanetLayer* findLayer(osg::Node* startNode);
   static ossimPlanetLayer* findLayer(osg::NodePath& currentNodePath);
   
   virtual void traverse(osg::NodeVisitor& nv);
   
protected:
   ossimPlanet* thePlanet;   
   osg::ref_ptr<ossimPlanetGeoRefModel> theModel;
};

#endif
