#include "TorrentPieces.h"
#include "Utility.h"

using namespace utility;

TorrentPieces::TorrentPieces(valueDictionary torrent, std::vector<fileObj> fileList)
	: totalSize{ getTotalSize(fileList) }, 
	readablePieceSize { getReadablePieceSize() },
	readableTotalSize{ getReadableTotalSize() }
{
}

long TorrentPieces::getTotalSize(std::vector<fileObj> fileList)
{
	long total = 0;
	for (auto file : fileList)
	{
		total += file.fileSize;
	}
	return total;
}


std::string TorrentPieces::getReadablePieceSize()
{
	return humanReadableBytes(pieceSize);
}



std::string TorrentPieces::getReadableTotalSize()
{
	return humanReadableBytes(totalSize);
}