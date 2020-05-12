#pragma once

#include <string>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/asio/steady_timer.hpp>

#include "Torrent.h"
#include "ValueTypes.h"

namespace Bittorrent
{
	using boost::asio::ip::tcp;

	class Peer
		: public std::enable_shared_from_this<Peer>
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

		//status functions
		std::string piecesDownloaded();
		int piecesRequiredAvailable();
		int piecesDownloadedCount();
		bool isCompleted();
		int blocksRequested();

		//established connection methods
		void connectToCreatedPeer();

		//new connection methods
		void startNewConnect(tcp::resolver::results_type::iterator endpointItr);
		void check_deadline();


		//delete default constructor
		Peer() = delete;
		//client-opened connection constructors
		Peer(std::shared_ptr<Torrent> torrent, std::string& localID, 
			boost::asio::io_context io_context,
			tcp::resolver::results_type results);
		//peer-opened connection constructor
		//need io_context here to initialise timers
		Peer(std::shared_ptr<Torrent> torrent, std::string& localID, 
			boost::asio::io_context io_context, tcp::socket tcpClient);

	private:
		//tcp data
		tcp::socket socket;
		tcp::endpoint endpoint;
		boost::asio::steady_timer deadline;
		boost::asio::steady_timer heartbeatTimer;
	};
}


