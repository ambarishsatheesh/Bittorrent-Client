#pragma once
#include "ValueTypes.h"
#include "fileObj.h"
#include <boost/signals2.hpp>


namespace Bittorrent
{
	class TorrentPieces
	{
	public:

		long blockSize;
		long long pieceSize;
		long long totalSize;
		int pieceCount;
		std::vector<std::vector<byte>> pieces;

		//fill in pieces data
		void torrentToPiecesData(const std::vector<fileObj>& fileList,
			const valueDictionary& torrent);

		valueDictionary piecesDataToDictionary(valueDictionary& dict);

        std::string readablePieceSize();
        std::string readableTotalSize();

		int getPieceSize(int piece);
		int getBlockSize(int piece, int block);
		int getBlockCount(int piece);

		//constructor
		TorrentPieces();

	};
}
