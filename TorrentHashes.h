#pragma once
#include "ValueTypes.h"


namespace Bittorrent
{
	class TorrentHashes
	{
	public:
		//sha1 hash of torrent info
		std::vector<byte> infoHash;
		std::string hexStringInfoHash;
		std::string urlEncodedInfoHash;

		//fill hashes info
		void torrentToHashesData(const value& torrentInfo);

		//constructor
		TorrentHashes();

	private:

	};
}