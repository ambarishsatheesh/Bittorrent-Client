#pragma once

#include "Peer.h"
#include "Torrent.h"
#include "ValueTypes.h"
#include "WorkingTorrents.h"

#include <boost/asio.hpp>

namespace Bittorrent
{
	class Client
	{
	public:
		//port to use for listening on tcp
		int port;
        WorkingTorrents WorkingTorrents;
		std::vector<byte> localID;

		//slots
		void handleBlockRequested();
		void handleBlockCancelled();
		void handleBlockReceived();
		void handlePeerDisconnected();
		void handlePeerStateChanged();

		Client();

	};
}


