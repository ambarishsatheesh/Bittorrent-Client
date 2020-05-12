#pragma once
#pragma once

#include "Peer.h"
#include "Torrent.h"
#include "ValueTypes.h"

#include <boost/asio.hpp>

namespace Bittorrent
{
	class Client
	{
	public:
		std::shared_ptr<Torrent> torrent;
		std::string localID;
		
		void handlePeerListUpdated(std::vector<peer> peerList);

		Client();

	};
}


