#include "WorkingTorrents.h"
#include "Decoder.h"
#include "loguru.h"


#include <QDateTime>
#include <QtConcurrent>
#include <memory>
#include <boost/bind.hpp>

namespace Bittorrent
{
using namespace torrentManipulation;

WorkingTorrents::WorkingTorrents()
    : trackerTimer{std::make_unique<TrackerTimer>(clientID)}, seedingCount{0}
{
}

std::string WorkingTorrents::isDuplicateTorrent(Torrent* modifiedTorrent)
{
    //check if torrent already exists in list
    for (auto torrent : torrentList)
    {
        if (modifiedTorrent->hashesData.hexStringInfoHash ==
                torrent->hashesData.hexStringInfoHash)
        {
            return torrent->generalData.fileName;
        }
    }

    return "";
}


void WorkingTorrents::addNewTorrent(Torrent* modifiedTorrent)
{
    torrentList.push_back(std::make_shared<Torrent>(*modifiedTorrent));

    //connect signals and slots
    torrentList.back()->sig_addPeer->connect(
                boost::bind(&WorkingTorrents::addPeer,
                            this, _1, _2));

    torrentList.back()->piecesData.sig_pieceVerified->connect(
                boost::bind(&WorkingTorrents::handlePieceVerified,
                            this, _1));

    //get current time as appropriately formatted string
    addedOnList.push_back(
                QDateTime::currentDateTime().
                toString("yyyy/MM/dd HH:mm"));

    //store map of unique trackers and how many torrents use them
    for (auto trackers : modifiedTorrent->generalData.trackerList)
    {
        //get only main host portion of address
        std::string fullTrackerAdd = trackers.trackerAddress;
        std::string mainHost;

        //if not ip address
        if (std::count(fullTrackerAdd.begin(),
                       fullTrackerAdd.end(), '.') < 3)
        {

            auto portPos = fullTrackerAdd.find_last_of(':');
            if (std::count(fullTrackerAdd.begin(),
                           fullTrackerAdd.end(), '.') > 1)
            {
                auto firstDotPos = fullTrackerAdd.find('.') + 1;
                mainHost = fullTrackerAdd.substr(
                            firstDotPos, portPos - firstDotPos);
            }
            else
            {
                auto protocolPos = fullTrackerAdd.find("//") + 2;
                mainHost = fullTrackerAdd.substr(
                            protocolPos, portPos - protocolPos);
            }
        }
        else
        {
            mainHost = fullTrackerAdd;
        }

        // and increment if already exists
        infoTrackerMap[QString::fromStdString(mainHost)]++;;

        //use hex string as torrent identifier and store as vector of torrents
        //mapped to each tracker for use in filtering via tracker list
        trackerTorrentMap[QString::fromStdString(mainHost)].emplace(
                    QString::fromStdString(
                        modifiedTorrent->hashesData.hexStringInfoHash));
    }

    LOG_F(INFO, "Added Torrent '%s' to client!",
          modifiedTorrent->generalData.fileName.c_str());
}

void WorkingTorrents::removeTorrent(int position)
{
    auto logName = torrentList.at(position)->generalData.fileName;

    //stop torrent before removing from list
    stop(position);

    for (auto trackers : torrentList.at(position)->
         generalData.trackerList)
    {
        //get only main host portion of address
        std::string fullTrackerAdd = trackers.trackerAddress;
        std::string mainHost;

        //if not ip address
        if (std::count(fullTrackerAdd.begin(),
                       fullTrackerAdd.end(), '.') < 3)
        {
            auto portPos = fullTrackerAdd.find_last_of(':');
            if (std::count(fullTrackerAdd.begin(),
                           fullTrackerAdd.end(), '.') > 1)
            {
                auto firstDotPos = fullTrackerAdd.find('.') + 1;
                mainHost = fullTrackerAdd.substr(
                            firstDotPos, portPos - firstDotPos);
            }
            else
            {
                auto protocolPos = fullTrackerAdd.find("//") + 2;
                mainHost = fullTrackerAdd.substr(
                            protocolPos, portPos - protocolPos);
            }
        }
        else
        {
             mainHost = fullTrackerAdd;
        }

        //if count hits 0, remove from map, else just decrement
        if (--infoTrackerMap[QString::fromStdString(mainHost)]
                < 1)
        {
            infoTrackerMap.erase(
                        infoTrackerMap.find(
                            QString::fromStdString(mainHost)));
        }
    }

    //remove infohash associated with removed torrent f\rom the
    //torrent map used for filtering
    for (auto it = trackerTorrentMap.begin(),
           end = trackerTorrentMap.end(); it != end;)
    {
        auto infoHash = QString::fromStdString(torrentList.at(position)->
                hashesData.hexStringInfoHash);

        if (it.value().find(infoHash) != it.value().end())
        {
            it.value().erase(infoHash);
        }
        //remove key and value if value set is empty (needed to keep it
        //perfectly mapped to infoTrackerMap used for list view.
        if (it.value().empty())
        {
            it = trackerTorrentMap.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //remove entry from update set using infohash as identifier
    for (auto it = trackerUpdateSet.begin();
         it != trackerUpdateSet.end();)
    {
        if (it->second->hashesData.hexStringInfoHash ==
                torrentList.at(position).get()->
                hashesData.hexStringInfoHash)
        {
            it = trackerUpdateSet.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (trackerUpdateSet.empty())
    {
        trackerTimer->stop();
    }

    //remove relevant peers from dl and ul peer maps using infohash
    //no need to erase from ul_peerConnMap because disconnecting erases element
    auto dlRange = dl_peerConnMap.equal_range(
                torrentList.at(position)->hashesData.urlEncodedInfoHash);
    for (auto it = dlRange.first; it != dlRange.second; ++it)
    {
        dl_peerConnMap.erase(it);
    }

    //remove torrent info from list
    torrentList.erase(torrentList.begin() + position);
    addedOnList.erase(addedOnList.begin() + position);

    LOG_F(INFO, "Removed Torrent '%s' from client!",
          logName.c_str());
}

void WorkingTorrents::start(int position)
{
    //start seeding if torrent is completed
    if (torrentList.at(position)->statusData.currentState ==
            TorrentStatus::currentStatus::completed)
    {
        //only seed if no other torrents are seeding
        if (seedingCount == 0)
        {
            startSeeding(position);
            return;
        }
        else
        {
            LOG_F(ERROR, "Concurrent torrent seeding count is already at the "
                         "maximum (1)! Failed to begin seeding '%s'.",
                  torrentList.at(position)->generalData.fileName.c_str());

            return;
        }
    }

    //if torrent stopped, start it and process accordingly
    if (torrentList.at(position)->statusData.currentState ==
            TorrentStatus::currentStatus::stopped)
    {
        torrentList.at(position)->statusData.currentState =
                TorrentStatus::currentStatus::started;

        //not needed?
        runningTorrents.push_back(torrentList.at(position));

        std::vector<std::thread> threadVector;

        //run initial tracker updates in separate threads
        //if tracker list has more than 5 torrents,
        //process 5 trackers at a time
        if (torrentList.at(position)->generalData.trackerList.size() > 5)
        {
            int count = 0;

            for (auto& tracker :
                 torrentList.at(position)->generalData.trackerList)
            {
                //check if recently requested trackers
                if (std::chrono::high_resolution_clock::now() <
                        tracker.lastPeerRequest + std::chrono::minutes{5})
                {
                    continue;
                }

                threadVector.emplace_back([&]()
                {
                    tracker.update(
                    torrentList.at(position).get()->statusData.currentState,
                    clientID,
                    0,
                    torrentList.at(position).get()->
                    hashesData.urlEncodedInfoHash,
                    torrentList.at(position).get()->
                    hashesData.infoHash,
                    torrentList.at(position).get()->statusData.uploaded(),
                    torrentList.at(position).get()->statusData.downloaded(),
                    torrentList.at(position).get()->statusData.remaining());

                    LOG_F(INFO, "Processed tracker %s",
                          tracker.trackerAddress.c_str());
                });

                count++;

                //every 5 threads, join threads and clear thread vector to
                //limit number of concurrent threads
                if (count == 4 ||
                        &tracker == &runningTorrents.back()->
                        generalData.trackerList.back())
                {
                    for (auto& thread : threadVector)
                    {
                        if (thread.joinable())
                        {
                            thread.join();
                        }
                    }
                    threadVector.clear();
                    count = 0;
                }
            }
        }
        else
        {
            for (auto& tracker :
                 runningTorrents.back()->generalData.trackerList)
            {
                if (std::chrono::high_resolution_clock::now() <
                        tracker.lastPeerRequest + std::chrono::minutes{5})
                {
                    continue;
                }

                threadVector.emplace_back([&]()
                {
                    tracker.update(
                    torrentList.at(position).get()->statusData.currentState,
                    clientID,
                    0,
                    torrentList.at(position).get()->
                    hashesData.urlEncodedInfoHash,
                    torrentList.at(position).get()->
                    hashesData.infoHash,
                    torrentList.at(position).get()->statusData.uploaded(),
                    torrentList.at(position).get()->statusData.downloaded(),
                    torrentList.at(position).get()->statusData.remaining());

                    LOG_F(INFO, "Processed tracker %s",
                          tracker.trackerAddress.c_str());
                });
            }
            for (auto& thread : threadVector)
            {
                if (thread.joinable())
                {
                    thread.join();
                }
            }
        }

        for (auto& tracker : torrentList.at(position)->
             generalData.trackerList)
        {
            trackerUpdateSet.emplace(&tracker,torrentList.at(position).get());
        }

        LOG_F(INFO, "Processed trackers for torrent '%s'!",
              torrentList.at(position)->generalData.fileName.c_str());

        //only start new timer if timer is not already running and
        //set has some trackers in it
        if (!trackerTimer->isRunning() && !trackerUpdateSet.empty())
        {
            trackerTimer->start(&trackerUpdateSet);
        }

        //if peer list is empty, fill it with peers from trackers
        //else resume peer connections with existing peers
        if (torrentList.at(position)->generalData.uniquePeerList.empty())
        {
            torrentList.at(position)->generalData.getPeerList();
        }
        else
        {
            auto range = dl_peerConnMap.equal_range(
                        torrentList.at(position)->hashesData.urlEncodedInfoHash);

            for (auto it = range.first; it != range.second; ++it)
            {
                it->second->resume();
            }
        }

        //begin remaining process
        run();
    }
}

void WorkingTorrents::stop(int position)
{
    disablePeerConnection(torrentList.at(position).get());
}

void WorkingTorrents::run()
{

}

//only allow seeding of one torrent at a time
void WorkingTorrents::startSeeding(int position)
{
    seedingCount = 1;
    acceptNewConnection(torrentList.at(position).get());
}

//peers for downloading
void WorkingTorrents::addPeer(peer* singlePeer, Torrent* torrent)
{
    QFuture<void> future = QtConcurrent::run([&]()
    {
        //TCP setup
        boost::asio::io_context io_context;

        auto dl_peerConn =
            std::make_shared<Peer>(torrent, clientID, io_context);

        //lock mutex before adding to map
        std::lock_guard<std::mutex> guard(dl);
        //add to class member map so it can be accessed outside thread
        dl_peerConnMap.emplace(torrent->hashesData.urlEncodedInfoHash,
                            dl_peerConn);

        //connect signals here
        //peerConn->blockRequested->connect()

        dl_peerConn->startNew(singlePeer->ipAddress, singlePeer->port);

        io_context.run();
    });
}

//only
void WorkingTorrents::acceptNewConnection(Torrent* torrent)
{
    QFuture<void> future = QtConcurrent::run([&]()
    {
        boost::asio::io_context acc_io_context;
        tcp::acceptor acceptor(acc_io_context);

        acceptor.async_accept(
            [&](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    auto ul_peerConn =
                            std::make_shared<Peer>(
                                torrent, clientID, acc_io_context,
                                std::move(socket));

                    //lock mutex before adding to map
                    std::lock_guard<std::mutex> guard(ul);
                    //add to class member map so it can be accessed outside thread
                    ul_peerConnMap.emplace(
                                torrent->hashesData.urlEncodedInfoHash,
                                        ul_peerConn);
                }
                else
                {
                    //reset seeding count
                    seedingCount = 0;
                }

                //start listening for new peer connections for same torrent
                acceptNewConnection(torrent);
            });

        acc_io_context.run();
    });
}

void WorkingTorrents::disablePeerConnection(Torrent* torrent)
{
    //disconnect all peers with info hash equal to
    //argument torrent's infohash

    //downloading
    if (torrent->statusData.currentState ==
            TorrentStatus::currentStatus::started)
    {
        torrent->statusData.currentState =
                TorrentStatus::currentStatus::stopped;

        auto range = dl_peerConnMap.equal_range(
                    torrent->hashesData.urlEncodedInfoHash);

        for (auto it = range.first; it != range.second; ++it)
        {
            it->second->disconnect();
        }
    }
    //uploading
    else if (torrent->statusData.currentState ==
             TorrentStatus::currentStatus::completed)
    {
        auto range = ul_peerConnMap.equal_range(
                    torrent->hashesData.urlEncodedInfoHash);

        for (auto it = range.first; it != range.second; ++it)
        {
            it->second->disconnect();

            //erase since there is no point keeping the object
            ul_peerConnMap.erase(it);
        }
    }
}

void WorkingTorrents::handlePieceVerified(int piece)
{

}


}


