#include <ossim/init/ossimInit.h>
#include <ossim/base/ossimGeoidManager.h>
#include <ossimPredator/ossimPredatorApi.h>
#include <ossimPredator/ossimPredatorUavProjection.h>
#include <osg/ArgumentParser>
#include <fstream>
int main(int argc, char* argv[])
{
   ossimInit::instance()->initialize(argc, argv);
   ossimString tempString;
   ossimString tempString2;
   osg::ArgumentParser::Parameter stringParam(tempString);
   osg::ArgumentParser::Parameter stringParam2(tempString2);
   
   osg::ArgumentParser arguments(&argc,argv);
   arguments.getApplicationUsage()->addCommandLineOption("--video", 
                                                         "specify the input video to process");
   arguments.getApplicationUsage()->addCommandLineOption("--animation-path-out", 
                                                         "specify the filename to output the animation path to");
   arguments.getApplicationUsage()->addCommandLineOption("--test-sensor-projection", 
                                                         "Test sensor projection from KlvInfo");
   unsigned int helpType = 0;
   if ((helpType = arguments.readHelpType()))
   {
      arguments.getApplicationUsage()->write(std::cout, helpType);
      return 1;
   }
   while(arguments.read("--video", stringParam))
   {
      if(arguments.read("--test-sensor-projection"))
      {
         ossimRefPtr<ossimPredatorUavProjection> proj = new ossimPredatorUavProjection;
         ossimRefPtr<ossimPredatorVideo> predatorVideo = new ossimPredatorVideo();
         
         if(predatorVideo->open(ossimFilename(tempString)))
         {
            ossim_uint32 imageWidth  = predatorVideo->imageWidth();
            ossim_uint32 imageHeight = predatorVideo->imageHeight();
            ossimRefPtr<ossimPredatorVideo::KlvInfo> klvinfo;
            ossim_float64 prevTime = -1.0;
            while(( klvinfo = predatorVideo->nextKlv()).valid())
            {
               ossim_float64 lat,lon,elev;
               ossim_float32 hfov;
               ossim_float32 vfov;
               ossim_float32 h,p,r;
               ossim_float32 obliquityAngle;
               ossim_float32 angleToNorth;
               ossim_float32 slantRange;
               ossim_float32 sensorRoll = 0.0;
               klvinfo->table()->print(std::cout) << std::endl;
               if(klvinfo->table()->getSensorPosition(lat, lon, elev)&&
                  klvinfo->table()->getPlatformOrientation(h,p,r))
               {
                  if(!klvinfo->table()->getObliquityAngle(obliquityAngle))
                  {
                     obliquityAngle = 0.0;
                  }
                  if(!klvinfo->table()->getSlantRange(slantRange))
                  {
                     slantRange = 1.0;
                  }
                  bool gotHfov = true;
                  if(!klvinfo->table()->getHorizontalFieldOfView(hfov))
                  {
                     hfov = 1.0;
                     gotHfov = false;
                  }
                  if(!klvinfo->table()->getVerticalFieldOfView(vfov))
                  {
                     vfov = hfov;
                  }
                  else if(!gotHfov)
                  {
                     hfov = vfov;
                  }
                  klvinfo->table()->getSensorRollAngle(sensorRoll);
                  if(!klvinfo->table()->getAngleToNorth(angleToNorth))
                  {
                     angleToNorth = 0.0;
                  }
                  ossim_float64 value = ossimGeoidManager::instance()->offsetFromEllipsoid(ossimGpt(lat, lon, elev));
                  if(!ossim::isnan(value))
                  {
                     elev += value;
                  }
                  proj->setParameters(imageWidth, 
                                      imageHeight, 
                                      ossimGpt(lat, lon, elev), 
                                      ossimGpt(), 
                                      h, p, sensorRoll,
                                      hfov,
                                      vfov,
                                      obliquityAngle,
                                      angleToNorth,
                                      0.0,
                                      0.0);
                  ossimDpt ipt(imageWidth*.5, imageHeight*.5);
                  ossimGpt centerProj;
                  ossimGpt ul;
                  ossimGpt ur;
                  ossimGpt lr;
                  ossimGpt ll;
                  proj->lineSampleToWorld(ipt, centerProj);
                  proj->lineSampleToWorld(ossimDpt(0,0), ul);
                  proj->lineSampleToWorld(ossimDpt(imageWidth,0), ur);
                  proj->lineSampleToWorld(ossimDpt(imageWidth,imageHeight), lr);
                  proj->lineSampleToWorld(ossimDpt(imageHeight,0), ll);
                  
                  std::cout << std::setprecision(15);
                  std::cout << "position = " << ossimGpt(lat, lon, elev) << std::endl;
                  std::cout << "centerGpt = " << centerProj << std::endl;
                  std::cout << "ul        = " << ul << std::endl;
                  std::cout << "ur        = " << ur << std::endl;
                  std::cout << "lr        = " << lr << std::endl;
                  std::cout << "ll        = " << ll << std::endl;
                  
//                  std::cout << "angle to north = " << angleToNorth << std::endl;
//                  std::cout << "ObliquityAngle = " << obliquityAngle << std::endl;
//                  std::cout << "hpr = " << h << ", " << p << ", " << r << std::endl;
//                  std::cout << "Platform  = " << ossimGpt(lat, lon, elev) << std::endl;
//                  std::cout << "World point = " << centerProj << std::endl;
                  if(klvinfo->table()->getFrameCenter(lat, lon, elev))
                  {
                     std::cout << "Center frame = " << ossimGpt(lat, lon, elev) << std::endl;
                  }
               }
            }
         }
      }
      
      if(arguments.read("--animation-path-out", stringParam2))
      {
         std::ofstream out(tempString2.c_str());
         if(out.good())
         {
            out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
            out << "<AnimationPath>";
            out << "<GeospatialPath timeUnit='seconds' positionType='latlonhgt' orientationType='lsrhpr'>";
            out << "<description>Cool path</description>";
            out << "<coordinates>";
            
            ossimRefPtr<ossimPredatorVideo> predatorVideo = new ossimPredatorVideo();
            
            if(predatorVideo->open(ossimFilename(tempString)))
            {
               ossim_float32 srange;
               ossimRefPtr<ossimPredatorVideo::KlvInfo> klvinfo;
               ossim_float64 prevTime = -1.0;
               while(( klvinfo = predatorVideo->nextKlv()).valid())
               {
                  klvinfo->table()->getSlantRange(srange);
                  //std::cout << "range === " << srange << std::endl;
                  if(!ossim::almostEqual(klvinfo->time(), prevTime, 1e-10))
                  {
                     prevTime = klvinfo->time();
                     ossimString sensorLat, sensorLon, sensorAlt, h,p,r;
                     if(klvinfo->table()->valueAsString(sensorLat, KLV_KEY_SENSOR_LATITUDE)&&
                        klvinfo->table()->valueAsString(sensorLon, KLV_KEY_SENSOR_LONGITUDE)&&
                        klvinfo->table()->valueAsString(sensorAlt, KLV_KEY_SENSOR_TRUE_ALTITUDE))
                     {
                        klvinfo->table()->valueAsString(h,KLV_KEY_PLATFORM_HEADING_ANGLE);
                        klvinfo->table()->valueAsString(p,KLV_KEY_PLATFORM_PITCH_ANGLE);
                        klvinfo->table()->valueAsString(r,KLV_KEY_PLATFORM_ROLL_ANGLE);
                        
                        double headingAdjust = h.toDouble();
                        if(headingAdjust > 180.0) headingAdjust -= 360.0;
                        out << klvinfo->time() << "," 
                        << sensorLat <<"," << sensorLon << "," <<sensorAlt.toDouble()*0.304801 << ","
                        << headingAdjust << "," << p.toDouble() << "," << r.toDouble() <<std::endl;
                     }
                  }
               }
            }
            out << "</coordinates></GeospatialPath></AnimationPath>";
         }
      }
   }
}
