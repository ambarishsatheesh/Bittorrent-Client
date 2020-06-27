#include "TorrentPieces.h"
#include "Utility.h"
#include <iostream>


namespace Bittorrent
{
	using namespace utility;

	TorrentPieces::TorrentPieces()
        : blockSize{ 16384 }, pieceSize{ 0 }, totalSize{ 0 }, pieceCount{ 0 },
          sig_pieceVerified{std::make_shared<
                            boost::signals2::signal<void(int piece)>>()}
	{

	}

	valueDictionary TorrentPieces::piecesDataToDictionary(valueDictionary& dict)
	{
		dict.emplace("piece length", pieceSize);

		//pieces
		std::string piecesString;
		for (size_t i = 0; i < pieces.size(); ++i)
		{
			for (size_t j = 0; j < pieces.at(i).size(); ++j)
			{
				piecesString += pieces[i][j];
			}
		}
		dict.emplace("pieces", piecesString);

		return dict;
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
		pieceSize = static_cast<long>(boost::get<long long>(info.at("piece length")));


		//set pieces
		if (!info.count("pieces"))
		{
			throw std::invalid_argument("Error: no pieces specified in torrent!");
		}
		std::string tempString = boost::get<std::string>(info.at("pieces"));
		const size_t n = tempString.size();
		const size_t columns = 20;
		const size_t rows = n / columns;
		assert(rows * columns == n);
		pieces.resize(rows, std::vector<byte>(columns));
		for (size_t i = 0; i < rows; ++i)
		{
			for (size_t j = 0; j < columns; ++j)
			{
				pieces[i][j] = tempString.at(i * columns + j);
			}
		}

		//update pieceCount
		pieceCount = pieces.size();
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
		}
		return pieceSize;
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
		}
		return blockSize;
	}

	int TorrentPieces::setBlockCount(int piece)
	{
		return std::ceil(setPieceSize(piece) / static_cast<double>(blockSize));
	}

    std::string TorrentPieces::readablePieceSize()
    {
        return humanReadableBytes(pieceSize);
    }

    std::string TorrentPieces::readableTotalSize()
    {
        return humanReadableBytes(totalSize);
    }
}
