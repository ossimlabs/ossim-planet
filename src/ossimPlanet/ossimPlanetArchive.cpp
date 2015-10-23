
#include <ossim/base/ossimNotify.h>
#include <ossimPlanet/ossimPlanetArchive.h>



ossimPlanetArchive::ossimPlanetArchive()
{	
	useArchiveMapping = false;
}

ossimPlanetArchive::~ossimPlanetArchive()
{
}

void ossimPlanetArchive::addMapping(ossimPlanetArchiveMapping &mapping)
{
	OpenThreads::ScopedLock<ossimPlanetReentrantMutex> lock(theArchiveMutex);

	// test to see if the mapping has an ending slash... if not add one
	ossimFilename src = mapping.getSource();
	ossimFilename dest = mapping.getDestination();

	src = convertToDirectory(src);
	dest = convertToDirectory(dest);

	mapping.setSource(src);
	mapping.setDestination(dest);

	mappingList.push_back(mapping);
}


ossimFilename ossimPlanetArchive::convertToDirectory(ossimFilename &filename)
{
	ossimString drive;
	ossimString path;
	ossimString file;
	ossimString ext;

	filename.split(drive, path, file, ext);

	// if the drive is empty its a unix/linux drive
	if( drive.empty() )
	{
		return (filename[filename.length()-1] != '/')? ossimFilename(filename.append("/")) : filename;
	}
	// else its a windows drive
	else
	{
		return (filename[filename.length()-1] != '\\')? ossimFilename(filename.append("\\")) : filename;		
	}
}


void ossimPlanetArchive::removeMapping(ossimPlanetArchiveMapping &mapping)
{
	OpenThreads::ScopedLock<ossimPlanetReentrantMutex> lock(theArchiveMutex);
	ossim_uint32 row = 0;
	for( row = 0; row < mappingList.size(); row++ )
	{		
		if( (mapping.getSource() == mappingList[row].getSource()) && 
         (mapping.getDestination() == mappingList[row].getDestination()) )
		{
			break;
		}
	}

	if( row < mappingList.size() )
	{
		mappingList.erase(mappingList.begin() + row);
	}
}

ossimFilename ossimPlanetArchive::matchPath(const ossimFilename &filename)
{
	OpenThreads::ScopedLock<ossimPlanetReentrantMutex> lock(theArchiveMutex);
	std::vector<ossimPlanetArchiveMapping>::iterator it;

	if( mappingList.empty() || filename.exists() )
		return filename;

	ossimString file_drive;
	ossimString file_path;
	ossimString file_file;
	ossimString file_ext;
	
	ossimString src_drive;
	ossimString src_path;
	ossimString src_file;
	ossimString src_ext;
	
	ossimString dest_drive;
	ossimString dest_path;
	ossimString dest_file;
	ossimString dest_ext;

	ossimFilename src;
	ossimFilename dest;

	filename.split(file_drive, file_path, file_file, file_ext);
	ossimNotify(ossimNotifyLevel_INFO) << "ossimPlanetArchive::matchPath() - filename split: drive=" << file_drive << " path=" << file_path << " file=" << file_file << " ext=" << file_ext << std::endl;
	ossimPlanetArchiveMapping opam;

	// try to clean this up
	// first test for the exact match (drive and path)
	ossimNotify(ossimNotifyLevel_INFO) << "ossimPlanetArchive::matchPath() - finding exact file_path" << std::endl;

	// test for a path only  match
	for( it = mappingList.begin(); it != mappingList.end(); it++ )
	{
		opam = (*it);
		
		src = opam.getSource();
		src.split(src_drive, src_path, src_file, src_ext);
		ossimNotify(ossimNotifyLevel_INFO) << "ossimPlanetArchive::matchPath() - src split: drive=" << src_drive << " path=" << src_path << " file=" << src_file << " ext=" << src_ext << std::endl;

		if( (file_path == src_path) )
		{
			ossimNotify(ossimNotifyLevel_INFO) << "ossimPlanetArchive::matchPath() - match found. File: " << file_path << " Source: " << src_path << std::endl;
			dest = opam.getDestination();
			dest.split(dest_drive, dest_path, dest_file, dest_ext);
			dest.merge(dest_drive, dest_path, file_file, file_ext);
			//dest.convertForwardToBackSlashes();
			return dest;
		}
	}


	ossimNotify(ossimNotifyLevel_INFO) << "ossimPlanetArchive::matchPath() - finding exact file_drive/file_path" << std::endl;
	for( it = mappingList.begin(); it != mappingList.end(); it++ )
	{
		opam = (*it);
		
		src = opam.getSource();
		src.split(src_drive, src_path, src_file, src_ext); 
		ossimNotify(ossimNotifyLevel_INFO) << "ossimPlanetArchive::matchPath() - src split: drive=" << src_drive << " path=" << src_path << " file=" << src_file << " ext=" << src_ext << std::endl;

		if( (file_drive+file_path) == (src_drive+src_path) )
		{
			ossimNotify(ossimNotifyLevel_INFO) << "ossimPlanetArchive::matchPath() - match found. File: " << file_drive << " " << file_path << " Source: " << src_drive << " " << src_path << std::endl;
			dest = opam.getDestination();
			dest.split(dest_drive, dest_path, dest_file, dest_ext);
			dest.merge(dest_drive, dest_path, file_file, file_ext);
			//dest.convertForwardToBackSlashes();
			return dest;
		}
	}


	// any sub path of the given filename
	// Fail safe: this is a complete hack... need to find a better way to do this
	ossimNotify(ossimNotifyLevel_INFO) << "ossimPlanetArchive::matchPath() - if path contains src_path" << std::endl;
	for( it = mappingList.begin(); it != mappingList.end(); it++ )
	{
		opam = (*it);
		
		src = opam.getSource();
		src.split(src_drive, src_path, src_file, src_ext); 
		ossimNotify(ossimNotifyLevel_INFO) << "ossimPlanetArchive::matchPath() - src split: drive=" << src_drive << " path=" << src_path << " file=" << src_file << " ext=" << src_ext << std::endl;

		if( file_path.contains( src_path )  )
		{
			ossimNotify(ossimNotifyLevel_INFO) << "ossimPlanetArchive::matchPath() - match found. File: " << src_path << " Source: " << std::endl;
			dest = opam.getDestination();
			dest.split(dest_drive, dest_path, dest_file, dest_ext);
			dest.merge(dest_drive, file_path, file_file, file_ext);
			//dest.convertForwardToBackSlashes();
			
			return dest;
		}
	}
	
	// if all else fails...
	//dest = opam.getDestination();
	//dest.split(dest_drive, dest_path, dest_file, dest_ext);
	//dest.merge(ossimString("x:"), file_path, file_file, file_ext);
	//dest.convertForwardToBackSlashes();
	
	//return dest;

	return filename;
}

void ossimPlanetArchive::setArchiveMappingEnabledFlag(bool enabled)
{
	OpenThreads::ScopedLock<ossimPlanetReentrantMutex> lock(theArchiveMutex);
	useArchiveMapping = enabled;
}

bool ossimPlanetArchive::archiveMappingEnabled()
{
	return useArchiveMapping;
}

std::vector<ossimPlanetArchiveMapping> ossimPlanetArchive::getMappingList()
{
	return mappingList;
}