#include "TorrentPieces.h"
#include "Utility.h"
#include <iostream>

using namespace utility;

TorrentPieces::TorrentPieces()
	: totalSize{ 0 },
	readablePieceSize { "" },
	readableTotalSize{ "" }
{

}


//generate piece data from torrent object
void TorrentPieces::torrentToPiecesData(const std::vector<fileObj>& fileList, 
	const valueDictionary& torrent)
{
	for (auto file : fileList)
	{
		totalSize += file.fileSize;
	}

	valueDictionary info = boost::get<valueDictionary>(torrent.at("info"));

	if (!info.count("piece length"))
	{
		throw std::invalid_argument("Error: no piece length specified in torrent!");
	}
	pieceSize = static_cast<long>(boost::get<integer>(info.at("piece length")));


	//set pieces
	if (!info.count("pieces"))
	{
		throw std::invalid_argument("Error: no pieces specified in torrent!");
	}

	std::string tempString = boost::get<std::string>(info.at("pieces"));
	std::vector<byte> tempByte(tempString.size());
	for (auto i : tempString)
	{
		tempByte.push_back(i);
	}
	const size_t n = tempByte.size();
	const size_t columns = 20;
	const size_t rows = n/columns;
	assert(rows * columns == n);
	pieces.resize(rows, std::vector<byte>(columns));
	for (size_t i = 0; i < rows; ++i)
	{
		for (size_t j = 0; j < columns; ++j)
		{
			pieces[i][j] = tempByte.at(i * columns + j);
		}
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