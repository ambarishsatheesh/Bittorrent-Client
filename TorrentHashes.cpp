#include "TorrentHashes.h"
#include "sha1.h"
#include "encodeVisitor.h"
#include "Utility.h"

#include <iostream>

namespace Bittorrent
{
	using namespace utility;

	TorrentHashes::TorrentHashes()
		: infoHash(20), hexStringInfoHash{ "" }, urlEncodedInfoHash{ "" }
	{
	}

	void TorrentHashes::torrentToHashesData(const value& torrentInfo)
	{
		//encode torrent (info section only)
		auto encodedInfo = boost::apply_visitor(encodeVisitor(), torrentInfo);

		const byte* dataArr = reinterpret_cast<byte*>(&encodedInfo.at(0));

		//calculate hash
		SHA1 sha1;
		infoHash = sha1(dataArr, encodedInfo.size());

		//convert infohash to hex string
		//const byte* infoHashArr = &infoHash.at(0);
		hexStringInfoHash = sha1.toHexHash(dataArr, infoHash.size());

		//URL encode infohash
		std::string infoHashString(infoHash.begin(), infoHash.end());
		urlEncodedInfoHash = urlEncode(infoHashString);
	}
}