// TODO: Change UDP from sync to async
// TODO: make sure all class members are initialised
// TODO: event handler for peer list (for creating both new torrents and processing exsting ones)
// TODO: re-design remaining download
// TODO: implement choice between compact and non compact tracker response - make compact and id a client setting
// TODO: clean up utility functions
// TODO: fix humanReadableBytes function - doesn't round properly
// TODO: clean up error catching - currently all invalid arguments
// TODO: implement method to add trackers to torrent
// TODO: test piece verification
// TODO: clean up decoder
// TODO: implement magnet link functionality
// TODO: tracker scrape (http and udp)
// TODO: multithreading

#include "Decoder.h"
#include "encodeVisitor.h"
#include "TorrentManipulation.h"

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include "trackerUrl.h"

using namespace Bittorrent;
using namespace torrentManipulation;
using namespace Decoder;

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "Too few arguments: needs a file name..\n";
		return -1;
	}

	const char* fullFilePath = argv[1];

	//std::string test = "udp://tracker.coppersurfer.tk:6969";
	//trackerUrl::trackerUrl(test);

	bool isPrivate = false;

	std::vector<trackerObj> trackers;
	trackerObj one, two, three, four;
	one.trackerAddress = "http://www.cplusplus.com/forum/beginner/104849/";
	two.trackerAddress = "https://www.google.com/";
	three.trackerAddress = "https://www.youtube.com/watch?v=q86g1aop6a8";
	four.trackerAddress = "http://www.reddit.com/";

	trackers.push_back(one);
	trackers.push_back(two);
	trackers.push_back(three);
	trackers.push_back(four);

	Torrent temp = createNewTorrent("test", fullFilePath, isPrivate, "test comment", trackers);

	//std::cout << "file name: " << temp.generalData.fileName << std::endl;
	//std::cout << "comment: " << temp.generalData.comment << std::endl;
	//std::cout << "createdBy: " << temp.generalData.createdBy << std::endl;
	//std::cout << "creationDate: " << temp.generalData.creationDate << std::endl;
	//std::cout << "encoding: " << temp.generalData.encoding << std::endl;
	//std::cout << "isPrivate: " << temp.generalData.isPrivate << std::endl << std::endl;

	//std::cout << "trackers: " << std::endl;
	//for (auto i : temp.generalData.trackerList)
	//{
	//	std::cout << i.trackerAddress << std::endl;
	//}

	//std::cout << std::endl;

	//std::cout << "File info:" << std::endl << std::endl;

	//for (size_t i = 0; i < temp.fileList.size(); ++i)
	//{
	//	std::cout << "file name: " << temp.fileList.at(i).filePath << "; ";
	//	std::cout << "original file size: " << temp.fileList.at(i).fileSize << "; ";
	//	std::cout << "readable file size: " << temp.fileList.at(i).readableFileSize << std::endl;
	//}

	std::string buffer = loadFromFile(fullFilePath);

	std::random_device dev;
	std::mt19937 rng(dev());
	const std::uniform_int_distribution<int32_t>
		dist6(0, std::numeric_limits<int32_t>::max());
	auto rndInt = dist6(rng);

	std::vector<byte> testClientID;
	for (size_t i = 0; i < 20; ++i)
	{
		auto x = (dist6(rng) >> 8) & 0xff;
		testClientID.push_back(x);
	}

	valueDictionary torrent = boost::get<valueDictionary>(Decoder::decode(buffer));

	Torrent udpTestTorrent = toTorrentObj(fullFilePath, torrent);

	std::cout << udpTestTorrent.generalData.trackerList[3].trackerAddress << std::endl;

	udpTestTorrent.generalData.trackerList[3].update(trackerObj::trackerEvent::started,
		testClientID, 6681, udpTestTorrent.hashesData.urlEncodedInfoHash, udpTestTorrent.hashesData.infoHash, 0, 0, 0, 1);

	/*Torrent testTorrent = toTorrentObj(fullFilePath, torrent);
	testTorrent.generalData.trackerList[0].update(trackerObj::trackerEvent::started, 
		19028907, 6969, testTorrent.hashesData.urlEncodedInfoHash, testTorrent.hashesData.infoHash, 0, 0, 0,1);*/



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

