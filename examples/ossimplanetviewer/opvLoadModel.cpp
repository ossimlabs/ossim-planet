//
// opvLoadModel.cpp
//
// osgPlanetView load model functionality
//

#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>

#include <ossimPlanet/ossimPlanet.h>

#include <ossim/base/ossimEcefPoint.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/projection/ossimUtmProjection.h>
#include <ossim/base/ossimGeoidManager.h>
#include <ossim/elevation/ossimElevManager.h>

//
// This visitor modifies each geometry it finds by creating
//  a transformArrayVisitor.
// 
class geomVisitor : public osg::NodeVisitor
{
public:
   geomVisitor()
         :osg::NodeVisitor(TRAVERSE_ALL_CHILDREN),
          theMode(ossimPlanetLandType_NORMALIZED_ELLIPSOID)
      {
      }

   virtual void apply(osg::Geode& node);
   virtual void apply(osg::Node& node);
   ossimPlanetLandType       theMode;
   osg::Vec3d              theCenter;
   ossimMapProjection*     theProj;
   ossimDpt                theLocation;
};

//
// This visitor modifies a vertex list by transforming each
//  vertex to ossimPlanet coordinates.
// 
class transformArrayVisitor: public osg::ArrayVisitor
{
public:
   transformArrayVisitor(ossimPlanetLandType mode=ossimPlanetLandType_NORMALIZED_ELLIPSOID):theMode(mode)
      {
      }
   virtual void apply(osg::Vec3Array& array);

   ossimPlanetLandType      theMode;
   osg::Vec3d             theCenter;
   ossimMapProjection*    theProj;
   ossimDpt               theLocation;
};

void transformArrayVisitor::apply(osg::Vec3Array& array)
{
   unsigned int idx = 0;

   switch(theMode)
   {
      case ossimPlanetLandType_NORMALIZED_ELLIPSOID:
      {
         ossimGpt gpt;
         ossimEcefPoint ecef;
         ecef = gpt;
         double normalizationFactor = ecef.getMagnitude();
         for(idx = 0; idx < array.size(); ++idx)
         {
            // Treat the coordinate of the model as meters
            ossimDpt point(array[idx][0],
    	                   array[idx][1]);

            // Offset to place the model at the desired UTM X,Y location
            point += theLocation;

            // Transform to geographic coordinates
            theProj->eastingNorthingToWorld(point,
                                            gpt);
            double deltaH = ossimElevManager::instance()->getHeightAboveMSL(gpt);
            if(ossim::isnan(deltaH))
            {
               deltaH = 0.0;
            }
            gpt.height(array[idx][2] +deltaH+ ossimGeoidManager::instance()->offsetFromEllipsoid(gpt));
            // Transform to ossimPlanet coordinates
            ecef = gpt;
            array[idx][0] = ecef.x()/normalizationFactor;
            array[idx][1] = ecef.y()/normalizationFactor;
            array[idx][2] = ecef.z()/normalizationFactor;
         }
         break;
      }
      case ossimPlanetLandType_FLAT:
      {
         ossimGpt gpt;
         ossimEcefPoint ecef;
         for(idx = 0; idx < array.size(); ++idx)
         {
            theProj->eastingNorthingToWorld(ossimDpt(array[idx][0],
                                                     array[idx][1]),
                                            gpt);
            gpt.height(array[idx][2]);
            array[idx][0] = gpt.lond();
            array[idx][1] = gpt.latd();
            array[idx][2] = gpt.height()/1000;
         }
         break;
      }
      default:
      {
         for(idx = 0; idx < array.size(); ++idx)
         {
            array[idx][0] -= theCenter[0];
            array[idx][1] -= theCenter[1];
            array[idx][2] -= theCenter[2];
         }
         break;
      }
   }
}

void geomVisitor::apply(osg::Geode& node)
{
   unsigned int drawableIdx = 0;

   for(drawableIdx = 0; drawableIdx < node.getNumDrawables();++drawableIdx)
   {
      if(node.getDrawable(drawableIdx))
      {
         osg::Geometry* geomNode = node.getDrawable(drawableIdx)->asGeometry();

         if(geomNode)
         {
            transformArrayVisitor visitor;
            visitor.theProj      = theProj;
            visitor.theCenter    = theCenter;
            visitor.theLocation  = theLocation;
            visitor.theMode      = theMode;
            osg::Array* vertexArray = geomNode->getVertexArray();

            vertexArray->accept(visitor);
            geomNode->dirtyBound();
         }
      }
   }

   traverse(node);
}

void geomVisitor::apply(osg::Node& node)
{
   std::string name = node.getName();

   if(name == "o1") // hack: remove the ground polygon so that only the city show up
   {
      osg::Group* group = node.asGroup();
      if(group)
      {
         group->removeChild(0, group->getNumChildren());
      }
   }
   osg::NodeVisitor::apply(node);
}


void transformPointsToLandType(osg::ref_ptr<osg::Node>& node,
                               const ossimFilename& /* file */ ,
                               ossimPlanetLandType landType,
                               int utmZone, int utmX, int utmY)
{
   geomVisitor visitor;
   ossimRefPtr<ossimUtmProjection> proj = new ossimUtmProjection;
   visitor.theMode = landType;
   proj->setZone(utmZone);
   visitor.theProj = proj.get();
   visitor.theLocation.x = utmX;
   visitor.theLocation.y = utmY;
   node->accept(visitor);
   node->dirtyBound();

   osg::Vec3d v = node->getBound().center();
   visitor.theMode = ossimPlanetLandType_NONE;
   visitor.theCenter = v;
   node->accept(visitor);
   node->dirtyBound();
   osg::MatrixTransform* mTransform = new osg::MatrixTransform;
   osg::Matrixd localToWorld;
   localToWorld.makeTranslate(v);
   mTransform->setMatrix(localToWorld);
   mTransform->addChild(node.get());
//    osgUtil::SmoothingVisitor smoother;
//    mTransform->accept(smoother);

   node = mTransform;
}

/**
 * Create a city by loading an OSG-supported 3D model file and
 * transforming it onto the ossimPlanet ellipsoid.
 *
 * \param file Filename of the 3D model.
 * \param landType Landtype of the ossimPlanet terrain.
 * \param utmZone UTM Zone (from 1 to 60, or -1 to -60 for the
 *		southern hemisphere) in which to place the model.
 * \param utmX X (easting) utm coordinate.
 * \param utmY Y (northing) utm coordinate.
 *
 * \return The osg Node of the transformed model.  Note that you
 *		will still need to add it to the scene graph with
 *		ossimPlanet::addChild()
 */
osg::ref_ptr<osg::Node> createCityModel(const ossimFilename& file,
                                        ossimPlanetLandType landType,
                                        int utmZone, int utmX, int utmY)
{
   osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(std::string(file.c_str()));

   transformPointsToLandType(loadedModel, file, landType, utmZone, utmX, utmY);

   return loadedModel;
}


#if 0
//--------------------------------------------------------------------------------------------------
class osgTestGroup : public osg::Group
{
public:
   osgTestGroup(){}

   virtual void traverse(osg::NodeVisitor& nv)
      {
         switch(nv.getVisitorType())
         {
            case osg::NodeVisitor::CULL_VISITOR:
            {
               if((nv.getFrameStamp()->getFrameNumber()%5)==0)
               {
                  nv.getDatabaseRequestHandler()->requestNodeFile("TEST",
                                                                  this,
                                                                  99999999,
                                                                  nv.getFrameStamp());
               }
               break;
            }
            default:
            {
               break;
            }
         }
         osg::Group::traverse(nv);
      }
   virtual bool addChild(osg::Node* node)
      {
         osg::Geode* geode = dynamic_cast<osg::Geode*>(node);

         if(geode)
         {
            if(getNumChildren() > 0)
            {
               removeChild(0, getNumChildren());
            }
         }
         return osg::Group::addChild(node);
      }
};

static int texCount = 0;
class osgMyTexture : public osg::Texture2D
{
public:
   osgMyTexture()
      {
         ++texCount;
         std::cout << "Texture " << texCount << std::endl;
      }
   virtual ~osgMyTexture()
      {
         --texCount;
         std::cout << "Texture " << texCount << std::endl;
      }
};
class osgTestReaderWriter : public osgDB::ReaderWriter
{
public:
   virtual ReadResult readNode(const std::string& fileName, const Options*)const
   {
      ReadResult result = 0;
      if(fileName == "TEST")
      {
      std::cout << "Allocating geometry" << std::endl;
      osg::ref_ptr<osg::Geode> geode = new osg::Geode;
      osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

      osg::ref_ptr<osg::Vec3Array> vArray = new osg::Vec3Array;
      osg::ref_ptr<osg::Vec2Array> tArray = new osg::Vec2Array;
      vArray->push_back(osg::Vec3d(-.5, .5,0.0));
      vArray->push_back(osg::Vec3d(-.5, -.5, 0.0));
      vArray->push_back(osg::Vec3d(.5, -.5, 0.0));
      vArray->push_back(osg::Vec3d(.5, .5, 0.0));

      tArray->push_back(osg::Vec2d(0.0,0.0));
      tArray->push_back(osg::Vec2d(0,1.0));
      tArray->push_back(osg::Vec2d(1.0,1.0));
      tArray->push_back(osg::Vec2d(1.0,0.0));

      osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
//          osg::ref_ptr<osg::Texture2D> tex = new osgMyTexture;
      osg::ref_ptr<osg::Image> img   = new osg::Image;
      osg::ref_ptr<osg::StateSet> dstate = geom->getOrCreateStateSet();
      unsigned char* newData = new unsigned char[256*256*4];
      memset(newData, 255, 256*256*4);
      img->setImage(256, 256, 1,
                      GL_RGBA,
                      GL_RGBA,
                      GL_UNSIGNED_BYTE,
                      newData,
                      osg::Image::USE_NEW_DELETE);

      tex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
      tex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
      tex->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
      tex->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
      tex->setDataVariance(osg::Object::DYNAMIC);
      tex->setImage(img.get());
      osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
      color->push_back(osg::Vec4(1.0,
                                  1.0,
                                  1.0,
                                  1.0));
      geom->setColorBinding(osg::Geometry::BIND_OVERALL);
      geom->setColorArray(color.get());

      dstate->setTextureAttributeAndModes(0, tex.get(),
                                          osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);

      geom->setVertexArray(vArray.get());
      geom->setTexCoordArray( 0, tArray.get());

      osg::DrawArrays* drawArray = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 1);
      geom->addPrimitiveSet(drawArray);

      geode->addDrawable(geom.get());
      result = geode.get();
      }
      return result;
   }
};
//-------------------------------------------------------------------------------------------------------------
#endif


