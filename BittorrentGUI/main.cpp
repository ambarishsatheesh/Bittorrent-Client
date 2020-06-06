#include "mainwindow.h"

#include <QApplication>

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
//    //start logging to file
//    const char* orderByTimeLog = "timeLog.log";
//    const char* orderByThreadLog = "threadLog.log";
//    loguru::init(argc, argv);
//    loguru::add_file(orderByTimeLog, loguru::Append, loguru::Verbosity_MAX);

//    const char* fullFilePath = argv[1];

//    const char* testingload = "test.log";

//    loadFromFile(testingload);

//    bool isPrivate = false;

//    std::vector<std::string> trackers;
//    trackers.push_back("http://www.cplusplus.com/forum/beginner/104849/");
//    trackers.push_back("https://www.google.com/");
//    trackers.push_back("https://www.youtube.com/watch?v=q86g1aop6a8");
//    trackers.push_back("http://www.reddit.com/");

//    //Torrent temp = createNewTorrent("test", fullFilePath, isPrivate, "test comment", trackers);

//    std::string buffer = loadFromFile(fullFilePath);
//    if (buffer.empty())
//    {
//        return 0;
//    }

//    Client client;

//    auto decodeTest = Decoder::decode(buffer);

//    if (decodeTest.type() == typeid(int) && boost::get<int>(decodeTest) == 0)
//    {
//        std::cout << "Fail!" << "\n";
//    }
//    else
//    {
//        //print true values here
//    }

//    valueDictionary torrent = boost::get<valueDictionary>(Decoder::decode(buffer));

//    Torrent udpTestTorrent = toTorrentObj(fullFilePath, torrent);

//    auto thread1 = std::thread([&]() {
//        loguru::set_thread_name("Tracker Update 1");
//        LOG_F(INFO, "Starting new thread...");
//        udpTestTorrent.generalData.trackerList.at(4).update(
//            trackerObj::trackerEvent::started,
//            client.localID, 0, udpTestTorrent.hashesData.urlEncodedInfoHash,
//            udpTestTorrent.hashesData.infoHash, 2500, 1200, 0);
//        });

//    auto thread2 = std::thread([&]() {
//        loguru::set_thread_name("Tracker Update 2");
//        LOG_F(INFO, "Starting new thread...");
//        udpTestTorrent.generalData.trackerList.at(8).update(trackerObj::trackerEvent::started,
//            client.localID, 0, udpTestTorrent.hashesData.urlEncodedInfoHash, udpTestTorrent.hashesData.infoHash, 2500, 1200, 0);
//        });

//    auto thread3 = std::thread([&]() {
//        loguru::set_thread_name("Tracker Update 3");
//        LOG_F(INFO, "Starting new thread...");
//        udpTestTorrent.generalData.trackerList.at(11).update(
//            trackerObj::trackerEvent::started,
//            client.localID, 0, udpTestTorrent.hashesData.urlEncodedInfoHash,
//            udpTestTorrent.hashesData.infoHash, 2500, 1200, 0);
//        });

//    thread1.join();
//    thread2.join();
//    thread3.join();



    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
