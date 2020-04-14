#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <memory>
#include "fileObj.h"
#include "trackerObj.h"
#include "ValueTypes.h"

using byte = uint8_t;

class Torrent
{
public:

	valueDictionary decodedTorrent;

	//general info
	//std::vector<trackerObj> trackerList;
	std::string comment;
	std::string createdBy;
	//POSIX time
	std::time_t creationDate;
	std::string encoding; //unsure

	//file info
	std::vector<fileObj> fileList;
	//std::string fileName;
	//bool isPrivate;
	//int pieceSize;
	//int blockSize;
	long totalSize;
	//std::vector<std::vector<byte>> pieceHashes;

	////hashes
	//std::vector<byte> infoHash;
	//std::string hexStringInfoHash;
	//std::string urlSafeStringInfoHash;

	Torrent();
	Torrent(valueDictionary torrent);

private:
	//get general info
	std::string getComment();
	std::string getCreatedBy();
	std::time_t getCreationDate();
	std::string getEncoding();

	//get file info
	std::vector<fileObj> getFileList();
	long getTotalSize();



};

