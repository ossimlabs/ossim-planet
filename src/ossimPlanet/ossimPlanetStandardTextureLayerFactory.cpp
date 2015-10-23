#include <ossimPlanet/ossimPlanetStandardTextureLayerFactory.h>
#include <ossimPlanet/ossimPlanetOssimImageLayer.h>
#include <ossimPlanet/ossimPlanetWmsImageLayer.h>
#include <ossimPlanet/ossimPlanetTextureLayerGroup.h>
#include <OpenThreads/ScopedLock>
#include <ossim/base/ossimKeywordNames.h>
#include <sstream>
#include <wms/wmsUrl.h>

ossimPlanetStandardTextureLayerFactory* ossimPlanetStandardTextureLayerFactory::theInstance=0;

ossimPlanetStandardTextureLayerFactory::ossimPlanetStandardTextureLayerFactory()
{
   theInstance = this;
}

ossimPlanetStandardTextureLayerFactory* ossimPlanetStandardTextureLayerFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimPlanetStandardTextureLayerFactory;
   }

   return theInstance;
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetStandardTextureLayerFactory::createLayer(const ossimString& name, bool openAllEntriesFlag)const
{
   OpenThreads::ScopedLock<OpenThreads::Mutex> lock(theMutex);
   osg::ref_ptr<ossimPlanetTextureLayer> result;

   ossimFilename filename = name;
   
   if(filename.exists())
   {
      return createLayerFromFilename(filename, openAllEntriesFlag);
   }
   ossimKeywordlist kwl;
   std::istringstream in(name);

   if(kwl.parseStream(in))
   {
      return createLayerFromKwl(kwl);
   }
   if((name == "ossimPlaneTextureLayerGroup")||
      (name == "group"))
   {
      return new ossimPlanetTextureLayerGroup;
   }
#ifndef OSGPLANET_WITHOUT_WMS
   else if((name == "ossimPlanetWmsImageLayer")||
           (name == "wms"))
   {
      return new ossimPlanetWmsImageLayer;
   }
#endif  
   return result;
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetStandardTextureLayerFactory::createLayerFromFilename(const ossimFilename& name, bool openAllEntriesFlag)const
{
   osg::ref_ptr<ossimPlanetOssimImageLayer> imageLayer = new ossimPlanetOssimImageLayer;
   imageLayer->openImage(name);
   if(!imageLayer->isStateSet(ossimPlanetTextureLayer_NO_SOURCE_DATA))
   {
      if(openAllEntriesFlag&&imageLayer->isMultiEntry())
			
      {
			return imageLayer->groupAllEntries().get();			
		}
      else
      {
         return imageLayer.get();
      }
   }
	
   return 0;
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetStandardTextureLayerFactory::createLayerFromKwl(const ossimKeywordlist& kwl,
                                                                                                 const ossimString& prefix)const
{
   if(kwl.find(prefix, "archive0.type"))
   {
      return createLayerFromOldKwl(kwl, prefix);
   }

   return 0;
}

osg::ref_ptr<ossimPlanetTextureLayer> ossimPlanetStandardTextureLayerFactory::createLayerFromOldKwl(const ossimKeywordlist& kwl,
                                                                                                    const ossimString& prefix)const
{
   osg::ref_ptr<ossimPlanetTextureLayerGroup> groupLayer = new ossimPlanetTextureLayerGroup;
   
   ossimString archiveKey;
   ossimString archive = kwl.find(prefix, "archiv0.type");
   unsigned int idx = 0;
   bool done = false;
   while(!done)
   {
      stringstream s;
      s << "archive" << idx << ".";
      archiveKey = s.str();

      // we will need to move all this to an archive factory
      // later.  For now we will hard code a local and remote archive
      // creations here.
      //
      ossimString archiveType = kwl.find(prefix+archiveKey, "type");
      if(archiveType == "local")
      {
         ossimString localPrefix = archiveKey;
         osg::ref_ptr<ossimPlanetTextureLayerGroup> groupLocal = new ossimPlanetTextureLayerGroup;
         int idx = 0;
         ossimFilename file;
         bool doneLocal = false;
         while(!doneLocal)
         {
            std::stringstream out;
            out << "file" << idx;
            
            file = kwl.find(localPrefix, out.str().c_str());
            if((file!="")&&(file.exists()))
            {
               osg::ref_ptr<ossimPlanetOssimImageLayer> layer = new ossimPlanetOssimImageLayer;
               layer->openImage(file);
               if(!layer->isStateSet(ossimPlanetTextureLayer_NO_SOURCE_DATA)&&
                  !layer->isStateSet(ossimPlanetTextureLayer_NO_GEOM))
               {
                  groupLocal->addBottom(layer.get());
               }
            }
            else
            {
               doneLocal = true;
            }
            ++idx;
         }
         if(groupLocal->numberOfLayers() > 0)
         {
            groupLayer->addBottom(groupLocal.get());
         }
//          ossimPlanetTextureArchive* archive = new ossimPlanetOssimArchive;
//          archive->loadState(kwl,
//                             prefix+archiveKey);
//          addArchive(archive);
      }
      else if(archiveType == "wms")
      {
#ifndef OSGPLANET_WITHOUT_WMS
         osg::ref_ptr<ossimPlanetWmsImageLayer> layer = new ossimPlanetWmsImageLayer;

         ossimString server = kwl.find(prefix+archiveKey, "server");
         ossimString cache  = kwl.find(prefix+archiveKey, "cache_dir");

         if(!server.empty())
         {
            layer->setServer(server);
            if(!cache.empty())
            {
               layer->setCacheDirectory(cache);
            }
            groupLayer->addBottom(layer.get());
            
         }
         
//          ossimPlanetTextureArchive* archive = new ossimPlanetWmsArchive;

//          archive->loadState(kwl,
//                             prefix+archiveKey);
//          addArchive(archive);
#endif
      }
      else if(archiveType == "background_wms")
      {
#ifndef OSGPLANET_WITHOUT_WMS
//          ossimPlanetTextureArchive* background = new ossimPlanetWmsArchive;
//          background->addCallback(theArchiveCallback.get());
//          background->loadState(kwl,
//                                prefix+archiveKey);
//          setBackgroundArchive(background);
#endif
      }
      else if(archiveType == "background_local")
      {
//          ossimPlanetTextureArchive* background = new ossimPlanetOssimArchive;
//          background->addCallback(theArchiveCallback.get());
//          background->loadState(kwl,
//                                prefix+archiveKey);
//          setBackgroundArchive(background);
      }
      else
      {
         done = true;
      }
      
      ++idx;
   }

   if(groupLayer->numberOfLayers() < 1)
   {
      groupLayer = 0;
   }
   
   return groupLayer.get();
}
