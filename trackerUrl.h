
#pragma once

#include <iostream>
#include <string>
#include <boost/asio.hpp>

using a_udp = boost::asio::ip::udp;

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
		a_udp::endpoint endpoint;
		

		//constructor
		trackerUrl(std::string& url);
		//no default constructor - requires parameter
		trackerUrl() = delete;

	private:
		void isUDP();
	};

}

