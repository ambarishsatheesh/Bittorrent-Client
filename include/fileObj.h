#pragma once
#include <string>
#include "Utility.h"


namespace Bittorrent
{
	using namespace utility;

	class fileObj

	{
	public:
		//directory structure of files
		std::string filePath;
		long long fileSize;
		//sum of previous files in list
		long long fileOffset;
		std::string readableFileSize;

		fileObj() : filePath{ "" }, fileSize{ 0 }, fileOffset{ 0 },
			readableFileSize{ "" }
		{}

		void setReadableFileSize()
		{
			readableFileSize = humanReadableBytes(fileSize);
		}

	private:
		//let only Torrent class access setter
		friend class Torrent;
	};
}
