#ifndef ossimPlanetKmlReaderWriter_HEADER
#define ossimPlanetKmlReaderWriter_HEADER
#include <osgDB/ReaderWriter>

class ossimPlanetKmlReaderWriter : : public osgDB::ReaderWriter
{
public:
   ossimPlanetKmlReaderWriter();
   
   virtual ReadResult readNode(const std::string& /*fileName*/, const Options* =NULL) const;

protected:
   
};

#endif
