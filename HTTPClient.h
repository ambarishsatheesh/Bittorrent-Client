#pragma once
#include "ValueTypes.h"
#include "trackerUrl.h"

#include <unordered_map>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffers_iterator.hpp>

namespace Bittorrent
{
	using tcp = boost::asio::ip::tcp;
	namespace http = boost::beast::http;

	class HTTPClient
	{
	public:

		//general variables
		std::string peerHost;
		std::string peerPort;
		std::string target;
		int version;

		//response data
		boost::posix_time::seconds peerRequestInterval;
		int complete;
		int incomplete;
		std::vector<peer> peerList;

		void dataTransmission(bool isAnnounce);

		//constructor
		HTTPClient(trackerUrl& parsedUrl, bool isAnnounce);
		~HTTPClient();

	private:
		boost::asio::io_context io_context;
		tcp::resolver resolver;
		tcp::socket socket;
		tcp::endpoint remoteEndpoint;

		void scrapeRequest(boost::system::error_code& err);
		void announceRequest(boost::system::error_code& err);

		void handleScrapeResp(http::response<http::dynamic_body>& response);
		void handleAnnounceResp(http::response<http::dynamic_body>& response);

	};


}

