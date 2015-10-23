#ifndef ossimPlanetLandCullCallback_HEADER
#define ossimPlanetLandCullCallback_HEADER
#include <osg/Node>
#include <osg/Vec3d>
#include <osg/NodeCallback>
#include <ossimPlanet/ossimPlanetExport.h>
#include <ossimPlanet/ossimPlanetConstants.h>
class ossimPlanetPagedLandLod;

class OSSIMPLANET_DLL ossimPlanetLandCullCallback :  public osg::NodeCallback
{
public:
   ossimPlanetLandCullCallback();
   ossimPlanetLandCullCallback(const ossimPlanetLandCullCallback& src)
      :osg::NodeCallback(src),
      theFreezeRequestFlag(src.theFreezeRequestFlag),
      theCullingFlag(src.theCullingFlag),
      theSplitMetric(src.theSplitMetric),
      theSplitPriorityType(src.theSplitPriorityType),
      theLineOfSiteValidFlag(src.theLineOfSiteValidFlag),
      theLineOfSite(src.theLineOfSite)
   {
   }
   ossimPlanetLandCullCallback* clone()const
   {
      return new ossimPlanetLandCullCallback(*this);
   }
   virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
   void setLineOfSite(const osg::Vec3d& lineOfSite);
   void setLineOfSiteValidFlag(bool flag);
   bool isLineOfSiteValid()const;

   void setSplitMetricRatio(double ratio);
   double getSplitMetricRatio()const;

   void setSplitPriorityType(ossimPlanetPriorityType priorityType);
   ossimPlanetPriorityType getSplitPriorityType()const;

   void setCullingFlag(bool flag);
   bool getCullingFlag()const;

   void setFreezeRequestFlag(bool flag);
   bool getFreezRequestFlag()const;
protected:
   void applyStandardCull(ossimPlanetPagedLandLod* n, osg::NodeVisitor* nv);
 //  void applyOrthoCull(ossimPlanetPagedLandLod* n, osg::NodeVisitor* nv);

   bool                    theFreezeRequestFlag;
   bool                    theCullingFlag;
   double                  theSplitMetric;
   ossimPlanetPriorityType theSplitPriorityType;
   bool                    theLineOfSiteValidFlag;
   osg::Vec3d              theLineOfSite;
};
#endif
