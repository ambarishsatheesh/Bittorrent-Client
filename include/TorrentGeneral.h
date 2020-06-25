#pragma once
#include "ValueTypes.h"
#include "trackerObj.h"
#include "Utility.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/container_hash/hash.hpp>

#include <ctime>
#include <functional>
#include <unordered_set>

namespace std
{
    using namespace Bittorrent;

    template <>
    struct hash<peer>
    {
        std::size_t operator()(const peer& p) const
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, p.ipAddress);
            boost::hash_combine(seed, p.port);
            boost::hash_combine(seed, p.peerID);
            return seed;
        }
    };
}

namespace Bittorrent
{
	class TorrentGeneral
	{
	public:
		std::string fileName;
		std::string downloadDirectory;
		std::vector<trackerObj> trackerList;
		std::string comment;
		std::string createdBy;
		boost::posix_time::ptime creationDate;
		std::string encoding; //unsure
		bool isPrivate;
		std::string urlEncodedClientID;
        std::unordered_set<peer> uniquePeerList;

		//fill general info
		void torrentToGeneralData(const char* fullFilePath, 
			const valueDictionary& torrent);
		valueDictionary generalDataToDictionary(valueDictionary& dict);

		//tracker processing
        void updateTrackers(TorrentStatus::currentStatus currentState,
			std::vector<byte> clientID,
			int port, std::string urlEncodedInfoHash, std::vector<byte> infoHash,
			long long uploaded, long long downloaded, long long remaining);

		void resetTrackersLastRequest();

        //update peer list from tracker response
        void getPeerList();

		//constructor
        TorrentGeneral();

	private:


	};

}
