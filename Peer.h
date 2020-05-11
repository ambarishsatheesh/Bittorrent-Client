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

		std::shared_ptr<Torrent> peerTorrent;

		std::string key;

		//piece info
		std::vector<bool> isPieceDownloaded;

		//status info
		bool isDisconnected;
		bool isPositionSent;
		bool isChokeSent;
		bool isInterestSent;
		bool isHandshakeReceived;
		bool IsChokeReceived;
		bool IsInterestedReceived;
		std::vector<std::vector<bool>> isBlockRequested;
		

		boost::posix_time::ptime lastActive;
		boost::posix_time::ptime lastKeepAlive;

		long long uploaded;
		long long downloaded;

		//tcp data
		boost::asio::io_context io_context;
		tcp::socket socket;
		tcp::endpoint endpoint;

		std::string piecesDownloaded();
		int piecesRequiredAvailable();
		int piecesDownloadedCount();
		bool isCompleted();
		int blocksRequested();

		//delete default constructor
		Peer() = delete;
		//client opened connection constructors
		Peer(Torrent& torrent, std::string& localID, tcp::endpoint& endpoint);
		Peer(Torrent& torrent, std::string& localID);
		//peer-opened connection constructor
		Peer(Torrent& torrent, std::string& localID, tcp::socket* tcpClient);


	};
}


