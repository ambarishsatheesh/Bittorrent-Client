#pragma once

#include "Peer.h"
#include "Torrent.h"
#include "ValueTypes.h"
#include "workingTorrentList.h"

#include <boost/asio.hpp>

namespace Bittorrent
{
	class Client
	{
	public:
		//port to use for listening on tcp
		int port;
        workingTorrentList workingTorrentList;
		std::vector<byte> localID;

		//context and acceptor for accepting peer connections
		boost::asio::io_context acc_io_context;
		tcp::acceptor acceptor;

		//peer connection
		void handlePeerListUpdated(std::vector<peer> peerList);
		void enablePeerConnections();
		void doAccept();


		//slots
		void handleBlockRequested();
		void handleBlockCancelled();
		void handleBlockReceived();
		void handlePeerDisconnected();
		void handlePeerStateChanged();


		Client();

	};
}


