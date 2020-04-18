#pragma once

#include "Torrent.h"
#include "Utility.h"
#include "encodeVisitor.h"

#include "boost/filesystem.hpp"


using namespace utility;

//need to declare inline for each function?

namespace torrentManipulation
{
	//encode torrent object
	std::string encode(const value& torrent)
	{
		return boost::apply_visitor(encodeVisitor(), torrent);
	}

	//create complete torrent object from bencoded data
	Torrent toTorrentObj(const char* fullFilePath, const valueDictionary& torrent)
	{
		if (torrent.empty())
		{
			throw std::invalid_argument("Error: not a torrent file!");
		}
		if (!torrent.count("info"))
		{
			throw std::invalid_argument("Error: missing info section in torrent!");
		}

		//create torrent
		Torrent newTorrent(fullFilePath, torrent);
		//fill file list data
		newTorrent.setFileList(torrent);
		//fill pieces data
		newTorrent.generalData.torrentToGeneralData(fullFilePath, torrent);
		newTorrent.piecesData.torrentToPiecesData(newTorrent.fileList, torrent);
		newTorrent.hashesData.torrentToHashesData(torrent);

		return newTorrent;
	}


	//create complete torrent object (pre-processing for encoding)
	value toBencodingObj(const Torrent& torrent)
	{

	}

	//create torrent with default empty tracker list and comment
	Torrent createNewTorrent(std::string fileName, const char* path, bool& isPrivate, 
		const std::string& comment = "", std::vector<trackerObj> trackerList = {})
	{
		Torrent createdTorrent(path);

		// General data
		createdTorrent.generalData.fileName = fileName;
		createdTorrent.generalData.comment = comment;
		createdTorrent.generalData.createdBy = "myBittorrent";
		createdTorrent.generalData.creationDate = 
			boost::posix_time::second_clock::local_time();
		createdTorrent.generalData.encoding = "UTF-8";
		createdTorrent.generalData.isPrivate = isPrivate;
		createdTorrent.generalData.trackerList = trackerList;
		
		// File data 
		std::vector<fileObj> files;
		fileObj resObj;

		// query file/folder existence and sizes
		if (boost::filesystem::exists(path))
		{
			if (boost::filesystem::is_regular_file(path))
			{
				resObj.filePath = getFileNameAndExtension(path);
				resObj.fileSize = boost::filesystem::file_size(path);
				resObj.setReadableFileSize();
				files.push_back(resObj);
			}
			else if (boost::filesystem::is_directory(path))
			{
				boost::filesystem::recursive_directory_iterator end_itr;

				std::cout << path << std::endl;

				// cycle through the directory and push file path and size into fileObj
				for (boost::filesystem::recursive_directory_iterator itr(path); 
					itr != end_itr; ++itr)
				{
					if (is_regular_file(itr->path())) {
						//remove parent directory
						std::string curFilePath = itr->path().string();
						const size_t pos = curFilePath.find(path) + strlen(path);

						resObj.fileSize = boost::filesystem::file_size(curFilePath);
						resObj.filePath = curFilePath.substr(pos);
						resObj.setReadableFileSize();
						files.push_back(resObj);
					}
				}
			}
			else
			{
				throw std::invalid_argument
				("Provided path is neither a file or directory!");
			}
		}
		else
		{
			throw std::invalid_argument("Provided path does not exist!");
		}

		createdTorrent.fileList = files;

		
		// Piece data
		long long totalSize = 0;

		for (auto file : createdTorrent.fileList)
		{
			totalSize += file.fileSize;
		}

		createdTorrent.piecesData.totalSize = totalSize;
		createdTorrent.piecesData.pieceSize = recommendedPieceSize(totalSize);
		
		//create object for encoding
		value tempObj = toBencodingObj(createdTorrent);
		//encode and save
		std::string strFilePath = path;
		saveToFile(strFilePath, encode(tempObj));

		return createdTorrent;
	}



}