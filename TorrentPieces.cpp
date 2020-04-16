#include "TorrentPieces.h"
#include "Utility.h"

using namespace utility;

TorrentPieces::TorrentPieces(const valueDictionary& torrent, 
	const std::vector<fileObj>& fileList)
	: totalSize{ 0 },
	readablePieceSize { "" },
	readableTotalSize{ "" }
{
	torrentToPiecesObj(torrent, fileList);
}

void TorrentPieces::torrentToPiecesObj(const valueDictionary& torrent, 
	const std::vector<fileObj>& fileList)
{
	for (auto file : fileList)
	{
		totalSize += file.fileSize;
	}

	readablePieceSize = humanReadableBytes(pieceSize);

	readableTotalSize = humanReadableBytes(totalSize);
}


int TorrentPieces::setPieceSize(int piece)
{
	if (piece == pieceCount - 1)
	{
		const int remainder = totalSize % pieceSize;
		if (remainder != 0)
		{
			return remainder;
		}
		return pieceSize;
	}
}

int TorrentPieces::setBlockSize(int piece, int block)
{
	if (piece == setBlockCount(piece) - 1)
	{
		const int remainder = setPieceSize(piece) % blockSize;
		if (remainder != 0)
		{
			return remainder;
		}
		return blockSize;
	}
}

int TorrentPieces::setBlockCount(int piece)
{
	return std::ceil(setPieceSize(piece) / static_cast<double>(blockSize));
}