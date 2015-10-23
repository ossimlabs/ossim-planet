#ifndef ossimPlanetShaderProgramSetup_HEADER
#define ossimPlanetShaderProgramSetup_HEADER
#include <osg/Referenced>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Uniform>
#include <vector>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimString.h>
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetShaderProgramSetup : public osg::Referenced
{
public:
   enum ossimPlanetFragmentShaderType
   {
      NO_SHADER = 0,
      TOP,
      REFERENCE,
      OPACITY,
      HORIZONTAL_SWIPE,
      VERTICAL_SWIPE,
      BOX_SWIPE,
      CIRCLE_SWIPE,
      ABSOLUTE_DIFFERENCE,
      FALSE_COLOR_REPLACEMENT
   };
   ossimPlanetShaderProgramSetup(ossim_uint32 minNumberOfTextureArguments=2)
      :theMinNumberOfTextureArguments(minNumberOfTextureArguments),
	  theFragmentShaderType(NO_SHADER)
   {
   }
   void setProgram(osg::Program* program)
   {
      theProgram = program;
   }
   osg::Program* getProgram()
   {
      return theProgram.get();
   }
   const osg::Program* getProgram()const
   {
      return theProgram.get();
   }
   ossim_uint32 getNumberOfUniforms()const
   {
      return theUniformList.size();
   }
   osg::ref_ptr<osg::Uniform> getUniform(const ossimString& name);
   osg::ref_ptr<osg::Uniform> getUniform(ossim_uint32 idx)
   {
      if(idx < theUniformList.size())
      {
         return theUniformList[idx].get();
      }
      
      return 0;
   }
   void clearUniformList()
   {
      theUniformList.clear();
   }

   void addUniform(osg::Uniform* uniform)
   {
      theUniformList.push_back(uniform);
   }
   ossim_uint32 minNumberOfTextureArguments()const
   {
      return theMinNumberOfTextureArguments;
   }
   void setFragmentType(ossimPlanetFragmentShaderType fragType)
   {
      theFragmentShaderType = fragType;
   }
   ossimPlanetFragmentShaderType fragmentType()const
   {
      return theFragmentShaderType;
   }
protected:
   ossim_uint32                             theMinNumberOfTextureArguments;
   osg::ref_ptr<osg::Program>               theProgram;
   std::vector<osg::ref_ptr<osg::Uniform> > theUniformList;
   ossimPlanetFragmentShaderType            theFragmentShaderType;
};
#endif
