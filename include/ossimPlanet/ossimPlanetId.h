#ifndef ossimPlanetId_HEADER
#define ossimPlanetId_HEADER
#include <ossim/base/ossimConstants.h>
#include "ossimPlanetExport.h"

class OSSIMPLANET_DLL ossimPlanetId
{
public:
   ossimPlanetId(ossim_int64 value = theInvalidId)
      :theId(value)
   {
   }
   ossim_int64 id()const
   {
      return theId;
   }
   ossimPlanetId& operator ++()
   {
      ++theId;
      return *this;
   }
   ossimPlanetId operator ++(int)
   {
      ossimPlanetId id(theId);
      ++theId;
      return id;
   }
   bool operator()()const
   {
      return (theId != theInvalidId);
   }
   bool operator <(const ossimPlanetId& id)const
   {
      return (theId < id.theId);
   }
   bool operator <(ossim_int64 id)const
   {
      return (theId < id);
   }
   bool operator >(const ossimPlanetId& id)const
   {
      return (theId > id.theId);
   }
   bool operator >(ossim_int64 id)const
   {
      return (theId > id);
   }
   bool operator <=(const ossimPlanetId& id)const
   {
      return (theId <= id.theId);
   }
   bool operator <=(ossim_int64 id)const
   {
      return (theId <= id);
   }
   bool operator >=(const ossimPlanetId& id)const
   {
      return (theId >= id.theId);
   }
   bool operator >=(ossim_int64 id)const
   {
      return (theId >= id);
   }
   bool operator ==(const ossimPlanetId& id)const
   {
      return (theId == id.theId);
   }
   bool operator ==(ossim_int64 id)const
   {
      return (theId == id);
   }
   static ossim_int64 invalidId();
protected:
   ossim_int64 theId;

   static ossim_int64 theInvalidId;
};

#endif
