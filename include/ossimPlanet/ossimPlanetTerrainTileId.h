#ifndef ossimPlanetTerrainTileId_HEADER
#define ossimPlanetTerrainTileId_HEADER
#include <ossim/base/ossimConstants.h>
#include <iostream>

class OSSIMPLANET_DLL ossimPlanetTerrainTileId
{
public:
   friend std::ostream& operator <<(std::ostream& out, const ossimPlanetTerrainTileId& tileid)
   {
      return out << "<" << tileid.theFace <<", " << tileid.theLevel << ", " << tileid.theX << ", " << tileid.theY << ">";
   }
   ossimPlanetTerrainTileId()
   :theFace(0),
   theLevel(0),
   theX(0),
   theY(0)
   {
      setId(0,0,0,0);
   }
   ossimPlanetTerrainTileId(ossim_uint32 face, ossim_uint32 level, ossim_uint64 x, ossim_uint64 y)
   {
      setId(face, level, x, y);
   }
   bool operator == (const ossimPlanetTerrainTileId& rhs) const        
   {
      return (theLevel==rhs.theLevel) && (theX==rhs.theX) && (theY==rhs.theY) && (theFace == rhs.theFace);
   }
   
   bool operator != (const ossimPlanetTerrainTileId& rhs) const        
   {
      return (theLevel!=rhs.theLevel) || (theX!=rhs.theX) || (theY!=rhs.theY) || (theFace != rhs.theFace);
   }
   
   bool operator < (const ossimPlanetTerrainTileId& rhs) const
   {
      if (theLevel<rhs.theLevel) return true;
      if (theLevel>rhs.theLevel) return false;
      if (theX<rhs.theX) return true;
      if (theX>rhs.theX) return false;
      if (theY<rhs.theY) return true;
      if (theY>rhs.theY) return false;
      return theFace<rhs.theFace;
   }
   
   void setId(ossim_uint32 face, ossim_uint32 level, ossim_uint64 x, ossim_uint64 y)
   {
      theFace  = face;
      theLevel = level;
      theX     = x;
      theY     = y;
   }
   void splitQuad(ossimPlanetTerrainTileId& quad00, 
                  ossimPlanetTerrainTileId& quad10,
                  ossimPlanetTerrainTileId& quad11,
                  ossimPlanetTerrainTileId& quad01)
   {
      ossim_uint32 xOrigin=theX<<1;
      ossim_uint32 yOrigin=theY<<1;

      quad00.setId(theFace, theLevel+1, xOrigin, yOrigin);
      quad10.setId(theFace, theLevel+1, xOrigin+1, yOrigin);
      quad11.setId(theFace, theLevel+1, xOrigin+1, yOrigin+1);
      quad01.setId(theFace, theLevel+1, xOrigin, yOrigin+1);
   }
   const ossim_uint32& level()const{return theLevel;}
   const ossim_uint64& x()const{return theX;}
   const ossim_uint64& y()const{return theY;}
   const ossim_uint32& face()const{return theFace;}
   ossim_uint32 theFace;
   ossim_uint32 theLevel;
   ossim_uint64 theX;
   ossim_uint64 theY;
};

#endif
