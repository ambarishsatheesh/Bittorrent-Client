#pragma once
#include "ValueTypes.h"

class TorrentHashes
{
public:
	//sha1 hash of torrent info
	std::vector<byte> infoHash;
	std::string hexStringInfoHash;
	std::string urlEncodedInfoHash;

	//constructor
	TorrentHashes();

	//fill hashes info
	void torrentToHashesData(const value& torrentInfo);

private:

};

