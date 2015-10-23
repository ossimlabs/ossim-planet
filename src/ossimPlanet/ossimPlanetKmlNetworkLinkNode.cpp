#include <ossimPlanet/ossimPlanetKmlNetworkLinkNode.h>
#include <ossimPlanet/ossimPlanetKmlLayer.h>
#include <wms/wmsCurlMemoryStream.h>
#include <ossim/base/ossimTempFilename.h>

ossimPlanetKmlNetworkLinkNode::ossimPlanetKmlNetworkLinkNode(ossimPlanetKmlLayer* layer,
                                                             ossimPlanetKmlObject* obj)
:ossimPlanetKmlLayerNode(layer, obj)
{
   theScheduledFlag = false;
}

void ossimPlanetKmlNetworkLinkNode::traverse(osg::NodeVisitor& nv)
{
   ossimPlanetKmlLayerNode::traverse(nv);
   switch(nv.getVisitorType())
   {
      case osg::NodeVisitor::UPDATE_VISITOR:
      {
         if(!getNumChildren()&&!theScheduledFlag)
         {
            if(theKmlData.valid())
            {
               if(theLayer)
               {
                  theScheduledFlag = true;
                  theLayer->addKml(this, theKmlData.get());
               }
            }         
         }
         break;
      }
      default:
      {
         break;
      }
   }
}

bool ossimPlanetKmlNetworkLinkNode::init()
{
   ossimPlanetKmlNetworkLink* link = dynamic_cast<ossimPlanetKmlNetworkLink*>(theKmlObject.get());
   if(!link) return false;
   wmsRefPtr<wmsCurlMemoryStream> curlMemoryStream = new wmsCurlMemoryStream;
   curlMemoryStream->setMaxRedirects(1);
   curlMemoryStream->setUrl(link->link()->href().string());
   bool result = false;
   ossimTempFilename tempFile("","","kmz");
   theKmlData = 0;
//   std::cout << "CACHE DIRECTORY ================ " << theKmlObject->getCacheLocation() << std::endl;
//   std::cout << "TRYING LINK ========================== " << link->link()->href() << std::endl;
   if(ossimFilename(link->link()->href()).ext() == "kmz")
   {
 //     std::cout << "DOWNLOAD KMZ!!!!!!!!!!!!!!!!!!!" << std::endl;
      tempFile.generateRandomFile();
      result = curlMemoryStream->download(tempFile);
//      std::cout << "result ============= " << result << "  with file exists " << tempFile << std::endl;
   }
   else
   {
      result = curlMemoryStream->download();
//      std::cout << "result ============= " << result << std::endl;
   }
   if(curlMemoryStream->getStream().valid()&&!tempFile.exists())
   {
      ossimRefPtr<wmsMemoryStream> stream =  new wmsMemoryStream(curlMemoryStream->getStream()->getBuffer(),
                                                                  curlMemoryStream->getStream()->getBufferSize());
      if(stream->getBuffer())
      {
         stringstream in(ossimString(curlMemoryStream->getStream()->getBuffer(),
                                     curlMemoryStream->getStream()->getBuffer()+
                                     curlMemoryStream->getStream()->getBufferSize()));
//         std::cout << "PEEEEEK ==== " << (char)in.peek() << std::endl;
//         std::cout << "---------------------------------------------------------------" << std::endl;
      //   std::cout << stream->getBuffer() << std::endl;
         theKmlData = new ossimPlanetKml;
         theKmlData->setParent(theKmlObject.get());
         if(!theKmlData->parse(in, false))
         {
            in.clear();
            in.seekg(0);//, std::ios::beg);
            if(theKmlData->parse(in, true))
            {
            }
         }
         else
         {
         }
//         std::cout << "==============================================================" << std::endl;
      }
   }
   else if(result&&tempFile.exists())
   {
      theKmlData = new ossimPlanetKmz;
      theKmlData->setParent(theKmlObject.get());
      if(!theKmlData->parse(tempFile))
      {
//         std::cout << "NO KMZ!!!!!!!!!!!!!!!!!!!" << std::endl;
        theKmlData = 0;
      }
      else
      {
//         std::cout << "GOT KMZ!!!!!!!!!!!!!!!!!!!" << std::endl;
      }
   }
   
   return result;
}
