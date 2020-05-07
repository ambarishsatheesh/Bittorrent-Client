// TODO: Use UDP instead of HTTP for tracker communication - IMPORTANT
// TODO: make sure all class members are initialised
// TODO: event handler for peer list (for creating both new torrents and processing exsting ones)
// TODO: re-design remaining download
// TODO: implement choice between compact and non compact tracker response - make compact and id a client setting
// TODO: clean up utility functions
// TODO: fix humanReadableBytes function - doesn't round properly
// TODO: clean up error catching - currently all invalid arguments
// TODO: implement method to add trackers to torrent
// TODO: clean up decoder
// TODO: implement magnet link functionality
// TODO: tracker scrape (http and udp)
// TODO: multithreading (boost::asio async)

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

	std::vector<int8_t> testClientID;
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

//
//#include <boost/asio.hpp>
//#include <boost/array.hpp>
//#include <boost/bind.hpp>
//#include <thread>
//#include <iostream>
//#include <random>
//
//#define IPADDRESS "192.168.0.3" // "192.168.1.64"
//#define UDP_PORT 6881
//
//using boost::asio::ip::udp;
//using boost::asio::ip::address;
//
//void Sender(std::vector<uint8_t>& in) {
//    boost::asio::io_service io_context;
//    udp::socket socket(io_context);
//
//    udp::resolver resolver{ io_context };
//    // Look up the domain name
//    boost::asio::ip::basic_resolver_results<boost::asio::ip::udp> results = resolver.resolve("tracker.opentrackr.org", "1337");
//    //iterate and get successful connection endpoint
//
//    udp::endpoint remoteEndpoint = (boost::asio::connect(socket, results.begin(), results.end()))->endpoint();
//    std::cout << remoteEndpoint << "\n";
//    socket.close();
//
//    udp::socket socket1(io_context, udp::endpoint(udp::v4(), UDP_PORT));
//    std::cout << "source port: " << socket1.local_endpoint().port() << "\n";
//
//    boost::system::error_code err;
//    auto sent = socket1.send_to(boost::asio::buffer(in), remoteEndpoint, 0, err);
//    socket1.close();
//    std::cout << "Sent Payload --- " << sent << " bytes " << "\n";
//
//}
//
//struct Client {
//    boost::asio::io_service io_context;
//    udp::socket socket{ io_context };
//    std::vector<uint8_t> recv_buffer;
//    udp::endpoint remote_endpoint;
//
//    int count = 3;
//
//    void handle_receive(const boost::system::error_code& error, size_t bytes_transferred) {
//        if (error) {
//            std::cout << "Receive failed: " << error.message() << "\n";
//            return;
//        }
//        
//        std::cout << "Received " << bytes_transferred << " bytes" << "(" << error.message() << "): \n";
//
//        for (size_t i = 0; i < recv_buffer.size(); ++i)
//        {
//            printf(" %x ", (unsigned char)recv_buffer[i]);
//        }
//
//        wait();
//
//    }
//
//    void wait() {
//        socket.async_receive_from(boost::asio::buffer(recv_buffer),
//            remote_endpoint,
//            boost::bind(&Client::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
//    }
//
//    void Receiver()
//    {
//        socket.open(udp::v4());
//        socket.bind(udp::endpoint(address::from_string(IPADDRESS), UDP_PORT));
//
//        wait();
//
//        std::cout << "Receiving\n";
//        io_context.run();
//        std::cout << "Receiver exit\n";
//    }
//
//    Client()
//        : recv_buffer(16) {}
//};
//
//int main()
//{
//
//    //build connect request buffer
//    std::vector<uint8_t> protocolID = { 0x0, 0x0, 0x04, 0x17, 0x27, 0x10, 0x19, 0x80 };
//    std::vector<uint8_t> connectAction = { 0x0, 0x0, 0x0, 0x0 };
//
//    //generate random int32 and pass into transactionID array 
//    std::random_device dev;
//    std::mt19937 rng(dev());
//    const std::uniform_int_distribution<int32_t>
//        dist6(0, std::numeric_limits<int32_t>::max());
//    auto rndInt = dist6(rng);
//
//    //Big endian - but endianness doesn't matter since it's random anyway
//    std::vector<uint8_t> sentTransactionID;
//    sentTransactionID.resize(4);
//    for (int i = 0; i <= 3; ++i) {
//        sentTransactionID.at(i) = ((rndInt >> (8 * (3 - i))) & 0xff);
//    }
//
//    //copy to buffer
//    std::vector<uint8_t> connectVec;
//    connectVec.insert(connectVec.begin(), std::begin(protocolID), std::end(protocolID));
//    connectVec.insert(connectVec.begin() + 8, std::begin(connectAction), std::end(connectAction));
//    connectVec.insert(connectVec.begin() + 12, std::begin(sentTransactionID), std::end(sentTransactionID));
//
//    std::cout << std::endl;
//    std::cout << "Input is '";
//
//    for (auto i : connectVec)
//    {
//        printf("%x ", (unsigned char)i);
//    } 
//    std::cout << "'\nSending it to Sender Function...\n";
//
//    Client client;
//    std::thread r([&] { client.Receiver(); });
//    std::this_thread::sleep_for(std::chrono::milliseconds(100));
//    Sender(connectVec);
//
//    r.join();
//}