
#pragma once

#include <iostream>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

namespace Bittorrent
{
	//parsing tracker url into separate components
	class trackerUrl
	{
	public:
		enum class protocolType { http = 0, udp };
		protocolType protocol;
		std::string fullUrl;
		std::string hostname;
		std::string port;
		std::string target;
		bool isInvalidURL;

		//constructor
		trackerUrl(std::string& url);
		//no default constructor - requires parameter
		trackerUrl() = delete;

		void parse();
	};
}

