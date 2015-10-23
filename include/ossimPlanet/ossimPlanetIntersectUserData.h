#ifndef ossimPlanetIntersectUserData_HEADER
#define ossimPlanetIntersectUserData_HEADER
#include <osg/Referenced>
#include <ossimPlanet/ossimPlanetExport.h>

class OSSIMPLANET_DLL ossimPlanetIntersectUserData : public osg::Referenced
{
public:
   ossimPlanetIntersectUserData(int maxLevel=9999999)
      :theMaxLevelToIntersect(maxLevel)
   {
   }

      void setMaxLevelToIntersect(int maxLevel)
      {
         theMaxLevelToIntersect = maxLevel;
      }
   int getMaxLevelToIntersect()const
   {
      return theMaxLevelToIntersect;
   }
protected:
   virtual ~ossimPlanetIntersectUserData(){}

   int theMaxLevelToIntersect;
};
#endif
