#ifndef ossimPlanetMessage_HEADER
#define ossimPlanetMessage_HEADER
#include <ossim/base/ossimString.h>
#include <osg/Referenced>
#include <ossimPlanet/ossimPlanetExport.h>
#include <vector>

class OSSIMPLANET_DLL ossimPlanetMessage : public osg::Referenced
{
public:
   typedef unsigned char DataType;
   typedef std::vector<ossimPlanetMessage::DataType> StorageType;
   
   ossimPlanetMessage(const ossimString& id=ossimString("") )
   {
	   setId(id);
   }
   ossimPlanetMessage(const ossimString& id, const ossimString& data )
   {
      setId(id);
      setData(data);
   }
   ossimPlanetMessage(const ossimString& id, const std::vector<char>& byteBuffer):
   theId(id),
   theData(byteBuffer.begin(), byteBuffer.end())
   {
   }
   ossimPlanetMessage(const ossimString& id, const StorageType& buf):
   theId(id),
   theData(buf)
   {
   }
   ossimPlanetMessage(const ossimPlanetMessage& src)
   :theId(src.theId),
   theData(src.theData){}
   ossimPlanetMessage* clone()const{return new ossimPlanetMessage(*this);}
   const ossimString& id()const{return theId;}
   void setId(const ossimString& id){theId = id;}
   void setData(const ossimPlanetMessage::StorageType& data){theData = data;}
   void setData(const ossimString& data){theData.clear();theData.insert(theData.begin(), data.begin(), data.end());}
   ossim_uint32 dataSize()const{return (ossim_uint32)theData.size();}                                                                     
   const ossimPlanetMessage::StorageType& data()const{return theData;}
protected:
   ossimString                     theId;
   ossimPlanetMessage::StorageType theData;
};

#endif
