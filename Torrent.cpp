#include "Torrent.h"
#include "Decoder.h"
#include <iostream>
#include <boost/variant/get.hpp>

Torrent::Torrent(){}

Torrent::Torrent(valueDictionary torrent)
	: decodedTorrent{ torrent }, createdBy{ getCreatedBy() }, 
	creationDate{ getCreationDate() }, fileList{ getFileList() }, 
	totalSize{ getTotalSize() } {}

std::vector<fileObj> Torrent::getFileList()
{
	//redo
	std::vector<fileObj> vec;
	//std::string test = boost::get<std::string>(decodedTorrent.at("info"));
	//std::cout << test << std::endl;
	return vec;
}

std::string Torrent::getComment()
{
	return boost::get<std::string>(decodedTorrent.at("comment"));
}

std::string Torrent::getEncoding()
{
	return boost::get<std::string>(decodedTorrent.at("encoding"));
}

long Torrent::getTotalSize()
{
	long total = 0;
	for (auto file : fileList)
	{
		total += file.fileSize;
	}
	return total;
}

std::string Torrent::getCreatedBy()
{
	return boost::get<std::string>(decodedTorrent.at("created by"));
}

std::time_t Torrent::getCreationDate()
{
	return static_cast<std::time_t>(boost::get<integer>(decodedTorrent.at("creation date")));
}