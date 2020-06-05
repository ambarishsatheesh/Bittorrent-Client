#include "UDPClient.h"
#include "Utility.h"
#include "loguru.h"

#include <iostream>
#include <iomanip>
#include <random>
#include <limits>
#include <boost/date_time/posix_time/posix_time.hpp>

// TODO: implement time out/retransmit
// TODO: need to populate "remaining"
// TODO: port as argument (randomise?)

namespace Bittorrent
{
	using namespace utility;

	//port "0" in member initialisation list for socket_connect assigns any 
	//free port
	UDPClient::UDPClient(trackerUrl& parsedUrl, std::vector<byte>& clientID, 
		std::vector<byte>& infoHash, long long& uploaded, long long& downloaded, 
		long long& remaining, int& intEvent, int& port, bool isAnnounce)
		: connIDReceivedTime{}, lastRequestTime{}, peerRequestInterval{ 0 }, 
		leechers{ 0 }, seeders{ 0 }, completed{ 0 }, 
		peerHost{ parsedUrl.hostname }, peerPort{ parsedUrl.port }, 
		localPort{port}, isFail{ 0 },
		byteInfoHash{ infoHash }, ancClientID{ clientID },
		ancDownloaded{ downloaded }, ancUploaded{ uploaded },
		ancRemaining{ remaining }, ancIntEvent{ intEvent }, 
		recConnBuffer(16), recScrapeBuffer(200), recAncBuffer(320), 
		io_context(), socket_connect(io_context, udp::endpoint(udp::v4(), 0)), 
		socket_transmission(io_context, udp::endpoint(udp::v4(), port)),
		remoteEndpoint(), localEndpoint()
	{
		try
		{
			errorAction = { 0x0, 0x0, 0x0, 0x3 };

			udp::resolver resolver{ io_context };

			/*LOG_F(INFO, "Resolving UDP tracker (%s:%s)...",
				peerHost.c_str(), peerPort.c_str());*/

			// Look up the domain name
			const auto results = resolver.resolve(peerHost, peerPort);

			//iterate and get successful connection endpoint
			const auto remoteEndpointIter =
				boost::asio::connect(socket_connect, results.begin(),
					results.end());
			remoteEndpoint = remoteEndpointIter->endpoint();

			//not needed anymore
			socket_connect.close();

			LOG_F(INFO, "Resolved UDP tracker endpoint! Endpoint: %s:%hu (%s:%s).",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				peerHost.c_str(), peerPort.c_str());

			dataTransmission(parsedUrl, isAnnounce);
		}
		catch (const boost::system::system_error& e)
		{
			isFail = true;
			LOG_F(ERROR,
				"Failed to resolve UDP tracker %s:%s! Error msg: \"%s\".",
				peerHost.c_str(), peerPort.c_str(), e.what());
		}
	}

	UDPClient::~UDPClient()
	{
		socket_connect.close();
		socket_transmission.close();

		LOG_F(INFO,
			"Closed UDP socket used for tracker update (%s:%s).",
			peerHost.c_str(), peerPort.c_str());
	}

	std::vector<byte> UDPClient::buildConnectReq()
	{
		LOG_F(INFO, "Building UDP connect request to tracker %s:%hu...",
			remoteEndpoint.address().to_string(), remoteEndpoint.port());

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
		sentTransactionID.clear();
		sentTransactionID.resize(4);
		for (int i = 0; i <= 3; ++i) {
			sentTransactionID.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
		}

		//copy to buffer
		std::vector<byte> connectVec(16);
		auto last = std::copy(protocolID.begin(), protocolID.end(), 
			connectVec.begin());
		last = std::copy(connectAction.begin(), connectAction.end(), last);
		last = std::copy(sentTransactionID.begin(), sentTransactionID.end(), last);

		//convert to appropriate buffer for sending via UDP and return
		return connectVec;
	}

	std::vector<byte> UDPClient::buildScrapeReq()
	{
		LOG_F(INFO, "Building UDP scrape request to tracker %s:%hu...",
			remoteEndpoint.address().to_string(), remoteEndpoint.port());

		//build scrape request buffer
		scrapeAction = { 0x0, 0x0, 0x0, 0x2 };

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

		//copy to buffer
		std::vector<byte> scrapeVec(36);
		auto last = std::copy(connectionID.begin(), connectionID.end(), 
			scrapeVec.begin());
		last = std::copy(scrapeAction.begin(), scrapeAction.end(), last);
		last = std::copy(sentTransactionID.begin(), sentTransactionID.end(), 
			last);
		last = std::copy(byteInfoHash.begin(), byteInfoHash.end(), last);

		//convert to appropriate buffer for sending via UDP and return
		return scrapeVec;
	}

	std::vector<byte> UDPClient::buildAnnounceReq()
	{
		LOG_F(INFO, "Building UDP announce request to tracker %s:%hu...",
			remoteEndpoint.address().to_string(), remoteEndpoint.port());

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

		////print for testing
		//std::cout << "Transaction ID: ";
		//for (size_t i = 0; i < 4; ++i)
		//{
		//	printf("%x ", (unsigned char)sentTransactionID[i]);
		//}
		//std::cout << "\n";

		//std::cout << "Connection ID: ";
		//for (size_t i = 0; i < 8; ++i)
		//{
		//	printf("%x ", (unsigned char)connectionID[i]);
		//}
		//std::cout << "\n";

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
		int32_t intPort = localPort;
		for (int i = 0; i <= 1; ++i) {
			portVec.at(i) = ((intPort >> (8 * (1 - i))) & 0xff);
		}

		//copy each section to buffer
		std::vector<byte> announceVec(98);
		auto last = std::copy(connectionID.begin(), connectionID.end(), announceVec.begin());
		last = std::copy(ancAction.begin(), ancAction.end(), last);
		last = std::copy(sentTransactionID.begin(), sentTransactionID.end(), last);
		last = std::copy(byteInfoHash.begin(), byteInfoHash.end(), last);
		last = std::copy(ancClientID.begin(), ancClientID.end(), last);
		last = std::copy(downloadedVec.begin(), downloadedVec.end(), last);
		last = std::copy(remainingVec.begin(), remainingVec.end(), last);
		last = std::copy(uploadedVec.begin(), uploadedVec.end(), last);
		last = std::copy(eventVec.begin(), eventVec.end(), last);
		last = std::copy(ipVec.begin(), ipVec.end(), last);
		last = std::copy(keyVec.begin(), keyVec.end(), last);
		last = std::copy(numWantVec.begin(), numWantVec.end(), last);
		last = std::copy(portVec.begin(), portVec.end(), last);

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

	void UDPClient::connectRequest(boost::system::error_code& err)
	{
		try
		{
			//build connect request buffer
			const auto connectReqBuffer = buildConnectReq();

			//send connect request
			const auto sentConnBytes =
				socket_transmission.send_to(boost::asio::buffer(connectReqBuffer,
					connectReqBuffer.size()), remoteEndpoint, 0, err);

			std::string logBuffer = toHex(connectReqBuffer);

			LOG_F(INFO, "Sent UDP connect request from %s:%hu to tracker %s:%hu; "
				"Status: %s; Sent bytes: %d; Sent payload (hex): %s.",
				socket_transmission.local_endpoint().address().to_string(),
				socket_transmission.local_endpoint().port(),
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), sentConnBytes, logBuffer.c_str());

			//no need to bind socket for receiving because sending UDP 
			//automatically binds endpoint
			//read response into buffer
			const std::size_t recConnBytes =
				socket_transmission.receive_from(
					boost::asio::buffer(recConnBuffer,
						recConnBuffer.size()), remoteEndpoint, 0, err);

			logBuffer = toHex(recConnBuffer);

			LOG_F(INFO, "Received UDP connect response from tracker %s:%hu; "
				"Status: %s; Received bytes: %d; Received payload (hex): %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), recConnBytes, logBuffer.c_str());

			//update relevant timekeeping
			connIDReceivedTime = boost::posix_time::second_clock::local_time();
			lastRequestTime = boost::posix_time::second_clock::local_time();

			LOG_F(INFO, "Received UDP connect response at %s; "
				"Updated last request time. Tracker: %s%hu.",
				boost::posix_time::to_simple_string(
					boost::posix_time::ptime(connIDReceivedTime)).c_str(),
				remoteEndpoint.address().to_string(), remoteEndpoint.port());

			//handle connect response
			handleConnectResp(recConnBytes);
		}
		catch (const boost::system::system_error& e)
		{
			isFail = true;
			LOG_F(ERROR,
				"UDP connect request failure (tracker %s:%hu): %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				e.what());
		}
	}

	void UDPClient::scrapeRequest(boost::system::error_code& err)
	{
		try
		{
			//build scrape request buffer
			const auto scrapeReqBuffer = buildScrapeReq();

			//send connect request
			const auto sentScrapeBytes =
				socket_transmission.send_to(boost::asio::buffer(scrapeReqBuffer,
					scrapeReqBuffer.size()), remoteEndpoint, 0, err);

			std::string logBuffer = toHex(scrapeReqBuffer);

			LOG_F(INFO, "Sent UDP scrape request from %s:%hu to tracker %s:%hu; "
				"Status: %s; Sent bytes: %d; Sent payload (hex): %s.",
				socket_transmission.local_endpoint().address().to_string(),
				socket_transmission.local_endpoint().port(),
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), sentScrapeBytes, logBuffer.c_str());

			//no need to bind socket for receiving because sending UDP 
			//automatically binds endpoint
			//read response into buffer
			const std::size_t recScrapeBytes =
				socket_transmission.receive_from(
					boost::asio::buffer(recScrapeBuffer,
						recScrapeBuffer.size()), remoteEndpoint, 0, err);

			//remove redundant data at end
			recScrapeBuffer.erase(recScrapeBuffer.begin() + recScrapeBytes,
				recScrapeBuffer.end());
			logBuffer = toHex(recScrapeBuffer);

			LOG_F(INFO, "Received UDP scrape response from tracker %s:%hu; "
				"Status: %s; Received bytes: %d; Received payload (hex): %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), recScrapeBytes, logBuffer.c_str());

			//update relevant timekeeping
			connIDReceivedTime = boost::posix_time::second_clock::local_time();
			lastRequestTime = boost::posix_time::second_clock::local_time();

			LOG_F(INFO, "Received UDP scrape response at %s; "
				"Updated last request time. Tracker: %s%hu.",
				boost::posix_time::to_simple_string(
					boost::posix_time::ptime(connIDReceivedTime)).c_str(),
				remoteEndpoint.address().to_string(), remoteEndpoint.port());

			//handle connect response
			handleScrapeResp(recScrapeBytes);
		}
		catch (const boost::system::system_error& e)
		{
			isFail = true;
			LOG_F(ERROR,
				"UDP scrape request failure (tracker %s:%hu): %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				e.what());
		}
	}

	void UDPClient::announceRequest(boost::system::error_code& err)
	{
		try
		{
			//build connect request buffer
			const auto announceReqBuffer = buildAnnounceReq();

			///send announce request
			const auto sentAncBytes =
				socket_transmission.send_to(boost::asio::buffer(
					announceReqBuffer, announceReqBuffer.size()), remoteEndpoint);

			std::string logBuffer = toHex(announceReqBuffer);

			LOG_F(INFO, "Sent UDP announce request from %s:%hu to tracker %s:%hu; "
				"Status: %s; Sent bytes: %d; Sent payload (hex): %s.",
				socket_transmission.local_endpoint().address().to_string(),
				socket_transmission.local_endpoint().port(),
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), sentAncBytes, logBuffer.c_str());

			//read response into buffer
			//set max peers for this client to 50 - more than enough
			//(6 bytes * 50 = 300) + 20 for other info = 320 bytes max
			//size initialised in constructor
			const std::size_t recAncBytes =
				socket_transmission.receive_from(boost::asio::buffer(
					recAncBuffer), localEndpoint, 0, err);

			//remove redundant data at end
			recAncBuffer.erase(recAncBuffer.begin() + recAncBytes, 
				recAncBuffer.end());
			logBuffer = toHex(recAncBuffer);

			LOG_F(INFO, "Received UDP announce response from tracker %s:%hu; "
				"Status: %s; Received bytes: %d; Received payload (hex): %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), recAncBytes, logBuffer.c_str());

			//update relevant timekeeping
			lastRequestTime = boost::posix_time::second_clock::local_time();

			LOG_F(INFO, "Received UDP announce response at %s; "
				"Updated last request time. "
				"Tracker: %s%hu.",
				boost::posix_time::to_simple_string(
					boost::posix_time::ptime(connIDReceivedTime)).c_str(),
				remoteEndpoint.address().to_string(), remoteEndpoint.port());

			//handle announce response
			handleAnnounceResp(recAncBytes);
		}
		catch (const boost::system::system_error& e)
		{
			isFail = true;
			LOG_F(ERROR,
				"UDP announce request failure (tracker %s:%hu): %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				e.what());
		}
	}

	void UDPClient::handleConnectResp(const std::size_t& connBytesRec)
	{
		//validate size
		if (connBytesRec < 16)
		{
			isFail = true;
			LOG_F(ERROR,
				"UDP Connect response packet is less than the expected 16 bytes! "
			"Tracker: %s%hu.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());
			return;
		}

		//validate action
		std::vector<byte> receivedAction = { recConnBuffer.begin(),
			recConnBuffer.begin() + 4 };
		if (receivedAction != connectAction)
		{
			isFail = true;
			LOG_F(ERROR,
				"Received UDP connect response action is not \"connect\"! "
				"Tracker: %s%hu.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());
			return;
		}

		//validate transaction id
		std::vector<byte> receivedTransactionID = { recConnBuffer.begin() + 4,
			recConnBuffer.begin() + 8 };
		if (receivedTransactionID != sentTransactionID)
		{
			isFail = true;
			LOG_F(ERROR,
				"UDP connect response transaction ID is not equal to sent "
				"transaction ID! Tracker: %s%hu.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());
			return;
		}

		//store connection id
		connectionID = { recConnBuffer.begin() + 8,
			recConnBuffer.end() };

		std::string ConnIDStr = toHex(connectionID);

		LOG_F(INFO, "Handled UDP connect response from tracker %s:%hu; "
			"Connection ID (hex): %s.",
			remoteEndpoint.address().to_string(), remoteEndpoint.port(),
			ConnIDStr.c_str());
	}

	void UDPClient::handleScrapeResp(const std::size_t& scrapeBytesRec)
	{
		//validate size
		if (scrapeBytesRec < 8)
		{
			isFail = true;
			LOG_F(ERROR,
				"UDP connect response packet is less than the expected 8 bytes! " 
				"Tracker: %s%hu.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());
			return;
		}

		//validate transaction id
		std::vector<byte> receivedTransactionID = { recScrapeBuffer.begin() + 4,
			recScrapeBuffer.begin() + 8 };
		if (receivedTransactionID != receivedTransactionID)
		{
			isFail = true;
			LOG_F(ERROR,
				"UDP scrape response transaction ID is not equal to sent "
				"transaction ID! Tracker: %s%hu.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());
			return;
		}

		//validate action
		std::vector<byte> receivedAction = { recScrapeBuffer.begin(),
			recScrapeBuffer.begin() + 4 };

		if (receivedAction == scrapeAction)
		{
			//convert seeders bytes to int
			for (size_t i = 8; i < 12; ++i)
			{
				seeders <<= 8;
				seeders |= recScrapeBuffer.at(i);
			}

			//convert completed bytes to int
			for (size_t i = 12; i < 16; ++i)
			{
				completed <<= 8;
				completed |= recScrapeBuffer.at(i);
			}

			//convert leechers bytes to int
			for (size_t i = 12; i < 16; ++i)
			{
				leechers <<= 8;
				leechers |= recScrapeBuffer.at(i);
			}

			LOG_F(INFO, "Handled UDP scrape response from tracker %s:%hu; "
				"Updated peer data.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());

		}
		else if (receivedAction == errorAction)
		{
			//get error message
			std::string scrapeErrorMsg(recScrapeBuffer.begin() + 8,
				recScrapeBuffer.begin() + scrapeBytesRec);
			
			isFail = true;
			LOG_F(ERROR,
				"Tracker UDP scrape error (tracker %s:%hu): %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				scrapeErrorMsg.c_str());
		}
	}


	void UDPClient::handleAnnounceResp(const std::size_t& AncBytesRec)
	{
		//validate size
		if (AncBytesRec < 20)
		{
			isFail = true;
			LOG_F(ERROR,
				"UDP announce response packet is less than the minimum size "
				"of 20 bytes! Tracker: %s%hu.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());
			return;
		}

		//validate action
		std::vector<byte> receivedAction = { recAncBuffer.begin(),
			recAncBuffer.begin() + 4 };
		if (receivedAction != ancAction)
		{
			isFail = true;
			LOG_F(ERROR,
				"Received UDP announce response action is not \"announce\"! "
				"Tracker: %s%hu.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());
			return;
		}

		//validate transaction id
		std::vector<byte> receivedTransactionID = { recAncBuffer.begin() + 4,
			recAncBuffer.begin() + 8 };
		if (receivedTransactionID != sentTransactionID)
		{
			isFail = true;
			LOG_F(ERROR,
				"UDP announce response transaction ID is not equal to sent "
				"transaction ID! Tracker: %s%hu.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port());
			return;
		}

		//convert interval bytes to integer and store as seconds
		int intInterval = 0;
		for (size_t i = 8; i < 12; ++i)
		{
			intInterval <<= 8;
			intInterval |= recAncBuffer.at(i);
		}
		peerRequestInterval = boost::posix_time::seconds(intInterval);

		//convert leechers bytes to int
		for (size_t i = 12; i < 16; ++i)
		{
			leechers <<= 8;
			leechers |= recAncBuffer.at(i);
		}

		//convert seeders bytes to int
		for (size_t i = 16; i < 20; ++i)
		{
			seeders <<= 8;
			seeders |= recAncBuffer.at(i);
		}

		//convert ip address bytes to int
		std::vector<byte> peerInfo(recAncBuffer.begin() + 20, 
			recAncBuffer.begin() + AncBytesRec);
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
			peer singlePeer;
			singlePeer.ipAddress = ipAddress;
			singlePeer.port = peerPort;
			peerList.push_back(singlePeer);
		}

		LOG_F(INFO, "Handled UDP announce response from tracker %s:%hu; "
			"Updated peer data.",
			remoteEndpoint.address().to_string(), remoteEndpoint.port());
	}

	void UDPClient::dataTransmission(trackerUrl& parsedUrl, bool isAnnounce)
	{
		boost::system::error_code err;

		connectRequest(err);

		//use interval value to check if we have announced before
		if (isAnnounce && !isFail)
		{
			announceRequest(err);
		}
		else if (!isFail)
		{
			scrapeRequest(err);
		}
	}
}