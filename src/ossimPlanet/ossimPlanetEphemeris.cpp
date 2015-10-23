#ifdef OSSIMPLANET_ENABLE_EPHEMERIS
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <osg/io_utils>
//#include <gpstk/Geodetic.hpp>
#include <gpstk/SunPosition.hpp>
#include <gpstk/MoonPosition.hpp>
#include <gpstk/Position.hpp>
#include <gpstk/WGS84Ellipsoid.hpp>
#include <gpstk/SystemTime.hpp>
#include <gpstk/ReferenceFrame.hpp>
//#include <gpstk/ECEF.hpp>
//#include <gpstk/WGS84Geoid.hpp>
#include <ossimPlanet/ossimPlanetEphemeris.h>
#include <ossimPlanet/ossimPlanet.h>
#include <ossimPlanet/ossimPlanetPointModel.h>
#include <ossimPlanet/ossimPlanetBillboardIcon.h>
#include <ossimPlanet/ossimPlanetViewer.h>
#include <ossimPlanet/ossimPlanetGeoRefModel.h>
#include <ossimPlanet/ossimPlanetCloudLayer.h>
#include <ossimPlanet/ossimPlanetGrid.h>
#include <ossim/base/ossimUnitConversionTool.h>
#include <ossim/imaging/ossimJpegWriter.h>
#include <ossim/imaging/ossimMemoryImageSource.h>
#include <ossim/base/ossimUnitConversionTool.h>
#include <osg/CoordinateSystemNode>
#include <osg/VertexProgram>
#include <osg/Fog>
#include <osg/Camera>
#include <osg/io_utils>
#include <osg/Depth>
#include <osg/BlendFunc>
#include <osg/AlphaFunc>
#include <osg/FragmentProgram>
#include <osgGA/EventVisitor>
#include <osgUtil/IntersectVisitor>
#include <osgUtil/IntersectionVisitor>

#include <osg/io_utils>
#include <time.h>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <queue>




class EnvEffect : public osg::Referenced
{
public:
   
   EnvEffect( const std::string& name = "EnvEffect" )
   {
      
   }
   
protected:
   
   virtual ~EnvEffect()
   {
      
   }
   
public:
   
   /// Must override this to supply the repainting routine
   virtual void Repaint(const osg::Vec3& skyColor, 
                        const osg::Vec3& fogColor,
                        double sunAngle, 
                        double sunAzimuth,
                        double visibility,
                        double altitude=0.0) = 0;
   
   ///required by DeltaDrawable
   osg::Node* GetOSGNode(){return mNode.get();}
   const osg::Node* GetOSGNode() const{return mNode.get();}
   
   void SetOSGNode(osg::Node* pNode){mNode = pNode;}
   
private:
   
   // Disallowed to prevent compile errors on VS2003. It apparently
   // creates this functions even if they are not used, and if
   // this class is forward declared, these implicit functions will
   // cause compiler errors for missing calls to "ref".
   EnvEffect& operator=( const EnvEffect& ); 
   EnvEffect( const EnvEffect& );
   
   osg::ref_ptr<osg::Node> mNode;
};

class  MoveEarthySkyWithEyePointTransform : public osg::Transform
{
public:
   
   ///Get the transformation matrix which moves from local coords to world coords.
   virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const;
   
   ///Get the transformation matrix which moves from world coords to local coords.
   virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const;
};

class  MoveEarthySkyWithEyePointTransformAzimuth : public osg::Transform
{
public:
   
   ///Get the transformation matrix which moves from local coords to world coords.
   virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const;
   
   ///Get the transformation matrix which moves from world coords to local coords.
   virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const;
   
   float GetAzimuth() const { return mAzimuth; }
   void SetAzimuth( float azimuth ) { mAzimuth = azimuth; }
   
private:
   
   float mAzimuth;
};
bool MoveEarthySkyWithEyePointTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const
{
   if ( osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv) )
   {
      osg::Vec3 eyePointLocal = cv->getEyeLocal();
      matrix.preMult(osg::Matrix::translate(eyePointLocal.x(),eyePointLocal.y(),eyePointLocal.z()));
   }
   return true;
}

///Get the transformation matrix which moves from world coords to local coords.
bool MoveEarthySkyWithEyePointTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const
{
   if ( osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv) )
   {
      osg::Vec3 eyePointLocal = cv->getEyeLocal();
      matrix.postMult(osg::Matrix::translate(-eyePointLocal.x(),-eyePointLocal.y(),-eyePointLocal.z()));
   }
   return true;
}

///Get the transformation matrix which moves from local coords to world coords.
bool MoveEarthySkyWithEyePointTransformAzimuth::computeLocalToWorldMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const
{
   if ( osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv) )
   {
      osg::Vec3 eyePointLocal = cv->getEyeLocal();
      matrix.preMult(osg::Matrix::translate(eyePointLocal.x(),eyePointLocal.y(),eyePointLocal.z()));
      matrix.preMult(osg::Matrix::rotate(osg::DegreesToRadians(mAzimuth-90.0f), 0.0f, 0.0f, 1.0f));
   }
   return true;
}

///Get the transformation matrix which moves from world coords to local coords.
bool MoveEarthySkyWithEyePointTransformAzimuth::computeWorldToLocalMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const
{
  if ( osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv) )
   {
      osg::Vec3 eyePointLocal = cv->getEyeLocal();
      matrix.postMult(osg::Matrix::translate(-eyePointLocal.x(),-eyePointLocal.y(),-eyePointLocal.z()));
   }
   return true;
}

class SkyDomeShader : public osg::Referenced
{
public:
   SkyDomeShader()
   {
      mLightScatterinVP = new osg::VertexProgram();
      mDomeFP = new osg::FragmentProgram();
      
      char data[] =
      "!!ARBvp1.0                                              \n"
      "                                                        \n"
      "   PARAM mvp[4] = { state.matrix.mvp };                 \n"
      "   PARAM camera = program.local[0];                     \n"
      "   PARAM dir = program.local[1];                        \n"
      "   PARAM betaRay = program.local[2];                    \n"
      "   PARAM betaMie = program.local[3];                    \n"
      "                                                          \n"
      "   # Energy / (betaRay + betaMie)                       \n"
      "   PARAM energyOverRayMie = program.local[4];              \n"
      "                                                            \n"
      "   # G = greenstein                                         \n"
      "   # (-G * 2.0, G * G + 1.0, (1.0 - G) * (1.0 - G))  \n"
      "   PARAM greenstein = program.local[5];                  \n"
      "                                                        \n"
      "   PARAM time = program.local[6];                         \n"
      "                                                         \n"
      "   ATTRIB xyz = vertex.position;                        \n"
      "   ATTRIB texcoord = vertex.texcoord;                    \n"
      "   ATTRIB norm = vertex.normal;                         \n"
      "                                                         \n"
      "   DP4 result.position.x, mvp[0], xyz;                  \n"
      "   DP4 result.position.y, mvp[1], xyz;                  \n"
      "   DP4 result.position.z, mvp[2], xyz;                   \n"
      "   DP4 result.position.w, mvp[3], xyz;                \n"
      "                                                        \n"
      "   TEMP temp;                                  \n"
      "   SUB temp, camera, xyz;                      \n"
      "   DP3 temp.w, temp, temp;                      \n"
      "   RSQ temp.w, temp.w;                         \n"
      "   MUL temp, temp, temp.w;                        \n"
      "                                               \n"
      "   TEMP cos;                                         \n"
      "   DP3 cos.x, temp, dir;                          \n"
      "   MUL cos.y, cos.x, cos.x;   # cos * cos           \n"
      "   SUB cos.z, 1.0, cos.y;      # 1.0 - cos * cos       \n"
      "                                                      \n"
      "   TEMP ray;                                             \n"
      "   MUL ray, cos.z, 0.0597;      # 3.0 / (16.0 * PI)  \n"
      "   MUL ray, ray, betaRay;                            \n"
      "                                                     \n"
      "   TEMP mie;                                         \n"
      "   MAD mie, greenstein.x, cos.x, greenstein.y;       \n"
      "   MOV cos.w, 1.5;                                   \n"
      "   POW mie.x, mie.x, cos.w;                          \n"
      "   RCP mie.x, mie.x;                                 \n"
      "   MUL mie, mie.x, greenstein.z;                      \n"
      "   MUL mie, mie, 0.0796;      # 1.0 / (4.0 * PI)       \n"
      "   MUL mie, mie, betaMie;                            \n"
      "                                                     \n"
      "   SUB temp, camera, xyz;                         \n"
      "   DP3 temp.w, temp, temp;                           \n"
      "   RSQ temp.w, temp.w;                               \n"
      "   RCP temp.w, temp.w;      # distance to camera        \n"
      "                                                     \n"
      "   TEMP fog;                                       \n"
      "   ADD fog, betaRay, betaMie;                         \n"
      "   MUL fog, fog, temp.w;                                \n"
      "   MUL fog, fog, 0.693;   # ln(2.0)                  \n"
      "                                                     \n"
      "   EX2 fog.x, -fog.x;                                \n"
      "   EX2 fog.y, -fog.y;                                \n"
      "   EX2 fog.z, -fog.z;                                \n"
      "                                                     \n"
      "   SUB temp, 1.0, fog;      # 1.0 - fog                 \n"
      "                                                     \n"
      "   TEMP scattering;                                  \n"
      "   ADD scattering, ray, mie;                          \n"
      "   MUL scattering, scattering, energyOverRayMie;        \n"
      "   MUL scattering, scattering, temp;                  \n"
      "                                                        \n"
      "   MOV result.texcoord[0], texcoord;                    \n"
      "   MOV result.texcoord[1], scattering;                 \n"
      "   MOV result.texcoord[2], fog;                         \n"
      "   MOV result.texcoord[3], norm;                        \n"
      "   MOV result.color, vertex.color;                      \n"
      "   END                                                  \n";
      
      mLightScatterinVP->setVertexProgram(data);
      
      char data2[] =
      "!!ARBfp1.0                                     \n"
      "                                                \n"
      "   # brightness, contrast                      \n"
      "   PARAM bc = program.local[0];                 \n"
      "                                                \n"
      "   TEMP clouds;                                   \n"
      "   MOV clouds, fragment.color;                    \n"
      "                                                  \n"
      "   TEMP color;                                 \n"
      "   LRP color, fragment.texcoord[2], clouds, fragment.texcoord[1];  \n"
      "                           \n"
      "   TEMP temp; \n"
      "   SUB temp, color, 0.5;  \n"
      "   MAD color, temp, bc.y, color;   \n"
      "   ADD result.color, color, bc.x;  \n"
      "                                      \n"
      "   END  \n";
      mDomeFP->setFragmentProgram( data2 );
      
      lambda = osg::Vec3(1.0f/650e-9f, 1.0f/570e-9f, 1.0f/475e-9f);
      
      for (int i = 0; i < 3; i++)
      {
         lambda2[i] = lambda[i] * lambda[i];
         lambda4[i] = lambda2[i] * lambda2[i];
      }
      
      //constants
      n = 1.003f; //refractive index of air
      pn = 0.035f; //depolarization factor of air
      
      mBrightness = 0.125f;
      mContrast = 0.15f;
      
      greenstein = 0.8f * 1.5f; ///< Magic number
      
   }
   virtual ~SkyDomeShader(){
   }
   
   ///Update the shader with new values
   void Update(const osg::Vec3d& sunVec,//const osg::Vec2& sunDir,
               float turbidity, 
               float energy, 
               float molecules )
   {
      mLightScatterinVP->setProgramLocalParameter(0, osg::Vec4(0.f, 0.f, 0.f, 0.f) );
      mLightScatterinVP->setProgramLocalParameter(1, osg::Vec4( sunVec[0], sunVec[1], sunVec[2], 0));
      
      float tempMie = 0.434 * ConcentrationFactor(turbidity) * osg::PI * (2 * osg::PI) * (2 * osg::PI) * 0.5;
      betaMie = osg::Vec3(lambda2[0] * 0.685,lambda2[1] * 0.679,lambda2[2] * 0.67) * tempMie;
      
      //Rayleigh scattering
      float tempRay = osg::PI * osg::PI * (n * n - 1.0) * (n * n - 1.0) * (6.0 + 3.0 * pn) / (6.0 - 7.0 * pn) / molecules;
      betaRay = lambda4 * 8.0 * tempRay * osg::PI / 3.0;
      
      mLightScatterinVP->setProgramLocalParameter(2, osg::Vec4(betaRay,0) );
      mLightScatterinVP->setProgramLocalParameter(3, osg::Vec4(betaMie, 0));
      mLightScatterinVP->setProgramLocalParameter(4, osg::Vec4(energy / (betaRay[0] + betaMie[0]),
                                                               energy / (betaRay[1] + betaMie[1]),
                                                               energy / (betaRay[2] + betaMie[2]),0));
      mLightScatterinVP->setProgramLocalParameter(5, osg::Vec4(-greenstein * 2.0,
                                                               greenstein * greenstein + 1.0,
                                                               (1.0 - greenstein) * (1.0 - greenstein),0));
      
      mDomeFP->setProgramLocalParameter(0, osg::Vec4( mBrightness, mContrast, 0, 0));
   }
   
   osg::VertexProgram* GetLightScatterinVP() { return mLightScatterinVP.get(); }
   osg::FragmentProgram* GetDomeFP() { return mDomeFP.get(); }
   
private:
   
   ///our vertex program pointer
   osg::ref_ptr<osg::VertexProgram> mLightScatterinVP;
   ///our fragment program pointer
   osg::ref_ptr<osg::FragmentProgram> mDomeFP;
   
   osg::Vec3 lambda;
   osg::Vec3 lambda2;
   osg::Vec3 lambda4;
   float n; ///<Refractive index of air
   float pn; ///<depolarization factor of air
   float greenstein; ///<eccentricity value
   float mBrightness; ///<scene brightness adjustment
   float mContrast; ///<scene contrast adjustment
   osg::Vec3 betaRay;
   osg::Vec3 betaMie;
   
   float ConcentrationFactor(float turbidity)
   {
      return (6.544 * turbidity - 6.51) * 1e-17; ///<more magic numbers
   }
   
};

class  MakeSkyDome
   {
   public:
      
      MakeSkyDome(float radius, bool capEnabled = true);
      ~MakeSkyDome();
      
      osg::Geode* Compute();
      
   protected:
      
   private:
      unsigned int GetNumLevels();
      void SetCoordinatesAndColors();
      void SetCapCoordinatesAndColors();
      void CreateTriangleStrips();
      osg::StateSet* CreateStateSet() const;
      
      float                  mRadius;
      osg::Geometry*         mGeom;
      osg::Vec3Array*        mCoordArray;
      osg::Vec4Array*        mColorArray;
      std::vector<float>     mLevelHeight;
      std::vector<osg::Vec3> mCCArray;
      bool                   theCapEnabled;
      
      static const unsigned int VERTS_IN_CIRCUM;
   };
const unsigned int MakeSkyDome::VERTS_IN_CIRCUM = 19;

////////////////////////////////////////////////////////////////////////////////
MakeSkyDome::MakeSkyDome(float radius, bool capEnabled)
: mRadius(radius)
, mGeom(new osg::Geometry()),
theCapEnabled(capEnabled)
{
   mLevelHeight.push_back(-9.0f);
   mLevelHeight.push_back(-9.0f);
   mLevelHeight.push_back(0.0f);
   mLevelHeight.push_back(7.2f);
   mLevelHeight.push_back(15.0f);
   mLevelHeight.push_back(90.0f);
   
   mCCArray.push_back(osg::Vec3(0.15f, 0.25f, 0.1f));
   mCCArray.push_back(osg::Vec3(0.6f, 0.6f, 0.7f));
   mCCArray.push_back(osg::Vec3(0.4f, 0.4f, 0.7f));
   mCCArray.push_back(osg::Vec3(0.2f, 0.2f, 0.6f));
   mCCArray.push_back(osg::Vec3(0.1f, 0.1f, 0.6f));
   mCCArray.push_back(osg::Vec3(0.1f, 0.1f, 0.7f));
   
   mCoordArray = new osg::Vec3Array(VERTS_IN_CIRCUM * mLevelHeight.size());
   mColorArray = new osg::Vec4Array(VERTS_IN_CIRCUM * mLevelHeight.size());
}

////////////////////////////////////////////////////////////////////////////////
MakeSkyDome::~MakeSkyDome()
{
}

////////////////////////////////////////////////////////////////////////////////
osg::Geode* MakeSkyDome::Compute()
{
   SetCoordinatesAndColors();
   
   CreateTriangleStrips();
   
   mGeom->setVertexArray(mCoordArray);
   mGeom->setColorArray(mColorArray);
   mGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
   
   mGeom->setStateSet(CreateStateSet());
   //mGeom->setUseVertexBufferObjects(true);
//   mGeom->setFastPathHint(false);
   osg::Geode* geode = new osg::Geode;
   geode->addDrawable(mGeom);
   geode->setName("Sky");
  
   return geode;
}

////////////////////////////////////////////////////////////////////////////////
unsigned int MakeSkyDome::GetNumLevels()
{
   return (mLevelHeight.size() - (theCapEnabled? 0: 1));
}

////////////////////////////////////////////////////////////////////////////////
void MakeSkyDome::SetCoordinatesAndColors()
{
   unsigned int ci = theCapEnabled?VERTS_IN_CIRCUM:0;
   
   // Set dome coordinates & colors
   for(unsigned int i = theCapEnabled ? 1: 0; i < GetNumLevels(); i++)
   {
      for(unsigned int j = 0; j < VERTS_IN_CIRCUM; j++)
      {
         float alpha = osg::DegreesToRadians(mLevelHeight[i+(theCapEnabled ? 0: 1)]);
         float theta = osg::DegreesToRadians((float)(j * 20));
         
         float x = mRadius * cos(alpha) * cos(theta);
         float y = mRadius * cos(alpha) * -sin( theta );
         float z = mRadius * sin(alpha);
         
         assert(ci < mCoordArray->size());
         (*mCoordArray)[ci].set(x,y,z);
         
         assert(ci < mColorArray->size());
         assert(i < mCCArray.size());
         (*mColorArray)[ci].set(mCCArray[i].x(), mCCArray[i].y(), mCCArray[i].z(), 1.f);
         
         ci++;
      }
   }
   
   SetCapCoordinatesAndColors();
}

////////////////////////////////////////////////////////////////////////////////
void MakeSkyDome::SetCapCoordinatesAndColors()
{
   if(theCapEnabled)
   {
      osg::Vec3 capCenter(0.0f, 0.0f, mRadius * osg::DegreesToRadians(sin(mLevelHeight[0])));
      
      for(unsigned int j = 0; j < VERTS_IN_CIRCUM; j++)
      {
         (*mCoordArray)[j] = capCenter;
         
         assert(j < mColorArray->size());
         (*mColorArray)[j].set(mCCArray.back().x(), mCCArray.back().y(), mCCArray.back().z(), 1.0f);
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
void MakeSkyDome::CreateTriangleStrips()
{
   for(unsigned int i = 0; i < GetNumLevels() - 1; i++)
   {
      osg::DrawElementsUShort* drawElements = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP);
      drawElements->reserve(38);
      
      for(unsigned j = 0; j < VERTS_IN_CIRCUM; j++)
      {
         drawElements->push_back((i + 1) * VERTS_IN_CIRCUM + j);
         drawElements->push_back((i + 0) * VERTS_IN_CIRCUM + j);
      }
      
      mGeom->addPrimitiveSet(drawElements);
   }
}

////////////////////////////////////////////////////////////////////////////////
osg::StateSet* MakeSkyDome::CreateStateSet() const
{
   osg::StateSet *dstate = new osg::StateSet;
   
   dstate->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
   dstate->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
   
   // clear the depth to the far plane.
   osg::Depth* depth = new osg::Depth(osg::Depth::ALWAYS);
   depth->setWriteMask(false);   
   dstate->setAttributeAndModes(depth,osg::StateAttribute::ON);
   dstate->setMode(GL_FOG, osg::StateAttribute::OFF);
   dstate->setMode(GL_BLEND, osg::StateAttribute::ON);
   
   dstate->setRenderBinDetails(-2,"RenderBin");
   
   return dstate;
}
#if 0
static double fade_coefficient(double currentAltitude, 
                             double refAltitude)
{
   return exp( - currentAltitude / refAltitude );
}
#endif
static void fade_to_black(osg::Vec3 sky_color[], 
                          double currentAltitude, 
                          double refAltitude,
                          int count)
{
   double d = mkUtils::fadeCoefficient(currentAltitude, refAltitude);
   d = ossim::clamp(d, 0.0, 1.0);
   for(int i = 0; i < count ; i++)
   {
      sky_color[i] *= d;
   }
}
static double fade_to_black(double curretValue, 
                            double currentAltitude, 
                            double refAltitude)
{
   double d = mkUtils::fadeCoefficient(currentAltitude, refAltitude);
   d = ossim::clamp(d, 0.0, 1.0);
   return curretValue*d;
}

class SkyDome : public EnvEffect
{
public:
   SkyDome(const std::string& name = "SkyDome", bool createCapGeometry = true, float radius = 6000.0f)
   :mEnableCap(createCapGeometry),
   theMaxAltitude(80000)
   {
      SetOSGNode(new osg::Group());
      mBaseColor.set(0.5f, 0.5f, 0.2f);
      Config(radius);
   }
protected:
   virtual ~SkyDome()
   {
      
   }
   
public:
   ///sets the base color
   void SetBaseColor(const osg::Vec3& color)
   {
      osg::Geometry* geom = mGeode->getDrawable(0)->asGeometry();
      osg::Array* array = geom->getColorArray();
      if (array && array->getType() == osg::Array::Vec4ArrayType)
      {
         mBaseColor.set(color);
         
         osg::Vec4Array* color = static_cast<osg::Vec4Array*>(array);
         unsigned int limit = mEnableCap ? 38: 19;
         
         for (unsigned int i=0; i<limit; i++)
         {
            assert(i<color->size());
            (*color)[i].set(mBaseColor[0], mBaseColor[1], mBaseColor[2], 1.f);
         }
         geom->dirtyDisplayList();
      }
      
   }
   
   ///gets the base color
   void GetBaseColor(osg::Vec3& color) const { color.set(mBaseColor); }
   
   bool GetCapEnabled() const { return mEnableCap; }
   
   ///the virtual paint function
   /** 0 degrees  = horizon
    *  90 degrees = high noon
    *  - degrees  = below horizon
    */
   virtual void Repaint(const osg::Vec3& skyColor, 
                        const osg::Vec3& fogColor,
                        double sunAngle, 
                        double sunAzimuth,
                        double visibility,
                        double altitude = 0.0)
   {
      outer_param.set(0.0, 0.0, 0.0);
      middle_param.set(0.0, 0.0, 0.0);
      
      outer_diff.set(0.0, 0.0, 0.0);
      middle_diff.set(0.0, 0.0, 0.0);
      
      // Check for sunrise/sunset condition
      if(IsSunsetOrSunrise(sunAngle))
      {
         // 0.0 - 0.4
         outer_param.set((10.0 - std::abs(sunAngle)) / 20.0,
                         (10.0 - std::abs(sunAngle)) / 40.0,
                         -(10.0 - std::abs(sunAngle)) / 30.0);
         
         middle_param.set((10.0 - std::abs(sunAngle)) / 40.0,
                          (10.0 - std::abs(sunAngle)) / 80.0,
                          0.0);
         
         outer_diff = outer_param / 9.0;
         
         middle_diff = middle_param / 9.0;
      } 
      
      outer_amt.set(outer_param);
      middle_amt.set(middle_param);
      
      // First, recaclulate the basic colors
      
      CalcNewColors(visibility, skyColor, fogColor, altitude);
      
      AssignColors();
   }
   void setMaxAltitude(double maxAltitude)
   {
      theMaxAltitude = maxAltitude;
   }
private:
   
   
   /// Build the sky dome
   void Config(float radius)
   {
      osg::Group* group = new osg::Group();

      mGeode = MakeSkyDome(radius, mEnableCap).Compute();
      GetOSGNode()->asGroup()->addChild(mGeode.get());
   }
   bool IsSunsetOrSunrise(double sunAngle) const
   {
      return ((sunAngle > -10.0) && (sunAngle < 10.0));
   }
   osg::Vec3 CalcCenterColors(double vis_factor,
                              const osg::Vec3& skyColor, 
                              const osg::Vec3& fogColor) const
   {
      osg::Vec3 center_color;
      
      for (unsigned int j = 0; j < 3; j++) 
      {
         const osg::Vec3::value_type diff = skyColor[j] - fogColor[j];
         center_color[j] = skyColor[j] - diff * (1.0 - vis_factor);
      }
      
      return center_color;
      
   }
   
   void CalcNewColors(double visibility, const osg::Vec3& skyColor, 
                      const osg::Vec3& fogColor,
                      double altitude = 0.0)
   {
      center_color = CalcCenterColors(GetVisibilityFactor(visibility), skyColor, fogColor);
      
      for (unsigned int i = 0; i < 9; i++) 
      {
         SetUpperMiddleLowerColors(skyColor, fogColor, i, visibility, altitude);
         
         outer_amt -= outer_diff;
         middle_amt -= middle_diff;
      }
      
      outer_amt.set(0.0, 0.0, 0.0);
      middle_amt.set(0.0, 0.0, 0.0);
      
      for (unsigned int i = 9; i < 19; i++) 
      {
         SetUpperMiddleLowerColors(skyColor, fogColor, i, visibility, altitude);
         
         outer_amt += outer_diff;
         middle_amt += middle_diff;
      }
      
      for (unsigned int i = 0; i < 19; i++) 
      {
         bottom_color[i] = fogColor;
      }
   }
   
   
   double CalcCVF(double visibility) const
   {
      return ossim::clamp(visibility, 0.0, 20000.0);
   }
   
   void SetUpperMiddleLowerColors(const osg::Vec3& skyColor, const osg::Vec3& fogColor,
                                  unsigned int i, double visibility, double altitude=0.0 )
   {
      const double cvf = CalcCVF(visibility);
      
      for (unsigned int j = 0; j < 3; j++) 
      {
         const osg::Vec3::value_type diff = skyColor[j] - fogColor[j];
         
         upper_color[i][j] = skyColor[j] - diff *
         (1.0 - GetVisibilityFactor(visibility) * (0.7 + 0.3 * cvf/20000.f));
         
         middle_color[i][j] = skyColor[j] - diff *
         (1.0 - GetVisibilityFactor(visibility) * (0.1 + 0.85 * cvf/20000.f))
         + middle_amt[j];
         
         lower_color[i][j] = fogColor[j] + outer_amt[j];
         
         
         upper_color[i][j] = ossim::clamp(upper_color[i][j], 0.f, 1.f);
         middle_color[i][j] = ossim::clamp(middle_color[i][j], 0.f, 1.f);
         lower_color[i][j] = ossim::clamp(lower_color[i][j], 0.f, 1.f);
         

      }
      //fade_to_black(&(*dome_cl)[0], asl * center_elev, 1);
      fade_to_black(&upper_color[i], altitude, theMaxAltitude, 1);
      fade_to_black(&middle_color[i], altitude, theMaxAltitude, 1);
      fade_to_black(&lower_color[i], altitude, theMaxAltitude, 1);
   }
   
   void AssignColors() const
   {
      osg::Geometry* geom = mGeode->getDrawable(0)->asGeometry();
      osg::Array* array = geom->getColorArray();
      if (array && array->getType()==osg::Array::Vec4ArrayType)
      {
         osg::Vec4Array* color = dynamic_cast<osg::Vec4Array*>(array);
         // Set cap color
         if(mEnableCap)
         {
            for (unsigned int i = 0; i < 19; i++)
            {
               (*color)[i].set(bottom_color[i][0], bottom_color[i][1], bottom_color[i][2], 1.0);
            }
         }
         
         // Set dome colors
         unsigned int c = mEnableCap?19:0;
         for(unsigned int i = 0; i < 19; i++)
         {
            (*color)[c].set(bottom_color[i][0], bottom_color[i][1], bottom_color[i][2], 1.0);
            (*color)[c+19].set(lower_color[i][0], lower_color[i][1], lower_color[i][2], 1.0);
            (*color)[c+19+19].set(middle_color[i][0], middle_color[i][1], middle_color[i][2], 1.0);
            (*color)[c+19+19+19].set(upper_color[i][0], upper_color[i][1], upper_color[i][2], 1.0);
            (*color)[c+19+19+19+19].set(center_color[0], center_color[1], center_color[2], 1.0);
            c++;
         }
         geom->dirtyDisplayList();
     }
   }
   double GetVisibilityFactor(double visibility) const
   {
      if (visibility < 3000.0) 
      {
         double vis_factor = (visibility - 1000.0) / 2000.0;
         
         ossim::clamp(vis_factor, 0.0, 1.0);
         return vis_factor;
      }
      
      return 1.0;
      
   }
   osg::ref_ptr<osg::Geode> mGeode;
   osg::ref_ptr<MoveEarthySkyWithEyePointTransformAzimuth> mXform;
   
   bool mEnableCap;
   osg::Vec3 mBaseColor;
   osg::Vec3 outer_param, outer_amt, outer_diff;
   osg::Vec3 middle_param, middle_amt, middle_diff;
   osg::Vec3 center_color;
   osg::Vec3 upper_color[19];
   osg::Vec3 middle_color[19];
   osg::Vec3 lower_color[19];
   osg::Vec3 bottom_color[19];
   osg::ref_ptr<osg::Group> theOsgNode;
   ossim_float64 theMaxAltitude;
};

class ossimPlanetEphemeris::EphemerisData
{
public:
   typedef std::vector<osg::ref_ptr<ossimPlanetCloudLayer> > CloudLayers;
   ~EphemerisData()
   {
      theLayer = 0;
   }
   EphemerisData(ossimPlanetEphemeris* layer,
                 ossim_uint64 membersBitMap);
   void setMembers(ossim_uint64 membersBitMap);
   void traverse(osg::NodeVisitor& nv);
   void updatePositions(osg::NodeVisitor& nv);
   void setMoonLightIndex(ossim_uint32 idxNumber)
   {
      theMoonLightIdx = idxNumber;
      if(theMoonLightSource.valid())
      {
         theMoonLightSource->getLight()->setLightNum(theMoonLightIdx);
      }
   }
   ossim_uint32 moonLightIndex()const
   {
      return theMoonLightIdx;
   }
   void setSunLightIndex(ossim_uint32 idxNumber)
   {
      theSunLightIdx = idxNumber;
      if(theSunLightSource.valid())
      {
         theSunLightSource->getLight()->setLightNum(theSunLightIdx);
      }
   }
   ossim_uint32 sunLightIndex()const
   {
      return theSunLightIdx;
   }
   ossim_uint64 members()const
   {
      return theMembers;
   }
   void setDate(const ossimLocalTm& date)
   {
      theDate = date;
   }
   const ossimLocalTm& date()const
   {
      return theDate;
   }
   void setAutoUpdateToCurrentTimeFlag(bool flag)
   {
      theAutoUpdateCurrenTimeFlag = flag;
   }
   bool autoUpdateCurrentTimeFlag()const
   {
      return theAutoUpdateCurrenTimeFlag;
   }
   void setApplySimulationTimeOffsetFlag(bool flag)
   {
      theApplySimulationTimeOffsetFlag = flag;
   }
   void setAdjustedVisibility(ossim_float64 vis)
   {
      double sqrt_m_log01 = sqrt(-log(0.01));
      float density = sqrt_m_log01 / vis;
      
      theFog->setDensity(density);
      if(theLayer->model())
      {
         theFog->setEnd(vis/theLayer->model()->getNormalizationScale());
      }
   }
   void setVisibility(ossim_float64 visibility)
   {
      theVisibility = visibility;
      setAdjustedVisibility(visibility);
   }
   double getVisibility()const
   {
      return theVisibility;
   }
   void setFogNear(ossim_float64 val)
   {
      theFogNear = val;
      theFogNear = val;
      
      if (theFogNear < 0.f)
      {
         theFogNear = 0.f;
      }
      if (theFogNear > theVisibility)
      {
         theFogNear = theVisibility;
      }
      
      if(theLayer->model())
      {
         theFog->setEnd(theVisibility/theLayer->model()->getNormalizationScale());
         theFog->setStart(theFogNear/theLayer->model()->getNormalizationScale());///osg::WGS_84_RADIUS_EQUATOR);
      }
   }
   void setFogFar(ossim_float64 val)
   {
      theFog->setEnd(val);///osg::WGS_84_RADIUS_EQUATOR);
   }
   void setFogDensity(ossim_float64 val)
   {
      theFog->setDensity(val);
   }
   void setFogMode(FogMode mode)
   {
      theFogMode = mode;
      osg::Fog::Mode fm;
      short attr = osg::StateAttribute::OFF;
      
      switch (mode)
      {
         case ossimPlanetEphemeris::LINEAR:  fm = osg::Fog::LINEAR; break;
         case ossimPlanetEphemeris::EXP:     fm = osg::Fog::EXP;    break;
         case ossimPlanetEphemeris::EXP2:    fm = osg::Fog::EXP2;   break;
#if 0
         case ossimPlanetEphemer::ADV:
         {
            fm = osg::Fog::LINEAR;
            if (GetFogEnable())  { attr = osg::StateAttribute::ON;  }
            else                 { attr = osg::StateAttribute::OFF; }
         }
#endif
            break;
         default: fm = osg::Fog::LINEAR; break;
      }

      theFog->setMode(fm);
   }
   void updateSunLight()
   {
      double coefficient = mkUtils::fadeCoefficient(theEyeLlh[2], theMaxAltitudeToDoSunriseSunsetColorAdjustment);
      coefficient = ossim::clamp(coefficient, 0.0, 1.0);
      if(!theUseFadingFlagForSunriseSunsetCalculation)
      {
         if(theEyeLlh[2] > theMaxAltitudeToDoSunriseSunsetColorAdjustment)
         {
            coefficient = 0.0;
         }
         else
         {
            coefficient = 1.0;
         }
      }
      osg::Vec3d position = theSunXyz;
      osg::Vec3d direction = -theSunXyz;
      direction.normalize();
      if(!theSunLightSource.valid())
      {
         return;
      }
      double red   = theSunElevation * 0.5;
      double green = theSunElevation * 0.25;
      double blue  = theSunElevation * 0.125;
      red = ossim::clamp(red,   0.0, 1.0);
      green = ossim::clamp(green, 0.0, 1.0);
      blue = ossim::clamp(blue,  0.0, 1.0);
      osg::Vec3 diff1(1.0, 1.0, 1.0);
      osg::Vec3 amb1(.01, 0.01, 0.01);
      osg::Vec3 amb2(red, green, blue);
      osg::Vec3 diff2(red, green, blue);
      
      // setup a calculated ambient based on sun elevation 
      //
      red   = (theSunElevation + 10.0) * 0.04;
      green = (theSunElevation + 10.0) * 0.02;
      blue  = (theSunElevation + 10.0) * 0.01;
      red = ossim::clamp(red,   0.01, 0.3);
      green = ossim::clamp(green, 0.01, 0.3);
      blue = ossim::clamp(blue,  0.01, 0.3);
      amb2 = osg::Vec3d(red, green, blue);
      
      // fade the colors together to the reference height
      //
      osg::Vec3 diff = diff2*(coefficient) + diff1*(1.0-coefficient);
      osg::Vec3 amb = amb2*(coefficient)   + amb1*(1.0-coefficient);
      osg::Vec4 d(diff[0], diff[1], diff[2], 1.0);

      osg::Vec4 a(amb[0], amb[1], amb[2], 1.0);
      theSunColor = diff;
      theSunLightSource->getLight()->setDiffuse(d);
      theSunLightSource->getLight()->setSpecular(d);
      theSunLightSource->getLight()->setAmbient(a);
      theSunLightSource->getLight()->setPosition(osg::Vec4d(position, 0.0));
      theSunLightSource->getLight()->setDirection(direction);
      
      if(theSunLightCallback.valid())
      {
         (*theSunLightCallback)(theLayer,
                                theSunLightSource.get());
         osg::Vec4d diff = theSunLightSource->getLight()->getDiffuse();
         theSunColor = osg::Vec3d(diff[0], diff[1], diff[2]);    
      }
      

      osg::Vec4 d2(diff1[0], diff1[1], diff1[2], 1.0);
      osg::Vec4 a2(amb1[0], amb1[1], amb1[2], 1.0);

      theSunLightSourceMoonPhase->getLight()->setDiffuse(d2);
      theSunLightSourceMoonPhase->getLight()->setSpecular(d2);
      theSunLightSourceMoonPhase->getLight()->setAmbient(a2);
      theSunLightSourceMoonPhase->getLight()->setPosition(osg::Vec4d(position, 0.0));
      theSunLightSourceMoonPhase->getLight()->setDirection(direction);
   }
   void updateMoonLight()
   {
      if(theMembers&ossimPlanetEphemeris::MOON_LIGHT)
      {
         osg::Vec3d position = theSunXyz;
         osg::Vec3d direction = -theSunXyz;
         direction.normalize();
         osg::Vec3d sunxyz=theSunXyz, moonxyz = theMoonXyz;
         sunxyz.normalize();
         moonxyz.normalize();
         // Part of the calculations are used from osgEpehermis code
         //
         const double angle = (sunxyz*moonxyz);
         const double moonBrightness = ((angle) * -0.5 + 0.5);
         const double moonlight = moonBrightness*.5;
         // Make the final values a little bit blue, and always add in a tiny bit of ambient starlight
         osg::Vec4 ambient(moonlight * 0.32 + 0.05, moonlight * 0.32 + 0.05, moonlight * 0.4 + 0.05, 1);
         osg::Vec4 diffuse(moonlight * 0.8, moonlight * 0.8, moonlight, 1);
         osg::Light &light = *(theMoonLightSource->getLight());
         light.setAmbient( ambient );
         light.setDiffuse( diffuse );
         light.setSpecular( diffuse );
         
         theMoonLightSource->getLight()->setPosition( osg::Vec4d(theMoonXyz,0.0) );
         theMoonLightSource->getLight()->setDirection(-moonxyz);
         if(theMoonLightCallback.valid())
         {
             (*theMoonLightCallback)(theLayer,
                                     theMoonLightSource.get());
         }
      }
   }
   void updateEnvColors()
   {
      float skyBright = theSkyLightTable->Interpolate(theSunElevation);
      theModSkyColor = theSkyColor * skyBright;
      
      // Modify the fog color based on sky brightness
      theModFogColor = theFogColor * skyBright;
   }
   void updateFogColor()
   {
      // Calculate the fog color in the direction of the sun for
      // sunrise/sunset effects.
      float red   = (theModFogColor[0] + 2.f * theSunColor[0] * theSunColor[0]) / 3.f;
      float green = (theModFogColor[1] + 2.f * theSunColor[1] * theSunColor[1]) / 3.f;
      float blue  = (theModFogColor[2] + 2.f * theSunColor[2]) / 3.f;
      
      // interpolate between the sunrise/sunset color and the color
      // at the opposite direction of this effect. Take in account
      // the current visibility.
      double vis = getVisibility();
      const float MAX_VISIBILITY = 20000;
      
      // Clamp visibility
      if (vis > MAX_VISIBILITY)
      {
         vis = MAX_VISIBILITY;
      }
      
      double sunRotation = osg::DegreesToRadians(-95.0);
      double heading     = osg::DegreesToRadians(-95.0);
      
      double rotation = -(sunRotation + osg::PI) - heading;
      
      float inverseVis = 1.f - (MAX_VISIBILITY - vis) / MAX_VISIBILITY;
      float sif = 0.5f - cos(osg::DegreesToRadians(theSunElevation) * 2.f) / 2.f + 0.000001f;
      
      float rf1  = std::abs((rotation-osg::PI) / osg::PI); // difference between eyepoint heading and sun heading (rad)
      float rf2 = inverseVis * pow(rf1 * rf1, 1.0f / sif);
      
      float rf3  = 1.f - rf2;
      
      theModFogColor[0] = rf3 * theModFogColor[0] + rf2 * red;
      theModFogColor[1] = rf3 * theModFogColor[1] + rf2 * green;
      theModFogColor[2] = rf3 * theModFogColor[2] + rf2 * blue;
      
      // now apply the fog's color
      theFog->setColor(osg::Vec4(theModFogColor[0], theModFogColor[1], theModFogColor[2], 1.f));
   }
   bool getFogEnableFlag()const
   {
      return theFogEnableFlag;
   }
   void setFogEnableFlag(bool enable)
   {
      theFogEnableFlag = enable;
      if(theRootStateSet.valid())
      {
         short attr = osg::StateAttribute::ON;
         
         if (enable)
         {
            attr = osg::StateAttribute::ON;
         }
         else
         {
            attr = osg::StateAttribute::OFF;
         }
         
         theRootStateSet->setAttributeAndModes(theFog.get(), attr);
#if 0
         if (GetFogMode() == Environment::ADV)
         {
            // if we're using ADV, then we turn on/off this shader which overrides
            // the standard openGL fog
            state->setAttributeAndModes(mSunlightShader->GetLightScatterinVP(), attr);
            state->setAttributeAndModes(mSunlightShader->GetTerrainFP(), attr);
            
            // if we're using a skyDome, turn on/off its shader
            if (mSkyDome.valid())
            {
               state = mSkyDome.get()->GetOSGNode()->getOrCreateStateSet();
               state->setAttributeAndModes(mSkyDomeShader->GetLightScatterinVP(), attr);
               state->setAttributeAndModes(mSkyDomeShader->GetDomeFP(), attr);
            }
         }
#endif
      }
   }
   osg::Texture2D* createBillboardTexture(osg::Image* image)
   {
      osg::Texture2D* texture = 0;
      if ( image )
      {
         texture = new osg::Texture2D( image );
         texture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
         texture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);
         texture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
         texture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
      }
      
      return texture;
   }
   ossimPlanetBillboardIcon* createPlanetBillBoard(double diameter,
                                                   const string& name,
                                                   const string& imageFile)
   {
      ossimPlanetBillboardIcon* result = new ossimPlanetBillboardIcon();
      result->setName(name);
      result->setGroundObjectSize(diameter);
      if(!imageFile.empty())
      {
         osg::Image* image = osgDB::readImageFile( imageFile );
         if ( image )
         {
            result->setIcon(createBillboardTexture(image));
         }
      }
      
      return result;
   }
   void setNumberOfCloudLayers(ossim_uint32 numberOfLayers)
   {
      ossim_uint32 idx = 0;
      if(theCloudLayers.size() == numberOfLayers) return; // nothing to do
      if(numberOfLayers == 0)
      {
         theCloudLayers.clear();
         return;
      }
      theCloudLayers.resize(numberOfLayers);
      for(idx = 0; idx < numberOfLayers;++idx)
      {
         if(!theCloudLayers[idx].valid())
         {
            theCloudLayers[idx] = new ossimPlanetCloudLayer();
            theCloudLayers[idx]->setModel(theLayer->theModel.get());
            theCloudLayers[idx]->setCullingActive(false);
         }
      }
   }
   void removeClouds(ossim_uint32 idx, ossim_uint32 count)
   {
      if((idx < theCloudLayers.size())&&
         (count != 0))
      {
         ossim_uint32 maxIdx = ossim::min(idx+count, (ossim_uint32)theCloudLayers.size());;
         CloudLayers::iterator start = theCloudLayers.begin()+idx;
         CloudLayers::iterator end   = theCloudLayers.begin()+maxIdx;
         theCloudLayers.erase(start, end);
      }
   }
                              
   ossimPlanetCloudLayer* cloudLayer(ossim_uint32 idx)
   {
      if(idx < theCloudLayers.size())
      {
         return theCloudLayers[idx].get();
      }
      return 0;
   }
  public:
   class InterpTable : public osg::Referenced
      {
      public:
         InterpTable()
         {
            
         }
         ~InterpTable()
         {
            
         }
         void addEntry(double ind, double dep)
         {
            theTable.insert(std::make_pair(ind, dep));
         }
         double Interpolate(double x) const
         {
            double result = 0.0;
            if(theTable.size() == 0) return result;
            IntensityTableType::const_iterator upper = theTable.lower_bound(x);
            IntensityTableType::const_iterator lower = upper;
            if(upper != theTable.end())
            {
               if(upper != theTable.begin())
               {
                  --lower;
               }
            }
            if(upper!=theTable.end())
            {
               double t = 0.0;
               if(lower!=upper)
               {
                  t = (x-lower->first)/(upper->first-lower->first);
                  result = lower->second + (upper->second-lower->second)*t;
               }
               else
               {
                  result = lower->second;
               }
            }
            else if(upper==theTable.end())
            {
               if(lower!=theTable.end())
               {
                  result = lower->second;
               }
            }
            return result;
         }
       const ossimPlanetEphemeris::IntensityTableType& table()const
         {
            return theTable;
         }
         void setTable(const ossimPlanetEphemeris::IntensityTableType& value)
         {
            theTable = value;
         }
      protected:
         ossimPlanetEphemeris::IntensityTableType theTable;
      };
   
   osg::ref_ptr<InterpTable> theSkyLightTable;

   ossimPlanetEphemeris* theLayer;
   ossim_uint64          theMembers;
   ossimLocalTm          theDate;
   gpstk::SunPosition    theSunPosition;
   gpstk::MoonPosition   theMoonPosition;
   osg::Vec3d            theSunLlh;
   osg::Vec3d            theMoonLlh;
   osg::Vec3d            theSunXyz;
   osg::Vec3d            theMoonXyz;
   
   gpstk::WGS84Ellipsoid theEllipsoidModel;
   //gpstk::WGS84Geoid     theGeoidModel;
   ossim_uint32          theSunLightIdx;
   ossim_uint32          theMoonLightIdx;
   ossim_uint32          theAmbientLightIdx;
   osg::Vec3d            theEyeLlh;
   osg::Vec3d            theEyeXyz;
   osg::ref_ptr<osg::LightSource> theSunLightSource;
   osg::ref_ptr<osg::LightSource> theSunLightSourceMoonPhase;
   osg::ref_ptr<osg::LightSource> theMoonLightSource;
   osg::ref_ptr<osg::LightSource> theAmbientLightSource;
   
   osg::ref_ptr<osg::Group> theLightGroup;
   osg::ref_ptr<osg::MatrixTransform> theObjectGroup;
   osg::ref_ptr<osg::Group> theMoonGroup;
   osg::ref_ptr<osg::Group> theSunGroup;
   
   osg::ref_ptr<osg::MatrixTransform> theSkyDomeTransform;
   osg::ref_ptr<SkyDome>            theSkyDome;

   /**
    * This is the root stateset used for light source definitions
    */
   osg::observer_ptr<osg::StateSet> theRootStateSet;
   osg::observer_ptr<osg::Group>    theRootStateSetGroup;
   
   bool theAutoUpdateCurrenTimeFlag;
   bool theApplySimulationTimeOffsetFlag;
   bool theSettingsChangedFlag;
   
   ossim_float64 theSunElevation;
   ossim_float64 theSunAzimuth;
   osg::Vec3d theGlobalAmbientColor;
   osg::Vec3d theSunColor;
   osg::ref_ptr<ossimPlanetEphemeris::LightingCallback> theSunLightCallback;
   osg::ref_ptr<ossimPlanetEphemeris::LightingCallback> theMoonLightCallback;
   osg::ref_ptr<ossimPlanetEphemeris::LightingCallback> theGlobalAmbientLightCallback;
   osg::Vec3d theSkyColor;
   osg::Vec3d theFogColor;
   osg::Vec3d theModSkyColor;
   osg::Vec3d theModFogColor;
   
   ossim_float64 theVisibility;
   osg::ref_ptr<osg::Fog> theFog;
   ossim_float64 theFogNear;
   ossimPlanetEphemeris::FogMode theFogMode;
   bool theFogEnableFlag;
   
   osg::ref_ptr<ossimPlanetPointModel>    theSunPointModel;
   osg::ref_ptr<ossimPlanetBillboardIcon> theSunBillboard;
   osg::ref_ptr<ossimPlanetPointModel>    theMoonPointModel;
   
   osg::ref_ptr<osg::PositionAttitudeTransform> theMoonModel;
   osg::ref_ptr<osg::PositionAttitudeTransform> theSunModel;
   osg::ref_ptr<osg::Camera> theSkyDomeCamera;
   
   ossim_float64 theMaximumAltitudeToShowDome;
   ossim_float64 theMaximumAltitudeToShowFog;
   ossim_float64 theMaxAltitudeToDoSunriseSunsetColorAdjustment;
   bool          theUseFadingFlagForSunriseSunsetCalculation;
   ossim_int64 theFrameStamp;

   CloudLayers theCloudLayers;
};
ossimPlanetEphemeris::EphemerisData::EphemerisData(ossimPlanetEphemeris* layer,
                                                   ossim_uint64 membersBitMap)
:theLayer(layer),
theSunLightIdx(0),
theMoonLightIdx(1),
theAmbientLightIdx(2),
theAutoUpdateCurrenTimeFlag(false),
theApplySimulationTimeOffsetFlag(false),
theSettingsChangedFlag(false),
theMaximumAltitudeToShowDome(40000),
theMaximumAltitudeToShowFog(20000),
theMaxAltitudeToDoSunriseSunsetColorAdjustment(20000),
theUseFadingFlagForSunriseSunsetCalculation(true),
theFrameStamp(-1)
{
   theFogNear = 1.0;
   theRootStateSet = 0;
   theRootStateSetGroup = 0;
   theGlobalAmbientColor = osg::Vec3d(0.2,0.2,0.2);
   theSkyLightTable = new InterpTable();
   theObjectGroup = new osg::MatrixTransform;
   theLightGroup  = new osg::Group;
   theMoonGroup   = new osg::Group;
   theSunGroup    = new osg::Group;
   theMoonLightSource = new osg::LightSource;
   theSunLightSource  = new osg::LightSource;
   theSunLightSourceMoonPhase = new osg::LightSource;
   theAmbientLightSource = new osg::LightSource;
   theLightGroup->addChild(theSunLightSource.get());
   theLightGroup->addChild(theMoonLightSource.get());
   theLightGroup->addChild(theAmbientLightSource.get());
   theLightGroup->setCullingActive(false);
   theSunLightSource->getLight()->setLightNum(theSunLightIdx);
   theSunLightSource->setLocalStateSetModes(osg::StateAttribute::OFF); 
   theSunLightSourceMoonPhase->getLight()->setLightNum(theSunLightIdx);
   theSunLightSourceMoonPhase->setLocalStateSetModes(osg::StateAttribute::OFF); 
   theMoonLightSource->getLight()->setLightNum(theMoonLightIdx);
   theMoonLightSource->setLocalStateSetModes(osg::StateAttribute::OFF); 
   theAmbientLightSource->getLight()->setLightNum(theAmbientLightIdx);
   theAmbientLightSource->setLocalStateSetModes(osg::StateAttribute::OFF); 

   theAmbientLightSource->getLight()->setDiffuse(osg::Vec4d(0.0,0.0,0.0,1.0));
   theAmbientLightSource->getLight()->setSpecular(osg::Vec4d(0.0,0.0,0.0,1.0));
   theAmbientLightSource->getLight()->setAmbient(osg::Vec4d(theGlobalAmbientColor[0],
                                                            theGlobalAmbientColor[1],
                                                            theGlobalAmbientColor[2], 1.0));
   theSunPointModel = new ossimPlanetPointModel;
   theMoonPointModel = new ossimPlanetPointModel;
   theMoonModel = new osg::PositionAttitudeTransform;
   theSunModel = new osg::PositionAttitudeTransform;
   
   osg::Geode* geode = new osg::Geode();
   
   osg::TessellationHints* hints = new osg::TessellationHints;
   hints->setDetailRatio(2.0);
   
   geode->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3d(0.0f,0.0f,0.0f), 1.0), hints));
   
   geode->setCullingActive(false);

   theMoonModel->addChild(geode);
   theMoonModel->setScale(osg::Vec3d(1.0, 1.0, 1.0));
   theMoonModel->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
   theMoonModel->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
  
   theSunModel->addChild(geode);
   theSunModel->setScale(osg::Vec3d(1.0, 1.0, 1.0));
   theSunModel->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
   theSunModel->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);

   theSkyDome = new SkyDome("SkyDome", false, .4);//osg::WGS_84_RADIUS_EQUATOR);
   theSkyDome->setMaxAltitude(theMaximumAltitudeToShowDome);
   theSkyDomeTransform = new osg::MatrixTransform;
   theSkyDomeTransform->addChild(theSkyDome->GetOSGNode());
   theSkyDomeTransform->setCullingActive(false);
   setMembers(membersBitMap);
   theSkyLightTable->addEntry(-90.0, 0.080);
   theSkyLightTable->addEntry(-50.0, 0.080);
   theSkyLightTable->addEntry(-40.0, 0.090);
   theSkyLightTable->addEntry(-25.0, 0.090);
   theSkyLightTable->addEntry(-20.0, 0.110);
   theSkyLightTable->addEntry(-15.0, 0.120);
   theSkyLightTable->addEntry(-10.0, 0.200);
   theSkyLightTable->addEntry(-5.0, 0.350);
   theSkyLightTable->addEntry(0.0, 0.616);
   theSkyLightTable->addEntry(5.0, 0.806);
   theSkyLightTable->addEntry(10.0, 0.895);
   theSkyLightTable->addEntry(20.0, 0.962);
   theSkyLightTable->addEntry(30.0, 0.989);
   theSkyLightTable->addEntry(40.0, 0.997);
   theSkyLightTable->addEntry(50.0, 1.0);
   theSkyLightTable->addEntry(90.0, 1.0);
   
   theSkyColor.set(0.39f, 0.50f, 0.74f);
   theFogColor.set(0.84f, 0.87f, 1.f);

   theSunColor.set(1.f, 1.f, 1.f);
   theModFogColor.set(theFogColor);
   theModSkyColor.set(theSkyColor);
   theVisibility = 20000;
   theFog = new osg::Fog();
   theFog->setDensity(0.0);
   theMoonGroup->addChild(theSunLightSourceMoonPhase.get());
   theObjectGroup->setUpdateCallback(new ossimPlanetTraverseCallback());
   theObjectGroup->setCullCallback(new ossimPlanetTraverseCallback());
   theObjectGroup->addChild(theSkyDomeTransform.get());

   theObjectGroup->addChild(theMoonGroup.get());
   theObjectGroup->addChild(theSunGroup.get());

   setFogMode(ossimPlanetEphemeris::LINEAR);   
   
}

void ossimPlanetEphemeris::EphemerisData::setMembers(ossim_uint64 membersBitMap)
{
   theMembers = membersBitMap;
   if(theMembers&ossimPlanetEphemeris::SUN_LIGHT)
   {
      theSunLightSource->setLocalStateSetModes(osg::StateAttribute::ON); 
      theSunLightSource->getLight()->setLightNum(theSunLightIdx);
      theSunLightSource->setNodeMask(0xffffffff);

      theSunLightSourceMoonPhase->setLocalStateSetModes(osg::StateAttribute::ON); 
      theSunLightSourceMoonPhase->getLight()->setLightNum(theSunLightIdx);
      theSunLightSourceMoonPhase->setNodeMask(0xffffffff);
   }
   else
   {
      theSunLightSource->setLocalStateSetModes(osg::StateAttribute::OFF); 
      theSunLightSource->setNodeMask(0x0);
      theSunLightSourceMoonPhase->setLocalStateSetModes(osg::StateAttribute::OFF); 
      theSunLightSourceMoonPhase->setNodeMask(0x0);
   }
   if(theMembers&ossimPlanetEphemeris::MOON_LIGHT)
   {
      theMoonLightSource->getLight()->setLightNum(theMoonLightIdx);
      theMoonLightSource->setLocalStateSetModes(osg::StateAttribute::ON); 
      theMoonLightSource->setNodeMask(0xffffffff);
   }
   else
   {
      theMoonLightSource->setLocalStateSetModes(osg::StateAttribute::OFF); 
      theMoonLightSource->setNodeMask(0x0);
   }
   if(theMembers&ossimPlanetEphemeris::SKY)
   {
      theSkyDome->GetOSGNode()->setNodeMask(0xffffffff);
   }
   else
   {
      theSkyDome->GetOSGNode()->setNodeMask(0x0);
   }
   

   if(theMembers&ossimPlanetEphemeris::AMBIENT_LIGHT)
   {
      theAmbientLightSource->getLight()->setLightNum(theAmbientLightIdx);
      theAmbientLightSource->setLocalStateSetModes(osg::StateAttribute::ON); 
      theAmbientLightSource->setNodeMask(0xffffffff);
   }
   else
   {
      theAmbientLightSource->setLocalStateSetModes(osg::StateAttribute::OFF); 
      theAmbientLightSource->setNodeMask(0x0);
   }

   theMoonGroup->removeChild(theMoonModel.get());
   
   if(theMembers&ossimPlanetEphemeris::MOON)
   {
      theMoonGroup->addChild(theMoonModel.get());
   }

   theSunGroup->removeChild(theSunModel.get());
   if(theMembers&ossimPlanetEphemeris::SUN)
   {
     theSunGroup->addChild(theSunModel.get());
   }

   if(theMembers&ossimPlanetEphemeris::FOG)
   {
      setFogEnableFlag(true);
   }
   else
   {
      setFogEnableFlag(false);
   }
   if(theRootStateSet.valid())
   {
      if(theMembers&ossimPlanetEphemeris::MOON_LIGHT)
      {
         theMoonLightSource->setStateSetModes(*theRootStateSet,osg::StateAttribute::ON);
      }
      else
      {
         theMoonLightSource->setStateSetModes(*theRootStateSet,osg::StateAttribute::OFF);
      }
      if(theMembers&ossimPlanetEphemeris::SUN_LIGHT)
      {
         theSunLightSource->setStateSetModes(*theRootStateSet,osg::StateAttribute::ON);
         theSunLightSourceMoonPhase->setStateSetModes(*theRootStateSet,osg::StateAttribute::ON);
      }
      else
      {
         theSunLightSource->setStateSetModes(*theRootStateSet,osg::StateAttribute::OFF);
         theSunLightSourceMoonPhase->setStateSetModes(*theRootStateSet,osg::StateAttribute::OFF);
      }

      if(theMembers&ossimPlanetEphemeris::AMBIENT_LIGHT)
      {
         theAmbientLightSource->setStateSetModes(*theRootStateSet,osg::StateAttribute::ON);
      }
      else
      {
         theAmbientLightSource->setStateSetModes(*theRootStateSet,osg::StateAttribute::OFF);
      }
   }
   
   theLayer->setRedrawFlag(true);
}

void ossimPlanetEphemeris::EphemerisData::traverse(osg::NodeVisitor& nv)
{
   if(dynamic_cast<osgUtil::IntersectVisitor*> (&nv) ||
      dynamic_cast<osgUtil::IntersectionVisitor*> (&nv))
   {
      return;
   }
  switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::CULL_VISITOR:
      {
         break;
      }
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
      
         bool updatePositionFlag = true;
         
         if(nv.getFrameStamp())
         {
            if(theFrameStamp == nv.getFrameStamp()->getFrameNumber())
            {
               updatePositionFlag = false;
            }
            theFrameStamp = nv.getFrameStamp()->getFrameNumber();
         }
         updatePositions(nv);
         break;
      }
      case osg::NodeVisitor::EVENT_VISITOR:
      {
         static bool slaveAdded = false;
         osgGA::EventVisitor* ev = dynamic_cast<osgGA::EventVisitor*>(&nv);
         if(ev)
         {
            ossimPlanetViewer* viewer = dynamic_cast<ossimPlanetViewer*>(ev->getActionAdapter());
            if(viewer&&viewer->currentCamera())
            {
               theEyeLlh[0] = viewer->currentCamera()->lat(); 
               theEyeLlh[1] = viewer->currentCamera()->lon(); 
               theEyeLlh[2] = viewer->currentCamera()->altitude();
            }
            if(theSkyDomeCamera.valid())
            {
               theSkyDomeCamera->setNearFarRatio(.0001);
            }
#if 1
            if(theEyeLlh[2] > theMaximumAltitudeToShowFog)
            {
               if(theRootStateSet.valid()&&getFogEnableFlag())
               {
                  if(theRootStateSet->getMode(GL_FOG) == osg::StateAttribute::ON)
                  {
                     theRootStateSet->setAttributeAndModes(theFog.get(), osg::StateAttribute::OFF);
                  }
               }
            }
            else
            {
               if(theRootStateSet.valid()&&getFogEnableFlag())
               {
                  if(theRootStateSet->getMode(GL_FOG) == osg::StateAttribute::OFF)
                  {
                     theRootStateSet->setAttributeAndModes(theFog.get(), osg::StateAttribute::ON);
                  }
               }
            }
#endif
         }
         break;
      }
      default:
      {
         break;
      }
   }
   if(!theSkyDomeCamera.valid())
   {
      theObjectGroup->accept(nv);
   }
   ossim_uint32 idx = 0;
   for(idx = 0; idx < theCloudLayers.size();++idx)
   {
      if(theCloudLayers[idx].valid())
      {
         theCloudLayers[idx]->accept(nv);
      }
   }
   return;
}

void ossimPlanetEphemeris::EphemerisData::updatePositions(osg::NodeVisitor& nv)
{
   if(!theLayer||!theLayer->model()) return;
   
   osg::ref_ptr<ossimPlanetGeoRefModel> model = theLayer->model();
 
   if(theAutoUpdateCurrenTimeFlag)
   {
      theDate.now();
      theLayer->setRedrawFlag(true);
   }
   ossimLocalTm adjustedDate(theDate);
   if(theApplySimulationTimeOffsetFlag&&nv.getFrameStamp())
   {
      double time = nv.getFrameStamp()->getSimulationTime();
      adjustedDate.addSeconds(time);
      theLayer->setRedrawFlag(true);
   }
 //gpstk::SystemTime now;
 //ossimLocalTm gmt = adjustedDate;//.convertToGmt();
 gpstk::UnixTime now(adjustedDate);
 now.setTimeSystem(gpstk::TimeSystem::UTC);
   //gpstk::DayTime now(gmt.getYear(),
   //                   gmt.getMonth(),
   //                   gmt.getDay(),
   //                   gmt.getHour(),
   //                   gmt.getMin(),
   //                   gmt.getSec(),
   //                   gpstk::DayTime::UTC);
   gpstk::Position sunEcef;
   sunEcef = theSunPosition.getPosition(now);

//std::cout << "x: " << theEyeLlh[0] << std::endl;
   gpstk::Position eyeLocation(theEyeLlh[0],
                               theEyeLlh[1],
                               theEyeLlh[2],
                               gpstk::Position::Geodetic,
                               &theEllipsoidModel,
                               gpstk::ReferenceFrame::WGS84);

   gpstk::Position sunLlh = gpstk::Position(sunEcef,
                                            gpstk::Position::Cartesian,
                                            &theEllipsoidModel).asGeodetic(&theEllipsoidModel);
   
   //gpstk::Position sunPosition(sunLlh.getLatitude(),
   //                            sunLlh.getLongitude(),
   //                            sunLlh.getAltitude(),
   //                            gpstk::Position::Geodetic,
   //                            &theEllipsoidModel);

   theSunAzimuth   = eyeLocation.azimuth(sunLlh);
   theSunElevation = eyeLocation.elevation(sunLlh);

   //std::cout << "Eye Position:  " << eyeLocation << std::endl;
   //std::cout << "Position:      " << sunLlh << std::endl;
   //std::cout << "Azimuth:       " << theSunAzimuth << std::endl;
   //std::cout << "Elevation:     " << theSunElevation << std::endl;

   theSunLlh = osg::Vec3d(sunLlh.getGeodeticLatitude(),
                          sunLlh.getLongitude(),
                          sunLlh.getAltitude());

   gpstk::Position moonEcef;
   moonEcef = theMoonPosition.getPosition(now);
   gpstk::Position moonLlh = moonEcef.asGeodetic(&theEllipsoidModel);
   
   theMoonLlh = osg::Vec3d(moonLlh.getGeodeticLatitude(),
                           moonLlh.getLongitude(),
                           moonLlh.getAltitude());
   
   model->latLonHeightToXyz(theMoonLlh,
                            theMoonXyz);
   
   osg::Matrixd m;
   osg::Vec3d shiftedEye(theEyeLlh[0], theEyeLlh[1], 0.0);//theEyeLlh[2]);
   theLayer->model()->lsrMatrix(shiftedEye, m, theSunAzimuth-90.0);
   theLayer->model()->latLonHeightToXyz(theSunLlh, 
                                        theSunXyz);

   theSkyDomeTransform->setMatrix(m);//*osg::Matrixd::Scale(.5,.5,.5));
   osg::Vec3d eyexyzNorm;
   theLayer->model()->latLonHeightToXyz(theEyeLlh, theEyeXyz);
   eyexyzNorm = theEyeXyz;
   eyexyzNorm.normalize();

   theAmbientLightSource->getLight()->setDirection(-eyexyzNorm);
   theAmbientLightSource->getLight()->setPosition(osg::Vec4d(theEyeXyz[0],
                                                             theEyeXyz[1],
                                                             theEyeXyz[2],
                                                              0.0));

   theSunModel->setPosition(theSunXyz);
   theMoonModel->setPosition(theMoonXyz);
#if 0
      osg::Vec3d adjMoonXyz = theMoonXyz - theEyeXyz;
      adjMoonXyz.normalize();
      osg::Vec3d newMoonPos = theEyeXyz + adjMoonXyz*2;
      osg::Vec3d newMoonLlh;
      theLayer->theModel->xyzToLatLonHeight(newMoonPos, newMoonLlh);

#endif
   
   updateSunLight();
   updateMoonLight();
   updateEnvColors();
   updateFogColor();

   ossim_float64 vis = theVisibility;
   osg::Vec3 fogColor = theModFogColor;
   osg::Vec3 skyColor = theModSkyColor;
   if (!getFogEnableFlag()) 
   { 
      vis = 200000.f; 
      fogColor = theModSkyColor;
   }
   fade_to_black(&fogColor, theEyeLlh[2], theMaximumAltitudeToShowFog, 1);
   fade_to_black(&skyColor, theEyeLlh[2], theMaximumAltitudeToShowDome, 1);
   // if fog is enabled, use the modified fog color otherwise just use
   // the modified sky color
   theSkyDome->Repaint(skyColor,
                       fogColor,
                       theSunElevation, 
                       theSunAzimuth, 
                       vis,
                       theEyeLlh[2]);
}

ossimPlanetEphemeris::ossimPlanetEphemeris(ossim_uint64 membersBitMap)
:theEphemerisData(new EphemerisData(this, membersBitMap))
{
   
}

ossimPlanetEphemeris::~ossimPlanetEphemeris()
{
   if(theEphemerisData)
   {
      delete theEphemerisData;
      theEphemerisData = 0;
   }
}

void ossimPlanetEphemeris::setMembers(ossim_uint64 membersBitMap)
{
   if(theEphemerisData)
   {
      theEphemerisData->setMembers(membersBitMap);
   }
}

ossim_uint64 ossimPlanetEphemeris::members()const
{
   return theEphemerisData->members();
}

void ossimPlanetEphemeris::setRoot(osg::Group* group)
{
   if(theEphemerisData->theRootStateSetGroup.valid())
   {
      theEphemerisData->theRootStateSetGroup->removeChild(theEphemerisData->theLightGroup.get());
   }
   theEphemerisData->theRootStateSetGroup = group;
   if(group)
   {
      theEphemerisData->theRootStateSet = group->getOrCreateStateSet();
      group->addChild(theEphemerisData->theLightGroup.get());
   }
}

void ossimPlanetEphemeris::setMoonLightCallback(LightingCallback* callback)
{
   theEphemerisData->theMoonLightCallback = callback;
}

void ossimPlanetEphemeris::setMoonCullCallback(osg::NodeCallback* callback)
{
   theEphemerisData->theMoonGroup->setCullCallback(callback);
}

void ossimPlanetEphemeris::setMoonLightIndex(ossim_uint32 idxNumber)
{
   theEphemerisData->setMoonLightIndex(idxNumber);
}

ossim_uint32 ossimPlanetEphemeris::moonLightIndex()const
{
   return theEphemerisData->moonLightIndex();
}

osg::Vec3d ossimPlanetEphemeris::moonPositionXyz()const
{
   return theEphemerisData->theMoonXyz;
}

osg::Vec3d ossimPlanetEphemeris::moonPositionLatLonHeight()const
{
   return theEphemerisData->theMoonLlh;
}

void ossimPlanetEphemeris::setSunLightIndex(ossim_uint32 idxNumber)
{
   theEphemerisData->setSunLightIndex(idxNumber);
}

ossim_uint32 ossimPlanetEphemeris::sunLightIndex()const
{
   return theEphemerisData->sunLightIndex();
}
void ossimPlanetEphemeris::setSunLightCallback(LightingCallback* callback)
{
   theEphemerisData->theSunLightCallback = callback;
}

void ossimPlanetEphemeris::setSunCullCallback(osg::NodeCallback* callback)
{
   theEphemerisData->theSunGroup->setCullCallback(callback);
}

osg::Vec3d ossimPlanetEphemeris::sunPositionXyz()const
{
   return theEphemerisData->theSunXyz;
}

osg::Vec3d ossimPlanetEphemeris::sunPositionLatLonHeight()const
{
   return theEphemerisData->theSunLlh;
}

void ossimPlanetEphemeris::setDate(const ossimLocalTm& date)
{
   theEphemerisData->setDate(date);
   theEphemerisData->setAutoUpdateToCurrentTimeFlag(false);
}

void ossimPlanetEphemeris::setAutoUpdateToCurrentTimeFlag(bool flag)
{
   theEphemerisData->setAutoUpdateToCurrentTimeFlag(flag);
}

void ossimPlanetEphemeris::setApplySimulationTimeOffsetFlag(bool flag)
{
   theEphemerisData->setApplySimulationTimeOffsetFlag(flag);
   theEphemerisData->setAutoUpdateToCurrentTimeFlag(false);
}


void ossimPlanetEphemeris::traverse(osg::NodeVisitor& nv)
{
   if(!theEnableFlag) return;
   
   theEphemerisData->traverse(nv);
   
   ossimPlanetLayer::traverse(nv);
}

osg::Vec3d ossimPlanetEphemeris::eyePositionXyz()const
{
   return theEphemerisData->theEyeXyz;
}

osg::Vec3d ossimPlanetEphemeris::eyePositionLatLonHeight()const
{
   return theEphemerisData->theEyeLlh;
}

void ossimPlanetEphemeris::setCamera(osg::Camera* camera)
{
   if(theEphemerisData->theSkyDomeCamera.get() == camera) return;
   
   if(theEphemerisData->theSkyDomeCamera.valid())
   {
      theEphemerisData->theSkyDomeCamera->removeChild(theEphemerisData->theObjectGroup.get());
   }
   theEphemerisData->theSkyDomeCamera = camera;
   if(camera)
   {
      theEphemerisData->theSkyDomeCamera->addChild(theEphemerisData->theObjectGroup.get());
   }
}

void ossimPlanetEphemeris::setVisibility(ossim_float64 visibility)
{
   theEphemerisData->setVisibility(visibility);
}


ossim_float64 ossimPlanetEphemeris::visibility()const
{
   return theEphemerisData->theVisibility;
}

osg::BoundingSphere ossimPlanetEphemeris::computeBound() const
{
   if(getNumChildren() == 0)
   {
      return osg::BoundingSphere(osg::Vec3(0.0,0.0,0.0),
                                 1.0);
   }
   return ossimPlanetLayer::computeBound();
   
}

void ossimPlanetEphemeris::setSunTextureFromFile(const ossimFilename& texture)
{
   osg::Image* image = osgDB::readImageFile( texture );
   setSunTextureFromImage(image);
}

void ossimPlanetEphemeris::setSunTextureFromImage(osg::Image* texture)
{
  if ( texture )
  {
     osg::Texture2D* tex = new osg::Texture2D();
     tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_NEAREST);
     tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture2D::LINEAR_MIPMAP_NEAREST);
     tex->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
     tex->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
     tex->setWrap(osg::Texture2D::WRAP_R, osg::Texture2D::CLAMP_TO_EDGE);

     tex->setImage(texture);
     theEphemerisData->theSunModel->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON);
  }
  else
  {
     theEphemerisData->theSunModel->getOrCreateStateSet()->setTextureAttributeAndModes(0, 0, osg::StateAttribute::OFF);
  }
}

void ossimPlanetEphemeris::setMoonTextureFromFile(const ossimFilename& texture)
{
   osg::Image* image = osgDB::readImageFile( texture );
   setMoonTextureFromImage(image);
}

void ossimPlanetEphemeris::setMoonScale(const osg::Vec3d& scale)
{
   theEphemerisData->theMoonModel->setScale(scale);
}

void ossimPlanetEphemeris::setSunScale(const osg::Vec3d& scale)
{
   theEphemerisData->theSunModel->setScale(scale);
}

void ossimPlanetEphemeris::setMoonTextureFromImage(osg::Image* texture)
{
   if ( texture )
   {
      osg::Texture2D* tex = new osg::Texture2D();
      tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_NEAREST);
      tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture2D::LINEAR_MIPMAP_NEAREST);
      tex->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
      tex->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
      tex->setWrap(osg::Texture2D::WRAP_R, osg::Texture2D::CLAMP_TO_EDGE);

      tex->setImage(texture);
      theEphemerisData->theMoonModel->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON);
   }
   else
   {
      theEphemerisData->theMoonModel->getOrCreateStateSet()->setTextureAttributeAndModes(0, 0, osg::StateAttribute::OFF);
   }
}


void ossimPlanetEphemeris::setSunMinMaxPixelSize(ossim_uint32 minPixelSize,
                                                 ossim_uint32 maxPixelSize)
{
   
   theEphemerisData->theSunBillboard->setMinPixelSize(minPixelSize);
   theEphemerisData->theSunBillboard->setMaxPixelSize(maxPixelSize);
}

void ossimPlanetEphemeris::setMaximumAltitudeToShowDomeInMeters(ossim_float64 maxAltitude)
{
   theEphemerisData->theMaximumAltitudeToShowDome = maxAltitude;
}

void ossimPlanetEphemeris::setMaximumAltitudeToShowFogInMeters(ossim_float64 maxAltitude)
{
   theEphemerisData->theMaximumAltitudeToShowFog = maxAltitude;
}
void ossimPlanetEphemeris::setMaxAltitudeToDoSunriseSunsetColorAdjustment(ossim_float64 maxAltitude,
                                                                          bool useFading)
{
   theEphemerisData->theMaxAltitudeToDoSunriseSunsetColorAdjustment = maxAltitude;
   theEphemerisData->theUseFadingFlagForSunriseSunsetCalculation = useFading;
}

ossim_float64  ossimPlanetEphemeris::maxAltitudeToDoSunriseSunsetColorAdjustment()const
{
   return theEphemerisData->theMaxAltitudeToDoSunriseSunsetColorAdjustment;
}


void ossimPlanetEphemeris::setGlobalAmbientLight(const osg::Vec3d& ambient)
{
   theEphemerisData->theGlobalAmbientColor = ambient;
   theEphemerisData->theAmbientLightSource->getLight()->setAmbient(osg::Vec4d(theEphemerisData->theGlobalAmbientColor[0],
                                                            theEphemerisData->theGlobalAmbientColor[1],
                                                            theEphemerisData->theGlobalAmbientColor[2], 1.0));
}

void ossimPlanetEphemeris::setGlobalAmbientLightIndex(ossim_uint32 idx)
{
   theEphemerisData->theAmbientLightIdx = idx;
   theEphemerisData->theAmbientLightSource->getLight()->setLightNum(theEphemerisData->theAmbientLightIdx);
}

ossim_uint32 ossimPlanetEphemeris::globalAmbientLightIndex()const
{
   return theEphemerisData->theAmbientLightIdx;
}

void ossimPlanetEphemeris::setSkyColorAdjustmentTable(IntensityTableType& table)
{
   theEphemerisData->theSkyLightTable->setTable(table);
}

const ossimPlanetEphemeris::IntensityTableType* ossimPlanetEphemeris::skyColorAdjustmentTable()const
{
   return &theEphemerisData->theSkyLightTable->table();
}

void ossimPlanetEphemeris::setBaseSkyColor(const osg::Vec3d& color)
{
   theEphemerisData->theSkyColor = color;
}
osg::Vec3d ossimPlanetEphemeris::getBaseSkyColor()const
{
   return theEphemerisData->theSkyColor;
}

void ossimPlanetEphemeris::setBaseFogColor(const osg::Vec3d& color)
{
   theEphemerisData->theFogColor = color;
}

osg::Vec3d ossimPlanetEphemeris::getBaseFogColor()const
{
   return theEphemerisData->theFogColor;
}

void ossimPlanetEphemeris::setFogMode(FogMode mode)
{
   theEphemerisData->setFogMode(mode);
}

void ossimPlanetEphemeris::setFogNear(ossim_float64 value)
{
   theEphemerisData->setFogNear(value);///osg::WGS_84_RADIUS_EQUATOR);
}
void ossimPlanetEphemeris::setFogFar(ossim_float64 value)
{
   theEphemerisData->setFogFar(value);///osg::WGS_84_RADIUS_EQUATOR);
}
void ossimPlanetEphemeris::setFogDensity(ossim_float64 value)
{
   theEphemerisData->setFogDensity(value);
}

void ossimPlanetEphemeris::setFogEnableFlag(bool flag)
{
   theEphemerisData->setFogEnableFlag(flag);
}
void ossimPlanetEphemeris::setNumberOfCloudLayers(ossim_uint32 numberOfLayers)
{
   theEphemerisData->setNumberOfCloudLayers(numberOfLayers);
}

void ossimPlanetEphemeris::removeClouds(ossim_uint32 idx, ossim_uint32 count)
{
   theEphemerisData->removeClouds(idx, count);
}

ossimPlanetCloudLayer* ossimPlanetEphemeris::cloudLayer(ossim_uint32 idx)
{
   return theEphemerisData->cloudLayer(idx);
}

ossim_uint32 ossimPlanetEphemeris::numberOfCloudLayers()const
{
   return theEphemerisData->theCloudLayers.size();
}

void ossimPlanetEphemeris::createCloudPatch(ossim_uint32 cloudLayerIndex,
                                            const osg::Vec3d& theCenterLatLonHeight,
                                            ossim_float64 numberOfMeshSamples,
                                            ossim_float64 patchSizeInDegrees,
                                            ossim_uint64  seed,
                                            ossim_float64 coverage,
                                            ossim_float64 sharpness)
{
   if(cloudLayerIndex == numberOfCloudLayers())
   {
      setNumberOfCloudLayers(cloudLayerIndex+1);
   }
   osg::ref_ptr<ossimPlanetCloudLayer> layer = cloudLayer(cloudLayerIndex);
   if(layer.valid())
   {
      layer->setGrid(new ossimPlanetCloudLayer::Patch(patchSizeInDegrees, patchSizeInDegrees));
      layer->computeMesh(0.0, numberOfMeshSamples, numberOfMeshSamples, 0);
      layer->updateTexture(seed, coverage, sharpness);
      layer->moveToLocationLatLonAltitude(theCenterLatLonHeight);
   }
}

void ossimPlanetEphemeris::createGlobalCloud(ossim_uint32 cloudLayerIndex,
                                             ossim_float64 altitude,
                                             ossim_float64 numberOfMeshSamples,
                                             ossim_uint64  seed,
                                             ossim_float64 coverage,
                                             ossim_float64 sharpness)
{
   if(cloudLayerIndex == numberOfCloudLayers())
   {
      setNumberOfCloudLayers(cloudLayerIndex+1);
   }
   osg::ref_ptr<ossimPlanetCloudLayer> layer = cloudLayer(cloudLayerIndex);
   if(layer.valid())
   {
      layer->computeMesh(altitude, numberOfMeshSamples, numberOfMeshSamples, 0);
      layer->updateTexture(seed, coverage, sharpness);
   }
}

#else
#include <iostream>
#include <ossimPlanet/ossimPlanetEphemeris.h>
static void noossimPlanetEphemerisSupportMessage()
{
   static bool messageDisplayed = false;
   if(!messageDisplayed)
   {
      messageDisplayed = true;
      std::cout << "ossimPlanetEphemeris support disabled, please recompile with OSSIMPLANET_ENABLE_EPHEMERIS defined\n";
   }
}
ossimPlanetEphemeris::ossimPlanetEphemeris(ossim_uint64 )
:theEphemerisData(0)
{
   noossimPlanetEphemerisSupportMessage();
}

ossimPlanetEphemeris::~ossimPlanetEphemeris()
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::traverse(osg::NodeVisitor& nv)
{
   noossimPlanetEphemerisSupportMessage();
   ossimPlanetLayer::traverse(nv);
}

osg::Vec3d ossimPlanetEphemeris::eyePositionXyz()const
{
   noossimPlanetEphemerisSupportMessage();
   return osg::Vec3d(0.0,0.0,0.0);
}
osg::Vec3d ossimPlanetEphemeris::eyePositionLatLonHeight()const
{
   noossimPlanetEphemerisSupportMessage();
   return osg::Vec3d(0.0,0.0,0.0);
}
void ossimPlanetEphemeris::setMembers(ossim_uint64 )
{
   noossimPlanetEphemerisSupportMessage();
}

ossim_uint64 ossimPlanetEphemeris::members()const
{
   noossimPlanetEphemerisSupportMessage();
   return 0;
}

void ossimPlanetEphemeris::setRoot(osg::Group* )
{
   noossimPlanetEphemerisSupportMessage();
}


void ossimPlanetEphemeris::setMoonLightIndex(ossim_uint32 )
{
   noossimPlanetEphemerisSupportMessage();
}

ossim_uint32 ossimPlanetEphemeris::moonLightIndex()const
{
   noossimPlanetEphemerisSupportMessage();

   return 0;
}

void ossimPlanetEphemeris::setMoonLightCallback(LightingCallback* )
{
   noossimPlanetEphemerisSupportMessage();
}

osg::Vec3d ossimPlanetEphemeris::moonPositionXyz()const
{
   noossimPlanetEphemerisSupportMessage();

   return osg::Vec3d(0.0,0.0,0.0);
}
osg::Vec3d ossimPlanetEphemeris::moonPositionLatLonHeight()const
{
   noossimPlanetEphemerisSupportMessage();

   return osg::Vec3d(0.0,0.0,0.0);
}

void ossimPlanetEphemeris::setSunLightIndex(ossim_uint32 )
{
   noossimPlanetEphemerisSupportMessage();
}

ossim_uint32 ossimPlanetEphemeris::sunLightIndex()const
{
   noossimPlanetEphemerisSupportMessage();
   return 0;
}

void ossimPlanetEphemeris::setSunLightCallback(LightingCallback* )
{
   noossimPlanetEphemerisSupportMessage();
}

osg::Vec3d ossimPlanetEphemeris::sunPositionXyz()const
{
   noossimPlanetEphemerisSupportMessage();
   return osg::Vec3d(0.0,0.0,0.0);
}
osg::Vec3d ossimPlanetEphemeris::sunPositionLatLonHeight()const
{
   noossimPlanetEphemerisSupportMessage();
   return osg::Vec3d(0.0,0.0,0.0);
}
void ossimPlanetEphemeris::setAutoUpdateSunColorFlag(bool )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setGlobalAmbientLightIndex(ossim_uint32 )
{
   noossimPlanetEphemerisSupportMessage();
}

ossim_uint32 ossimPlanetEphemeris::globalAmbientLightIndex()const
{
   noossimPlanetEphemerisSupportMessage();
   return 0;
}

void ossimPlanetEphemeris::setGlobalAmbientLight(const osg::Vec3d& )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setDate(const ossimLocalTm& )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setAutoUpdateToCurrentTimeFlag(bool )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setApplySimulationTimeOffsetFlag(bool )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setCamera(osg::Camera* )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setVisibility(ossim_float64 )
{
   noossimPlanetEphemerisSupportMessage();
}

ossim_float64 ossimPlanetEphemeris::visibility()const
{
   noossimPlanetEphemerisSupportMessage();
   return 0.0;
}

void ossimPlanetEphemeris::setSunTextureFromFile(const ossimFilename& )
{
   noossimPlanetEphemerisSupportMessage();
}
void ossimPlanetEphemeris::setSunTextureFromImage(osg::Image* )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setMoonTextureFromFile(const ossimFilename& )
{
   noossimPlanetEphemerisSupportMessage();
}
void ossimPlanetEphemeris::setMoonTextureFromImage(osg::Image* )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setMoonScale(const osg::Vec3d& )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setSunScale(const osg::Vec3d& )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setSunMinMaxPixelSize(ossim_uint32 ,
                           ossim_uint32 )
{
   noossimPlanetEphemerisSupportMessage();
}


void ossimPlanetEphemeris::setMaximumAltitudeToShowDomeInMeters(ossim_float64 )
{
   noossimPlanetEphemerisSupportMessage();
}


void ossimPlanetEphemeris::setMaximumAltitudeToShowFogInMeters(ossim_float64 /*maxAltitude*/)
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setMaxAltitudeToDoSunriseSunsetColorAdjustment(ossim_float64 /*maxAltitude*/,
                                                                          bool /*useFading*/)
{
   noossimPlanetEphemerisSupportMessage();
}

ossim_float64  ossimPlanetEphemeris::maxAltitudeToDoSunriseSunsetColorAdjustment()const
{
   noossimPlanetEphemerisSupportMessage();
   return 0.0;
}

void ossimPlanetEphemeris::setSkyColorAdjustmentTable(IntensityTableType& )
{
   noossimPlanetEphemerisSupportMessage();
}

const ossimPlanetEphemeris::IntensityTableType* ossimPlanetEphemeris::skyColorAdjustmentTable()const
{
   noossimPlanetEphemerisSupportMessage();
   return 0;
}

void ossimPlanetEphemeris::setBaseSkyColor(const osg::Vec3d& )
{
   noossimPlanetEphemerisSupportMessage();
}

osg::Vec3d ossimPlanetEphemeris::getBaseSkyColor()const
{
   noossimPlanetEphemerisSupportMessage();
   return osg::Vec3d(0.0,0.0,0.0);
}

void ossimPlanetEphemeris::setBaseFogColor(const osg::Vec3d& )
{
   noossimPlanetEphemerisSupportMessage();
}
osg::Vec3d ossimPlanetEphemeris::getBaseFogColor()const
{
   noossimPlanetEphemerisSupportMessage();
   return osg::Vec3d(0.0,0.0,0.0);
}

void ossimPlanetEphemeris::setFogMode(FogMode )
{
   noossimPlanetEphemerisSupportMessage();
}
void ossimPlanetEphemeris::setFogNear(ossim_float64 )
{
   noossimPlanetEphemerisSupportMessage();

}
void ossimPlanetEphemeris::setFogFar(ossim_float64 )
{
   noossimPlanetEphemerisSupportMessage();
}
void ossimPlanetEphemeris::setFogDensity(ossim_float64 )
{
   noossimPlanetEphemerisSupportMessage();
}
void ossimPlanetEphemeris::setFogEnableFlag(bool )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::setNumberOfCloudLayers(ossim_uint32 )
{
   noossimPlanetEphemerisSupportMessage();
}
void ossimPlanetEphemeris::removeClouds(ossim_uint32 , ossim_uint32 )
{
   noossimPlanetEphemerisSupportMessage();
}
ossimPlanetCloudLayer* ossimPlanetEphemeris::cloudLayer(ossim_uint32 )
{
   noossimPlanetEphemerisSupportMessage();
   return 0;
}
ossim_uint32 ossimPlanetEphemeris::numberOfCloudLayers()const
{
   noossimPlanetEphemerisSupportMessage();
   return 0;
}

void ossimPlanetEphemeris::createCloudPatch(ossim_uint32 ,
                                            const osg::Vec3d& ,
                                            ossim_float64 ,
                                            ossim_float64 ,
                                            ossim_uint64  ,
                                            ossim_float64 ,
                                            ossim_float64 )
{
   noossimPlanetEphemerisSupportMessage();
}

void ossimPlanetEphemeris::createGlobalCloud(ossim_uint32 cloudLayerIndex,
                                             ossim_float64 altitude,
                                             ossim_float64 numberOfMeshSamples,
                                             ossim_uint64  seed,
                                             ossim_float64 coverage,
                                             ossim_float64 sharpness)
{
   noossimPlanetEphemerisSupportMessage();
}

osg::BoundingSphere ossimPlanetEphemeris::computeBound() const
{
   noossimPlanetEphemerisSupportMessage();
   if(getNumChildren() == 0)
   {
      return osg::BoundingSphere(osg::Vec3(0.0,0.0,0.0),
                                 1.0);
   }
   return ossimPlanetLayer::computeBound();
}

#endif

