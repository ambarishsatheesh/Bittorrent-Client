#include "TorrentStatus.h"

TorrentStatus::TorrentStatus(const TorrentPieces& pieces, const valueDictionary& torrent)
	: remaining{ getRemaining(pieces) }
{
}

long TorrentStatus::getRemaining(const TorrentPieces& pieces)
{
	return pieces.totalSize - downloaded;
}