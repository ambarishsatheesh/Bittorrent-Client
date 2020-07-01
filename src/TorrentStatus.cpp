#include "TorrentStatus.h"

namespace Bittorrent
{
    TorrentStatus::TorrentStatus(std::shared_ptr<TorrentPieces> pieces)
        : currentState{currentStatus::stopped}, ptr_piecesData(pieces),
          uploaded{0}
	{
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
