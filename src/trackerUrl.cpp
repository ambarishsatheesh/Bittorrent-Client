#include "trackerUrl.h"
#include "loguru.h"

namespace Bittorrent
{
	trackerUrl::trackerUrl(std::string& url)
		: protocol(), fullUrl{ url }, 
		hostname{ "" }, port{ "" }, target{ "" }, isInvalidURL{}
	{
	}

	void trackerUrl::parse()
	{
		//get protocol
		const size_t protocolColonIdx = fullUrl.find_first_of(":");
		std::string protocolString = fullUrl.substr(0, protocolColonIdx);

		//get hostname
		std::string urlNoProtocol = fullUrl.substr(protocolColonIdx + 3);
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
			LOG_F(ERROR, "Tracker URL does not contain a port! (%s)",
				fullUrl.c_str());
			isInvalidURL = true;
			return;
		}

		//get target if not empty
		const size_t targetSlashIndx = urlNoProtocol.find_first_of("\\/");
		if (targetSlashIndx != std::string::npos)
		{
			target = urlNoProtocol.substr(targetSlashIndx);
		}
		else
		{
			LOG_F(ERROR, "Tracker URL does not contain a target! (%s)",
				fullUrl.c_str());
			isInvalidURL = true;
			return;
		}

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
			LOG_F(ERROR, "Invalid protocol \"%s\" for tracker %s:%s!",
				protocolString.c_str(),
				hostname.c_str(), port.c_str());
			isInvalidURL = true;
			return;
		}
	}
}
