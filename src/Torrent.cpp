#include "Torrent.h"
#include <iostream>
#include <boost/variant/get.hpp>
#include <boost/variant/get.hpp>
#include <boost/bind.hpp>

namespace Bittorrent
{
	//add tracker processing
    Torrent::Torrent()
        : generalData(), piecesData(), hashesData(),
          sig_addPeer{
              std::make_shared<boost::signals2::signal<void(peer*, Torrent*)>>()},
        statusData(std::make_shared<TorrentPieces>(piecesData))
    {
        //connect peer-list-updated signal
        generalData.sig_peersUpdated->connect(
                    boost::bind(&Torrent::handlePeerListUpdated,
                                this, _1));
    }

	valueDictionary Torrent::filesToDictionary(valueDictionary& dict)
	{
		if (fileList.empty())
		{
			throw std::invalid_argument("Error: no files in torrent!");
		}
		else if (fileList.size() == 1)
		{
			dict.emplace("name", fileList.at(0).filePath);
			dict.emplace("length", fileList.at(0).fileSize);
		}
		else
		{
			valueList multiFilesList;
			std::string majorPath;
			for (size_t i = 0; i < fileList.size(); ++i)
			{
				valueDictionary fileDataDict;
				valueList subPathList;
				//add file size
				fileDataDict.emplace("length", fileList.at(i).fileSize);

				//format file paths (recursive folders check)
				majorPath = fileList.at(i).filePath;
				auto found = majorPath.find_first_of("/\\");
				if (found == std::string::npos)
				{
					subPathList.push_back(majorPath);
				}
				else
				{
					std::string subPath;
					while (found != std::string::npos)
					{
						subPath = majorPath.substr(0, found);
						majorPath = majorPath.substr(found + 1);
						subPathList.push_back(subPath);
						found = majorPath.find_first_of("/\\");
						if (found == std::string::npos)
						{
							subPathList.push_back(majorPath);
							break;
						}
					}
				}
				//add path to subdict
				fileDataDict.emplace("path", subPathList);
				//add subdict to file list
				multiFilesList.push_back(fileDataDict);
			}
			//add file list to parent files dict
			dict.emplace("files", multiFilesList);
		}
		return dict;
	}

    void Torrent::setFileList(const valueDictionary& torrent)
	{
		valueDictionary info = boost::get<valueDictionary>(torrent.at("info"));
		fileObj resObj;
		//if torrent contains only one file
		if (info.count("name") && info.count("length"))
		{
			resObj.filePath = boost::get<std::string>(info.at("name"));
			resObj.fileSize = static_cast<long long>(boost::get<long long>(info.at("length")));
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
					long long size = static_cast<long>(boost::get<long long>(fileData.at("length")));
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

    void Torrent::handlePeerListUpdated(peer* singlePeer)
    {
        if (sig_addPeer->empty())
        {
            sig_addPeer->operator()(singlePeer, this);
        }
    }

	std::shared_ptr<Torrent> Torrent::getPtr()
	{
		return shared_from_this();
	}
}
