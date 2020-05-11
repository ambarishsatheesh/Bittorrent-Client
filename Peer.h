#pragma once

#include <string>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include "Torrent.h"
#include "ValueTypes.h"

namespace Bittorrent
{
	using boost::asio::ip::tcp;

	class Peer
	{
	public:
		boost::signals2::signal<void()> disconnected;
		boost::signals2::signal<void()> stateChanged;
		boost::signals2::signal<void()> blockRequested;
		boost::signals2::signal<void()> blockCancelled;
		boost::signals2::signal<void()> blockReceived;

		std::string localID;
		std::string peerID;

		Torrent torrent;
		//peer endpoint;
		tcp::endpoint endpoint;



	};
}


