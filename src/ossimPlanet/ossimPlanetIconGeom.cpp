#include <ossimPlanet/ossimPlanetIconGeom.h>
#include <iostream>

ossimPlanetIconGeom::ossimPlanetIconGeom( const osg::Vec3d& corner,
                                          const osg::Vec3d& width,
                                          const osg::Vec3d& height)
{
   setUseDisplayList(false);
   setupGeom(corner, width, height);
   theAlpha = 1.0;
}

void ossimPlanetIconGeom::setTexture(osg::ref_ptr<osg::Image> img)
{
   if(!theTexture.valid())
   {
      theTexture = new osg::Texture2D;
      theTexture->setResizeNonPowerOfTwoHint(false);
      theTexture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
      theTexture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
   }
   theTexture->setImage(img.get());   
}

void ossimPlanetIconGeom::setTexture(osg::ref_ptr<osg::Texture2D> texture)
{
   theTexture = texture.get();
   getOrCreateStateSet()->setTextureAttributeAndModes(0,theTexture.get(),osg::StateAttribute::ON);   
}

void ossimPlanetIconGeom::resetToUnitGeometry()
{
   setGeometry(osg::Vec3d(-.5,0.0,-.5),
               osg::Vec3d(1.0,0.0,0.0),
               osg::Vec3d(0.0,0.0,1.0));
}


void ossimPlanetIconGeom::setGeometry(const osg::Vec3d& corner,
                                      const osg::Vec3d& width,
                                      const osg::Vec3d& height)
{
   osg::ref_ptr<osg::Vec3dArray> coords = dynamic_cast<osg::Vec3dArray*>(getVertexArray());
   if(coords.valid())
   {
      (*coords)[0] = corner;
      (*coords)[1] = corner+width;
      (*coords)[2] = corner+width+height;
      (*coords)[3] = corner+height;
      setVertexArray(coords.get());
   }
   osg::ref_ptr<osg::Vec3Array> norms = dynamic_cast<osg::Vec3Array*>(getNormalArray());
   if(norms.valid())
   {
      (*norms)[0] = width^height;
      (*norms)[0].normalize();
      setNormalArray(norms.get());
   }
}

void ossimPlanetIconGeom::drawImplementation(osg::RenderInfo& renderInfo) const
{
   osg::Vec4 save = (*theColorArray)[0];
   (*theColorArray)[0][3] = theAlpha;
   osg::Geometry::drawImplementation(renderInfo);
   (*theColorArray)[0] = save;
}

void ossimPlanetIconGeom::setupGeom(const osg::Vec3d& corner,
                                    const osg::Vec3d& width,
                                    const osg::Vec3d& height)
{
   osg::Vec3dArray* coords = new osg::Vec3dArray(4);
   (*coords)[0] = corner;
   (*coords)[1] = corner+width;
   (*coords)[2] = corner+width+height;
   (*coords)[3] = corner+height;
   setVertexArray(coords);
   
   osg::Vec3dArray* norms = new osg::Vec3dArray(1);
   (*norms)[0] = width^height;
   (*norms)[0].normalize();
   
   setNormalArray(norms);
   setNormalBinding(osg::Geometry::BIND_OVERALL);
   theColorArray = new osg::Vec4dArray;
   theColorArray->push_back(osg::Vec4(1.0,
                              1.0,
                              1.0,
                              1.0));
   setColorBinding(osg::Geometry::BIND_OVERALL);
   setColorArray(theColorArray.get());
   
   osg::Vec2dArray* tcoords = new osg::Vec2dArray(4);
   (*tcoords)[0].set(0.0,0.0);
   (*tcoords)[1].set(1.0,0.0);
   (*tcoords)[2].set(1.0,1.0);
   (*tcoords)[3].set(0.0,1.0);
   setTexCoordArray(0,tcoords);
   addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4));
   osg::StateSet* stateset = new osg::StateSet;
   theTexture = new osg::Texture2D;
   theTexture->setResizeNonPowerOfTwoHint(false);
   stateset->setTextureAttributeAndModes(0,theTexture.get(),osg::StateAttribute::ON);
   setStateSet(stateset);
}

void ossimPlanetIconGeom::setTextureCoordinatesGivenPixels(int originX,
                                                           int originY,
                                                           int pixelWidth,
                                                           int pixelHeight)
{
   if(theTexture.valid())
   {
      float w  = theTexture->getTextureWidth();
      float h  = theTexture->getTextureHeight();
      osg::ref_ptr<osg::Vec2Array> tcoords = dynamic_cast<osg::Vec2Array*>(getVertexArray());
      float cornertx = (float)originX/w;
      float cornerty = (float)originY/h;
      
      (*tcoords)[0].set(cornertx, cornerty);
      (*tcoords)[1].set(cornertx+pixelWidth/w, cornerty);
      (*tcoords)[2].set(cornertx+pixelWidth/w, cornerty+pixelHeight/h);
      (*tcoords)[3].set(cornertx, cornerty+pixelHeight/h);
      dirtyBound();
   }
}

ossim_uint32 ossimPlanetIconGeom::width()const
{
   if(theTexture.valid())
   {
      theTexture->getTextureWidth();
   }
   return 0;
}

ossim_uint32 ossimPlanetIconGeom::height()const
{
   if(theTexture.valid())
   {
      theTexture->getTextureHeight();
   }
   
   return 0;
}


osg::ref_ptr<osg::Texture2D> ossimPlanetIconGeom::texture()
{
   return theTexture;
}

const osg::ref_ptr<osg::Texture2D> ossimPlanetIconGeom::texture()const
{
   return theTexture;
}
