#include "Torrent.h"
#include "Decoder.h"
#include <iostream>
#include <boost/variant/get.hpp>
#include <boost/variant/get.hpp>

Torrent::Torrent(const valueDictionary& torrent)
	: decodedTorrent{ torrent }, fileList{ getFileList() },
	isPrivate{ getIsPrivate() }, generalData( decodedTorrent), piecesData( decodedTorrent, fileList ),
	hashesData(decodedTorrent), statusData( piecesData, decodedTorrent)
{}

std::vector<fileObj> Torrent::getFileList()
{
	//redo
	std::vector<fileObj> vec;
	//std::string test = boost::get<std::string>(decodedTorrent.at("info"));
	//std::cout << test << std::endl;
	return vec;
}

bool Torrent::getIsPrivate()
{
	valueDictionary info =
		boost::get<valueDictionary>(decodedTorrent.at("info"));
	if (info.count("private"))
	{
		if (boost::get<integer>(info.at("private")) == 1)
		{
			return true;
		}
		return false;
	}
	return false;
}