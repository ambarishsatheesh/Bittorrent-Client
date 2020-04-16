#pragma once
#include <string>
#include "Utility.h"

using namespace utility;

class fileObj
	
{
public:
	//directory structure of files
	std::string filePath;
	long fileSize;
	//sum of previous files in list
	long fileOffset;
	std::string readableFileSize;

	fileObj() : filePath{ "" }, fileSize{ 0 }, fileOffset{ 0 },
		readableFileSize{ "" }
	{}

private:
	void setReadableFileSize()
	{
		readableFileSize = humanReadableBytes(fileSize);
	}
	//let only Torrent class access setter
	friend class Torrent;
};

