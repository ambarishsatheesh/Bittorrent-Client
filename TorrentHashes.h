#pragma once
#include "ValueTypes.h"


namespace Bittorrent
{
	class TorrentHashes
	{
	public:
		//sha1 hash of torrent info
		//need int8_t for udp buffer later
		std::vector<int8_t> infoHash;
		std::string hexStringInfoHash;
		std::string urlEncodedInfoHash;

		//fill hashes info
		void torrentToHashesData(const value& torrentInfo);

		//constructor
		TorrentHashes();

	private:

	};
}