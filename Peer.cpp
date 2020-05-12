#include "Peer.h"

#include <iostream>
#include <boost/algorithm/string/join.hpp>

namespace Bittorrent
{
	Peer::Peer(std::shared_ptr<Torrent> torrent, std::string& localID, 
		boost::asio::io_context& io_context, tcp::resolver::results_type& results)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ torrent->getPtr() }, key{ "" },
		isPieceDownloaded(peerTorrent.get()->piecesData.pieceCount), 
		isDisconnected{}, isHandshakeSent{}, isPositionSent{}, 
		isChokeSent{ true }, isInterestSent{ false }, isHandshakeReceived{}, 
		IsChokeReceived{ true }, IsInterestedReceived{ false },  
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 }, 
		downloaded{ 0 }, socket(io_context), peerResults(results),
		endpoint(), deadline{io_context}, heartbeatTimer{io_context}, 
		sendBuffer(68), recBuffer(68)
	{
		isBlockRequested.resize(peerTorrent.get()->piecesData.pieceCount);
		for (size_t i = 0; i < peerTorrent.get()->piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(peerTorrent->piecesData.setBlockCount(i));
		}

		//begin connecting
		connectToNewPeer(results.begin());

		// Start the deadline actor. The connect and input actors will
		// update the deadline prior to each asynchronous operation.
		deadline.async_wait(boost::bind(&Peer::check_deadline, this));
	}

	//construct peer from accepted connection started by another peer
	Peer::Peer(std::shared_ptr<Torrent> torrent, std::string& localID, 
		boost::asio::io_context& io_context, tcp::socket tcpClient)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ torrent->getPtr() }, key{ "" },
		isPieceDownloaded(peerTorrent.get()->piecesData.pieceCount), 
		isDisconnected{}, isHandshakeSent{}, isPositionSent{}, 
		isChokeSent{ true }, isInterestSent{ false }, isHandshakeReceived{}, 
		IsChokeReceived{ true }, IsInterestedReceived{ false }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 },
		downloaded{ 0 }, socket(std::move(tcpClient)), endpoint(), 
		deadline{ io_context }, heartbeatTimer{ io_context }, sendBuffer(68),
		recBuffer(68)
	{
		isBlockRequested.resize(peerTorrent.get()->piecesData.pieceCount);
		for (size_t i = 0; i < peerTorrent.get()->piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(peerTorrent.get()->
				piecesData.setBlockCount(i));
		}

		//get endpoint from accepted connection
		endpoint = socket.remote_endpoint();
	}

	std::string Peer::piecesDownloaded()
	{
		//create temp vec to store string bool, then return all as single string
		std::vector<std::string> tempVec(isPieceDownloaded.size());
		for (auto i : isPieceDownloaded)
		{
			if (i == 1)
			{
				tempVec.at(i) = "1";
			}
			else
			{
				tempVec.at(i) = "0";
			}
		}
		return boost::algorithm::join(tempVec, "");
	}

	//count how many pieces that aren't verified (i.e. are required) 
	//are downloaded
	int Peer::piecesRequiredAvailable()
	{
		//custom comparator (!false verified && true downloaded)
		const auto lambda = [this](bool i) {return i &&
			!peerTorrent.get()->statusData.isPieceVerified.at(i); };

		return std::count_if(isPieceDownloaded.cbegin(),
			isPieceDownloaded.cend(), lambda);
	}

	int Peer::piecesDownloadedCount()
	{
		return std::count(isPieceDownloaded.cbegin(), 
			isPieceDownloaded.cend(), true);
	}

	bool Peer::isCompleted()
	{
		return piecesDownloadedCount() == peerTorrent.get()->piecesData.pieceCount;
	}

	//count how many total blocks are requested
	int Peer::blocksRequested()
	{
		int sum = 0;
		for (size_t i = 0; i < isBlockRequested.size(); ++i)
		{
			sum += std::count(isBlockRequested.at(i).cbegin(), 
				isBlockRequested.at(i).cend(), true);
		}

		return sum;
	}

	void Peer::connectToNewPeer(tcp::resolver::results_type::iterator endpointItr)
	{
		if (endpointItr != peerResults.end())
		{
			std::cout << "Trying to connect to: " << endpointItr->endpoint() 
				<< "...\n";

			// Set a deadline for the connect operation (10s)
			deadline.expires_after(boost::asio::chrono::seconds(10));

			// Start the asynchronous connect operation.
			socket.async_connect(endpointItr->endpoint(),
				boost::bind(&Peer::handleNewConnect, this,
					boost::placeholders::_1, endpointItr));
		}
		else
		{
			//No more endpoints to try. Shut down tcp client.
			std::cout << "Peer connection attempt failed! " << 
				"No more endpoints to try connecting to." << "\n";
			disconnect();
		}
	}

	void Peer::handleNewConnect(const boost::system::error_code& ec,
		tcp::resolver::results_type::iterator endpointItr)
	{
		if (isDisconnected)
		{
			return;
		}

		//async_connect() automatically opens the socket
		//Check if socket is closed, if it is, the timeout handler must have run
		if (!socket.is_open())
		{
			std::cout << "Connect timed out" << "\n";

			// Try the next available endpoint.
			connectToNewPeer(++endpointItr);
		}
		// Check if connect operation failed before the deadline expired.
		else if (ec)
		{
			std::cout << "Connect error: " << ec.message() << "\n";

			// We need to close the socket used in the previous connection attempt
			// before starting a new one.
			socket.close();

			// Try the next available endpoint.
			connectToNewPeer(++endpointItr);
		}
		// else connection successful
		else
		{
			std::cout << "Connected to " << endpointItr->endpoint() << "\n";

			startNewRead();
			sendHandShake();
		}

	}

	void Peer::startNewRead()
	{
		// Set a deadline for the read operation.
		deadline.expires_after(boost::asio::chrono::seconds(30));

		// start async receive and immediately call the handler func
		boost::asio::async_read(socket,
			boost::asio::buffer(recBuffer),
			boost::bind(&Peer::handleNewRead, this,
				boost::placeholders::_1, boost::placeholders::_2));
	}

	void Peer::handleNewRead(const boost::system::error_code& ec, 
		std::size_t receivedBytes)
	{
		if (isDisconnected)
		{
			return;
		}

		if (!ec)
		{
			//handshake message
			if (!isHandshakeReceived)
			{
				//process handshake
				processBuffer = recBuffer;
				handleMessage();

				//clear and resize buffer to receive new header packet
				recBuffer.clear();
				recBuffer.resize(4);

				startNewRead();
			}
			//if header, copy data, clear recBuffer and wait for entire message
			//use header data to find remaining message length
			else if (receivedBytes == 4)
			{
				processBuffer = recBuffer;
				int messageLength = getMessageLength();

				//clear and resize buffer to receive rest of the message
				recBuffer.clear();
				recBuffer.resize(messageLength);

				startNewRead();
			}
			//rest of the message
			else
			{
				//insert rest of message after stored header
				processBuffer.insert(processBuffer.begin() + 4,
					recBuffer.begin(), recBuffer.end());

				//process complete message
				handleMessage();

				//clear and resize buffer to receive new header packet
				recBuffer.clear();
				recBuffer.resize(4);

				startNewRead();
			}
		}
		else
		{
			std::cout << "Error on receive: " << ec.message() << "\n";

			disconnect();
		}
	}

	int Peer::getMessageLength()
	{
		//header packet (4 bytes) gives message length
		//convert bytes to int
		int length = 0;
		for (size_t i = 0; i < 4; ++i)
		{
			length <<= 8;
			length |= recBuffer.at(i);
		}
		return length;
	}

	void Peer::sendNewBytes()
	{
		if (isDisconnected)
		{
			return;
		}

		// start async send and immediately call the handler func
		boost::asio::async_write(socket,
			boost::asio::buffer(sendBuffer),
			boost::bind(&Peer::handleNewSend, this,
				boost::placeholders::_1, boost::placeholders::_2));
	}

	void Peer::handleNewSend(const boost::system::error_code& ec, 
		std::size_t receivedBytes)
	{
		if (isDisconnected)
		{
			return;
		}

		if (!ec)
		{
			std::cout << "Sent " << receivedBytes << " bytes"<< "\n";
			sendBuffer.clear();
		}
		else
		{
			std::cout << "Error on send: " << ec.message() << "\n";

			disconnect();
		}

	}

	void Peer::sendHandShake()
	{
		if (isHandshakeSent)
		{
			return;
		}

		//add error to end 
		std::cout << "Sending handshake..." << "...\n";

		//sendhandshake code here

		isHandshakeSent = true;
	}

	void Peer::disconnect()
	{
		std::cout << "\n" << "Disconnecting..." << "\n";

		isDisconnected = true;
		boost::system::error_code ignored_ec;
		socket.close(ignored_ec);
		deadline.cancel();
		heartbeatTimer.cancel();

		std::cout << "\n" << "Disconnected from peer, downloaded: " << downloaded
			<< ", uploaded: " << uploaded << "\n";

		//call slot
		disconnected();
	}

	void Peer::handleMessage()
	{

	}

	bool Peer::decodeHandshake(std::vector<byte>& hash, std::string& id)
	{
		hash.resize(20);
		id = "";

		if (processBuffer.size() != 68 || processBuffer.at(0) != 19)
		{
			std::cout << "Invalid handshake! Must be 68 bytes long and the " <<
				"byte must equal 19." << "\n";
			return false;
		}

		//get first 19 byte string (UTF-8) after length byte
		std::string protocolStr(processBuffer.begin() + 1,
			processBuffer.begin() + 20);

		if (protocolStr != "Bittorrent protocol")
		{
			std::cout << "Invalid handshake! Protocol must equal " <<
				"\"Bittorrent protocol\"." << "\n";
			return false;
		}

		//byte 21-28 represent flags (all 0) - can be ignored
		hash = { processBuffer.begin() + 28, processBuffer.begin() + 48 };
		id = { processBuffer.begin() + 48, processBuffer.end()};

		return true;
	}

	bool Peer::encodeHandshake(std::vector<byte>& hash, std::string& id)
	{
		std::vector<byte> message(68);
		std::string protocolStr = "Bittorrent protocol";

		message.at(0) = 19;
		auto last = std::copy(protocolStr.begin(), protocolStr.end(), 
			message.begin()+1);
		//bytes 21-28 are already 0 due to initialisation, can skip to byte 29
		last = std::copy(peerTorrent->hashesData.infoHash.begin(), 
			peerTorrent->hashesData.infoHash.end(), message.begin() + 28);
		last = std::copy(localID.begin(), localID.end(), last);
	}

	void Peer::check_deadline()
	{
		if (isDisconnected)
		{
			return;
		}

		// Check whether the deadline has passed. We compare the deadline against
		// the current time since a new asynchronous operation may have moved the
		// deadline before this actor had a chance to run.
		if (deadline.expiry() <= boost::asio::steady_timer::clock_type::now())
		{
			// The deadline has passed. The socket is closed so that any outstanding
			// asynchronous operations are cancelled.
			socket.close();

			// There is no longer an active deadline. The expiry is set to the
			// maximum time point so that the actor takes no action until a new
			// deadline is set.
			deadline.expires_at(boost::asio::steady_timer::time_point::max());
		}

		// Put the actor back to sleep.
		deadline.async_wait(boost::bind(&Peer::check_deadline, this));
	}
}
