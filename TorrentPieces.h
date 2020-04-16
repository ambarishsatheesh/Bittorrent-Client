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
	int pieceCount;

	//constructor
	TorrentPieces(const valueDictionary& torrent, const std::vector<fileObj>& fileList);
	//no default constructor - requires parameter
	TorrentPieces() = delete;

private:
	void torrentToPiecesObj(const valueDictionary& torrent, const std::vector<fileObj>& fileList);
	int setPieceSize(int piece);
	int setBlockSize(int piece, int block);
	int setBlockCount(int piece);

};
