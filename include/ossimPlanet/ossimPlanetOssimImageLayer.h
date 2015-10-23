#ifndef ossimPlanetOssimImageLayer_HEADER
#define ossimPlanetOssimImageLayer_HEADER
#include "ossimPlanetTextureLayer.h"
#include <osg/Referenced>
#include "ossimPlanetId.h"
#include "ossimPlanetExport.h"
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/imaging/ossimGeoAnnotationSource.h>
#include <ossim/imaging/ossimGeoAnnotationPolyObject.h>
#include <ossim/imaging/ossimHistogramRemapper.h>
#include <ossim/imaging/ossimRectangleCutFilter.h>
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/projection/ossimLlxyProjection.h>
#include <queue>
class OSSIMPLANET_DLL ossimPlanetOssimImageLayer : public ossimPlanetTextureLayer
{
public:
   ossimPlanetOssimImageLayer();
   ossimPlanetOssimImageLayer(const ossimPlanetOssimImageLayer& src);
   ossimPlanetTextureLayerStateCode openImage(const ossimFilename& filename, ossim_int32 entryIdx = -1);
   ossimPlanetTextureLayerStateCode setCurrentEntry(ossim_int32 idx);
   ossim_uint32 getNumberOfEntries()const;
   void setOverviewFile(const ossimFilename& overviewFile);
   void setHistogramFile(const ossimFilename& histogram);
   void setHistogramStretchMode(const ossimString& mode);
   void setHistogramStretchEnableFlag(bool flag);
   bool histogramStretchEnableFlag()const;
   ossimString histogramStretchMode()const;
   void getHistogramStretchModes(std::vector<ossimString>& modes);
   virtual ossimPlanetTextureLayer* dup()const;
   virtual ossimPlanetTextureLayer* dupType()const;
   virtual ossimString getClassName()const;
   virtual ossimPlanetTextureLayerStateCode updateExtents();
   virtual void updateStats()const;
   virtual void resetStats()const;
   ossimScalarType scalarType()const;
   virtual bool hasTexture(ossim_uint32 width,
                           ossim_uint32 height,
                           const ossimPlanetTerrainTileId& tileId,
                           const ossimPlanetGrid& grid);
   virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 width,
                                                     ossim_uint32 height,
                                                     const ossimPlanetTerrainTileId& tileId,
                                                     const ossimPlanetGrid& theGrid,
                                                     ossim_int32 padding=0);
  virtual osg::ref_ptr<ossimPlanetImage> getTexture(ossim_uint32 level,
                                                     ossim_uint64 row,
                                                     ossim_uint64 col,
                                                     const ossimPlanetGridUtility& utility);
   ossimPlanetTextureLayerStateCode setHandler(ossimRefPtr<ossimImageHandler> handler);
   ossimRefPtr<ossimImageHandler> getHandler();
   const ossimRefPtr<ossimImageHandler> getHandler()const;
   
   virtual double getApproximateHypotneusLength()const;
   virtual void getCenterLatLonLength(double& centerLat,
                                      double& centerLon,
                                      double& length)const;
   void setFilterType(const ossimString& filterType);

   bool isMultiEntry()const;

   virtual bool buildOverview();
   
   osg::ref_ptr<ossimPlanetTextureLayer> groupAllEntries();
   virtual void getMetadata(ossimRefPtr<ossimXmlNode> metadata)const;

   virtual ossimRefPtr<ossimXmlNode> saveXml(bool recurseFlag=true)const;
   virtual bool loadXml(ossimRefPtr<ossimXmlNode> node);
   
protected:
   struct ResamplePoint
   {
   public:
      friend ostream& operator <<(ostream& out, const ResamplePoint& point)
      {
         out << point.theGlobalNdcPoint << "\n"
         << point.theModelPoint << "\n"
         << point.theImagePoint << "\n"
         << point.theTileImagePoint << "\n"
         << point.theDecimation << "\n"
         << point.theResolutionLevel;
         
         return out;
      }
      ResamplePoint(){}
      ResamplePoint(const ossimPlanetGrid::GridPoint& globalPoint,
                    const ossimPlanetGrid::ModelPoint& modelPt,
                    const ossimDpt& imgPt,
                    const ossimDpt& tileImgPt,
                    const ossimDpt& decimation,
                    ossim_uint32 rLevel)
      :theGlobalNdcPoint(globalPoint),
      theModelPoint(modelPt),
      theImagePoint(imgPt),
      theTileImagePoint(tileImgPt),
      theDecimation(decimation),
      theResolutionLevel(rLevel)
      {}
      ossimPlanetGrid::GridPoint  theGlobalNdcPoint;
      ossimPlanetGrid::ModelPoint theModelPoint;
      ossimDpt                    theImagePoint;
      ossimDpt                    theTileImagePoint;
      ossimDpt                    theDecimation;
      ossim_uint32                theResolutionLevel;
   };
   struct ResampleCorner
   {
   public:
      friend std::ostream& operator <<(std::ostream& out, const ResampleCorner& corners)
      {
         out << corners.theP1 << "\n" << corners.theP2 << "\n" 
             << corners.theP3 << "\n" << corners.theP4; 
         
         return out;
      }
     ResampleCorner(){}
      ResampleCorner(const ResamplePoint& p1, 
                     const ResamplePoint& p2,
                     const ResamplePoint& p3,
                     const ResamplePoint& p4)
      :theP1(p1),
      theP2(p2),
      theP3(p3),
      theP4(p4)
      {}
      bool canBilinearInterpolate(ossimProjection* proj,
                                  const ossimPlanetGrid& grid)const
      {
         ossimPlanetGrid::GridPoint globalGridCenter(theP1.theGlobalNdcPoint.face(),
                                                     (theP1.theGlobalNdcPoint.x()+
                                                      theP2.theGlobalNdcPoint.x()+
                                                      theP3.theGlobalNdcPoint.x()+
                                                      theP4.theGlobalNdcPoint.x())*.25,
                                                     (theP1.theGlobalNdcPoint.y()+
                                                      theP2.theGlobalNdcPoint.y()+
                                                      theP3.theGlobalNdcPoint.y()+
                                                      theP4.theGlobalNdcPoint.y())*.25);
         ossimDpt inputImageCenter((theP1.theImagePoint.x + 
                                    theP2.theImagePoint.x +
                                    theP3.theImagePoint.x +
                                    theP4.theImagePoint.x)*.25,
                                   (theP1.theImagePoint.y + 
                                    theP2.theImagePoint.y +
                                    theP3.theImagePoint.y +
                                    theP4.theImagePoint.y)*.25);
         
              
         ossimPlanetGrid::ModelPoint centerModel;
         ossimDpt centerImageTest;
         grid.globalGridToModel(globalGridCenter, centerModel);
         ossimGpt gpt(centerModel.y(),
                     centerModel.x());
         if(proj->isAffectedByElevation())
         {
            gpt.height(ossimElevManager::instance()->getHeightAboveEllipsoid(gpt));
         }
         proj->worldToLineSample(gpt, 
                                 centerImageTest);
         double errorMetric = (inputImageCenter-centerImageTest).length();
         
         //std::cout << "face == " << theP1.theGlobalNdcPoint.face() << std::endl;
         //std::cout << "ERROR == " << errorMetric << std::endl;
         return (errorMetric<1.0);
      }
      ossim_uint32 tileWidth()const
      {
         return (theP1.theTileImagePoint - theP2.theTileImagePoint).length();
      }
      ossim_uint32 tileHeight()const
      {
         return (theP1.theTileImagePoint - theP4.theTileImagePoint).length();
      }
      ossimDrect boundingInputImageRect()const
      {
         return ossimDrect(ossim::min(theP1.theImagePoint.x, 
                                      ossim::min(theP2.theImagePoint.x, 
                                                 ossim::min(theP3.theImagePoint.x, theP4.theImagePoint.x))),
                           ossim::max(theP1.theImagePoint.y, 
                                      ossim::max(theP2.theImagePoint.y, 
                                                 ossim::max(theP3.theImagePoint.y, theP4.theImagePoint.y))),
                           ossim::max(theP1.theImagePoint.x, 
                                      ossim::max(theP2.theImagePoint.x, 
                                                 ossim::max(theP3.theImagePoint.x, theP4.theImagePoint.x))),
                           ossim::min(theP1.theImagePoint.y, 
                                      ossim::min(theP2.theImagePoint.y, 
                                                 ossim::min(theP3.theImagePoint.y, theP4.theImagePoint.y))),
                           OSSIM_RIGHT_HANDED);
      }

     ossimDrect boundingInputImageRect(const ossimDpt& decimation)const
      {
         return boundingInputImageRect()*decimation;
      }
      void split(ossimProjection* proj,
                 const ossimPlanetGrid& grid,
                 ResampleCorner& bound1,
                 ResampleCorner& bound2,
                 ResampleCorner& bound3,
                 ResampleCorner& bound4)
      {
         // use parametric to split along each line going in
         // counter clockwise for the edges.
         //
         
         //
         ossimPlanetGrid::ModelPoint model;
         ResamplePoint centerP1P2;
         ResamplePoint centerP2P3;
         ResamplePoint centerP3P4;
         ResamplePoint centerP4P1;
         ResamplePoint centerPt;
         
         centerP1P2.theGlobalNdcPoint.theFace = theP1.theGlobalNdcPoint.theFace;
         centerP2P3.theGlobalNdcPoint.theFace = theP1.theGlobalNdcPoint.theFace;
         centerP3P4.theGlobalNdcPoint.theFace = theP1.theGlobalNdcPoint.theFace;
         centerP4P1.theGlobalNdcPoint.theFace = theP1.theGlobalNdcPoint.theFace;
         centerPt.theGlobalNdcPoint.theFace   = theP1.theGlobalNdcPoint.theFace;
         

       
         centerP1P2.theGlobalNdcPoint.setX((theP2.theGlobalNdcPoint.x()+theP1.theGlobalNdcPoint.x())*.5);
         centerP1P2.theGlobalNdcPoint.setY(theP1.theGlobalNdcPoint.y());
         centerP1P2.theTileImagePoint.x   = (ossim_int32)((theP2.theTileImagePoint.x+theP1.theTileImagePoint.x)*.5);
         centerP1P2.theTileImagePoint.y   = (ossim_int32)theP2.theTileImagePoint.y;
         centerP1P2.theDecimation = theP1.theDecimation;
         centerP1P2.theResolutionLevel = theP1.theResolutionLevel;
         grid.globalGridToModel(centerP1P2.theGlobalNdcPoint, model);
         ossimGpt gpt(model.y(),
                      model.x());
         
         if(proj->isAffectedByElevation())
         {
            gpt.height(ossimElevManager::instance()->getHeightAboveEllipsoid(gpt));
         }
         //std::cout << "class = " << proj->getClassName() << std::endl;
         //std::cout << "gpt = " << gpt << std::endl;
         proj->worldToLineSample(gpt, 
                                 centerP1P2.theImagePoint);
         
         centerP2P3.theGlobalNdcPoint.setX(theP2.theGlobalNdcPoint.x()); 
         centerP2P3.theGlobalNdcPoint.setY((theP3.theGlobalNdcPoint.y()+theP2.theGlobalNdcPoint.y())*.5);
         centerP2P3.theTileImagePoint.x   = theP2.theTileImagePoint.x;
         centerP2P3.theTileImagePoint.y   = (ossim_int32)((theP2.theTileImagePoint.y+theP3.theTileImagePoint.y)*.5);
         centerP2P3.theDecimation = theP2.theDecimation;
         centerP2P3.theResolutionLevel = theP2.theResolutionLevel;
         grid.globalGridToModel(centerP2P3.theGlobalNdcPoint, model);
         gpt = ossimGpt(model.y(),
                        model.x());
         if(proj->isAffectedByElevation())
         {
            gpt.height(ossimElevManager::instance()->getHeightAboveEllipsoid(gpt));
         }
         proj->worldToLineSample(gpt, 
                                 centerP2P3.theImagePoint);
         
         
         centerP3P4.theGlobalNdcPoint.setX((theP4.theGlobalNdcPoint.x()+theP3.theGlobalNdcPoint.x())*.5);
         centerP3P4.theGlobalNdcPoint.setY(theP3.theGlobalNdcPoint.y());
         centerP3P4.theTileImagePoint.x   = (ossim_int32)((theP3.theTileImagePoint.x+theP4.theTileImagePoint.x)*.5);
         centerP3P4.theTileImagePoint.y   = theP3.theTileImagePoint.y;
         centerP3P4.theDecimation = theP3.theDecimation;
         centerP3P4.theResolutionLevel = theP3.theResolutionLevel;
         grid.globalGridToModel(centerP3P4.theGlobalNdcPoint, model);
         gpt = ossimGpt(model.y(),
                        model.x());
         if(proj->isAffectedByElevation())
         {
            gpt.height(ossimElevManager::instance()->getHeightAboveEllipsoid(gpt));
         }
         proj->worldToLineSample(gpt, 
                                 centerP3P4.theImagePoint);

      
         centerP4P1.theGlobalNdcPoint.setX(theP1.theGlobalNdcPoint.x());
         centerP4P1.theGlobalNdcPoint.setY((theP1.theGlobalNdcPoint.y()+theP4.theGlobalNdcPoint.y())*.5);
         centerP4P1.theTileImagePoint.x   = theP1.theTileImagePoint.x;
         centerP4P1.theTileImagePoint.y   = (ossim_int32)((theP1.theTileImagePoint.y + theP4.theTileImagePoint.y)*.5);
         centerP4P1.theDecimation = theP4.theDecimation;
         centerP4P1.theResolutionLevel = theP4.theResolutionLevel;
         grid.globalGridToModel(centerP4P1.theGlobalNdcPoint, model);
         gpt = ossimGpt(model.y(),
                        model.x());
         if(proj->isAffectedByElevation())
         {
            gpt.height(ossimElevManager::instance()->getHeightAboveEllipsoid(gpt));
         }
         proj->worldToLineSample(gpt, 
                                 centerP4P1.theImagePoint);
         
         centerPt.theGlobalNdcPoint.setX(centerP1P2.theGlobalNdcPoint.x());         
         centerPt.theGlobalNdcPoint.setY(centerP4P1.theGlobalNdcPoint.y());       
         centerPt.theTileImagePoint.x = centerP1P2.theTileImagePoint.x;
         centerPt.theTileImagePoint.y = centerP4P1.theTileImagePoint.y;
         grid.globalGridToModel(centerPt.theGlobalNdcPoint, model);
         gpt = ossimGpt(model.y(),
                        model.x());
         if(proj->isAffectedByElevation())
         {
            gpt.height(ossimElevManager::instance()->getHeightAboveEllipsoid(gpt));
         }
         proj->worldToLineSample(gpt, 
                                 centerPt.theImagePoint);
         centerPt.theDecimation      = theP4.theDecimation;
         centerPt.theResolutionLevel = theP4.theResolutionLevel;
        
         bound1.theP1 = theP1;
         bound1.theP2 = centerP1P2;
         bound1.theP3 = centerPt;
         bound1.theP4 = centerP4P1;
         
         bound2.theP1 = centerP1P2;
         bound2.theP2 = theP2;
         bound2.theP3 = centerP2P3;
         bound2.theP4 = centerPt;
         
         bound3.theP1 = centerPt;
         bound3.theP2 = centerP2P3;
         bound3.theP3 = theP3;
         bound3.theP4 = centerP3P4;
         
         bound4.theP1 = centerP4P1;
         bound4.theP2 = centerPt;
         bound4.theP3 = centerP3P4;
         bound4.theP4 = theP4;
      }
      ResamplePoint theP1;
      ResamplePoint theP2;
      ResamplePoint theP3;
      ResamplePoint theP4;
   };
   typedef std::queue<ResampleCorner> ResampleCorners;
   virtual ~ossimPlanetOssimImageLayer();
   void clearChains();
   virtual ossimPlanetTextureLayerStateCode updateExtentsNoMutex();
   ossimPlanetTextureLayerStateCode buildChain();
   
   void initializeResamplePoints(const ossimPlanetTerrainTileId& tileId,
                                 const ossimPlanetGrid& grid,
                                 ossim_uint32 tileWidth,
                                 ossim_uint32 tileHeight,
                                 ResampleCorners& corners);
                                 
   ossimFilename                               theFilename;
   ossimFilename                               theOverviewFile;
   ossimFilename                               theHistogramFile;
   ossimRectangleCutFilter*                    theCut;
   mutable ossimRefPtr<ossimImageHandler>      theSource;
   ossimRefPtr<ossimImageRenderer>             theRenderer;
   ossimViewInterface*                         theViewInterface;
   ossimHistogramRemapper*                     theHistogramRemapper;
   ossimHistogramRemapper::StretchMode         theHistogramStretchMode;
   bool                                        theHistogramStretchEnableFlag;
   std::vector<ossimRefPtr<ossimImageSource> > theChain;
   std::vector<ossimRefPtr<ossimImageSource> > theImageSpaceChain;
   //   ossimRefPtr<ossimEquDistCylProjection>      theProjection;
   ossimRefPtr<ossimImageGeometry>  theImageGeometry;
   ossimRefPtr<ossimMapProjection>      theProjection;
   ossimRefPtr<ossimProjection>                theInputProjection;
   mutable double theCenterLat;
   mutable double theCenterLon;
   mutable double theLength;                   // approximate length in meters of the diagonal image
   mutable OpenThreads::Mutex         theOssimImageLayerMutex;
};

#endif
