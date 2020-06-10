#pragma once
#include "ValueTypes.h"
#include "trackerObj.h"
#include "Utility.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

#include <ctime>


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


		//fill general info
		void torrentToGeneralData(const char* fullFilePath, 
			const valueDictionary& torrent);
		valueDictionary generalDataToDictionary(valueDictionary& dict);

		//tracker processing
		void updateTrackers(trackerObj::trackerEvent trkEvent, 
			std::vector<byte> clientID,
			int port, std::string urlEncodedInfoHash, std::vector<byte> infoHash,
			long long uploaded, long long downloaded, long long remaining);
		void resetTrackersLastRequest();

		//constructor
        TorrentGeneral();

	private:


	};

}
