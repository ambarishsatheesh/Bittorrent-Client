#pragma once
#include <string>
#include "Utility.h"

using namespace utility;

class fileObj
	
{
public:

	fileObj() : readableFileSize{ getReadableFileSize() } {}

	std::string filePath;
	long fileSize;
	long fileOffset;
	std::string readableFileSize;

	std::string getReadableFileSize()
	{
		return humanReadableBytes(fileSize);
	}
};

