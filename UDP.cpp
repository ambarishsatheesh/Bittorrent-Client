#include "UDP.h"
#include "Utility.h"

#include <iostream>
#include <iomanip>
#include <random>
#include <limits>


// TODO: implement time out/retransmit
// TODO: needs proper testing of all parts

namespace Bittorrent
{
	using namespace utility;

	UDP::UDP(trackerUrl parsedUrl, long clientID, std::vector<int8_t> infoHash,
		long long uploaded, long long downloaded, long long remaining, int intEvent)
		: connIDReceivedTime{}, ancClientId{ clientID }, ancInfoHash{ infoHash }, 
		ancDownloaded{ downloaded }, ancUploaded{ uploaded }, 
		ancRemaining{ remaining }, ancIntEvent{ intEvent },
		peerHost{ parsedUrl.hostname }, peerPort{ "" },
		peerTarget{""}, io_context(), socket(io_context), receiverEndpoint(), 
		senderEndpoint()
	{
		buildConnectReq();
	}

	UDP::~UDP()
	{
		socket.close();
	}

	boost::asio::mutable_buffers_1 UDP::buildConnectReq()
	{
		//build connect request buffer
		protocolID = { 0x0, 0x0, 0x04, 0x17, 0x27, 0x10, 0x19, 0x80 };
		connectAction = { 0x0, 0x0, 0x0, 0x0 };

		//generate random int32 and pass into transactionID array 
		std::random_device dev;
		std::mt19937 rng(dev());
		const std::uniform_int_distribution<int32_t> 
			dist6(0, std::numeric_limits<int32_t>::max());
		auto rndInt = dist6(rng);

		//Big endian - but endianness doesn't matter since it's random anyway
		sentTransactionID.resize(4);
		for (int i = 0; i <= 3; ++i) {
			sentTransactionID.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
		}

		//copy to buffer
		std::vector<byte> connectVec;
		connectVec.insert(connectVec.begin(), std::begin(protocolID), std::end(protocolID));
		connectVec.insert(connectVec.begin() + 8, std::begin(connectAction), std::end(connectAction));
		connectVec.insert(connectVec.begin() + 12, std::begin(sentTransactionID), std::end(sentTransactionID));

		//convert to appropriate buffer for sending via UDP and return
		return boost::asio::buffer(connectVec, connectVec.size());
	}


	void UDP::handleConnectResp(std::vector<byte>& receivedConnBuffer,
		const std::size_t& connBytesRec)
	{
		if (connBytesRec < 16)
		{
			throw std::invalid_argument("Connect response packet is less than the expected 16 bytes!");
		}

		//validate action
		std::vector<byte> receivedAction = { receivedConnBuffer.begin(), 
			receivedConnBuffer.begin() + 4 };
		if (receivedAction != connectAction)
		{
			throw std::invalid_argument("Response action is not \"connect\"!");
		}

		//store transaction id and validate
		receivedTransactionID = { receivedConnBuffer.begin() + 4,
			receivedConnBuffer.begin() + 8 };
		if (receivedTransactionID != sentTransactionID)
		{
			throw std::invalid_argument("Response transaction ID is not equal to sent transaction ID!");
		}

		//store connection id
		connectionID = { receivedConnBuffer.begin() + 8,
			receivedConnBuffer.end() };
	}

	boost::asio::mutable_buffers_1 UDP::buildAnnounceReq()
	{
		//generate announce action (1)
		ancAction = { 0x0, 0x0, 0x0, 0x01 };

		//generate random int32 and pass into transactionID array 
		std::random_device dev;
		std::mt19937 rng(dev());
		const std::uniform_int_distribution<int32_t>
			dist6(0, std::numeric_limits<int32_t>::max());
		auto rndInt = dist6(rng);

		//Big endian - but endianness doesn't matter since it's random anyway
		sentTransactionID.clear();
		sentTransactionID.resize(4);
		for (int i = 0; i <= 3; ++i) {
			sentTransactionID.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
		}

		//forgo id convention - this client will use 20 bytes of random numbers
		std::vector<int8_t> clientIDVec;
		clientIDVec.resize(20);
		for (int i = 0; i <= 19; ++i) {
			clientIDVec.at(i) = ((ancClientId >> (8 * (19 - i))) & 0xff);
		}

		//downloaded bytes
		std::vector<int8_t> downloadedVec;
		downloadedVec.resize(8);
		for (int i = 0; i <= 7; ++i) {
			downloadedVec.at(i) = ((ancDownloaded >> (8 * (7 - i))) & 0xff);
		}

		//remaining bytes
		std::vector<int8_t> remainingVec;
		remainingVec.resize(8);
		for (int i = 0; i <= 7; ++i) {
			remainingVec.at(i) = ((ancRemaining >> (8 * (7 - i))) & 0xff);
		}

		//uploaded bytes
		std::vector<int8_t> uploadedVec;
		uploadedVec.resize(8);
		for (int i = 0; i <= 7; ++i) {
			uploadedVec.at(i) = ((ancUploaded >> (8 * (7 - i))) & 0xff);
		}

		//event
		std::vector<int8_t> eventVec;
		eventVec.resize(4);
		for (int i = 0; i <= 3; ++i) {
			eventVec.at(i) = ((ancIntEvent >> (8 * (3 - i))) & 0xff);
		}

		//default 0 - optional - can be determined from udp request
		std::vector<int8_t> ipVec = { 0x0, 0x0, 0x0, 0x0 };

		//generate random int32 for key value and store in vec
		//only in case ip changes
		rndInt = dist6(rng);
		std::vector<int8_t> keyVec;
		keyVec.resize(4);
		for (int i = 0; i <= 3; ++i) {
			keyVec.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
		}

		//default -1 - optional - number of peers that the client would like to 
		//receive from the tracker
		std::vector<int8_t> numWantVec;
		numWantVec.resize(4);
		int32_t numWantInt = -1;
		for (int i = 0; i <= 3; ++i) {
			numWantVec.at(i) = ((numWantInt >> (8 * (3 - i))) & 0xff);
		}

		//port
		std::vector<int8_t> portVec;
		portVec.resize(2);
		int32_t intPort = 6681;
		for (int i = 0; i <= 1; ++i) {
			portVec.at(i) = ((intPort >> (8 * (1 - i))) & 0xff);
		}

		//copy to buffer
		std::vector<byte> announceVec;
		announceVec.insert(announceVec.begin(), std::begin(connectionID), std::end(connectionID));
		announceVec.insert(announceVec.begin() + 8, std::begin(ancAction), std::end(ancAction));
		announceVec.insert(announceVec.begin() + 12, std::begin(sentTransactionID), std::end(sentTransactionID));
		announceVec.insert(announceVec.begin() + 16, std::begin(ancInfoHash), std::end(ancInfoHash));
		announceVec.insert(announceVec.begin() + 36, std::begin(clientIDVec), std::end(clientIDVec));
		announceVec.insert(announceVec.begin() + 56, std::begin(downloadedVec), std::end(downloadedVec));
		announceVec.insert(announceVec.begin() + 64, std::begin(remainingVec), std::end(remainingVec));
		announceVec.insert(announceVec.begin() + 72, std::begin(uploadedVec), std::end(uploadedVec));
		announceVec.insert(announceVec.begin() + 80, std::begin(eventVec), std::end(eventVec));
		announceVec.insert(announceVec.begin() + 84, std::begin(ipVec), std::end(ipVec));
		announceVec.insert(announceVec.begin() + 88, std::begin(keyVec), std::end(keyVec));
		announceVec.insert(announceVec.begin() + 92, std::begin(numWantVec), std::end(numWantVec));
		announceVec.insert(announceVec.begin() + 96, std::begin(portVec), std::end(portVec));

		//convert to appropriate buffer for sending via UDP and return
		return boost::asio::buffer(announceVec, announceVec.size());
	}

	void UDP::handleAnnounceResp(std::vector<byte>& receivedAncBuffer,
		const std::size_t& AncBytesRec)
	{

	}

	void UDP::send(const std::string& msg, trackerUrl parsedUrl)
	{
		try
		{
			//if empty use default values
			peerPort = parsedUrl.port.empty() ? "80" : parsedUrl.port;
			peerTarget = parsedUrl.target.empty() ? "/" : parsedUrl.target;

			udp::resolver resolver{ io_context };

			// Look up the domain name
			auto const results = resolver.resolve(peerHost, peerPort);

			//iterate and get successful connection endpoint
			const auto receiverEndpointIter = 
				boost::asio::connect(socket, results.begin(), results.end());
			receiverEndpoint = receiverEndpointIter->endpoint();

			//build connect request buffer
			const auto connectReqBuffer = buildConnectReq();

			//open socket and send connect request
			socket.open(udp::v4());
			socket.send_to(connectReqBuffer, receiverEndpoint);

			//read response into buffer
			std::vector<byte> receivedConnBuffer(16);
			const std::size_t connBytesRec = 
				socket.receive_from(boost::asio::buffer(receivedConnBuffer),
					senderEndpoint);

			//update relevant timekeeping
			connIDReceivedTime = boost::posix_time::second_clock::universal_time();
			lastRequestTime = boost::posix_time::second_clock::universal_time();

			//handle connect response
			handleConnectResp(receivedConnBuffer, connBytesRec);

			//build connect request buffer and send announce request
			const auto announceReqBuffer = buildAnnounceReq();
			socket.send_to(announceReqBuffer, receiverEndpoint);

			//read response into buffer
			//set max peers for this client to 50 - more than enough
			//(6 bytes * 50 = 300) + 20 for other info = 320 bytes max
			std::vector<byte> receivedAncBuffer(320);
			const std::size_t ancBytesRec =
				socket.receive_from(boost::asio::buffer(receivedAncBuffer),
					senderEndpoint);

			//update relevant timekeeping
			lastRequestTime = boost::posix_time::second_clock::universal_time();

			//handle announce response
			handleAnnounceResp(receivedConnBuffer, connBytesRec);

		}
		catch (const boost::system::system_error& e)
		{
			std::cout << "Error occured! Error code = " << e.code()
				<< ". Message: " << e.what();
		}
	}


}
