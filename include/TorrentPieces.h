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

		//piece verification signal
		using sigPieceVer = boost::signals2::signal<void(int piece)>;
		std::shared_ptr<sigPieceVer> pieceVerifiedSig;

        std::string readablePieceSize();
        std::string readableTotalSize();

		int setPieceSize(int piece);
		int setBlockSize(int piece, int block);
		int setBlockCount(int piece);

		//constructor
		TorrentPieces();

	};
}
