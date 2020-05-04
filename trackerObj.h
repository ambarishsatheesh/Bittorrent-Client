#pragma once
//need to link winsock library file to use sockets in http requests
//(or in VS project properties - additional library dependency: ws2_32.lib)
#pragma comment(lib, "Ws2_32.lib")


#include "boost/date_time/posix_time/posix_time.hpp"
#include "HTTPRequest.h"

#include <string>

namespace Bittorrent
{
	class trackerObj
	{
	public:
		std::string trackerAddress;
		//current state of client
		enum class trackerEvent
		{
			started = 0,
			paused,
			stopped
		};

		boost::posix_time::ptime lastPeerRequest;
		boost::posix_time::seconds peerRequestInterval;

		void update(trackerEvent trkEvent, int id,
			int port, std::string urlEncodedInfoHash, long long uploaded,
			long long downloaded, long long remaining);

		void resetLastRequest();
		void httpRequest(std::string url);

		//default constructor
		trackerObj();
	};
}