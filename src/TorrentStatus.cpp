#include "TorrentStatus.h"

namespace Bittorrent
{
    TorrentStatus::TorrentStatus(std::shared_ptr<TorrentPieces> pieces, const valueDictionary& torrent)
        : ptr_piecesData(pieces)
	{
	}

    TorrentStatus::TorrentStatus(std::shared_ptr<TorrentPieces> pieces)
        : ptr_piecesData(pieces)
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

    long long TorrentStatus::uploaded()
    {
        //need to implement properly
        return 0;
    }


    long long TorrentStatus::remaining()
	{
        return ptr_piecesData->totalSize - downloaded();
	}
}
