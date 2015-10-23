#include <ossimPlanet/ossimPlanetShaderProgramSetup.h>

osg::ref_ptr<osg::Uniform> ossimPlanetShaderProgramSetup::getUniform(const ossimString& name)
{
   ossim_uint32 idx = 0;

   for(idx = 0; idx < theUniformList.size(); ++idx)
   {
      if(theUniformList[idx]->getName() == name.string())
      {
         return theUniformList[idx].get();
      }
   }

   return 0;
}

