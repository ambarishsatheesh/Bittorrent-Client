#include "TorrentStatus.h"

namespace Bittorrent
{
    TorrentStatus::TorrentStatus(TorrentPieces& pieces)
        : currentState{currentStatus::stopped}, ptr_piecesData(&pieces),
          isSeeding{false}, isVerifying{false},
          pieceCount{0}, pieceSize{0}, blockSize{0}, totalSize{0},
          uploaded{0}, dataIntervalTotal{0},
          lastReceivedTime{std::chrono::high_resolution_clock::time_point::min()},
          downloadSpeed{0}
	{
	}

    void TorrentStatus::torrentToStatusData()
    {
        pieceCount = ptr_piecesData->pieceCount;
        pieceSize = ptr_piecesData->pieceSize;
        blockSize = ptr_piecesData->blockSize;
        totalSize = ptr_piecesData->totalSize;

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
        return verifiedPiecesCount() / float(pieceCount);
    }

    bool TorrentStatus::isCompleted()
    {
        return verifiedPiecesCount() == pieceCount;
    }

    bool TorrentStatus::isStarted()
    {
        return verifiedPiecesCount() > 0;
    }

    long long TorrentStatus::downloaded()
    {
        auto acquiredBlocksCount =
                std::accumulate(isBlockAcquired.begin(), isBlockAcquired.end(),
                                0, [](int sum, const std::vector<bool>& b)
        {
            return sum + std::count(b.begin(), b.end(), true);
        });

        return acquiredBlocksCount * blockSize;
    }

    long long TorrentStatus::remaining()
	{
        return totalSize - downloaded();
	}
}
