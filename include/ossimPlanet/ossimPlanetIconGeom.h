#ifndef ossimPlanetIconGeom_HEADER
#define ossimPlanetIconGeom_HEADER
#include <osg/Geometry>
#include <osg/Vec3d>
#include <osg/Texture2D>
#include <ossim/base/ossimConstants.h>

class ossimPlanetIconGeom : public osg::Geometry
{
public:
   /**
    *  We will setup a unit geometry for the icon.  
    */ 
   ossimPlanetIconGeom( const osg::Vec3d& corner=osg::Vec3d(-.5,
                                                            0.0,
                                                            -.5),
                        const osg::Vec3d& width=osg::Vec3d(1.0,0.0,0.0),
                        const osg::Vec3d& height=osg::Vec3d(0.0,0.0,1.0));
   void setTexture(osg::ref_ptr<osg::Image> img);
   void setTexture(osg::ref_ptr<osg::Texture2D> texture);
   void resetToUnitGeometry();
   void setGeometry(const osg::Vec3d& corner,
                    const osg::Vec3d& width,
                    const osg::Vec3d& height);
   void setTextureCoordinatesGivenPixels(int originX,
                                         int originY,
                                         int pixelWidth,
                                         int pixelHeight);
   virtual void drawImplementation(osg::RenderInfo& renderInfo) const;
   ossim_uint32 width()const;
   ossim_uint32 height()const;
   osg::ref_ptr<osg::Texture2D> texture();
   const osg::ref_ptr<osg::Texture2D> texture()const;
protected:
   void setupGeom(const osg::Vec3d& corner,
                  const osg::Vec3d& width,
                  const osg::Vec3d& height);
   osg::ref_ptr<osg::Texture2D> theTexture;
   mutable osg::ref_ptr<osg::Vec4dArray> theColorArray;
   float theAlpha;
};


#endif
