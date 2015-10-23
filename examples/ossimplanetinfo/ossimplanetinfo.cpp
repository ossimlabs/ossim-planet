#include <ossim/init/ossimInit.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimRefPtr.h>
#include <iostream>
#ifdef OSSIMPLANET_ENABLE_PREDATOR
#include <ossimPredator/ossimPredatorApi.h>
#include <iomanip>

bool printPredatorInfo(const ossimFilename& file, std::ostream& out)
{
	out << std::setprecision(20);
	bool result = false;
	ossimRefPtr<ossimPredatorVideo> video = new ossimPredatorVideo();
	ossimRefPtr<ossimPredatorVideo::FrameInfo> frameInfo;
	result = video->open(file);
	if(result)
	{
		// look at the first couple of seconds worth of video to find a klv stream
		//
		ossim_uint32 maxFramesToCheck = video->videoFrameRate()*2;
		bool foundInfo = false;
		ossim_uint32 frameCount=0;
		while((!foundInfo)&&
				(frameCount <= maxFramesToCheck)&&
				((frameInfo = video->nextFrame()).valid()))
		{
			++frameCount;
			ossimRefPtr<ossimPredatorKlvTable> klvTable = frameInfo->klvTable();
			if(klvTable.valid())
			{
				ossimGpt ulg, urg, lrg, llg;
				ossimString prefix = "video0.frame0.";
				foundInfo = klvTable->getCornerPoints(ulg, urg, lrg, llg);
				if(!foundInfo)
				{
					ossim_float64 lat, lon, elev;
					
					foundInfo = klvTable->getFrameCenter(lat, lon, elev);
					if(foundInfo)
					{
						ulg = ossimGpt(lat, lon, elev);
						urg = ulg;
						lrg = ulg;
						llg = ulg;
					}
				}					
				if(foundInfo)
				{
					out << "video0.type: ossimPredatorVideo\n"
					    << prefix << "ul_lat: " << ulg.latd() << "\n"
					    << prefix << "ul_lon: " << ulg.lond() << "\n"
					    << prefix << "ur_lat: " << urg.latd() << "\n"
					    << prefix << "ur_lon: " << urg.lond() << "\n"
					    << prefix << "lr_lat: " << lrg.latd() << "\n"
					    << prefix << "lr_lon: " << lrg.lond() << "\n"
					    << prefix << "ll_lat: " << llg.latd() << "\n"
					    << prefix << "ll_lon: " << llg.lond() << "\n";
				}
			}
		}
	}
	return result;
}
#endif


bool printVideoInfo(const ossimFilename& file, std::ostream& out)
{
	bool result = false;
#ifdef OSSIMPLANET_ENABLE_PREDATOR
	result = printPredatorInfo(file, out);
#endif
	
	return result;
}

int main(int argc, char* argv[])
{
	ossimInit::instance()->initialize(argc, argv);

	if(argc < 2)
	{
		std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
		return 0;
	}
	ossimFilename file(argv[1]);
	if(printVideoInfo(file, std::cout))
	{
		return 0;
	}
	
	return 0;
}
