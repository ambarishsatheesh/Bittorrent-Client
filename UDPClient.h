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
		std::vector<byte> receivedTransactionID;
		std::vector<byte> receivedAction;
		std::vector<byte> connectionID;
		boost::posix_time::ptime connIDReceivedTime;
		boost::posix_time::ptime lastRequestTime;

		//announce (send) variables
		//using byte instead of byte (uint8_t) because announce buffer 
		//contains -1 for num_want value
		std::vector<byte> ancAction;
		std::vector<byte> ancClientID;
		std::vector<byte> ancInfoHash;
		long long ancDownloaded;
		long long ancUploaded;
		long long ancRemaining;
		int ancIntEvent;

		//general UDP variables
		std::string peerHost;
		std::string peerPort;
		std::string peerTarget;


		std::vector<byte> buildConnectReq();
		void dataTransmission(trackerUrl& parsedUrl);
		void handleConnectResp(std::vector<byte>& receivedBuffer, 
			const std::size_t& connBytesRec);
		std::vector<byte> buildAnnounceReq();
		void handleAnnounceResp(std::vector<byte>& receivedAncBuffer,
			const std::size_t& AncBytesRec);

		//send/receive buffers
		std::vector<byte> receivedConnBuffer;
		std::vector<byte> receivedAncBuffer;

		//announce response data
		boost::posix_time::seconds interval;
		int leechers;
		int seeders;
		std::unordered_map<std::string, std::string> peers;


		//default constructor & destructor
		UDPClient(trackerUrl& parsedUrl, std::vector<byte>& clientId, std::vector<byte>& infoHash,
			long long& uploaded, long long& downloaded, long long& remaining,
			int& intEvent);
		~UDPClient();

	private:
		boost::asio::io_context io_context;
		udp::socket socket;
		udp::endpoint remoteEndpoint;
		udp::endpoint localEndpoint;
	};
}
