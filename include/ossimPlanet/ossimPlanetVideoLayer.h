#ifndef ossimPlanetVideoLayer_HEADER
#define ossimPlanetVideoLayer_HEADER
#include "ossimPlanetLayer.h"
#include <ossim/base/ossimFilename.h>
#include <ossimPlanet/ossimPlanetVideoLayerNode.h>

class ossimPlanet;
class OSSIMPLANET_DLL ossimPlanetVideoLayer : public ossimPlanetLayer
{
public:
   ossimPlanetVideoLayer()
   {
   }
   virtual osg::Object* cloneType() const { return new ossimPlanetVideoLayer(); }
   virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const ossimPlanetVideoLayer *>(obj)!=0; }
   virtual const char* className() const { return "ossimPlanetVideoLayer"; } 
   virtual const char* libraryName() const { return ""; }
   
   virtual void traverse(osg::NodeVisitor& nv);

   virtual bool add(const ossimFilename& file);

   
   virtual bool addChild( Node *child )
   {
      if(dynamic_cast<ossimPlanetVideoLayerNode*>(child))
      {
         return ossimPlanetLayer::addChild(child);
      }
      return false;
   }
   virtual bool insertChild( unsigned int index, Node *child )
   {
      if(dynamic_cast<ossimPlanetVideoLayerNode*>(child))
      {
         return ossimPlanetLayer::insertChild(index, child);
      }
      return false;
      
   }
   virtual bool replaceChild( Node *origChild, Node* newChild )
   {
      if(dynamic_cast<ossimPlanetVideoLayerNode*>(newChild))
      {
         return ossimPlanetLayer::replaceChild(origChild, newChild);
      }
      return false;
   }
   virtual bool setChild( unsigned  int i, Node* node )
   {
      if(dynamic_cast<ossimPlanetVideoLayerNode*>(node))
      {
         return ossimPlanetLayer::setChild(i, node);
      }
      return false;
   }

   
};

#endif
