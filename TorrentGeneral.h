#pragma once
#include "ValueTypes.h"
#include "trackerObj.h"
#include "Utility.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/posix_time/conversion.hpp"

#include <ctime>

class TorrentGeneral
{
public:
	std::string fileName;
	std::string downloadDirectory; //TODO: check how this works
	std::vector<trackerObj> trackerList;
	std::string comment;
	std::string createdBy;
	boost::posix_time::ptime creationDate;
	std::string encoding; //unsure
	bool isPrivate;

	//constructor
	TorrentGeneral(const char* fullFilePath);
	//no default constructor - requires parameter
	TorrentGeneral() = delete;

	//fill general info
	void torrentToGeneralData(const char* fullFilePath, const valueDictionary& torrent);
	valueDictionary generalDataToDictionary(valueDictionary& dict);

private:


};

