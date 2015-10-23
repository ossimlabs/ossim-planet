#ifndef ossimPlanetConstants_HEADER
#define ossimPlanetConstants_HEADER

const double OSSIMPLANET_WGS_84_RADIUS_EQUATOR = 6378137.0;
const double OSSIMPLANET_WGS_84_RADIUS_POLAR = 6356752.3142;

enum ossimPlanetTimeUnit
{
   ossimPlanetTimeUnit_SECONDS = 0,
   ossimPlanetTimeUnit_MINUTES,
   ossimPlanetTimeUnit_HOURS,
};

enum ossimPlanetLandRefreshType
{
   ossimPlanetLandRefreshType_NONE    = 0,
   ossimPlanetLandRefreshType_TEXTURE = 1,
   ossimPlanetLandRefreshType_GEOM    = 2,
   ossimPlanetLandRefreshType_PRUNE   = 4,
};

enum ossimPlanetLandType
{
   ossimPlanetLandType_NONE      = 0,
   ossimPlanetLandType_FLAT      = 1,
   ossimPlanetLandType_NORMALIZED_ELLIPSOID = 2,
   ossimPlanetLandType_ELLIPSOID = 3,
   ossimPlanetLandType_ORTHOFLAT = 4
};

enum ossimPlanetPriorityType
{
   ossimPlanetLandPriorityType_NONE                      = 0,
   ossimPlanetLandPriorityType_LINE_OF_SITE_INTERSECTION = 1,
   ossimPlanetLandPriorityType_EYE_POSITION              = 2 
};

enum ossimPlanetAltitudeMode
{
   ossimPlanetAltitudeMode_NONE               = 0,
   ossimPlanetAltitudeMode_CLAMP_TO_GROUND    = 1,
   ossimPlanetAltitudeMode_RELATIVE_TO_GROUND = 2,
   ossimPlanetAltitudeMode_ABSOLUTE           = 3
};

enum ossimPlanetKmlUnits
{
   ossimPlanetKmlUnits_NONE           = 0,
   ossimPlanetKmlUnits_FRACTION       = 1,
   ossimPlanetKmlUnits_PIXELS         = 2,
   ossimPlanetKmlUnits_INSET_PIXELS   = 3
};

enum ossimPlanetKmlStyleState
{
   ossimPlanetKmlStyleState_NONE        = 0,
   ossimPlanetKmlStyleState_NORMAL      = 1,
   ossimPlanetKmlStyleState_HIGHLIGHT   = 2,
};

enum ossimPlanetKmlRefreshMode
{
   ossimPlanetKmlRefreshMode_NONE        = 0,
   ossimPlanetKmlRefreshMode_ON_CHANGE   = 1,
   ossimPlanetKmlRefreshMode_ON_INTERVAL = 2,
   ossimPlanetKmlRefreshMode_ON_EXPIRE   = 3
};

enum ossimPlanetKmlViewRefreshMode
{
   ossimPlanetKmlViewRefreshMode_NONE       = 0,
   ossimPlanetKmlViewRefreshMode_NEVER      = 1,
   ossimPlanetKmlViewRefreshMode_ON_REQUEST = 2,
   ossimPlanetKmlViewRefreshMode_ON_STOP    = 3,
   ossimPlanetKmlViewRefreshMode_ON_REGION  = 4
};

enum ossimPlanetKmlColorMode
{
   ossimPlanetKmlColorMode_NONE        = 0,
   ossimPlanetKmlColorMode_NORMAL      = 1,
   ossimPlanetKmlColorMode_RANDOM      = 2
};

enum ossimPlanetTextureLayerStateCode // bit vector
{
   ossimPlanetTextureLayer_VALID          = 0,
   ossimPlanetTextureLayer_NO_SOURCE_DATA = 1,
   ossimPlanetTextureLayer_NO_GEOM        = 2,
   ossimPlanetTextureLayer_NO_OVERVIEWS   = 4,
   ossimPlanetTextureLayer_NO_HISTOGRAMS  = 8,
   ossimPlanetTextureLayer_NOT_OPENED     = 16,
   ossimPlanetTextureLayer_ALL            = (ossimPlanetTextureLayer_NO_SOURCE_DATA|
                                             ossimPlanetTextureLayer_NO_GEOM|
                                             ossimPlanetTextureLayer_NO_OVERVIEWS|
															ossimPlanetTextureLayer_NO_HISTOGRAMS|
                                             ossimPlanetTextureLayer_NOT_OPENED)
};

enum ossimPlanetIoDirection
{
   ossimPlanetIoDirection_NONE = 0,
   ossimPlanetIoDirection_IN   = 1,
   ossimPlanetIoDirection_OUT  = 2,
   ossimPlanetIoDirection_INOUT= 3
};

#include <cmath>

#define OSSIMPLANET_NULL_HEIGHT ((ossim_float32)1.0/FLT_EPSILON)
#endif
