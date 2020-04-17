#include "TorrentGeneral.h"
#include "boost/variant/get.hpp"

using namespace utility;

TorrentGeneral::TorrentGeneral(const char* fullFilePath)
	: fileName{ "" },
	downloadDirectory{ "" },
	comment{ "" }, createdBy{ "" },
	creationDate{ },
	encoding{ "" }, isPrivate{ false }
{
	
}

void TorrentGeneral::torrentToGeneralData(const char* fullFilePath, const valueDictionary& torrent)
{
	fileName = getFileName(fullFilePath);
	downloadDirectory = getFileDirectory(fullFilePath);

	//get trackers
	trackerObj tracker;
	if (torrent.count("announce-list"))
	{
		valueList list = boost::get<valueList>(torrent.at("announce-list"));
		for (size_t i = 0; i < list.size(); ++i)
		{
			valueList subList = boost::get<valueList>(list.at(i));
			tracker.trackerAddress = boost::get<std::string>(subList.at(0));
			trackerList.push_back(tracker);
		}
	}
	else if (torrent.count("announce"))
	{
		tracker.trackerAddress = boost::get<std::string>(torrent.at("announce"));
		trackerList.push_back(tracker);
	}
	else
	{
		throw std::invalid_argument("Error: no trackers specified in torrent!");
	}

	if (torrent.count("comment"))
	{
		comment = boost::get<std::string>(torrent.at("comment"));
	}

	if (torrent.count("created by"))
	{
		createdBy = boost::get<std::string>(torrent.at("created by"));
	}

	if (torrent.count("creation date"))
	{
		creationDate = boost::posix_time::from_time_t(boost::get<integer>
			(torrent.at("creation date")));
	}

	if (torrent.count("encoding"))
	{
		encoding = boost::get<std::string>(torrent.at("encoding"));
	}

	valueDictionary info =
		boost::get<valueDictionary>(torrent.at("info"));
	if (info.count("private"))
	{
		if (boost::get<integer>(info.at("private")) == 1)
		{
			isPrivate = true;
		}
		isPrivate = false;
	}
	isPrivate = false;

}
