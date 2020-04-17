#include "Torrent.h"
#include "Decoder.h"
#include <iostream>
#include <boost/variant/get.hpp>
#include <boost/variant/get.hpp>

Torrent::Torrent(const char* fullFilePath, const valueDictionary& torrent)
	: generalData(fullFilePath), fileList{ }, piecesData(), hashesData(),
	statusData( piecesData, torrent)
{
	
}

Torrent::Torrent(const char* fullFilePath)
	: generalData(fullFilePath), fileList{ }, piecesData(), hashesData(), 
	statusData(piecesData)
{

}

void Torrent::setFileList(const valueDictionary& torrent)
{
	valueDictionary info = boost::get<valueDictionary>(torrent.at("info"));
	fileObj resObj;
	//if torrent contains only one file
	if (info.count("name") && info.count("length"))
	{
		resObj.filePath = boost::get<std::string>(info.at("name"));
		resObj.fileSize = static_cast<long long>(boost::get<integer>(info.at("length")));
		fileList.push_back(resObj);
	}
	//if torrent contains multiple files
	else if (info.count("files"))
	{
		long long runningOffset = 0;
		valueList list = boost::get<valueList>(info.at("files"));
		for (size_t i = 0; i < list.size(); ++i)
		{
			valueDictionary fileData =
				boost::get<valueDictionary>(list.at(i));
			if (fileData.empty() || !fileData.count("length") || !fileData.count("path"))
			{
				throw std::invalid_argument("Error: incorrect file specification!");
			}
			//get file size and offset
			if (fileData.count("length"))
			{
				long long size = static_cast<long>(boost::get<integer>(fileData.at("length")));
				resObj.fileSize = size;
				resObj.setReadableFileSize();
				resObj.fileOffset = runningOffset;
				//add to running size total
				runningOffset += size;
			}
			//get file path
			if (fileData.count("path"))
			{
				valueList pathList = boost::get<valueList>(fileData.at("path"));
				std::string fullPath;
				for (auto path : pathList)
				{
					fullPath += boost::get<std::string>(path);
					fullPath += "/";
				}
				fullPath.erase(fullPath.find_last_of("/"));
				resObj.filePath = fullPath;
			}
			fileList.push_back(resObj);
		}
	}
	else
	{
		throw std::invalid_argument("Error: no files specified in torrent!");
	}
	//std::string test = boost::get<std::string>(decodedTorrent.at("info"));
	//std::cout << test << std::endl;
}
