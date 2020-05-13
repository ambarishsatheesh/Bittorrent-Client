#include "Peer.h"

#include <iostream>
#include <boost/algorithm/string/join.hpp>
#include <boost/dynamic_bitset.hpp>

namespace Bittorrent
{
	Peer::Peer(std::shared_ptr<Torrent> torrent, std::string& localID, 
		boost::asio::io_context& io_context, tcp::resolver::results_type& results)
		: localID{ localID }, peerID{ "" }, 
		peerTorrent{ torrent->getPtr() }, key{ "" },
		isPieceDownloaded(peerTorrent->piecesData.pieceCount), 
		isDisconnected{}, isHandshakeSent{}, isPositionSent{}, 
		isChokeSent{ true }, isInterestSent{ false }, isHandshakeReceived{}, 
		IsChokeReceived{ true }, IsInterestedReceived{ false },  
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 }, 
		downloaded{ 0 }, socket(io_context), peerResults(results),
		endpoint(), deadline{io_context}, heartbeatTimer{io_context}, 
		sendBuffer(68), recBuffer(68)
	{
		isBlockRequested.resize(peerTorrent->piecesData.pieceCount);
		for (size_t i = 0; i < peerTorrent->piecesData.pieceCount; ++i)
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
		isPieceDownloaded(peerTorrent->piecesData.pieceCount), 
		isDisconnected{}, isHandshakeSent{}, isPositionSent{}, 
		isChokeSent{ true }, isInterestSent{ false }, isHandshakeReceived{}, 
		IsChokeReceived{ true }, IsInterestedReceived{ false }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 },
		downloaded{ 0 }, socket(std::move(tcpClient)), endpoint(), 
		deadline{ io_context }, heartbeatTimer{ io_context }, sendBuffer(68),
		recBuffer(68)
	{
		isBlockRequested.resize(peerTorrent->piecesData.pieceCount);
		for (size_t i = 0; i < peerTorrent->piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(peerTorrent->
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
			!peerTorrent->statusData.isPieceVerified.at(i); };

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
		return piecesDownloadedCount() == peerTorrent->piecesData.pieceCount;
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

	bool Peer::decodeKeepAlive()
	{
		//convert bytes to int
		int lengthVal = 0;

		for (size_t i = 0; i < 4; ++i)
		{
			lengthVal <<= 8;
			lengthVal |= processBuffer.at(i);
		}

		if (processBuffer.size() != 4 || lengthVal != 0)
		{
			std::cout << "Invalid keepAlive!" << "\n";
			return false;
		}
		return true;
	}

	bool Peer::decodeChoke()
	{
		return decodeState(messageType::choke);
	}

	bool Peer::decodeUnchoke()
	{
		return decodeState(messageType::unchoke);
	}

	bool Peer::decodeInterested()
	{
		return decodeState(messageType::interested);
	}

	bool Peer::decodeNotInterested()
	{
		return decodeState(messageType::notInterested);
	}

	//all state messages consist of 4 byte header with value 1, 
	//and then a 1 byte type value
	bool Peer::decodeState(messageType type)
	{
		//convert bytes to int
		int lengthVal = 0;

		for (size_t i = 0; i < 4; ++i)
		{
			lengthVal <<= 8;
			lengthVal |= processBuffer.at(i);
		}

		if (processBuffer.size() != 5 || lengthVal != 1 ||
			processBuffer.at(4) != static_cast<byte>(type))
		{
			std::cout << "Invalid state type " << type << "\n";
			return false;
		}

		return true;
	}

	bool Peer::decodeHave(int& index)
	{
		//convert bytes to int
		int lengthVal = 0;

		for (size_t i = 0; i < 4; ++i)
		{
			lengthVal <<= 8;
			lengthVal |= processBuffer.at(i);
		}

		if (processBuffer.size() != 9 || lengthVal != 5)
		{
			std::cout << "Invalid \"have\" message! First four bytes must equal "
				<< "0x05" << "\n";
			return false;
		}

		//get index from last 4 bytes
		index = 0;
		for (size_t i = 5; i < 9; ++i)
		{
			index <<= 8;
			index |= processBuffer.at(i);
		}

		return index;
	}

	bool Peer::decodeBitfield(int& pieces, std::vector<bool>& recIsPieceDownloaded)
	{
		recIsPieceDownloaded.resize(pieces);

		//get length of data after header packet
		//if number of pieces is not divisible by 8, the end of the bitfield is
		//padded with 0s
		int expectedLength = std::ceil(pieces / 8.0) + 1;

		//convert first 4 bytes to int
		int lengthVal = 0;

		for (size_t i = 0; i < 4; ++i)
		{
			lengthVal <<= 8;
			lengthVal |= processBuffer.at(i);
		}

		if (processBuffer.size() != expectedLength + 4 ||
			lengthVal != expectedLength)
		{
			std::cout << "Invalid bitfield! Expected length is " << 
				expectedLength << "\n";
			return false;
		}

		//convert bitfield value to int
		int bitVal = 0;

		for (size_t i = 5; i < processBuffer.size(); ++i)
		{
			bitVal <<= 8;
			bitVal |= processBuffer.at(i);
		}

		//create set of bits representing value
		boost::dynamic_bitset<> bitArray(pieces, bitVal);

		//set values of array according to bitset
		for (size_t i = 0; i < pieces; ++i)
		{
			recIsPieceDownloaded[i] = bitArray[bitArray.size() - 1 - i];
		}


		return true;
	}


	std::vector<byte> Peer::encodeHandshake(std::vector<byte>& hash,
		std::string& id)
	{
		std::vector<byte> message(68);
		std::string protocolStr = "Bittorrent protocol";

		message.at(0) = static_cast<byte>(19);

		auto last = std::copy(protocolStr.begin(), protocolStr.end(),
			message.begin() + 1);

		//bytes 21-28 are already 0 due to initialisation, can skip to byte 29
		last = std::copy(peerTorrent->hashesData.infoHash.begin(),
			peerTorrent->hashesData.infoHash.end(), message.begin() + 28);

		last = std::copy(localID.begin(), localID.end(), last);

		return message;
	}

	std::vector<byte> Peer::encodeKeepAlive()
	{
		std::vector<byte> newKeepAlive(4, 0);
		return newKeepAlive;
	}

	std::vector<byte> Peer::encodeChoke()
	{
		return encodeState(messageType::choke);
	}

	std::vector<byte> Peer::encodeUnchoke()
	{
		return encodeState(messageType::unchoke);
	}

	std::vector<byte> Peer::encodeInterested()
	{
		return encodeState(messageType::interested);
	}

	std::vector<byte> Peer::encodeNotInterested()
	{
		return encodeState(messageType::notInterested);
	}

	std::vector<byte> Peer::encodeState(messageType type)
	{
		std::vector<byte> newMessage(5, 0);

		//4 byte length
		newMessage.at(3) = static_cast<byte>(1);

		//1 byte type value
		newMessage.at(4) = static_cast<byte>(type);

		return newMessage;
	}

	std::vector<byte> Peer::encodeHave(int& index)
	{
		std::vector<byte> newHave(9, 0);

		//first 4 bytes representing length
		newHave.at(3) = static_cast<byte>(5);

		//type byte
		newHave.at(4) = static_cast<byte>(messageType::have);

		//index bytes (big endian)
		newHave.at(0) = (index >> 24) & 0xFF;
		newHave.at(1) = (index >> 16) & 0xFF;
		newHave.at(2) = (index >> 8) & 0xFF;
		newHave.at(3) = index & 0xFF;

		return newHave;
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
