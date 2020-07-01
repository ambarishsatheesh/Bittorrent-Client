#pragma once
#include "ValueTypes.h"
#include "TorrentPieces.h"


namespace Bittorrent
{
	class TorrentStatus
	{
	public:
        //current state of torrent
        enum class currentStatus
        {
            completed = 1,
            started,
            stopped
        };

        currentStatus currentState;
		std::vector<std::vector<bool>> isBlockAcquired;
		std::vector<bool> isPieceVerified;
        std::shared_ptr<TorrentPieces> ptr_piecesData;

		//constructor
        TorrentStatus(const std::shared_ptr<TorrentPieces> pieces);
		//no default constructor - requires parameter
		TorrentStatus() = delete;

        int verifiedPiecesCount();
        float verifiedRatio();
        bool isCompleted();
        bool isStarted();
        long long downloaded();
        long long remaining();
        long long uploaded;

	private:
	};

}
