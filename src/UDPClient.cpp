#include "UDPClient.h"
#include "Utility.h"
#include "loguru.h"

#include <iostream>
#include <iomanip>
#include <random>
#include <limits>
#include <boost/date_time/posix_time/posix_time.hpp>

#define UDP_PORT 6881

// TODO: implement time out/retransmit
// TODO: need to populate "remaining"
// TODO: port as argument (randomise?)

namespace Bittorrent
{
	using namespace utility;

	UDPClient::UDPClient(trackerUrl& parsedUrl, std::vector<byte>& clientID, 
		std::vector<byte>& infoHash, long long& uploaded, long long& downloaded, 
		long long& remaining, int& intEvent, bool isAnnounce)
		: connIDReceivedTime{}, lastRequestTime{}, peerRequestInterval{ 0 }, 
		leechers{ 0 }, seeders{ 0 }, completed{ 0 }, 
		peerHost{ parsedUrl.hostname }, peerPort{ parsedUrl.port }, 
		byteInfoHash{ infoHash }, ancClientID{ clientID },
		ancDownloaded{ downloaded }, ancUploaded{ uploaded },
		ancRemaining{ remaining }, ancIntEvent{ intEvent }, 
		recConnBuffer(16), recScrapeBuffer(200), recAncBuffer(320), 
		io_context(), socket_connect(io_context, udp::endpoint(udp::v4(), 2)), 
		socket_transmission(io_context, udp::endpoint(udp::v4(), 6681)),
		remoteEndpoint(), localEndpoint()
	{
		try
		{
			errorAction = { 0x0, 0x0, 0x0, 0x3 };

			udp::resolver resolver{ io_context };

			LOG_F(INFO, "Resolving UDP tracker (%s:%s)...",
				peerHost, peerPort);

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
				peerHost, peerPort);

			dataTransmission(parsedUrl, isAnnounce);
		}
		catch (const boost::system::system_error& e)
		{
			LOG_F(ERROR,
				"Failed to resolve UDP tracker %s:%s! Error msg: \"%s\".",
				peerHost, peerPort, e.what());
		}
	}

	UDPClient::~UDPClient()
	{
		socket_connect.close();
		socket_transmission.close();

		LOG_F(INFO,
			"Closed UDP socket used for tracker update (%s:%s).",
			peerHost, peerPort);
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
		int32_t intPort = 6681;
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
				"Status: %s; Sent bytes: %d; Sent payload: %s.",
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
				"Status: %s; Received bytes: %d; Received payload: %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), recConnBytes, logBuffer.c_str());

			//update relevant timekeeping
			connIDReceivedTime = boost::posix_time::second_clock::local_time();
			lastRequestTime = boost::posix_time::second_clock::local_time();

			LOG_F(INFO, "Received connection ID at %s; "
				"Updated last request time.",
				boost::posix_time::to_simple_string(
					boost::posix_time::ptime(connIDReceivedTime)).c_str());

			//handle connect response
			handleConnectResp(recConnBytes);

		}
		catch (const boost::system::system_error& e)
		{
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
				"Status: %s; Sent bytes: %d; Sent payload: %s.",
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

			logBuffer = toHex(recScrapeBuffer);

			LOG_F(INFO, "Received UDP scrape response from tracker %s:%hu; "
				"Status: %s; Received bytes: %d; Received payload: %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), recScrapeBytes, logBuffer.c_str());

			//update relevant timekeeping
			connIDReceivedTime = boost::posix_time::second_clock::local_time();
			lastRequestTime = boost::posix_time::second_clock::local_time();

			LOG_F(INFO, "Received connection ID at %s; "
				"Updated last request time.",
				boost::posix_time::to_simple_string(
					boost::posix_time::ptime(connIDReceivedTime)).c_str());

			//handle connect response
			handleScrapeResp(recScrapeBytes);
		}
		catch (const boost::system::system_error& e)
		{
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
				"Status: %s; Sent bytes: %d; Sent payload: %s.",
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
				"Status: %s; Received bytes: %d; Received payload: %s.",
				remoteEndpoint.address().to_string(), remoteEndpoint.port(),
				err.message().c_str(), recAncBytes, logBuffer.c_str());

			//update relevant timekeeping
			lastRequestTime = boost::posix_time::second_clock::local_time();

			LOG_F(INFO, "Updated last request time.");

			//handle announce response
			handleAnnounceResp(recAncBytes);
		}
		catch (const boost::system::system_error& e)
		{
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
			throw std::invalid_argument("Connect response packet is less than the expected 16 bytes!");
		}

		//validate action
		std::vector<byte> receivedAction = { recConnBuffer.begin(),
			recConnBuffer.begin() + 4 };
		if (receivedAction != connectAction)
		{
			throw std::invalid_argument("Response action is not \"connect\"!");
		}

		//validate transaction id
		std::vector<byte> receivedTransactionID = { recConnBuffer.begin() + 4,
			recConnBuffer.begin() + 8 };
		if (receivedTransactionID != sentTransactionID)
		{
			throw std::invalid_argument("Response transaction ID is not equal to sent transaction ID!");
		}

		//store connection id
		connectionID = { recConnBuffer.begin() + 8,
			recConnBuffer.end() };
	}

	void UDPClient::handleScrapeResp(const std::size_t& scrapeBytesRec)
	{
		//validate size
		if (scrapeBytesRec < 8)
		{
			throw std::invalid_argument("Connect response packet is less than the expected 8 bytes!");
		}

		//validate transaction id
		std::vector<byte> receivedTransactionID = { recScrapeBuffer.begin() + 4,
			recScrapeBuffer.begin() + 8 };
		if (receivedTransactionID != receivedTransactionID)
		{
			throw std::invalid_argument("Response transaction ID is not equal to sent transaction ID!");
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
		}
		else if (receivedAction == errorAction)
		{
			//get error message
			std::string scrapeErrorMsg(recScrapeBuffer.begin() + 8,
				recScrapeBuffer.begin() + scrapeBytesRec);
			
			std::cout << "\n" << "Tracker scrape error: " 
				<< scrapeErrorMsg << " (" << "Host: " << peerHost  << ", Port: "
				<<  peerPort << ")" << "\n";
		}
	}


	void UDPClient::handleAnnounceResp(const std::size_t& AncBytesRec)
	{
		//validate size
		if (AncBytesRec < 20)
		{
			throw std::invalid_argument("Connect response packet is less than the minimum size of 20 bytes!");
		}

		//validate action
		std::vector<byte> receivedAction = { recAncBuffer.begin(),
			recAncBuffer.begin() + 4 };
		if (receivedAction != ancAction)
		{
			throw std::invalid_argument("Response action is not \"announce\"!");
		}

		//validate transaction id
		std::vector<byte> receivedTransactionID = { recAncBuffer.begin() + 4,
			recAncBuffer.begin() + 8 };
		if (receivedTransactionID != sentTransactionID)
		{
			throw std::invalid_argument("Response transaction ID is not equal to sent transaction ID!");
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
	}

	void UDPClient::dataTransmission(trackerUrl& parsedUrl, bool isAnnounce)
	{

		boost::system::error_code err;

		connectRequest(err);
		//use interval value to check if we have announced before
		if (isAnnounce)
		{
			announceRequest(err);
		}
		else
		{
			scrapeRequest(err);
		}
	}
}



////////////////////////////////////////////////////////////////////////
//	Testing ssynchronous UDP server/client for tracker communication //
//	In case I decide to switch from synchronous at some point		//
/////////////////////////////////////////////////////////////////////
//#include <ctime>
//#include <iostream>
//#include <string>
//#include <boost/array.hpp>
//#include <boost/bind/bind.hpp>
//#include <boost/shared_ptr.hpp>
//#include <boost/asio.hpp>
//#include <boost/asio/connect.hpp>
//#include <random>
//
//using byte = uint8_t;
//using boost::asio::ip::udp;
//
//class udp_server
//{
//public:
//    udp_server(boost::asio::io_context& io_context)
//        : socket_connect(io_context, udp::endpoint(udp::v4(), 20)), 
//        socket_transmission(io_context, udp::endpoint(udp::v4(), 6881)), 
//        recv_buffer_(16)
//    {
//        udp::resolver resolver{ io_context };
//
//        // Look up the domain name
//        //auto const results = resolver.resolve(peerHost, peerPort);
//        const auto results = resolver.resolve("open.stealth.si", "80");
//
//        //iterate and get successful connection endpoint
//        const auto remoteEndpointIter =
//            boost::asio::connect(socket_connect, results.begin(), results.end());
//        remote_endpoint_ = remoteEndpointIter->endpoint();
//        start();
//    }
//
//private:
//    void start()
//    {
//        std::cout << "source port: " << socket_transmission.local_endpoint().port() << "\n";
//        //build connect request buffer
//        std::vector<byte> protocolID = { 0x0, 0x0, 0x04, 0x17, 0x27, 0x10, 0x19, 0x80 };
//        std::vector<byte> connectAction = { 0x0, 0x0, 0x0, 0x0 };
//
//        //generate random int32 and pass into transactionID array 
//        std::random_device dev;
//        std::mt19937 rng(dev());
//        const std::uniform_int_distribution<int32_t>
//            dist6(0, std::numeric_limits<int32_t>::max());
//        auto rndInt = dist6(rng);
//
//        //Big endian - but endianness doesn't matter since it's random anyway
//        std::vector<byte> sentTransactionID;
//        sentTransactionID.resize(4);
//        for (int i = 0; i <= 3; ++i) {
//            sentTransactionID.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
//        }
//
//        //copy to buffer
//        std::vector<byte> connectVec;
//        connectVec.insert(connectVec.begin(), std::begin(protocolID), std::end(protocolID));
//        connectVec.insert(connectVec.begin() + 8, std::begin(connectAction), std::end(connectAction));
//        connectVec.insert(connectVec.begin() + 12, std::begin(sentTransactionID), std::end(sentTransactionID));
//
//        std::cout << "Sending: " << "\n";
//        for (size_t i = 0; i < 16; ++i)
//        {
//            printf("%x ", (unsigned char)connectVec[i]);
//        }
//        std::cout << "\n" << "\n";
//
//        socket_transmission.async_receive_from(
//            boost::asio::buffer(recv_buffer_), remote_endpoint_1,
//            boost::bind(&udp_server::handle_receive, this,
//                boost::asio::placeholders::error,
//                boost::asio::placeholders::bytes_transferred));
//
//        const auto sentConnBytes =
//            socket_transmission.send_to(boost::asio::buffer(connectVec,
//                connectVec.size()), remote_endpoint_);
//    }
//
//    void handle_receive(const boost::system::error_code& /*error*/,
//        std::size_t /*bytes_transferred*/)
//    {
//        std::cout << "Response: " << "\n";
//        for (size_t i = 0; i < 16; ++i)
//        {
//            printf("%x ", (unsigned char)recv_buffer_[i]);
//        }
//        std::cout << "\n" << "\n";
//    }
//
//    boost::system::error_code ec;
//    boost::asio::io_context io_context;
//
//    //need two sockets since the connect free function will close the socket and bind to an unspecified port
//    //easier to create a separate socket bound to the correct port
//    udp::socket socket_connect;
//    udp::socket socket_transmission;
//    udp::endpoint remote_endpoint_;
//    udp::endpoint remote_endpoint_1;
//    std::vector<byte> recv_buffer_;
//};
//
//int main()
//{
//    try
//    {
//        boost::asio::io_context io_context;
//        udp_server server(io_context);
//        io_context.run();
//    }
//    catch (std::exception& e)
//    {
//        std::cerr << e.what() << std::endl;
//    }
//
//    return 0;
//}