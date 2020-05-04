#include "trackerObj.h"


namespace Bittorrent
{

	trackerObj::trackerObj()
		: trackerAddress{ "" },
		lastPeerRequest{ boost::posix_time::ptime(boost::posix_time::min_date_time) },
		peerRequestInterval{ 1800 }
	{
	}

	//create required url for GET request
	void trackerObj::update(trackerEvent trkEvent, int id,
		int port, std::string urlEncodedInfoHash, long long uploaded,
		long long downloaded, long long remaining)
	{
		//switch case to get enumerator string
		std::string stringEvent;
		switch (trkEvent)
		{
		case trackerEvent::started:
				stringEvent = "started";
				break;
		case trackerEvent::paused:
				stringEvent = "paused";
				break;
		case trackerEvent::stopped:
				stringEvent = "stopped";
				break;
		}

		//wait until time interval has elapsed before requesting new peers
		if (trkEvent == trackerObj::trackerEvent::started &&
			boost::posix_time::second_clock::universal_time() <
			(lastPeerRequest + peerRequestInterval))
		{
			return;
		}

		lastPeerRequest = boost::posix_time::second_clock::universal_time();

		//compact defaulted to 1 for now
		std::string url = trackerAddress + "?info_hash=" + urlEncodedInfoHash +
			"&peer_id=" + std::to_string(id) + "&port=" + std::to_string(port) + "&uploaded=" +
			std::to_string(uploaded) + "&downloaded=" +
			std::to_string(downloaded) + "&left=" + std::to_string(remaining) +
			"&event=" + stringEvent + "&compact=1";

		//request url
		httpRequest(url);
	}

	void trackerObj::resetLastRequest()
	{
		lastPeerRequest = boost::posix_time::ptime(boost::posix_time::min_date_time);
	}

	void trackerObj::httpRequest(std::string url)
	{

	}
}