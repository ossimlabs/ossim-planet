#include <ossimPlanet/ossimPlanetId.h>

ossim_int64 ossimPlanetId::theInvalidId = -1;

ossim_int64 ossimPlanetId::invalidId()
{
   return theInvalidId;
}
