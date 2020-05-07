#pragma once

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "ValueTypes.h"
#include "trackerUrl.h"




namespace Bittorrent
{
	using boost::asio::ip::udp;

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
		long ancClientId;
		std::vector<int8_t> ancInfoHash;
		long long ancDownloaded;
		long long ancUploaded;
		long long ancRemaining;
		int ancIntEvent;

		//general UDP variables
		std::string peerHost;
		std::string peerPort;
		std::string peerTarget;


		boost::asio::mutable_buffers_1 buildConnectReq();
		void send(const std::string& msg, trackerUrl parsedUrl);
		void handleConnectResp(std::vector<byte>& receivedBuffer, 
			const std::size_t& connBytesRec);
		boost::asio::mutable_buffers_1 buildAnnounceReq();
		void handleAnnounceResp(std::vector<byte>& receivedAncBuffer,
			const std::size_t& AncBytesRec);

		//default constructor & destructor
		UDP(trackerUrl parsedUrl, long clientId, std::vector<int8_t> infoHash,
			long long uploaded, long long downloaded, long long remaining, 
			int intEvent);
		~UDP();

	private:
		boost::asio::io_context io_context;
		udp::socket socket;
		udp::endpoint receiverEndpoint;
		udp::endpoint senderEndpoint;



	};
}
