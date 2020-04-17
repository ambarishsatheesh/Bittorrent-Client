#pragma once
#include "ValueTypes.h"
#include "fileObj.h"

class TorrentPieces
{
public:

	long blockSize;
	long long pieceSize;
	std::string readablePieceSize;
	long long totalSize;
	std::string readableTotalSize;
	int pieceCount;
	std::vector<std::vector<byte>> pieces;

	//constructor
	TorrentPieces();

	//fill in pieces data
	void torrentToPiecesData(const std::vector<fileObj>& fileList,
		const valueDictionary& torrent);

	void setReadablePieceSize()
	{
		readablePieceSize = humanReadableBytes(pieceSize);
	}

	void setReadableTotalSize()
	{
		readableTotalSize = humanReadableBytes(totalSize);
	}


private:

	int setPieceSize(int piece);
	int setBlockSize(int piece, int block);
	int setBlockCount(int piece);

};
