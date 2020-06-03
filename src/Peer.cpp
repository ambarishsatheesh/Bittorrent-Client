#include "Peer.h"
#include "sha1.h"
#include "Utility.h"

#include <iostream>
#include <boost/algorithm/string/join.hpp>
#include <boost/dynamic_bitset.hpp>

namespace Bittorrent
{
	using namespace utility;

	Peer::Peer(std::shared_ptr<Torrent> torrent, std::vector<byte>& localID,
		boost::asio::io_context& io_context)
		: localID{ localID }, peerID{ "" },
		torrent{ torrent->getPtr() }, endpointKey(),
		isPieceDownloaded(torrent->piecesData.pieceCount),
		isDisconnected{}, isHandshakeSent{}, isPositionSent{},
		isChokeSent{ true }, isInterestSent{ false }, isHandshakeReceived{},
		IsChokeReceived{ true }, IsInterestedReceived{ false },
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 },
		downloaded{ 0 }, context(io_context), socket(context), recBuffer(68)
	{
		isBlockRequested.resize(torrent->piecesData.pieceCount);
		for (size_t i = 0; i < torrent->piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(
				torrent->piecesData.setBlockCount(i));
		}

		//add message types to map
		messageType.insert({ "unknown", -3 });
		messageType.insert({ "handshake", -2 });
		messageType.insert({ "keepAlive", -1 });
		messageType.insert({ "choke", 0 });
		messageType.insert({ "unchoke", 0 });
		messageType.insert({ "interested", 2 });
		messageType.insert({ "notInterested", 3 });
		messageType.insert({ "have", 4 });
		messageType.insert({ "bitfield", 5 });
		messageType.insert({ "request", 6 });
		messageType.insert({ "piece", 7 });
		messageType.insert({ "cancel", 8 });
		messageType.insert({ "port", 9 });
	}

	//construct peer from accepted connection started by another peer
	Peer::Peer(std::shared_ptr<Torrent> torrent, std::vector<byte>& localID,
		boost::asio::io_context& io_context, tcp::socket tcpClient)
		: localID{ localID }, peerID{ "" }, 
		torrent{ torrent->getPtr() }, endpointKey(),
		isPieceDownloaded(torrent->piecesData.pieceCount), 
		isDisconnected{}, isHandshakeSent{}, isPositionSent{}, 
		isChokeSent{ true }, isInterestSent{ false }, isHandshakeReceived{}, 
		IsChokeReceived{ true }, IsInterestedReceived{ false }, 
		lastActive{ boost::posix_time::second_clock::local_time() },
		lastKeepAlive{ boost::posix_time::min_date_time }, uploaded{ 0 },
		downloaded{ 0 }, context(io_context), socket(std::move(tcpClient)), recBuffer(68)
	{
		isBlockRequested.resize(torrent->piecesData.pieceCount);
		for (size_t i = 0; i < torrent->piecesData.pieceCount; ++i)
		{
			isBlockRequested.at(i).resize(
				torrent->piecesData.setBlockCount(i));
		}

		//add message types to map
		messageType.insert({ "unknown", -3 });
		messageType.insert({ "handshake", -2 });
		messageType.insert({ "keepAlive", -1 });
		messageType.insert({ "choke", 0 });
		messageType.insert({ "unchoke", 0 });
		messageType.insert({ "interested", 2 });
		messageType.insert({ "notInterested", 3 });
		messageType.insert({ "have", 4 });
		messageType.insert({ "bitfield", 5 });
		messageType.insert({ "request", 6 });
		messageType.insert({ "piece", 7 });
		messageType.insert({ "cancel", 8 });
		messageType.insert({ "port", 9 });

		//get endpoint from accepted connection
		endpointKey = socket.remote_endpoint();
		isAccepted = true;
		readFromCreatedPeer();
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
	//are available from Peer
	int Peer::piecesRequiredAvailable()
	{
		//custom comparator (!false verified && true downloaded)
		const auto lambda = [this](bool i) {return i &&
			!torrent->statusData.isPieceVerified.at(i); };

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
		return piecesDownloadedCount() == torrent->piecesData.pieceCount;
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

	void Peer::startNew(const std::string& host, const std::string& port)
	{
		auto presolver = std::make_shared<tcp::resolver>(context);

		auto self(shared_from_this());
		presolver->async_resolve(tcp::resolver::query(host, port),
			[self, presolver](auto&& ec, auto iter)
			{
			self->connectToNewPeer(ec, presolver, iter);
			});
	}

	void Peer::readFromCreatedPeer()
	{
		//increase the shared_ptr reference count so when original shared_ptr
		//in Client is destroyed, this Peer object is not destroyed until
		//lambda function is called and its arguments are destructed
		auto self(shared_from_this());
		socket.async_read_some(boost::asio::buffer(recBuffer),
			[self](boost::system::error_code ec, std::size_t receivedBytes)
			{
				if (!ec)
				{
					self->handleRead(ec, receivedBytes);
				}
				else
				{
					std::cout << "Error on receive: " << ec.message() << "\n";

					self->disconnect();
				}
			});
	}

	void Peer::acc_sendNewBytes(std::vector<byte> sendBuffer)
	{
		if (isDisconnected)
		{
			return;
		}

		auto self(shared_from_this());
		boost::asio::async_write(socket, boost::asio::buffer(sendBuffer),
			[self](boost::system::error_code ec, std::size_t sentBytes)
			{
				if (!ec)
				{
					std::cout << "Sent " << sentBytes << " bytes" << "\n";
				}
				else
				{
					std::cout << "Error on send: " << ec.message() << "\n";

					self->disconnect();
				}
			});
	}

	void Peer::connectToNewPeer(boost::system::error_code const& ec, 
		std::shared_ptr<tcp::resolver> presolver, tcp::resolver::iterator iter)
	{
		if (ec) {
			std::cerr << "error resolving: " << ec.message() << std::endl;
		}
		else {
			auto self(shared_from_this());
			boost::asio::async_connect(socket, iter, 
				[self, presolver]
				(auto&& ec, auto iter)
				{
				self->handleNewConnect(ec, iter);
				//dropping presolver here - we don't need it any more
				});
		}
	}

	void Peer::handleNewConnect(const boost::system::error_code& ec,
		tcp::resolver::results_type::iterator endpointItr)
	{
		if (ec)
		{
			std::cout << "Connect error: " << ec.message() << "\n";
		}
		// else connection successful
		else
		{
			std::cout << "Connected to " << endpointItr->endpoint() << "\n";

			endpointKey = endpointItr->endpoint();

			startNewRead();
			sendHandShake();
		}

	}

	void Peer::startNewRead()
	{
		auto self(shared_from_this());
		boost::asio::async_read(socket, boost::asio::buffer(recBuffer),
			[self](auto&& ec, auto size)
			{
				self->handleRead(ec, size);
			});
	}

	void Peer::handleRead(const boost::system::error_code& ec, 
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

				if (isAccepted)
				{
					readFromCreatedPeer();
				}
				else
				{	
					startNewRead();
				}	
			}
			//if header, copy data, clear recBuffer and wait for entire message
			//use header data to find remaining message length
			else if (receivedBytes == 4)
			{
				processBuffer = recBuffer;
				int messageLength = getMessageLength();

				//4 byte header packet with value 0 is a keep alive message
				if (messageLength == 0)
				{
					//clear and resize buffer to receive new header packet
					recBuffer.clear();
					recBuffer.resize(4);

					if (isAccepted)
					{
						readFromCreatedPeer();
					}
					else
					{
						startNewRead();
					}

					//process keep alive message
					handleMessage();
				}
				//clear and resize buffer to receive rest of the message
				else
				{
					recBuffer.clear();
					recBuffer.resize(messageLength);

					if (isAccepted)
					{
						readFromCreatedPeer();
					}
					else
					{
						startNewRead();
					}
				}
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

				if (isAccepted)
				{
					readFromCreatedPeer();
				}
				else
				{
					startNewRead();
				}
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

	void Peer::sendNewBytes(std::vector<byte> sendBuffer)
	{
		if (isDisconnected)
		{
			return;
		}

		//capture the payload so it continues to exist during async send
		auto payload = std::make_shared<std::vector<byte>>(sendBuffer);

		// start async send and immediately call the handler func
		auto self = shared_from_this();
		boost::asio::async_write(socket, boost::asio::buffer(*payload),
			[self,payload]
				(auto&& ec, auto size)
			{
				self->handleNewSend(ec, size);
			});
	}

	void Peer::handleNewSend(const boost::system::error_code& ec, 
		std::size_t sentBytes)
	{
		if (isDisconnected)
		{
			return;
		}

		if (!ec)
		{
			std::cout << "Sent " << sentBytes << " bytes"<< "\n";
		}
		else
		{
			std::cout << "Error on send: " << ec.message() << "\n";

			disconnect();
		}
	}

	void Peer::disconnect()
	{
		std::cout << "\n" << "Disconnecting..." << "\n";

		isDisconnected = true;
		boost::system::error_code ignored_ec;
		socket.close(ignored_ec);

		std::cout << "\n" << "Disconnected from peer, downloaded: " << downloaded
			<< ", uploaded: " << uploaded << "\n";

		//call slot
		disconnected(*this);
	}

	void Peer::handleMessage()
	{
		//update clock
		lastActive == boost::posix_time::second_clock::local_time();

		int deducedtype = getMessageType(processBuffer);

		if (deducedtype == messageType.left.at("unknown"))
		{
			return;
		}
		else if (deducedtype == messageType.left.at("handshake"))
		{
			std::vector<byte> hash;
			std::string id;
			if (decodeHandshake(hash, id))
			{
				handleHandshake(hash, id);
				return;
			}
		}
		else if (deducedtype == messageType.left.at("keepAlive") 
			&& decodeKeepAlive())
		{
			handleKeepAlive();
			return;
		}
		else if (deducedtype == messageType.left.at("choke")
			&& decodeChoke())
		{
			handleChoke();
			return;
		}
		else if (deducedtype == messageType.left.at("unchoke")
			&& decodeUnchoke())
		{
			handleUnchoke();
			return;
		}
		else if (deducedtype == messageType.left.at("interested")
			&& decodeInterested())
		{
			handleInterested();
			return;
		}
		else if (deducedtype == messageType.left.at("notInterested")
			&& decodeNotInterested())
		{
			handleNotInterested();
			return;
		}
		else if (deducedtype == messageType.left.at("have"))
		{
			int index;
			if (decodeHave(index))
			{
				handleHave(index);
				return;
			}
		}
		else if (deducedtype == messageType.left.at("bitfield"))
		{
			std::vector<bool> recIsPieceDownloaded;
			if (decodeBitfield(isPieceDownloaded.size(), recIsPieceDownloaded))
			{
				handleBitfield(recIsPieceDownloaded);
				return;
			}
		}
		else if (deducedtype == messageType.left.at("request"))
		{
			int index;
			int offset;
			int dataSize;
			if (decodeDataRequest(index, offset, dataSize))
			{
				handleDataRequest(index, offset, dataSize);
				return;
			}
		}
		else if (deducedtype == messageType.left.at("cancel"))
		{
			int index;
			int offset;
			int dataSize;
			if (decodeCancel(index, offset, dataSize))
			{
				handleCancel(index, offset, dataSize);
				return;
			}
		}
		else if (deducedtype == messageType.left.at("piece"))
		{
			int index;
			int offset;
			std::vector<byte> data;
			if (decodePiece(index, offset, data))
			{
				handlePiece(index, offset, data);
				return;
			}
		}
		//this is the listen port for peer's DHT node
		else if (deducedtype == messageType.left.at("port"))
		{
			return;
		}

		//get hex representation of data
		std::cout << "Received an unhandled message: " << toHex(processBuffer) 
			<< "\n";

		//if unhandled message, disconnect from peer
		disconnect();
	}

	bool Peer::decodeHandshake(std::vector<byte>& hash, std::string& id)
	{
		hash.resize(20);
		id = "";

		if (processBuffer.size() != 68 || processBuffer.at(0) != 19)
		{
			std::cout << "Invalid Handshake! Must be 68 bytes long and the " <<
				"byte must equal 19." << "\n";
			return false;
		}

		//get first 19 byte string (UTF-8) after length byte
		std::string protocolStr(processBuffer.begin() + 1,
			processBuffer.begin() + 20);

		if (protocolStr != "Bittorrent protocol")
		{
			std::cout << "Invalid Handshake! Protocol must equal " <<
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
			std::cout << "Invalid Keep Alive message!" << "\n";
			return false;
		}
		return true;
	}

	bool Peer::decodeChoke()
	{
		return decodeState(messageType.left.at("choke"));
	}

	bool Peer::decodeUnchoke()
	{
		return decodeState(messageType.left.at("unchoke"));
	}

	bool Peer::decodeInterested()
	{
		return decodeState(messageType.left.at("interested"));
	}

	bool Peer::decodeNotInterested()
	{
		return decodeState(messageType.left.at("notInterested"));
	}

	//all state messages consist of 4 byte header with value 1, 
	//and then a 1 byte type value
	bool Peer::decodeState(int typeVal)
	{
		//convert bytes to int
		int lengthVal = 0;

		for (size_t i = 0; i < 4; ++i)
		{
			lengthVal <<= 8;
			lengthVal |= processBuffer.at(i);
		}

		if (processBuffer.size() != 5 || lengthVal != 1 ||
			processBuffer.at(4) != static_cast<byte>(typeVal))
		{
			std::cout << "Invalid state of type " << 
				messageType.right.at(typeVal) << "\n";
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
			std::cout << "Invalid Have message! First four bytes must equal "
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

	bool Peer::decodeBitfield(int pieces, 
		std::vector<bool>& recIsPieceDownloaded)
	{
		recIsPieceDownloaded.resize(pieces);

		//get length of data after header packet
		//if number of pieces is not divisible by 8, the end of the bitfield is
		//padded with 0s
		const int expectedLength = std::ceil(pieces / 8.0) + 1;

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
			std::cout << "Invalid Bitfield! Expected length is " << 
				expectedLength << "\n";
			return false;
		}

		//iterate through bitfield and set piece status based on bit value
		size_t k = 0;
		for (size_t i = 5; i < pieces; ++i)
		{
			//create set of bits representing each byte's value
			boost::dynamic_bitset<> bitArray(8, 
				static_cast<unsigned int>(processBuffer.at(i)));

			//set bools based on bit values (every 8 bits, restart iterator at 0)
			for (size_t j = 0; j < bitArray.size(); ++j, ++k)
			{
				recIsPieceDownloaded.at(k) = bitArray[bitArray.size() - j - 1];
			}
		}

		return true;
	}

	bool Peer::decodeDataRequest(int& index, int& offset, int& dataSize)
	{
		//convert first 4 bytes to int
		int lengthVal = 0;

		for (size_t i = 0; i < 4; ++i)
		{
			lengthVal <<= 8;
			lengthVal |= processBuffer.at(i);
		}

		if (processBuffer.size() != 17 || lengthVal != 13)
		{
			std::cout << "Invalid Data Request! Expected total length is "
				<< "17 bytes." << "\n";
			return false;
		}

		//get index val
		index = 0;
		for (size_t i = 5; i < 9; ++i)
		{
			index <<= 8;
			index |= processBuffer.at(i);
		}

		//get byte offset val
		offset = 0;
		for (size_t i = 9; i < 13; ++i)
		{
			offset <<= 8;
			offset |= processBuffer.at(i);
		}

		//get data length (number of bytes to send) val
		dataSize = 0;
		for (size_t i = 13; i < 17; ++i)
		{
			dataSize <<= 8;
			dataSize |= processBuffer.at(i);
		}

		return true;
	}

	bool Peer::decodeCancel(int& index, int& offset, int& dataSize)
	{
		//convert first 4 bytes to int
		int lengthVal = 0;

		for (size_t i = 0; i < 4; ++i)
		{
			lengthVal <<= 8;
			lengthVal |= processBuffer.at(i);
		}

		if (processBuffer.size() != 17 || lengthVal != 13)
		{
			std::cout << "Invalid Cancel message! Expected total length is "
				<< "17 bytes." << "\n";
			return false;
		}

		//get index val
		index = 0;
		for (size_t i = 5; i < 9; ++i)
		{
			index <<= 8;
			index |= processBuffer.at(i);
		}

		//get byte offset val
		offset = 0;
		for (size_t i = 9; i < 13; ++i)
		{
			offset <<= 8;
			offset |= processBuffer.at(i);
		}

		//get data length (number of bytes to send) val
		dataSize = 0;
		for (size_t i = 13; i < 17; ++i)
		{
			dataSize <<= 8;
			dataSize |= processBuffer.at(i);
		}

		return true;
	}

	bool Peer::decodePiece(int& index, int& offset, std::vector<byte>& data)
	{
		if (processBuffer.size() < 13)
		{
			std::cout << "Invalid Piece message! Minimum length is 13 bytes."
				<< "\n";
			return false;
		}

		//get index val
		index = 0;
		for (size_t i = 5; i < 9; ++i)
		{
			index <<= 8;
			index |= processBuffer.at(i);
		}

		//get byte offset val
		offset = 0;
		for (size_t i = 9; i < 13; ++i)
		{
			offset <<= 8;
			offset |= processBuffer.at(i);
		}

		//convert first 4 bytes to int
		int lengthVal = 0;

		for (size_t i = 0; i < 4; ++i)
		{
			lengthVal <<= 8;
			lengthVal |= processBuffer.at(i);
		}

		//ignore bytes before piece data
		lengthVal -= 9;

		//get piece data
		data.resize(lengthVal);
		std::copy(processBuffer.begin() + 13, processBuffer.end(),
			data.begin());

		return true;
	}

	std::vector<byte> Peer::encodeHandshake(std::vector<byte>& hash,
		std::vector<byte>& id)
	{
		std::vector<byte> message(68);
		std::string protocolStr = "Bittorrent protocol";

		message.at(0) = static_cast<byte>(19);

		auto last = std::copy(protocolStr.begin(), protocolStr.end(),
			message.begin() + 1);

		//bytes 21-28 are already 0 due to initialisation, can skip to byte 29
		last = std::copy(torrent->hashesData.infoHash.begin(),
			torrent->hashesData.infoHash.end(), message.begin() + 28);

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
		return encodeState(messageType.left.at("choke"));
	}

	std::vector<byte> Peer::encodeUnchoke()
	{
		return encodeState(messageType.left.at("unchoke"));
	}

	std::vector<byte> Peer::encodeInterested()
	{
		return encodeState(messageType.left.at("interested"));
	}

	std::vector<byte> Peer::encodeNotInterested()
	{
		return encodeState(messageType.left.at("notInterested"));
	}

	std::vector<byte> Peer::encodeState(int typeVal)
	{
		std::vector<byte> newMessage(5, 0);

		//4 byte length
		newMessage.at(3) = static_cast<byte>(1);

		//1 byte type value
		newMessage.at(4) = static_cast<byte>(typeVal);

		return newMessage;
	}

	std::vector<byte> Peer::encodeHave(int index)
	{
		std::vector<byte> newHave(9, 0);

		//first 4 bytes representing length
		newHave.at(3) = static_cast<byte>(5);

		//type byte
		newHave.at(4) = static_cast<byte>(messageType.left.at("have"));

		//last 4 index bytes (big endian)
		newHave.at(5) = (index >> 24) & 0xFF;
		newHave.at(6) = (index >> 16) & 0xFF;
		newHave.at(7) = (index >> 8) & 0xFF;
		newHave.at(8) = index & 0xFF;

		return newHave;
	}

	std::vector<byte> Peer::encodeBitfield(
		std::vector<bool> recIsPieceDownloaded)
	{
		const int numPieces = recIsPieceDownloaded.size();
		const int numBytes = std::ceil(numPieces / 8.0);
		const int numBits = numBytes * 8;
		const int length = numBytes + 1;

		std::vector<byte> newBitfield(length + 4);

		//first 4 length bytes (big endian)
		newBitfield.at(0) = (length >> 24) & 0xFF;
		newBitfield.at(1) = (length >> 16) & 0xFF;
		newBitfield.at(2) = (length >> 8) & 0xFF;
		newBitfield.at(3) = length & 0xFF;

		//type byte
		newBitfield.at(4) = static_cast<byte>(messageType.left.at("bitfield"));

		//create bitset based on bools (+ extra 0s at end if numBits > numPieces)
		boost::dynamic_bitset<> downloadedBitArr(numBits);
		for (size_t i = 0; i < numBits; ++i)
		{
			downloadedBitArr[i] = recIsPieceDownloaded.at(numBits - 1 - i);
		}

		//iterate through bitset and convert binary to decimal every 8 bits
		//store in temporary vector
		std::vector<byte> tempBitfieldVec(numBytes);
		std::string bitStr = "";
		size_t k = 0;
		for (size_t i = 0; i < numBits; i += 8)
		{
			for (size_t j = 0; j < 8; ++j, ++k)
			{
				bitStr += std::to_string(downloadedBitArr[numBits - k - 1]);
			}
			auto numVal = std::stoi(bitStr, nullptr, 2);
			tempBitfieldVec.at(i / 8) = static_cast<byte>(numVal);
			bitStr = "";
		}

		//copy vector to complete bitfield vector
		std::copy(tempBitfieldVec.begin(), tempBitfieldVec.end(),
			newBitfield.begin() + 5);

		return newBitfield;
	}

	std::vector<byte> Peer::encodeDataRequest(int index, int offset, 
		int dataSize)
	{
		std::vector<byte> newDataRequest(17);

		//length byte
		newDataRequest.at(3) = static_cast<byte>(13);

		//type byte
		newDataRequest.at(4) = static_cast<byte>(
			messageType.left.at("request"));

		//index bytes (big endian)
		newDataRequest.at(5) = (index >> 24) & 0xFF;
		newDataRequest.at(6) = (index >> 16) & 0xFF;
		newDataRequest.at(7) = (index >> 8) & 0xFF;
		newDataRequest.at(8) = index & 0xFF;

		//offset bytes (big endian)
		newDataRequest.at(9) = (offset >> 24) & 0xFF;
		newDataRequest.at(10) = (offset >> 16) & 0xFF;
		newDataRequest.at(11) = (offset >> 8) & 0xFF;
		newDataRequest.at(12) = offset & 0xFF;

		//data size bytes (big endian)
		newDataRequest.at(13) = (dataSize >> 24) & 0xFF;
		newDataRequest.at(14) = (dataSize >> 16) & 0xFF;
		newDataRequest.at(15) = (dataSize >> 8) & 0xFF;
		newDataRequest.at(16) = dataSize & 0xFF;

		return newDataRequest;
	}

	std::vector<byte> Peer::encodeCancel(int index, int offset, int dataSize)
	{
		std::vector<byte> newCancelRequest(17);

		//length byte
		newCancelRequest.at(3) = static_cast<byte>(13);

		//type byte
		newCancelRequest.at(4) = static_cast<byte>(
			messageType.left.at("cancel"));

		//index bytes (big endian)
		newCancelRequest.at(5) = (index >> 24) & 0xFF;
		newCancelRequest.at(6) = (index >> 16) & 0xFF;
		newCancelRequest.at(7) = (index >> 8) & 0xFF;
		newCancelRequest.at(8) = index & 0xFF;

		//offset bytes (big endian)
		newCancelRequest.at(9) = (offset >> 24) & 0xFF;
		newCancelRequest.at(10) = (offset >> 16) & 0xFF;
		newCancelRequest.at(11) = (offset >> 8) & 0xFF;
		newCancelRequest.at(12) = offset & 0xFF;

		//data size bytes (big endian)
		newCancelRequest.at(13) = (dataSize >> 24) & 0xFF;
		newCancelRequest.at(14) = (dataSize >> 16) & 0xFF;
		newCancelRequest.at(15) = (dataSize >> 8) & 0xFF;
		newCancelRequest.at(16) = dataSize & 0xFF;

		return newCancelRequest;
	}

	std::vector<byte> Peer::encodePiece(int index, int offset, 
		std::vector<byte> data)
	{
		//set appropriate message size (including index, offset and length bytes)
		const auto dataSize = data.size() + 9;
		std::vector<byte> newPiece(dataSize + 4);

		//first 4 length bytes (big endian)
		newPiece.at(0) = (dataSize >> 24) & 0xFF;
		newPiece.at(1) = (dataSize >> 16) & 0xFF;
		newPiece.at(2) = (dataSize >> 8) & 0xFF;
		newPiece.at(3) = dataSize & 0xFF;

		//type byte
		newPiece.at(4) = static_cast<byte>(messageType.left.at("piece"));

		//index bytes (big endian)
		newPiece.at(5) = (index >> 24) & 0xFF;
		newPiece.at(6) = (index >> 16) & 0xFF;
		newPiece.at(7) = (index >> 8) & 0xFF;
		newPiece.at(8) = index & 0xFF;

		//offset bytes (big endian)
		newPiece.at(9) = (offset >> 24) & 0xFF;
		newPiece.at(10) = (offset >> 16) & 0xFF;
		newPiece.at(11) = (offset >> 8) & 0xFF;
		newPiece.at(12) = offset & 0xFF;

		//copy piece bytes to rest of message
		std::copy(data.begin(), data.end(), newPiece.begin() + 13);

		return newPiece;
	}

	void Peer::sendHandShake()
	{
		if (isHandshakeSent)
		{
			return;
		}

		std::cout << "Sending Handshake message..." << "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes(
				(encodeHandshake(torrent->hashesData.infoHash, localID)));
		}
		else
		{
			sendNewBytes(encodeHandshake(torrent->hashesData.infoHash, localID));
		}

		isHandshakeSent = true;
	}

	void Peer::sendKeepAlive()
	{
		if (lastKeepAlive > boost::posix_time::second_clock::local_time())
		{
			return;
		}

		std::cout << "Sending Keep Alive message..." << "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes((encodeKeepAlive()));
		}
		else
		{
			sendNewBytes(encodeKeepAlive());
		}
		lastKeepAlive = boost::posix_time::second_clock::local_time();
	}

	void Peer::sendChoke()
	{
		if (isChokeSent)
		{
			return;
		}

		std::cout << "Sending Choke message..." << "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes((encodeChoke()));
		}
		else
		{
			sendNewBytes(encodeChoke());
		}

		isChokeSent = true;
	}

	void Peer::sendUnchoke()
	{
		if (!isChokeSent)
		{
			return;
		}

		std::cout << "Sending Unchoke message..." << "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes((encodeUnchoke()));
		}
		else
		{
			sendNewBytes(encodeUnchoke());
		}

		isChokeSent = false;
	}

	void Peer::sendInterested()
	{
		if (isInterestSent)
		{
			return;
		}

		std::cout << "Sending Interested message..." << "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes((encodeInterested()));
		}
		else
		{
			sendNewBytes(encodeInterested());
		}

		isInterestSent = true;
	}

	void Peer::sendNotInterested()
	{
		if (!isInterestSent)
		{
			return;
		}

		std::cout << "Sending Not Interested message..." << "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes((encodeNotInterested()));
		}
		else
		{
			sendNewBytes(encodeNotInterested());
		}

		isInterestSent = false;
	}

	void Peer::sendHave(int index)
	{
		std::cout << "Sending Have message..." << "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes((encodeHave(index)));
		}
		else
		{
			sendNewBytes(encodeHave(index));
		}
	}

	void Peer::sendBitfield(std::vector<bool> isPieceDownloaded)
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
		std::string bitfieldStr = boost::algorithm::join(tempVec, "");

		std::cout << "Sending Bitfield message: " << bitfieldStr << "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes((encodeBitfield(isPieceDownloaded)));
		}
		else
		{
			sendNewBytes(encodeBitfield(isPieceDownloaded));
		}
	}

	void Peer::sendDataRequest(int index, int offset, int dataSize)
	{
		std::cout << "Sending Data request message..." << "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes((encodeDataRequest(index, offset, dataSize)));
		}
		else
		{
			sendNewBytes(encodeDataRequest(index, offset, dataSize));
		}
	}

	void Peer::sendCancel(int index, int offset, int dataSize)
	{
		std::cout << "Sending Cancel message..." << "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes((encodeCancel(index, offset, dataSize)));
		}
		else
		{
			sendNewBytes(encodeCancel(index, offset, dataSize));
		}
	}

	void Peer::sendPiece(int index, int offset, std::vector<byte> data)
	{
		std::cout << "Sending Piece message... " << "index: " << index 
			<< ", offset: " << offset << ", data size: " << data.size() 
			<<  "...\n";

		//create buffer and send
		if (isAccepted)
		{
			acc_sendNewBytes((encodePiece(index, offset, data)));
		}
		else
		{
			sendNewBytes(encodePiece(index, offset, data));
		}

		uploaded += data.size();
	}

	int Peer::getMessageType(std::vector<byte> data)
	{
		if (!isHandshakeReceived)
		{
			return messageType.left.at("handshake");
		}

		//keep alive message is handled by handleRead()

		//check if message type exists in map
		if (data.size() > 4 && messageType.right.find(
			static_cast<int>(data.at(4))) != messageType.right.end())
		{
			return static_cast<int>(data.at(4));
		}

		return messageType.left.at("unknown");
	}

	void Peer::handleHandshake(std::vector<byte> hash, std::string id)
	{
		std::cout << "Handling Handshake message..." << "...\n";

		//can't seem to use == overload to compare vector<byte>
		if (!std::equal(hash.begin(), hash.end(), 
			torrent->hashesData.infoHash.begin()))
		{
			std::cout << "Invalid handshake! Incorrect infohash. Expected " <<
				"\"" << torrent->hashesData.hexStringInfoHash << "\", " <<
				"received " << "\"" << toHex(hash) << "\" " 
				<< "(hex representation)" << "\n";

			disconnect();
			return;
		}

		peerID = id;
		isHandshakeReceived = true;

		//if the peer opened the connection and sent a handshake, we send a 
		//reply handshake. Otherwise, we sent the initial handshake and so we 
		//send the bitfield
		if (isAccepted)
		{
			sendHandShake();
		}
		else
		{
			sendBitfield(torrent->statusData.isPieceVerified);
		}
	}

	void Peer::handleKeepAlive()
	{
		//don't need to do anything since lastActive variable is updated on
		//any message received
		std::cout << "Handling Keep Alive message..." << "...\n";
	}

	void Peer::handleChoke()
	{
		std::cout << "Handling Choke message..." << "...\n";
		IsChokeReceived = true;

		//call slot
		if (!stateChanged.empty())
		{
			stateChanged(*this);
		}
	}

	void Peer::handleUnchoke()
	{
		std::cout << "Handling Unchoke message..." << "...\n";
		IsChokeReceived = false;

		//call slot
		if (!stateChanged.empty())
		{
			stateChanged(*this);
		}
	}

	void Peer::handleInterested()
	{
		std::cout << "Handling Interested message..." << "...\n";
		IsInterestedReceived = true;

		//call slot
		if (!stateChanged.empty())
		{
			stateChanged(*this);
		}
	}

	void Peer::handleNotInterested()
	{
		std::cout << "Handling Not Interested message..." << "...\n";
		IsInterestedReceived = false;

		//call slot
		if (!stateChanged.empty())
		{
			stateChanged(*this);
		}
	}

	void Peer::handleHave(int index)
	{
		isPieceDownloaded.at(index) = true;
		std::cout << "Handling Have message: " << "Peer has index: " << index
			<< ", number of pieces downloaded: " << piecesDownloadedCount() 
			<< ", pieces available: " << piecesDownloaded() << "\n";

		//call slot
		if (!stateChanged.empty())
		{
			stateChanged(*this);
		}
	}

	void Peer::handleBitfield(std::vector<bool> recIsPieceDownloaded)
	{
		//set true if we have either just been told that the peer has the 
		//piece downloaded or already set to true
		for (size_t i = 0; i < torrent->piecesData.pieceCount; ++i)
		{
			isPieceDownloaded.at(i) = isPieceDownloaded.at(i) ||
				recIsPieceDownloaded.at(i);
		}

		std::cout << "Handling Bitfield message: " 
			<< "Number of pieces downloaded: " << piecesDownloadedCount()
			<< ", pieces available: " << piecesDownloaded() << "\n";

		//call slot
		if (!stateChanged.empty())
		{
			stateChanged(*this);
		}
	}

	void Peer::handleDataRequest(int index, int offset, int dataSize)
	{
		std::cout << "Handling data request: " << "index: " << index
			<< ", offset: " << offset << ", data size: " << dataSize << "\n";

		dataRequest newDataRequest = { index, offset, dataSize };

		//pass struct and this peer's data to slot
		if (!blockRequested.empty())
		{
			blockRequested(*this, newDataRequest);
		}
	}

	void Peer::handleCancel(int index, int offset, int dataSize)
	{
		std::cout << "Handling cancel request: " << "index: " << index
			<< ", offset: " << offset << ", data size: " << dataSize << "\n";

		dataRequest newDataRequest = { index, offset, dataSize };

		//pass struct and this peer's data to slot
		if (!blockCancelled.empty())
		{
			blockCancelled(*this, newDataRequest);
		}
	}

	void Peer::handlePiece(int index, int offset, std::vector<byte> data)
	{
		std::cout << "Handling piece request: " << "index: " << index
			<< ", offset: " << offset << ", data size: " << data.size() << "\n";


		dataPackage newPackage = { index, offset/torrent->piecesData.blockSize,
			data};

		//pass struct and this peer's data to slot
		if (!blockReceived.empty())
		{
			blockReceived(*this, newPackage);
		}
	}
}