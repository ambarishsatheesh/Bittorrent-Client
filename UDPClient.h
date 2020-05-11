#pragma once
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "ValueTypes.h"
#include "trackerUrl.h"

#include <unordered_map>


namespace Bittorrent
{
	using boost::asio::ip::udp;
	using boost::asio::ip::address;

	class UDPClient
	{
	public:
		//sent variables
		std::vector<byte> protocolID;
		std::vector<byte> sentTransactionID;
		std::vector<byte> connectAction;

		//received variables
		std::vector<byte> connectionID;
		boost::posix_time::ptime connIDReceivedTime;
		boost::posix_time::ptime lastRequestTime;

		//scrape variables
		std::vector<byte> scrapeAction;
		std::vector<byte> byteInfoHash;

		//announce (send) variables
		//using byte instead of byte (uint8_t) because announce buffer 
		//contains -1 for num_want value
		std::vector<byte> ancAction;
		std::vector<byte> ancClientID;
		long long ancDownloaded;
		long long ancUploaded;
		long long ancRemaining;
		int ancIntEvent;

		//send/receive buffers
		std::vector<byte> receivedConnBuffer;
		std::vector<byte> receivedScrapeBuffer;
		std::vector<byte> receivedAncBuffer;

		//announce response data
		boost::posix_time::seconds peerRequestInterval;
		int leechers;
		int seeders;
		int completed;
		std::vector<peer> peers;

		//general variables
		std::string peerHost;
		std::string peerPort;
		std::vector<byte> errorAction;

		void dataTransmission(trackerUrl& parsedUrl, bool isAnnounce);

		//default constructor & destructor
		UDPClient(trackerUrl& parsedUrl, std::vector<byte>& clientID, 
			std::vector<byte>& infoHash, long long& uploaded, 
			long long& downloaded, long long& remaining, int& intEvent, 
			bool isAnnounce);
		~UDPClient();

	private:
		boost::asio::io_context io_context;
		//Need two sockets since the connect free function will close the 
		//socket and bind to an unspecified port.
		//Easier to create a separate socket bound to the correct port
		udp::socket socket_connect;
		udp::socket socket_transmission;
		udp::endpoint remoteEndpoint;
		udp::endpoint localEndpoint;

		std::vector<byte> buildScrapeReq();
		std::vector<byte> buildConnectReq();
		std::vector<byte> buildAnnounceReq();

		void handleConnectResp(const std::size_t& connBytesRec);
		void handleScrapeResp(const std::size_t& scrapeBytesRec);
		void handleAnnounceResp(const std::size_t& AncBytesRec);

		void connectRequest(boost::system::error_code& err);
		void scrapeRequest(boost::system::error_code& err);
		void announceRequest(boost::system::error_code& err);
	};
}
