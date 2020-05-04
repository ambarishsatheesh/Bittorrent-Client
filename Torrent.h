#pragma once
#include <string>
#include <vector>
#include <memory>

#include "fileObj.h"
#include "TorrentGeneral.h"
#include "TorrentPieces.h"
#include "TorrentHashes.h"
#include "TorrentStatus.h"


namespace Bittorrent
{
	class Torrent
	{
	public:

		//general info
		TorrentGeneral generalData;

		//file info
		std::vector<fileObj> fileList;

		//pieces & blocks
		TorrentPieces piecesData;

		//hashes
		TorrentHashes hashesData;

		//Status
		TorrentStatus statusData;

		//construct using torrent info
		Torrent(const char* fullFilePath, const valueDictionary& torrent);
		//construct without any info
		Torrent(const char* fullFilePath);
		//no default constructor - requires parameter
		Torrent() = delete;

		//set file info
		void setFileList(const valueDictionary& torrent);
		valueDictionary filesToDictionary(valueDictionary& dict);

	private:

	};
}
