#include "trackerObj.h"
#include "Decoder.h"
#include "Utility.h"


namespace Bittorrent
{
	using namespace utility;

	trackerObj::trackerObj()
		: trackerAddress{ "" },
		lastPeerRequest{ boost::posix_time::ptime(boost::posix_time::min_date_time) },
		peerRequestInterval{ 1800 }, seeders(std::numeric_limits<int>::min()), 
		leechers(std::numeric_limits<int>::min()), 
		complete{ std::numeric_limits<int>::min() }, 
		incomplete{ std::numeric_limits<int>::min() }
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
			boost::posix_time::second_clock::universal_time() <
			(lastPeerRequest + peerRequestInterval))
		{
			return;
		}

		//parse url
		trackerUrl parsedUrl(url);

		//request url using appropriate protocol
		if (parsedUrl.protocol == trackerUrl::protocolType::http)
		{
			if (complete == std::numeric_limits<int>::min() && 
				incomplete == std::numeric_limits<int>::min())
			{
				HTTPClient httpAnnounce(parsedUrl, 1);
				complete = httpAnnounce.complete;
				incomplete = httpAnnounce.incomplete;
				//not thread safe
				for (auto singlePeer : httpAnnounce.peerList)
				{
					peerList.push_back(singlePeer);
				}
			}
			else
			{
				HTTPClient httpAnnounce(parsedUrl, 0);
				//announce and update if seeders/leechers values change
				if (httpAnnounce.complete != complete || 
					httpAnnounce.incomplete != incomplete
					|| httpAnnounce.peerList != peerList)
				{
					std::cout << "\n" <<
						"Tracker info changed. Switching to announce request."
						<< "\n";
					httpAnnounce.dataTransmission(parsedUrl, 1);
					complete = httpAnnounce.complete;
					incomplete = httpAnnounce.incomplete;
					//not thread safe
					for (auto singlePeer : httpAnnounce.peerList)
					{
						peerList.push_back(singlePeer);
					}
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
					downloaded, remaining, intEvent, 1);
				//not thread safe
				for (auto singlePeer : udpAnnounce.peerList)
				{
					peerList.push_back(singlePeer);
				}
				seeders = udpAnnounce.seeders;
				leechers = udpAnnounce.leechers;
				peerRequestInterval = udpAnnounce.peerRequestInterval;
			}
			else
			{
				UDPClient udpGen(parsedUrl, clientID, infoHash, uploaded, 
					downloaded, remaining, intEvent, 0);
				//announce and update if seeders/leechers values change
				if (udpGen.seeders != seeders || udpGen.leechers != leechers
					|| udpGen.peerList != peerList)
				{
					std::cout << "\n" << 
						"Tracker info changed. Switching to announce request." 
						<< "\n";
					udpGen.dataTransmission(parsedUrl, 1);
					//not thread safe
					for (auto singlePeer : udpGen.peerList)
					{
						peerList.push_back(singlePeer);
					}
					seeders = udpGen.seeders;
					leechers = udpGen.leechers;
					peerRequestInterval = udpGen.peerRequestInterval;
				}
			}

			//handle peers, seeders, leechers, interval
			//need some event handling to scrape/announce after interval time has passed
		}
		else
		{
			throw std::invalid_argument("Invalid protocol for trackers!");
		}

		//update request time
		lastPeerRequest = boost::posix_time::second_clock::universal_time();

		std::cout << "\n" << "Received peer information from " << trackerAddress << "\n";
		std::cout << "Peer count:  " << peerList.size() << "\n";
		std::cout << "Peers: " << peerList.size() << "\n";
		for (auto peer : peerList)
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