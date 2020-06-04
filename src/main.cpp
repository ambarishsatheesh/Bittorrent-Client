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
#include <set>


using namespace Bittorrent;
using namespace torrentManipulation;
using namespace Decoder;

int main(int argc, char* argv[])
{
	//start logging to file
	loguru::init(argc, argv);
	loguru::add_file("log.log", loguru::Append, loguru::Verbosity_MAX);

	//std::cout << "Welcome to my test Bittorrent client!" << "\n" << "\n";

	//std::cout << "1. Choose a torrent file and start downloading" << "\n";
	//std::cout << "2. Create a new torrent and start seeding" << "\n" << "\n";

	//std::string functionInputStr = "";
	//bool isValidInput = false;

	//while (!isValidInput)
	//{
	//	std::cout << "\n" << "Please enter the number associated with the action you wish to " <<
	//		"perform:" << "\n" << "\n";

	//	std::cin >> functionInputStr;

	//	if (functionInputStr == "1" || functionInputStr == "2")
	//	{
	//		std::cout << "\n" << "\n";
	//		isValidInput = true;
	//	}
	//}
	//isValidInput = false;

	////need to declare this outside to it can be used out of local scope
	//std::string sourceFilePath = "";
	//std::string targetPath = "";
	//const char* pathPtr = "";
	//std::set<std::string> trackerSet;
	//bool isPrivate = 0;
	//std::string comment = "";
	//std::string name = "";

	//if (functionInputStr == "1")
	//{
	//	while (!isValidInput)
	//	{
	//		std::cout << "\n" << "Please enter the torrent file path " <<
	//			"(including file name and extension) to begin downloading:"
	//			<< "\n" << "\n";

	//		std::cin >> sourceFilePath;

	//		pathPtr = sourceFilePath.c_str();

	//		if (boost::filesystem::exists(pathPtr))
	//		{
	//			if (boost::filesystem::is_regular_file(pathPtr))
	//			{
	//				boost::filesystem::path p(sourceFilePath);
	//				if (p.extension().string() == ".torrent")
	//				{
	//					std::cout << "Torrent file found." << "\n";

	//					isValidInput = true;
	//				}
	//				else
	//				{
	//					std::cout << "\n" << "You did not provide a torrent file! " << "\n";
	//				}
	//			}
	//			else
	//			{
	//				std::cout << "\n" << "You provided a directory path! " << "\n";
	//			}
	//		}
	//		else
	//		{
	//			std::cout << "\n" << "This path does not exist! " << "\n";
	//		}
	//	}

	//	isValidInput = false;
	//		
	//}
	//else if (functionInputStr == "2")
	//{
	//	//source path input
	//	while (!isValidInput)
	//	{
	//		std::cout << "Please enter the path of the file or folder " <<
	//			"you wish to create a torrent of:" << "\n" << "\n";

	//		std::cin >> sourceFilePath;

	//		pathPtr = sourceFilePath.c_str();

	//		if (boost::filesystem::exists(pathPtr))
	//		{
	//			if (boost::filesystem::is_directory(pathPtr))
	//			{
	//				//get parent folder and store as name
	//				if (sourceFilePath.back() == '\\')
	//				{
	//					boost::filesystem::path p(sourceFilePath);
	//					name =  p.parent_path().filename().string();
	//				}
	//				else
	//				{
	//					sourceFilePath += '\\';
	//					boost::filesystem::path p(sourceFilePath);
	//					name = p.parent_path().filename().string();
	//				}

	//				std::cout << "Folder successfully processed." << "\n";
	//				isValidInput = true;
	//			}
	//			else
	//			{
	//				std::cout << "File successfully processed." << "\n" << "\n";
	//				isValidInput = true;
	//			}
	//		}
	//		else
	//		{
	//			std::cout << "\n" << "This path does not exist! " <<
	//				"Please enter in a valid file/folder path." << "\n" << "\n";
	//		}
	//	}

	//	isValidInput = false;

	//	//target path input
	//	while (!isValidInput)
	//	{
	//		std::cout << "Please enter a target file path to save to:" 
	//			<< "\n" << "\n";

	//		std::cin >> targetPath;

	//		const char* targetPtr = targetPath.c_str();

	//		auto targetDirectory = getFileDirectory(targetPtr);

	//		boost::filesystem::path p(targetPath);

	//		if (boost::filesystem::is_directory(targetDirectory) &&
	//			boost::filesystem::exists(targetDirectory))
	//		{
	//			if (targetDirectory == getFileDirectory(pathPtr))
	//			{
	//				std::cout << "\n" << "Target torrent file cannot be " <<
	//					"in the same location as the source file." << "\n";
	//			}
	//			if (boost::filesystem::is_directory(targetPath))
	//			{
	//				std::cout << "Target needs to be a file!" << "\n";
	//			}
	//			if (p.extension().string() == ".torrent")
	//			{
	//				std::cout << "Target path successfully set to " 
	//					<< targetPath << "\n" << "\n";

	//				isValidInput = true;
	//			}
	//		}
	//		else
	//		{
	//			std::cout << "\n" << "Invalid file path! "
	//				<< "(Tip: Make sure the extension is \".torrent\")" 
	//				<< "\n";
	//		}
	//	}

	//	isValidInput = false;

	//	//tracker input
	//	while (!isValidInput)
	//	{
	//		std::string trackerInput = "";

	//		std::cout << "Please enter a tracker one by one and enter \"done\" " <<
	//			"when complete. Type \"remove\" to remove a tracker." << "\n" << "\n";

	//		std::cin >> trackerInput;

	//		std::string trackerCriteria1 = "udp://";
	//		std::string trackerCriteria2 = "http://";

	//		if (trackerInput == "done" && !trackerSet.empty())
	//		{
	//			std::cout << "\n" << "Trackers added: \n";
	//			for (auto i : trackerSet)
	//			{
	//				std::cout << i << "\n";
	//			}
	//			std::cout << "\n" << "\n";

	//			isValidInput = true;
	//		}
	//		else if (trackerInput == "done" && trackerSet.empty())
	//		{
	//			std::cout << "\n" << "No trackers have been added. " <<
	//				"At least one tracker is required." << "\n";
	//		}
	//		else if (trackerInput.find(trackerCriteria1) == 0 ||
	//			trackerInput.find(trackerCriteria2) == 0)
	//		{
	//			if (trackerSet.insert(trackerInput).second == false)
	//			{
	//				std::cout << "\n" << "Tracker already added to torrent!" 
	//					<< "\n";
	//			}
	//			else
	//			{
	//				std::cout << "\n" << "Tracker added to torrent!" << "\n";
	//				std::cout << "\n" << "Current tracker list: \n";

	//				for (auto i : trackerSet)
	//				{
	//					std::cout << i << "\n";
	//				}
	//				std::cout << "\n";
	//			}
	//		}
	//		else if (trackerInput.find("https") == 0)
	//		{
	//			std::cout << "\n" << "This Bittorrent client does not support HTTPS "
	//				<< "trackers." << "\n";
	//		}
	//		else if (trackerInput == "remove")
	//		{
	//			if (trackerSet.empty())
	//			{
	//				std::cout << "\n" << "No trackers to remove. " << "\n";
	//				continue;
	//			}

	//			std::cout << "\n" << "Enter the tracker to remove:" << "\n";

	//			std::cin >> trackerInput;
	//			if (trackerSet.count(trackerInput))
	//			{
	//				trackerSet.erase(trackerInput);

	//				std::cout << "\n" << "Tracker removed from torrent!" << "\n";
	//				std::cout << "\n" << "Current tracker list: \n";

	//			}
	//			else
	//			{
	//				std::cout << "\n" << "Remove failed. That tracker "
	//					<< "had not been previously added." << "\n";
	//			}

	//			std::cout << "\n" << "Current tracker list: \n";
	//			for (auto i : trackerSet)
	//			{
	//				std::cout << i << "\n";
	//			}
	//			std::cout << "\n";
	//		}
	//		else
	//		{
	//			std::cout << "\n" << "Invalid tracker protocol! (Tip: Tracker needs to begin "
	//				<< "with \"udp://\" or \"http://\")" << "\n";
	//		}
	//	}

	//	isValidInput = false;

	//	//private torrent input
	//	while (!isValidInput)
	//	{
	//		std::string isPrivateStr = "";

	//		std::cout << "Please enter \"true\" if the torrent should " <<
	//			"only use these trackers, and \"false\" if DHT/PEX can be used." 
	//			<< "\n" << "\n";
	//		
	//		std::cin >> isPrivateStr;

	//		if (isPrivateStr == "true" || isPrivateStr == "false"
	//			|| isPrivateStr == "0" || isPrivateStr == "1")
	//		{
	//			if (isPrivateStr == "true" || isPrivateStr == "1")
	//			{
	//				isPrivate = true;
	//				isValidInput = true;
	//				std::cout << "Torrent status set to private." << "\n" << "\n";
	//			}
	//			else
	//			{
	//				isPrivate = false;
	//				isValidInput = true;
	//				std::cout << "Torrent status set to public." << "\n" << "\n";
	//			}
	//		}
	//		else
	//		{
	//			std::cout << "\n" << "Invalid input." << "\n";
	//		}
	//	}

	//	isValidInput = false;

	//	//comment input
	//	while (!isValidInput)
	//	{
	//		std::cout << "Please enter a torrent comment:" << "\n" << "\n";

	//		std::getline(std::cin, comment);

	//		if (!comment.empty())
	//		{
	//			isValidInput = true;
	//		}
	//		
	//		std::cout << "Comment added." << "\n" << "\n";
	//	}

	//	//copy tracker set set to vector
	//	std::vector<std::string> trackerVec;
	//	std::copy(trackerSet.begin(), trackerSet.end(), std::back_inserter(trackerVec));
	//	const char* fullPathptr = sourceFilePath.c_str();
	//	Torrent temp = createNewTorrent(name, fullPathptr, targetPath, isPrivate, comment, trackerVec);
	//}


	const char* fullFilePath = argv[1];

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
			client.localID, 6681, udpTestTorrent.hashesData.urlEncodedInfoHash, 
			udpTestTorrent.hashesData.infoHash, 2500, 1200, 0);
		});

	//auto thread2 = std::thread([&]() {
	//	loguru::set_thread_name("Tracker Update 2");
	//	LOG_F(INFO, "Starting new thread...");
	//	udpTestTorrent.generalData.trackerList.at(10).update(trackerObj::trackerEvent::started,
	//		client.localID, 6681, udpTestTorrent.hashesData.urlEncodedInfoHash, udpTestTorrent.hashesData.infoHash, 2500, 1200, 0);
	//	});

	/*auto thread3 = std::thread([&]() {
		loguru::set_thread_name("Tracker Update 3");
		LOG_F(INFO, "Starting new thread...");
		udpTestTorrent.generalData.trackerList.at(11).update(
			trackerObj::trackerEvent::started,
			client.localID, 6681, udpTestTorrent.hashesData.urlEncodedInfoHash, 
			udpTestTorrent.hashesData.infoHash, 2500, 1200, 0);
		});*/

	auto betweenTest = 5 + 10;
	betweenTest += 10;
	betweenTest += 10;
	betweenTest += 10;
	betweenTest += 10;
	LOG_F(INFO, "The answer is: %d", betweenTest);
	

	thread1.join();
	//thread2.join();
	//thread3.join();


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

