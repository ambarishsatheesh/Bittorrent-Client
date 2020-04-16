// TODO: refactor torrent class into subclasses - too many parameters (long member initialisation list)
// TODO: clean up utility functions
// TODO: fix humanReadableBytes function - doesn't round properly
// TODO: clean up error catching - currently all invalid arguments

#include "Decoder.h"
#include "bencodeVisitor.h"
#include "Utility.h"
#include "Torrent.h"

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

using namespace utility;

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "Too few arguments: needs a file name..\n";
		return -1;
	}

	const char* fullFilePath = argv[1];

	std::string buffer = get_file_contents(fullFilePath);

	valueDictionary torrent =
			boost::get<valueDictionary>(Decoder::decode(buffer));

	//create torrent obj
	Torrent testTorrent(fullFilePath, torrent);

	std::cout << testTorrent.generalData.downloadDirectory << std::endl;
	std::cout << testTorrent.generalData.fileName << std::endl;

	//std::time_t creationDate = boost::get<integer>(torrent.at("creation date"));

	//boost::apply_visitor(bencodeVisitor(), torrent.at("info"));

	//auto result = unixTime_to_ptime(creationDate);
	//auto result2 = ptime_to_unixTime(result);
	
	////testing access
	//valueDictionary info =
	//	boost::get<valueDictionary>(torrent.at("info"));
	//valueList files =
	//	boost::get<valueList>(info.at("files"));

	//valueDictionary subTorrent3;
	//for (int i = 0; i < files.size(); ++i)
	//{
	//	subTorrent3 =
	//		boost::get<valueDictionary>(files[i]);
	//	valueList final = boost::get<valueList>(subTorrent3.at("path"));
	//	std::cout << boost::get<std::string>(final[0]) << std::endl;
	//}

	for (size_t i = 0; i < testTorrent.fileList.size(); ++i)
	{
		std::cout << "file name: " << testTorrent.fileList.at(i).filePath << "; ";
		std::cout << "original file size: " << testTorrent.fileList.at(i).fileSize << "; ";
		std::cout << "readable file size: " << testTorrent.fileList.at(i).readableFileSize << std::endl;
	}

	for (size_t i = 0; i < testTorrent.generalData.trackerList.size(); ++i)
	{
		std::cout << "tracker " << i << ": " << testTorrent.generalData.trackerList.at(i).trackerAddress << std::endl;
	}

	//std::string pieces =
	//	boost::get<std::string>(info.at("pieces"));


	//std::cout << pieces << std::endl;
	//std::cout << urlEncode(pieces) << std::endl;

	//boost::apply_visitor(bencodeVisitor(), subTorrent3.at("path"));

	//std::cout << "created by: " << testTorrent.generalData.createdBy << std::endl;
	//std::cout << "created on: " << testTorrent.generalData.creationDate << std::endl;
	//std::cout << "is private: " << testTorrent.generalData.isPrivate << std::endl;
	//std::cout << "encoding: " << testTorrent.generalData.encoding << std::endl;

	//std::cout << humanReadableBytes(829083) <<std::endl;

	return 0;
}