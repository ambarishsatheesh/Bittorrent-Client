#include "trackerUrl.h"

namespace Bittorrent
{
	trackerUrl::trackerUrl(std::string& url)
		: protocol{ trackerUrl::protocolType::udp }, fullUrl{ "" }, 
		hostname{ "" }, port{ "" }, target{""}
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
	}
}
