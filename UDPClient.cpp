#include "UDPClient.h"
#include "Utility.h"

#include <boost/bind.hpp>

#include <iostream>
#include <iomanip>
#include <random>
#include <limits>

#define UDP_PORT 6881

// TODO: implement time out/retransmit
// TODO: need to populate "remaining"

namespace Bittorrent
{
	using namespace utility;

	UDPClient::UDPClient(trackerUrl& parsedUrl, std::vector<byte>& clientID, std::vector<byte>& infoHash,
		long long& uploaded, long long& downloaded, long long& remaining, int& intEvent)
		: connIDReceivedTime{}, ancClientID{ clientID }, ancInfoHash{ infoHash },
		ancDownloaded{ downloaded }, ancUploaded{ uploaded }, 
		ancRemaining{ remaining }, ancIntEvent{ intEvent },
		peerHost{ parsedUrl.hostname }, peerPort{ "" },
		peerTarget{ "" }, receivedConnBuffer(16), receivedAncBuffer(320),
		interval{ 1800 }, leechers{ 0 }, seeders{ 0 },
		io_context(), socket(io_context), 
		remoteEndpoint(), localEndpoint()
	{
		dataTransmission(parsedUrl);
	}

	UDPClient::~UDPClient()
	{
		socket.close();
	}

	std::vector<byte> UDPClient::buildConnectReq()
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
		return connectVec;
	}


	void UDPClient::handleConnectResp(std::vector<byte>& receivedConnBuffer,
		const std::size_t& connBytesRec)
	{
		//validate size
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

	std::vector<byte> UDPClient::buildAnnounceReq()
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

		//print for testing
		std::cout << "Transaction ID: ";
		for (size_t i = 0; i < 4; ++i)
		{
			printf("%x ", (unsigned char)sentTransactionID[i]);
		}
		std::cout << "\n";

		std::cout << "Connection ID: ";
		for (size_t i = 0; i < 8; ++i)
		{
			printf("%x ", (unsigned char)connectionID[i]);
		}
		std::cout << "\n";

		//downloaded bytes
		std::vector<byte> downloadedVec;
		downloadedVec.resize(8);
		for (int i = 0; i <= 7; ++i) {
			downloadedVec.at(i) = ((ancDownloaded >> (8 * (7 - i))) & 0xff);
		}

		//remaining bytes
		std::vector<byte> remainingVec;
		remainingVec.resize(8);
		for (int i = 0; i <= 7; ++i) {
			remainingVec.at(i) = ((ancRemaining >> (8 * (7 - i))) & 0xff);
		}

		//uploaded bytes
		std::vector<byte> uploadedVec;
		uploadedVec.resize(8);
		for (int i = 0; i <= 7; ++i) {
			uploadedVec.at(i) = ((ancUploaded >> (8 * (7 - i))) & 0xff);
		}

		//event
		std::vector<byte> eventVec;
		eventVec.resize(4);
		for (int i = 0; i <= 3; ++i) {
			eventVec.at(i) = ((ancIntEvent >> (8 * (3 - i))) & 0xff);
		}

		//default 0 - optional - can be determined from udp request
		std::vector<byte> ipVec = { 0x0, 0x0, 0x0, 0x0 };

		//generate random int32 for key value and store in vec
		//only in case ip changes
		rndInt = dist6(rng);
		std::vector<byte> keyVec;
		keyVec.resize(4);
		for (int i = 0; i <= 3; ++i) {
			keyVec.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
		}

		//number of peers that the client would like to receive from the tracker
		//this client will have max 10 (better for performance)
		std::vector<byte> numWantVec;
		numWantVec.resize(4);
		int32_t numWantInt = 10;
		for (int i = 0; i <= 3; ++i) {
			numWantVec.at(i) = ((numWantInt >> (8 * (3 - i))) & 0xff);
		}

		//port
		std::vector<byte> portVec;
		portVec.resize(2);
		int32_t intPort = 6681;
		for (int i = 0; i <= 1; ++i) {
			portVec.at(i) = ((intPort >> (8 * (1 - i))) & 0xff);
		}

		//copy each section to buffer
		std::vector<byte> announceVec;
		announceVec.insert(announceVec.begin(), std::begin(connectionID), std::end(connectionID));
		announceVec.insert(announceVec.begin() + 8, std::begin(ancAction), std::end(ancAction));
		announceVec.insert(announceVec.begin() + 12, std::begin(sentTransactionID), std::end(sentTransactionID));
		announceVec.insert(announceVec.begin() + 16, std::begin(ancInfoHash), std::end(ancInfoHash));
		announceVec.insert(announceVec.begin() + 36, std::begin(ancClientID), std::end(ancClientID));
		announceVec.insert(announceVec.begin() + 56, std::begin(downloadedVec), std::end(downloadedVec));
		announceVec.insert(announceVec.begin() + 64, std::begin(remainingVec), std::end(remainingVec));
		announceVec.insert(announceVec.begin() + 72, std::begin(uploadedVec), std::end(uploadedVec));
		announceVec.insert(announceVec.begin() + 80, std::begin(eventVec), std::end(eventVec));
		announceVec.insert(announceVec.begin() + 84, std::begin(ipVec), std::end(ipVec));
		announceVec.insert(announceVec.begin() + 88, std::begin(keyVec), std::end(keyVec));
		announceVec.insert(announceVec.begin() + 92, std::begin(numWantVec), std::end(numWantVec));
		announceVec.insert(announceVec.begin() + 96, std::begin(portVec), std::end(portVec));

		/*//print for testing
		std::cout << "Announce request data: " << "\n";
		for (size_t i = 0; i < 98; ++i)
		{
			printf("%x ", (unsigned char)announceVec[i]);
		}
		std::cout << "\n";*/

		//convert to appropriate buffer for sending via UDP and return
		return announceVec;
	}

	void UDPClient::handleAnnounceResp(std::vector<byte>& receivedAncBuffer,
		const std::size_t& AncBytesRec)
	{
		//validate size
		if (AncBytesRec < 20)
		{
			throw std::invalid_argument("Connect response packet is less than the minimum size of 20 bytes!");
		}

		//validate action
		std::vector<byte> receivedAction = { receivedAncBuffer.begin(),
			receivedAncBuffer.begin() + 4 };
		if (receivedAction != ancAction)
		{
			throw std::invalid_argument("Response action is not \"announce\"!");
		}

		//store transaction id and validate
		receivedTransactionID = { receivedAncBuffer.begin() + 4,
			receivedAncBuffer.begin() + 8 };
		if (receivedTransactionID != sentTransactionID)
		{
			throw std::invalid_argument("Response transaction ID is not equal to sent transaction ID!");
		}

		//convert interval bytes to integer and store as seconds
		int intInterval = 0;
		for (size_t i = 8; i < 12; ++i)
		{
			intInterval <<= 8;
			intInterval |= receivedAncBuffer.at(i);
		}
		interval = boost::posix_time::seconds(intInterval);

		//convert leechers bytes to int
		for (size_t i = 12; i < 16; ++i)
		{
			leechers <<= 8;
			leechers |= receivedAncBuffer.at(i);
		}

		//convert seeders bytes to int
		for (size_t i = 16; i < 20; ++i)
		{
			seeders <<= 8;
			seeders |= receivedAncBuffer.at(i);
		}

		//convert ip address bytes to int
		std::vector<byte> peerInfo(receivedAncBuffer.begin() + 20, 
			receivedAncBuffer.begin() + AncBytesRec);
		//info is split into chunks of 6 bytes
		for (size_t i = 0; i < peerInfo.size(); i+=6)
		{
			//first four bytes of each chunk form the ip address
			std::string ipAddress = std::to_string(peerInfo.at(i)) + "."
				+ std::to_string(peerInfo.at(i + 1)) + "." +
				std::to_string(peerInfo.at(i + 2)) + "."
				+ std::to_string(peerInfo.at(i + 3));
			//the two bytes representing port are in big endian
			//read in correct order directly using bit shifting
			std::string peerPort = std::to_string(
				(peerInfo.at(i + 4) << 8) | (peerInfo.at(i + 5)));
			//add to peers list
			peers.emplace(ipAddress, peerPort);
		}
	}

	void UDPClient::dataTransmission(trackerUrl& parsedUrl)
	{
		try
		{
			boost::system::error_code err;

			//if empty use default values
			peerPort = parsedUrl.port.empty() ? "80" : parsedUrl.port;
			peerTarget = parsedUrl.target.empty() ? "/" : parsedUrl.target;

			udp::resolver resolver{ io_context };

			// Look up the domain name
			//auto const results = resolver.resolve(peerHost, peerPort);
			const auto results = resolver.resolve("open.stealth.si", "80");

			//iterate and get successful connection endpoint
			const auto remoteEndpointIter =
				boost::asio::connect(socket, results.begin(), results.end());
			remoteEndpoint = remoteEndpointIter->endpoint();

			//not needed anymore
			socket.close();

			//build connect request buffer
			const auto connectReqBuffer = buildConnectReq();

			std::cout << "Sending..." << "\n";

			//create new socket with specific source port 
			//(to which tracker sends response)
			udp::socket socket(io_context, udp::endpoint(udp::v4(), UDP_PORT));
			//send connect request
			const auto sentConnBytes = 
				socket.send_to(boost::asio::buffer(connectReqBuffer,
					connectReqBuffer.size()), remoteEndpoint, 0, err);

			std::cout << "Sent connect request to " << remoteEndpoint << ": " << 
				sentConnBytes << " bytes" << " (" << err.message() << ") \n";

			std::cout << "Receiving..." << "\n";

			//no need to bind socket for receiving because sending UDP 
			//automatically binds endpoint
			//read response into buffer
			const std::size_t connBytesRec = 
				socket.receive_from(boost::asio::buffer(&receivedConnBuffer[0], 
					receivedConnBuffer.size()), remoteEndpoint, 0, err);

			std::cout << "Received connect response from " << remoteEndpoint << ": " << 
				connBytesRec << " bytes" << " (" << err.message() << ") \n";

			//update relevant timekeeping
			connIDReceivedTime = boost::posix_time::second_clock::universal_time();
			lastRequestTime = boost::posix_time::second_clock::universal_time();

			//handle connect response
			handleConnectResp(receivedConnBuffer, connBytesRec);
			//build connect request buffer
			const auto announceReqBuffer = buildAnnounceReq();

			std::cout << "Sending..." << "\n";

			///send announce request
			const auto sentAncBytes =
				socket.send_to(boost::asio::buffer(announceReqBuffer,
					announceReqBuffer.size()), remoteEndpoint);

			std::cout << "Sent announce request to " << remoteEndpoint << ": " <<
				sentAncBytes << " bytes" << " (" << err.message() << ") \n";

			std::cout << "Receiving..." << "\n";

			//read response into buffer
			//set max peers for this client to 50 - more than enough
			//(6 bytes * 50 = 300) + 20 for other info = 320 bytes max
			//size initialised in constructor
			const std::size_t ancBytesRec =
				socket.receive_from(boost::asio::buffer(receivedAncBuffer),
					localEndpoint, 0, err);

			std::cout << "Received connect response from " << remoteEndpoint << ": " <<
				ancBytesRec << " bytes" << " (" << err.message() << ") \n";

			//update relevant timekeeping
			lastRequestTime = boost::posix_time::second_clock::universal_time();

			//handle announce response
			handleAnnounceResp(receivedAncBuffer, ancBytesRec);

		}
		catch (const boost::system::system_error& e)
		{
			std::cout << "Error occured! Error code = " << e.code()
				<< ". Message: " << e.what();
		}
	}


}
