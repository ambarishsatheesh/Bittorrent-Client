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
		std::string verifiedPiecesString;
		int verifiedPiecesCount;
		double verifiedRatio;
		bool isStarted;
		bool isCompleted;
		//not sure about these
		long long uploaded;
		long long downloaded;
		long long remaining;

		//constructor
		TorrentStatus(const TorrentPieces& pieces, const valueDictionary& torrent);
		//temp
		TorrentStatus(const TorrentPieces& pieces);
		//no default constructor - requires parameter
		TorrentStatus() = delete;

	private:

		long setRemaining(const TorrentPieces& pieces);
	};

}