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

		enum messageType
		{
			unknown = -3,
			handshake = -2,
			keepAlive = -1,
			choke = 0,
			unchoke = 1,
			interested = 2,
			notInterested = 3,
			have = 4,
			bitfield = 5,
			request = 6,
			piece = 7,
			cancel = 8,
			port = 9,
		};

		std::string localID;
		std::string peerID;

		std::shared_ptr<Torrent> peerTorrent;

		std::string key;

		//piece info
		std::vector<bool> isPieceDownloaded;

		//status info
		bool isDisconnected;
		bool isHandshakeSent;
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

		//delete default constructor
		Peer() = delete;
		//client-opened connection constructors
		Peer(std::shared_ptr<Torrent> torrent, std::string& localID, 
			boost::asio::io_context& io_context,
			tcp::resolver::results_type& results);
		//peer-opened connection constructor
		//need io_context here to initialise timers
		Peer(std::shared_ptr<Torrent> torrent, std::string& localID, 
			boost::asio::io_context& io_context, tcp::socket tcpClient);

	private:
		//tcp data
		tcp::socket socket;
		tcp::resolver::results_type peerResults;
		tcp::endpoint endpoint;
		boost::asio::steady_timer deadline;
		boost::asio::steady_timer heartbeatTimer;

		//initialised to max block size (16384bytes)
		//since message size is generally unknown
		std::vector<byte> sendBuffer;
		std::vector<byte> processBuffer;
		std::vector<byte> recBuffer;


		//status functions
		std::string piecesDownloaded();
		int piecesRequiredAvailable();
		int piecesDownloadedCount();
		bool isCompleted();
		int blocksRequested();

		//established connection functions - maybe separate class?
		void connectToCreatedPeer();

		//new connection methods
		void connectToNewPeer(tcp::resolver::results_type::iterator endpointItr);
		void handleNewConnect(const boost::system::error_code& ec,
			tcp::resolver::results_type::iterator endpointItr);
		void sendHandShake();
		void startNewRead();
		void handleNewRead(const boost::system::error_code& ec,
			std::size_t receivedBytes);
		int getMessageLength();
		void sendNewBytes();
		void handleNewSend(const boost::system::error_code& ec,
			std::size_t receivedBytes);
		void check_deadline();
		void disconnect();

		//general processing
		void handleMessage();

		//decoding
		bool decodeHandshake(std::vector<byte>& hash, std::string& id);
		bool decodeKeepAlive();
		bool decodeChoke();
		bool decodeUnchoke();
		bool decodeInterested();
		bool decodeNotInterested();
		bool decodeState(messageType type);
		bool decodeHave(int& index);
		bool decodeBitfield(int& pieces, 
			std::vector<bool>& recIsPieceDownloaded);

		//encoding
		std::vector<byte> encodeHandshake(std::vector<byte>& hash,
			std::string& id);
		std::vector<byte> encodeKeepAlive();
		std::vector<byte> encodeChoke();
		std::vector<byte> encodeUnchoke();
		std::vector<byte> encodeInterested();
		std::vector<byte> encodeNotInterested();
		std::vector<byte> encodeState(messageType type);
		std::vector<byte> encodeHave(int& index);
		std::vector<byte> encodeBitfield(
			std::vector<bool>& recIsPieceDownloaded);
	};
}


