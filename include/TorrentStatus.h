#pragma once
#include "ValueTypes.h"
#include "TorrentPieces.h"


namespace Bittorrent
{
	class TorrentStatus
	{
	public:

		std::vector<std::vector<bool>> isBlockAcquired;
		std::vector<bool> isPieceVerified;
        std::shared_ptr<TorrentPieces> ptr_piecesData;

		//constructor
        TorrentStatus(std::shared_ptr<TorrentPieces> pieces,
                      const valueDictionary& torrent);
		//temp
        TorrentStatus(const std::shared_ptr<TorrentPieces> pieces);
		//no default constructor - requires parameter
		TorrentStatus() = delete;

        int verifiedPiecesCount();
        float verifiedRatio();
        bool isCompleted();
        bool isStarted();
        long long downloaded();
        long long uploaded();
        long long remaining();

	private:
	};

}
