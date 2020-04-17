#include "TorrentStatus.h"

TorrentStatus::TorrentStatus(const TorrentPieces& pieces, const valueDictionary& torrent)
	: remaining{ setRemaining(pieces) }
{
}

TorrentStatus::TorrentStatus(const TorrentPieces& pieces)
{

}

long TorrentStatus::setRemaining(const TorrentPieces& pieces)
{
	return pieces.totalSize - downloaded;
}