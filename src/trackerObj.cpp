#include "trackerObj.h"
#include "Decoder.h"
#include "Utility.h"
#include "loguru.h"


namespace Bittorrent
{
	using namespace utility;

	trackerObj::trackerObj()
		: trackerAddress{ "" },
		lastPeerRequest{ boost::posix_time::min_date_time },
		peerRequestInterval{ 1800 }, seeders(std::numeric_limits<int>::min()), 
		leechers(std::numeric_limits<int>::min()), 
		complete{ std::numeric_limits<int>::min() }, 
		incomplete{ std::numeric_limits<int>::min() }, 
		peerListUpdated{ std::make_shared<sigPeer>() }
	{
	}

	//create required url for GET request
	void trackerObj::update(trackerEvent trkEvent, std::vector<byte> clientID, 
		int port, std::string urlEncodedInfoHash, std::vector<byte> infoHash,
		long long uploaded, long long downloaded, long long remaining)
	{
		//switch case to get enumerator string
		std::string stringEvent;
		int intEvent = 0;
		switch (trkEvent)
		{
		case trackerEvent::started:
				stringEvent = "started";
				intEvent = 1;
				break;
		case trackerEvent::paused:
				stringEvent = "paused";
				intEvent = 2;
				break;
		case trackerEvent::stopped:
				stringEvent = "stopped";
				intEvent = 3;
				break;
		}

		//URL encode clientID
		std::string clientIDString(clientID.begin(), clientID.end());
		auto urlEncodedClientID = urlEncode(clientIDString);

		//compact will be 1 but client will also support non-compact response
		std::string url = trackerAddress + "?info_hash=" + urlEncodedInfoHash +
			"&peer_id=" + urlEncodedClientID + "&port=" + std::to_string(port)
			+ "&uploaded=" + std::to_string(uploaded) + "&downloaded=" +
			std::to_string(downloaded) + "&left=" + std::to_string(remaining) +
			"&event=" + std::to_string(intEvent) + "&compact=1";

		//wait until time interval has elapsed before requesting new peers
		if (trkEvent == trackerObj::trackerEvent::started &&
			boost::posix_time::second_clock::local_time() <
			(lastPeerRequest + peerRequestInterval))
		{
			return;
		}

		//parse url and check if valid
		trackerUrl parsedUrl(url);
		parsedUrl.parse();
		if (parsedUrl.isInvalidURL)
		{
			LOG_F(ERROR, "Tracker (%s) is invalid!",
				trackerAddress.c_str());
			return;
		}

		//temp vector for printing peers
		std::vector<peer> tempPeerList;

		//request url using appropriate protocol
		if (parsedUrl.protocol == trackerUrl::protocolType::http)
		{
			if (complete == std::numeric_limits<int>::min() && 
				incomplete == std::numeric_limits<int>::min())
			{
				HTTPClient httpAnnounce(parsedUrl, 1);
				complete = httpAnnounce.complete;
				incomplete = httpAnnounce.incomplete;
				//call signal to fire peerListUpdated event
				//not thread safe?
				(*peerListUpdated)(httpAnnounce.peerList);
				//add to temp for printing
				tempPeerList = httpAnnounce.peerList;
			}
			else
			{
				//scrape
				HTTPClient httpAnnounce(parsedUrl, 0);
				//announce and update if seeders/leechers values change
				if (httpAnnounce.complete != complete || 
					httpAnnounce.incomplete != incomplete)
				{
					LOG_F(INFO,
						"Tracker (%s:%s) info changed since last scrape. "
						"Switching to HTTP announce request.",
						parsedUrl.hostname.c_str(), parsedUrl.port.c_str());

					//reset tracker url target (scrape changes it)
					httpAnnounce.target = parsedUrl.target;

					httpAnnounce.dataTransmission(1);
					complete = httpAnnounce.complete;
					incomplete = httpAnnounce.incomplete;
					//call signal to fire peerListUpdated event
					//not thread safe?
					(*peerListUpdated)(httpAnnounce.peerList);
					//add to temp for printing
					tempPeerList = httpAnnounce.peerList;
				}
			}
			//handle peers, seeders, leechers, interval
			//need some event handling to scrape/announce after interval time has passed
		}
		else if (parsedUrl.protocol == trackerUrl::protocolType::udp)
		{
			//announce if first time, otherwise scrape
			if (seeders == std::numeric_limits<int>::min() &&
				leechers == std::numeric_limits<int>::min())
			{
				UDPClient udpAnnounce(parsedUrl, clientID, infoHash, uploaded, 
					downloaded, remaining, intEvent, port, 1);
				seeders = udpAnnounce.seeders;
				leechers = udpAnnounce.leechers;
				peerRequestInterval = udpAnnounce.peerRequestInterval;
				//call signal to fire peerListUpdated event
				//not thread safe?
				(*peerListUpdated)(udpAnnounce.peerList);
				//add to temp for printing
				tempPeerList = udpAnnounce.peerList;
			}
			else
			{
				UDPClient udpGen(parsedUrl, clientID, infoHash, uploaded, 
					downloaded, remaining, intEvent, port, 0);
				//announce and update if seeders/leechers values change
				if (udpGen.seeders != seeders || udpGen.leechers != leechers)
				{
					LOG_F(INFO,
						"Tracker (%s:%s) info changed since last scrape. "
						"Switching to UDP announce request.",
						parsedUrl.hostname.c_str(), parsedUrl.port.c_str());

					udpGen.dataTransmission(parsedUrl, 1);
					seeders = udpGen.seeders;
					leechers = udpGen.leechers;
					peerRequestInterval = udpGen.peerRequestInterval;
					//call signal to fire peerListUpdated event
					//not thread safe?
					(*peerListUpdated)(udpGen.peerList);
					//add to temp for printing
					tempPeerList = udpGen.peerList;
				}
			}

			//handle peers, seeders, leechers, interval
			//need some event handling to scrape/announce after interval time has passed
		}
		else
		{
			return;
		}

		//update request time
		lastPeerRequest = boost::posix_time::second_clock::local_time();

		std::cout << "\n" << "Received peer information from " << trackerAddress << "\n";
		std::cout << "Peer count:  " << tempPeerList.size() << "\n";
		std::cout << "Peers: ";
		for (auto peer : tempPeerList)
		{
			std::cout << "IP Address: " << peer.ipAddress << ", Port: " <<
				peer.port << "\n";
		}
	}

	void trackerObj::resetLastRequest()
	{
		lastPeerRequest = boost::posix_time::ptime(
			boost::posix_time::min_date_time);
	}
}