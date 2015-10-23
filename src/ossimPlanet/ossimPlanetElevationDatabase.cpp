#include <ossimPlanet/ossimPlanetElevationDatabase.h>

osg::ref_ptr<ossimPlanetImage> ossimPlanetElevationDatabase::getTexture(const ossimPlanetTerrainTileId& tileId,
                                                                                osg::ref_ptr<ossimPlanetGrid> theGrid)
{
   return 0;
}

void ossimPlanetElevationDatabase::mergeDataObjects(ossimRefPtr<ossimImageData> destData,
                                                    ossimRefPtr<ossimImageData> srcData)
{
   if(destData.valid()&&srcData.valid()&&
      destData->getBuf()&&srcData->getBuf())
   {
      if((destData->getScalarType() == OSSIM_FLOAT)&&
         (destData->getWidth()==srcData->getWidth())&&
         (destData->getHeight()==srcData->getHeight()))
      {
         ossim_float32* destBuf = (ossim_float32*)destData->getBuf();
         ossim_float32  destNp  = (ossim_float32)destData->getNullPix(0);
         ossim_uint32 idx = 0;
         ossim_uint32 area = destData->getWidth()*destData->getHeight();

         switch(srcData->getScalarType())
         {
            case OSSIM_UINT16:
            case OSSIM_USHORT11:
            {
               ossim_uint16* srcBuf = (ossim_uint16*)srcData->getBuf();
               ossim_uint16 srcNp = (ossim_uint16)srcData->getNullPix(0);
               
               for(;idx < area; ++idx)
               {
                  if(*destBuf == destNp)
                  {
                     if(*srcBuf != srcNp)
                     {
                        *destBuf = (ossim_float32)*srcBuf;
                     }
                  }
                  ++destBuf;
                  ++srcBuf;
               }
               break;
            }
            case OSSIM_SINT16:
            {
               ossim_sint16* srcBuf = (ossim_sint16*)srcData->getBuf();
               ossim_sint16 srcNp = (ossim_sint16)srcData->getNullPix(0);
               
               for(;idx < area; ++idx)
               {
                  if(*destBuf == destNp)
                  {
                     if(*srcBuf != srcNp)
                     {
                        *destBuf = (ossim_float32)*srcBuf;
                     }
                  }
                  ++destBuf;
                  ++srcBuf;
               }
               break;
            }
            case OSSIM_FLOAT32:
            {
               ossim_float32* srcBuf = (ossim_float32*)srcData->getBuf();
               ossim_float32 srcNp = (ossim_float32)srcData->getNullPix(0);
               
               for(;idx < area; ++idx)
               {
                  if(*destBuf == destNp)
                  {
                     if(*srcBuf != srcNp)
                     {
                        *destBuf = *srcBuf;
                     }
                  }
                  ++destBuf;
                  ++srcBuf;
               }
               break;
            }
            default:
            {
               break;
               // not supported
            }
         }
      }
   }
}
