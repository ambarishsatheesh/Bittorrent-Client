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
        } currentState;

		std::vector<std::vector<bool>> isBlockAcquired;
		std::vector<bool> isPieceVerified;
        TorrentPieces* ptr_piecesData;

        //fill data using decoded data
        void torrentToStatusData();

		//constructor
        TorrentStatus(TorrentPieces& pieces);
		//no default constructor - requires parameter
		TorrentStatus() = delete;

        int verifiedPiecesCount();
        float verifiedRatio();
        bool isCompleted();
        bool isStarted();
        long long downloaded();
        long long remaining();
        int pieceCount;
        int pieceSize;
        int blockSize;
        int totalSize;
        long acquiredBlocksCount;
        long long uploaded;

	private:
	};

}
