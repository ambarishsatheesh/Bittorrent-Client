#pragma once
#include <string>
#include <vector>
#include <memory>

#include "fileObj.h"
#include "TorrentGeneral.h"
#include "TorrentPieces.h"
#include "TorrentHashes.h"
#include "TorrentStatus.h"


class Torrent
{
public:
	valueDictionary decodedTorrent;

	//general info
	TorrentGeneral generalData;

	//file info
	std::vector<fileObj> fileList;
	std::string fileName;
	bool isPrivate;

	//pieces & blocks
	TorrentPieces piecesData;

	//hashes
	TorrentHashes hashesData;

	//Status
	TorrentStatus statusData;

	//constructor
	Torrent(const valueDictionary& torrent);
	//no default constructor - requires parameter
	Torrent() = delete;

private:
	//get file info
	std::vector<fileObj> getFileList();
	bool getIsPrivate();

};

