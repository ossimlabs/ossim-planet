//*******************************************************************
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author: Garrett Potts
//
//*************************************************************************
// $Id$
#ifndef ossimPlanetAnimationPath_HEADER
#define ossimPlanetAnimationPath_HEADER
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetExport.h>
#include <osg/AnimationPath>
class OSSIMPLANET_DLL ossimPlanetAnimationPath : public osg::AnimationPath
{
public:
   class OSSIMPLANET_DLL Tuple
   {
   public:
      Tuple(const osg::Vec3d& pos,
            const osg::Vec3d& orient,
            const osg::Vec3d& s = osg::Vec3(1.0,1.0,1.0))
      :thePosition(pos),
      theOrientation(orient),
      theScale(s)
      {
         
      }
      const osg::Vec3d& position()const{return thePosition;}
      const osg::Vec3d& orientation()const{return theOrientation;}
      const osg::Vec3d& scale()const{return theScale;}
      void setPosition(const osg::Vec3d& value){thePosition = value;}
      void setScale(const osg::Vec3d& value){theScale = value;}
      void setOrientation(const osg::Vec3d& value){theOrientation = value;}
      
      osg::Vec3d thePosition;
      osg::Vec3d theOrientation;
      osg::Vec3d theScale;
   };
   
   typedef std::map<double, Tuple> TimeTupleMap;
   class OSSIMPLANET_DLL GeospatialPath : public osg::Referenced
   {
   public:
      GeospatialPath(){}
      bool empty()const{return theTimeTupleMap.empty();}
      TimeTupleMap& timeTupleMap(){return theTimeTupleMap;}
      const TimeTupleMap& timeTupleMap()const{return theTimeTupleMap;}
      double firstTime() const { if (!theTimeTupleMap.empty()) return theTimeTupleMap.begin()->first; else return 0.0;}
      double lastTime() const { if (!theTimeTupleMap.empty()) return theTimeTupleMap.rbegin()->first; else return 0.0;}
      double period() const { return lastTime()-firstTime();}
   protected:
      TimeTupleMap theTimeTupleMap;
   };
   typedef std::vector<osg::Vec3d> PointList;
   ossimPlanetAnimationPath();
   void setGeoRefModel(ossimPlanetGeoRefModel* model){theModel = model;}
   ossimPlanetGeoRefModel* geoRefModel(){return theModel.get();}
   const ossimPlanetGeoRefModel* geoRefModel()const{return theModel.get();}
   bool openAnimationPathByXmlDocument(const ossimFilename& animationFile);
   bool setAnimationPathByXmlDocument(const ossimString& xml);
   bool setAnimationPathByXmlDocument(std::istream& xmlStream);
   bool openAnimationPathByXmlNode(const ossimFilename& animationFile);
   bool setAnimationPathByXmlNode(const ossimString& xml);
   bool setAnimationPathByXmlNode(std::istream& xmlStream);
   
   bool setAnimationPathByXmlNode(ossimRefPtr<ossimXmlNode> node);
   ossimXmlNode* saveXml()const;
   GeospatialPath* geospatialPath(){return thePath.get();};
   const GeospatialPath* geospatialPath()const{return thePath.get();};
   void setGeospatialPath(GeospatialPath* path);
   virtual bool getInterpolatedControlPoint(double time,
                                            osg::AnimationPath::ControlPoint& controlPoint) const;
   
   bool generateWorldCoordinates(PointList& worldPoints)const;
   bool generateModelCoordinates(PointList& modelPoints)const;
   
   bool moveToLocationLatLon(const osg::Vec2d& llh);
   
   
protected:
   virtual double adjustTime(double time)const;
   void lsrMatrix(osg::Matrixd& result,
                  const Tuple& tuple)const;
   osg::ref_ptr<GeospatialPath>         thePath;
   osg::ref_ptr<ossimPlanetGeoRefModel> theModel;
};

#endif
