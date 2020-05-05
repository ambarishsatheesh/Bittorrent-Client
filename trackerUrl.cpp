#include "trackerUrl.h"

namespace Bittorrent
{
	trackerUrl::trackerUrl(std::string& url)
		: protocol{ trackerUrl::protocolType::udp }, fullUrl{ "" }, 
		hostname{ "" }, port{ "" }
	{
		fullUrl = url;

		//get protocol
		const size_t protocolColonIdx = url.find_first_of(":");
		std::string protocolString = url.substr(0, protocolColonIdx);

		if (protocolString == "http")
		{
			protocol = protocolType::http;
		}
		else if (protocolString == "udp")
		{
			protocol = protocolType::udp;
		}
		else
		{
			throw std::invalid_argument("Invalid protocol!");
		}

		//get hostname
		std::string urlNoProtocol = url.substr(protocolColonIdx + 3);
		const size_t portColonIdx = urlNoProtocol.find_first_of(":");
		hostname = urlNoProtocol.substr(0, portColonIdx);
		std::cout << hostname << "\n";


		//get port if not empty
		if (portColonIdx != std::string::npos)
		{
			port = urlNoProtocol.substr(portColonIdx + 1,
				urlNoProtocol.find_first_of("\\/") - (portColonIdx + 1));
		}
		else
		{
			port = port;
		}

		//get target if not empty
		const size_t targetSlashIndx = urlNoProtocol.find_first_of("\\/");
		if (targetSlashIndx != std::string::npos)
		{
			target = urlNoProtocol.substr(targetSlashIndx);
		}
		else
		{
			target = target;
		}

		//if UDP, get endpoint
		if (protocol == protocolType::udp)
		{
			isUDP();
		}
		else
		{
			return;
		}
	}

	void trackerUrl::isUDP()
	{
		//DNS lookup to get ip address from hostname
		try
		{
			boost::asio::io_context io_context;
			a_udp::resolver resolver(io_context);
			//resolve ipv4 endpoints
			auto endpoints = resolver.resolve(a_udp::v4(), hostname, port);
			//create socket
			boost::asio::ip::udp::socket socket(io_context);
			//iterate over each endpoint until successful connection
			auto finalEndpoint = boost::asio::connect(socket,
				endpoints.begin(), endpoints.end());
			endpoint = finalEndpoint->endpoint();
		}
		catch (const boost::system::system_error& e)
		{
			std::cout << "Error occured! Error code = " << e.code()
				<< ". Message: " << e.what();
		}
	}
}
