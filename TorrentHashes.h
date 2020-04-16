#pragma once
#include "ValueTypes.h"


using byte = uint8_t;

class TorrentHashes
{
public:
	//sha1 hash of torrent info
	std::vector<byte> infoHash;
	std::string hexStringInfoHash;
	std::string urlEncodedInfoHash;
	std::vector<std::vector<byte>> pieceHashes;

	//constructor
	TorrentHashes(const valueDictionary& torrent);
	//no default constructor - requires parameter
	TorrentHashes() = delete;

private:

};

