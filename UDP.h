#pragma once

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "ValueTypes.h"
#include "trackerUrl.h"


namespace Bittorrent
{
	using boost::asio::ip::udp;
	using boost::asio::ip::address;

	class UDP
	{
	public:
		//sent variables
		std::vector<byte> protocolID;
		std::vector<byte> sentTransactionID;
		std::vector<byte> connectAction;

		//received variables
		std::vector<byte> receivedTransactionID;
		std::vector<byte> receivedAction;
		std::vector<int8_t> connectionID;
		boost::posix_time::ptime connIDReceivedTime;
		boost::posix_time::ptime lastRequestTime;

		//announce (send) variables
		//using int8_t instead of byte (uint8_t) because announce buffer 
		//contains -1 for num_want value
		std::vector<int8_t> ancAction;
		std::vector<int8_t> ancClientID;
		std::vector<int8_t> ancInfoHash;
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
		std::vector<int8_t> buildAnnounceReq();
		void handleAnnounceResp(std::vector<byte>& receivedAncBuffer,
			const std::size_t& AncBytesRec);


		//testing send/receive
		std::vector<byte> receivedConnBuffer;
		std::vector<byte> receivedAncBuffer;

		//default constructor & destructor
		UDP(trackerUrl& parsedUrl, std::vector<int8_t>& clientId, std::vector<int8_t>& infoHash,
			long long& uploaded, long long& downloaded, long long& remaining,
			int& intEvent);
		~UDP();

	private:
		boost::asio::io_context io_context;
		udp::socket socket;
		udp::endpoint remoteEndpoint;
		udp::endpoint remoteEndpoint1;
		udp::endpoint localEndpoint;



	};
}
