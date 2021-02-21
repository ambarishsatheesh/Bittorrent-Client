// TODO: REFACTOR TO PROPER OOP - encapsulate properly (getters/setters vs?
// TODO: Change UDP from sync to async?
// TODO: re-examine local time vs UTC for all time keeping
// TODO: make sure all class members are initialised
// TODO: event handler for peer list (for creating both new torrents and processing exsting ones)
// TODO: re-design remaining download
// TODO: implement choice between compact and non compact tracker response - make compact and id a client setting
// TODO: clean up utility functions
// TODO: fix humanReadableBytes function - doesn't round properly
// TODO: clean up error catching - currently all invalid arguments
// TODO: implement method to add trackers to torrent
// TODO: break up Peer class into smaller compositions
// TODO: test piece verification
// TODO: clean up decoder
// TODO: implement magnet link functionality
// TODO: tracker scrape (http and udp)
// TODO: multithreading

#include "loguru.h"
#include "Decoder.h"
#include "encodeVisitor.h"
#include "TorrentManipulation.h"
#include "trackerUrl.h"
#include "Client.h"

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>


using namespace Bittorrent;
using namespace torrentManipulation;
using namespace Decoder;

int main(int argc, char* argv[])
{
	//start logging to file
	const char* orderByTimeLog = "timeLog.log";
	const char* orderByThreadLog = "threadLog.log";
	loguru::init(argc, argv);
	loguru::add_file(orderByTimeLog, loguru::Append, loguru::Verbosity_MAX);

	const char* fullFilePath = argv[1];
	
	const char* testingload = "test.log";

	loadFromFile(testingload);

	bool isPrivate = false;

	std::vector<std::string> trackers;
	trackers.push_back("http://www.cplusplus.com/forum/beginner/104849/");
	trackers.push_back("https://www.google.com/");
	trackers.push_back("https://www.youtube.com/watch?v=q86g1aop6a8");
	trackers.push_back("http://www.reddit.com/");

	//Torrent temp = createNewTorrent("test", fullFilePath, isPrivate, "test comment", trackers);

	std::string buffer = loadFromFile(fullFilePath);
	if (buffer.empty())
	{
		return 0;
	}

	Client client;

	auto decodeTest = Decoder::decode(buffer);

	if (decodeTest.type() == typeid(int) && boost::get<int>(decodeTest) == 0)
	{
		std::cout << "Fail!" << "\n";
	}
	else
	{
		//print true values here
	}
	
	valueDictionary torrent = boost::get<valueDictionary>(Decoder::decode(buffer));

	Torrent udpTestTorrent = toTorrentObj(fullFilePath, torrent);
	
	auto thread1 = std::thread([&]() {
		loguru::set_thread_name("Tracker Update 1");
		LOG_F(INFO, "Starting new thread...");
		udpTestTorrent.generalData.trackerList.at(4).update(
			trackerObj::trackerEvent::started,
			client.localID, 0, udpTestTorrent.hashesData.urlEncodedInfoHash, 
			udpTestTorrent.hashesData.infoHash, 2500, 1200, 0);
		});

	auto thread2 = std::thread([&]() {
		loguru::set_thread_name("Tracker Update 2");
		LOG_F(INFO, "Starting new thread...");
		udpTestTorrent.generalData.trackerList.at(8).update(trackerObj::trackerEvent::started,
			client.localID, 0, udpTestTorrent.hashesData.urlEncodedInfoHash, udpTestTorrent.hashesData.infoHash, 2500, 1200, 0);
		});

	auto thread3 = std::thread([&]() {
		loguru::set_thread_name("Tracker Update 3");
		LOG_F(INFO, "Starting new thread...");
		udpTestTorrent.generalData.trackerList.at(11).update(
			trackerObj::trackerEvent::started,
			client.localID, 0, udpTestTorrent.hashesData.urlEncodedInfoHash, 
			udpTestTorrent.hashesData.infoHash, 2500, 1200, 0);
		});

	thread1.join();
	thread2.join();
	thread3.join();

	std::ifstream file(orderByTimeLog);
	std::string logLine;
	std::string threadName;
	std::unordered_multimap<std::string, std::string> logByThreadMap;

	while (std::getline(file, logLine)) {
		auto brack1 = logLine.find_first_of("[");
		brack1++;
		auto brack2 = logLine.find_last_of("]");
		if (brack1 != std::string::npos && brack2 != std::string::npos
			&& logLine.find("File:Line") == std::string::npos)
		{
			threadName = logLine.substr(brack1, brack2 - (brack1));
			logByThreadMap.emplace(threadName, logLine);
		}
	}

	std::ofstream outFile(orderByThreadLog, std::ofstream::out);
	for (auto thread : logByThreadMap)
	{
		outFile << thread.second << "\n";
	}

	//saveLogOrderByThread(orderByTimeLog, orderByThreadLog);


	//value tempObj = toBencodingObj(testTorrent);
	////encode and save
	////parameter needs to be adjustable
	//std::string strFilePath = "D:\\Documents\\Programming\\Bittorrent\\Bittorrent\\x64\\Release\\testencoding.torrent";
	//saveToFile(strFilePath, encode(tempObj));



	//create torrent obj
	//Torrent testTorrent = toTorrentObj(fullFilePath, torrent);


	/*std::cout << testTorrent.generalData.downloadDirectory << std::endl;
	std::cout << testTorrent.generalData.fileDirectory << std::endl;
	std::cout << testTorrent.generalData.fileName << std::endl;
	std::cout << testTorrent.fileList[0].filePath << std::endl;*/

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

	/*for (size_t i = 0; i < testTorrent.fileList.size(); ++i)
	{
		std::cout << "file name: " << testTorrent.fileList.at(i).filePath << "; ";
		std::cout << "original file size: " << testTorrent.fileList.at(i).fileSize << "; ";
		std::cout << "readable file size: " << testTorrent.fileList.at(i).readableFileSize << std::endl;
	}*/

	/*for (size_t i = 0; i < testTorrent.generalData.trackerList.size(); ++i)
	{
		std::cout << "tracker " << i << ": " << testTorrent.generalData.trackerList.at(i).trackerAddress << std::endl;
	}*/

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

