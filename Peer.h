#pragma once

#include <map>
#include <string>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bimap.hpp>

#include "Torrent.h"
#include "ValueTypes.h"

namespace Bittorrent
{
	using boost::asio::ip::tcp;

	class Peer
		: public std::enable_shared_from_this<Peer>
	{
	public:
		struct dataRequest
		{
			int piece;
			int offset;
			int dataSize;
			bool isCancelled;
		};

		struct dataPackage
		{
			int piece;
			int block;
			std::vector<byte> data;
		};

		boost::signals2::signal<void()> disconnected;
		boost::signals2::signal<void()> stateChanged;
		boost::signals2::signal<void(
			Peer& peer, dataRequest newDataRequest)> blockRequested;
		boost::signals2::signal<void()> blockCancelled;
		boost::signals2::signal<void()> blockReceived;

		boost::bimap<std::string, int> messageType;

		std::string localID;
		std::string peerID;

		std::shared_ptr<Torrent> torrent;

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

		//TCP transmission buffers
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
		void startNewRead();
		void handleNewRead(const boost::system::error_code& ec,
			std::size_t receivedBytes);
		int getMessageLength();
		void sendNewBytes(std::vector<byte> sendBuffer);
		void handleNewSend(const boost::system::error_code& ec,
			std::size_t receivedBytes);
		void check_deadline();
		void disconnect();

		//decoding
		bool decodeHandshake(std::vector<byte>& hash, std::string& id);
		bool decodeKeepAlive();
		bool decodeChoke();
		bool decodeUnchoke();
		bool decodeInterested();
		bool decodeNotInterested();
		bool decodeState(int typeVal);
		bool decodeHave(int& index);
		bool decodeBitfield(int pieces, 
			std::vector<bool>& recIsPieceDownloaded);
		bool decodeDataRequest(int& index, int& offset, int& dataSize);
		bool decodeCancel(int& index, int& offset, int& dataSize);
		bool decodePiece(int& index, int& offset, std::vector<byte>& data);


		//encoding
		std::vector<byte> encodeHandshake(std::vector<byte>& hash,
			std::string& id);
		std::vector<byte> encodeKeepAlive();
		std::vector<byte> encodeChoke();
		std::vector<byte> encodeUnchoke();
		std::vector<byte> encodeInterested();
		std::vector<byte> encodeNotInterested();
		std::vector<byte> encodeState(int typeVal);
		std::vector<byte> encodeHave(int index);
		std::vector<byte> encodeBitfield(
			std::vector<bool> recIsPieceDownloaded);
		std::vector<byte> encodeDataRequest(int index, int offset, 
			int dataSize);
		std::vector<byte> encodeCancel(int index, int offset, int dataSize);
		std::vector<byte> encodePiece(int index, int offset, 
			std::vector<byte> data);

		//sending
		void sendHandShake();
		void sendKeepAlive();
		void sendChoke();
		void sendUnchoke();
		void sendInterested();
		void sendNotInterested();
		void sendHave(int index);
		void sendBitfield(std::vector<bool> isPieceDownloaded);
		void sendDataRequest(int index, int offset, int dataSize);
		void sendCancel(int index, int offset, int dataSize);
		void sendPiece(int index, int offset, std::vector<byte> data);

		//receiving
		void handleMessage();
		int getMessageType(std::vector<byte> data);
		void handleHandshake(std::vector<byte> hash, std::string id);
		void handleKeepAlive();
		void handleChoke();
		void handleUnchoke();
		void handleInterested();
		void handleNotInterested();
		void handleHave(int index);
		void handleBitfield(std::vector<bool> isPieceDownloaded);
		void handleDataRequest(int index, int offset, int dataSize);
		void handleCancel(int index, int offset, int dataSize);
		void handlePiece(int index, int offset, std::vector<byte> data);
		//no DHT port stuff for now
		//void handlePort(int port);
	};
}


