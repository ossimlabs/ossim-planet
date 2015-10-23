#ifndef ossimPlanetPagedRequestNode_HEADER
#define ossimPlanetPagedRequestNode_HEADER
#include <osg/Node>
#include <osg/ref_ptr>
#include <osg/Texture2D>
#include <ossim/base/ossimConstants.h>
#include <iostream>
#include "ossimPlanetElevationGrid.h"
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetPagedRequestNode : public osg::Node
{
public:
   ossimPlanetPagedRequestNode(ossim_uint32 level=0,
                             ossim_uint32 row=0,
                             ossim_uint32 col=0,
                             osg::ref_ptr<osg::Texture2D> texture=0)
      :osg::Node(),
      theLevel(level),
      theRow(row),
      theCol(col),
      theTexture(texture)
      {
      }
   virtual ~ossimPlanetPagedRequestNode()
      {
      }
   osg::ref_ptr<osg::Texture2D> getTexture()
      {
         return theTexture;
      }
   osg::ref_ptr<ossimPlanetElevationGrid> getElevation()
   {
      return theElevationGrid;
   }
   const osg::ref_ptr<ossimPlanetElevationGrid> getElevation()const
   {
      return theElevationGrid;
   }
   void setElevation(osg::ref_ptr<ossimPlanetElevationGrid> elevationGrid)
   {
      theElevationGrid = elevationGrid;
   }
   void setTexture(osg::ref_ptr<osg::Texture2D> texture)
      {
         theTexture = texture;
      }
   ossim_uint32 getLevel()const
      {
         return theLevel;
      }
   ossim_uint32 getRow()const
      {
         return theRow;
      }
   ossim_uint32 getCol()const
      {
         return theCol;
      }
   
   
protected:
   ossim_uint32 theLevel;
   ossim_uint32 theRow;
   ossim_uint32 theCol;
   osg::ref_ptr<osg::Texture2D> theTexture;
   osg::ref_ptr<ossimPlanetElevationGrid> theElevationGrid;
};

#endif
