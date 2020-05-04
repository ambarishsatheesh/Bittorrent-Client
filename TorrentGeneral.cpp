#include "TorrentGeneral.h"
#include "boost/variant/get.hpp"

namespace Bittorrent
{
	using namespace utility;

	TorrentGeneral::TorrentGeneral(const char* fullFilePath)
		: fileName{ "" },
		downloadDirectory{ "" },
		comment{ "" }, createdBy{ "" },
		creationDate{ },
		encoding{ "" }, isPrivate{ false }
	{

	}

	valueDictionary TorrentGeneral::generalDataToDictionary(valueDictionary& dict)
	{
		//trackers
		assert(!trackerList.empty());
		if (trackerList.size() == 1)
		{
			dict.emplace("announce", trackerList.at(0).trackerAddress);
		}
		else
		{
			valueList trackerValueList;
			for (size_t i = 0; i < trackerList.size(); ++i)
			{
				trackerValueList.push_back(trackerList.at(i).trackerAddress);
			}
			dict.emplace("announce-list", trackerValueList);
		}

		if (!comment.empty())
		{
			dict.emplace("comment", comment);
		}

		if (!createdBy.empty())
		{
			dict.emplace("created by", createdBy);
		}

		if (!encoding.empty())
		{
			dict.emplace("encoding", encoding);
		}

		bool privateCheck = isPrivate ? 1 : 0;
		dict.emplace("private", privateCheck);

		//if creation date is not empty/if initialised to non-default value
		if (!creationDate.is_not_a_date_time())
		{
			dict.emplace("creation date", boost::posix_time::to_time_t(creationDate));
		}

		return dict;
	}

	void TorrentGeneral::torrentToGeneralData(const char* fullFilePath, const valueDictionary& torrent)
	{
		fileName = getFileName(fullFilePath);
		downloadDirectory = getFileDirectory(fullFilePath);

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
			creationDate = boost::posix_time::from_time_t(boost::get<long long>
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
			if (boost::get<long long>(info.at("private")) == 1)
			{
				isPrivate = true;
			}
			isPrivate = false;
		}
		isPrivate = false;

	}

	void TorrentGeneral::updateTrackers(trackerObj::trackerEvent trkEvent, int id,
		int port, std::string urlEncodedInfoHash, long long uploaded,
		long long downloaded, long long remaining)
	{
		for (auto tracker : trackerList)
		{
			tracker.update(trkEvent, id, port, urlEncodedInfoHash, uploaded,
				downloaded, remaining);
		}
	}

	void TorrentGeneral::resetTrackersLastRequest()
	{
		for (auto tracker : trackerList)
		{
			tracker.resetLastRequest();
		}
	}
}