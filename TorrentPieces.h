#pragma once
#include "ValueTypes.h"
#include "fileObj.h"

class TorrentPieces
{
public:

	long blockSize;
	long pieceSize;
	std::string readablePieceSize;
	long totalSize;
	std::string readableTotalSize;

	//constructor
	TorrentPieces(valueDictionary torrent, std::vector<fileObj> fileList);
	//no default constructor - requires parameter
	TorrentPieces() = delete;

private:
	long getTotalSize(std::vector<fileObj> fileList);
	std::string getReadablePieceSize();
	std::string getReadableTotalSize();

};
