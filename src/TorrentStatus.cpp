#include "TorrentStatus.h"

namespace Bittorrent
{
    TorrentStatus::TorrentStatus(TorrentPieces* pieces)
        : currentState{currentStatus::stopped}, ptr_piecesData(pieces),
          uploaded{0}
	{
	}

    void TorrentStatus::torrentToStatusData()
    {
        isPieceVerified.resize(ptr_piecesData->pieceCount);
        isBlockAcquired.resize(ptr_piecesData->pieceCount);

        for (int i = 0; i < ptr_piecesData->pieceCount; ++i)
        {
            isBlockAcquired.at(i).resize(ptr_piecesData->getBlockCount(i));
        }
    }

    int TorrentStatus::verifiedPiecesCount()
    {
        return std::count(isPieceVerified.cbegin(),
                    isPieceVerified.cend(), true);
    }

    float TorrentStatus::verifiedRatio()
    {
        return verifiedPiecesCount() / float(ptr_piecesData->pieceCount);
    }

    bool TorrentStatus::isCompleted()
    {
        return verifiedPiecesCount() == ptr_piecesData->pieceCount;
    }

    bool TorrentStatus::isStarted()
    {
        return verifiedPiecesCount() > 0;
    }

    long long TorrentStatus::downloaded()
    {
        return ptr_piecesData->pieceSize * verifiedPiecesCount();
    }

    long long TorrentStatus::remaining()
	{
        return ptr_piecesData->totalSize - downloaded();
	}
}
