#include "TorrentGeneral.h"
#include "boost/variant/get.hpp"

TorrentGeneral::TorrentGeneral(const valueDictionary& torrent)
	: comment{ getComment(torrent) }, createdBy{ getCreatedBy(torrent) },
	creationDate{ getCreationDate(torrent) }, 
	encoding{ getEncoding(torrent) }
{}

std::string TorrentGeneral::getComment(const valueDictionary& torrent) const
{
	if (torrent.count("comment"))
	{
		return boost::get<std::string>(torrent.at("comment"));
	}
	return "N/A";
}

std::string TorrentGeneral::getCreatedBy(const valueDictionary& torrent) const
{
	return boost::get<std::string>(torrent.at("created by"));
}

std::time_t TorrentGeneral::getCreationDate(const valueDictionary& torrent) const
{
	return static_cast<std::time_t>
		(boost::get<integer>(torrent.at("creation date")));
}

std::string TorrentGeneral::getEncoding(const valueDictionary& torrent) const
{
	if (torrent.count("encoding"))
	{
		return boost::get<std::string>(torrent.at("encoding"));
	}
	return "N/A";
}
