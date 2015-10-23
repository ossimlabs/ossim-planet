//
// opvLoadModel.h
//
// osgPlanetView load model functionality
//

osg::ref_ptr<osg::Node> createCityModel(const ossimFilename& file,
	ossimPlanetLandType landType, int utmZone, int utmX, int utmY);

