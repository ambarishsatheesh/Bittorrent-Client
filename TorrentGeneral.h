#pragma once
#include "ValueTypes.h"
#include "trackerObj.h"

#include <ctime>

class TorrentGeneral
{
public:
	std::vector<trackerObj> trackerList;
	std::string comment;
	std::string createdBy;
	//POSIX time
	std::time_t creationDate;
	std::string encoding; //unsure

	//constructor
	TorrentGeneral(const valueDictionary& torrent);
	//no default constructor - requires parameter
	TorrentGeneral() = delete;

private:
	//get general info
	std::string getComment(const valueDictionary& torrent) const;
	std::string getCreatedBy(const valueDictionary& torrent) const;
	std::time_t getCreationDate(const valueDictionary& torrent) const;
	std::string getEncoding(const valueDictionary& torrent) const;

};

