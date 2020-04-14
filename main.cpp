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

	char* filePath= argv[1];

	std::string buffer = get_file_contents(filePath);

	//std::cout << buffer << std::endl;
		
	valueDictionary torrent =
			boost::get<valueDictionary>(Decoder::decode(buffer));

	std::time_t creationDate = boost::get<integer>(torrent.at("creation date"));

	//boost::apply_visitor(bencodeVisitor(), torrent.at("info"));

	auto result = unixTime_to_ptime(creationDate);
	auto result2 = ptime_to_unixTime(result);
	
	//testing access
	valueDictionary info =
		boost::get<valueDictionary>(torrent.at("info"));
	valueList files =
		boost::get<valueList>(info.at("files"));

	valueDictionary subTorrent3;
	for (int i = 0; i < files.size(); ++i)
	{
		subTorrent3 =
			boost::get<valueDictionary>(files[i]);
	}


	boost::apply_visitor(bencodeVisitor(), subTorrent3.at("path"));

	Torrent testTorrent(torrent);

	std::cout << "created by " << testTorrent.createdBy << std::endl;
	std::cout << "created on " << testTorrent.creationDate << std::endl;

	return 0;
}