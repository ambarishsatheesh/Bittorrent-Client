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
    : networkPort{6880}, trackerTimer{std::make_unique<TrackerTimer>(clientID, networkPort)},
      downloadThrottle{maxDownloadBytesPerSecond, std::chrono::seconds{1}},
      uploadThrottle{maxUploadBytesPerSecond, std::chrono::seconds{1}},
      peerTimeout{std::chrono::seconds{30}}
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
    auto range = peerConnMap.equal_range(
                torrentList.at(position)->hashesData.urlEncodedInfoHash);
    for (auto it = range.first; it != range.second; ++it)
    {
        peerConnMap.erase(it);
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
        startSeeding(position);
        return;
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
                    torrentList.at(position).get()->statusData.uploaded,
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
                    networkPort,
                    torrentList.at(position).get()->
                    hashesData.urlEncodedInfoHash,
                    torrentList.at(position).get()->
                    hashesData.infoHash,
                    torrentList.at(position).get()->statusData.uploaded,
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
            auto infoHash = torrentList.at(position)->
                    hashesData.urlEncodedInfoHash;

            //check if peer already in map (e.g. due to tracker update signal)
            if (peerConnMap.find(infoHash) != peerConnMap.end())
            {
                auto range = peerConnMap.equal_range(infoHash);

                std::vector<std::string> mapHostRange;

                for (auto it = range.first; it != range.second; ++it)
                {
                    mapHostRange.push_back(it->second->peerHost);
                }

                for (auto singlePeer :
                     torrentList.at(position)->generalData.uniquePeerList)
                {
                    if (std::find(mapHostRange.begin(), mapHostRange.end(),
                                  singlePeer.ipAddress) != mapHostRange.end())
                    {
                        //add new peer if not in map
                            addPeer(&singlePeer,
                                    torrentList.at(position).get());
                    }
                }
            }
        }

        //begin remaining process
        run();
    }
}

void WorkingTorrents::stop(int position)
{
    disableTorrentConnection(torrentList.at(position).get());
}

void WorkingTorrents::run()
{

}

//only allow seeding of one torrent at a time
void WorkingTorrents::startSeeding(int position)
{
    acceptNewConnection(torrentList.at(position).get());
}

//peers for downloading
void WorkingTorrents::addPeer(peer* singlePeer, Torrent* torrent)
{
    QFuture<void> future = QtConcurrent::run([&]()
    {
        //TCP setup
        boost::asio::io_context io_context;

        auto peerConn =
            std::make_shared<Peer>(torrent, clientID, io_context, networkPort);

        //connect signals to slots
        peerConn->sig_blockRequested->connect(
                    boost::bind(&WorkingTorrents::handleBlockRequested,
                                this, _1, _2));

        peerConn->sig_blockCancelled->connect(
                    boost::bind(&WorkingTorrents::handleBlockCancelled,
                                this, _1, _2));

        peerConn->sig_blockReceived->connect(
                    boost::bind(&WorkingTorrents::handleBlockReceived,
                                this, _1, _2));

        peerConn->sig_disconnected->connect(
                    boost::bind(&WorkingTorrents::handlePeerDisconnected,
                                this, _1));

        peerConn->sig_stateChanged->connect(
                    boost::bind(&WorkingTorrents::handlePeerStateChanged,
                                this, _1));

        //lock mutex before adding to map
        std::lock_guard<std::mutex> guard(mtx_map);

        //add to class member map so it can be accessed outside thread
        peerConnMap.emplace(torrent->hashesData.urlEncodedInfoHash,
                            peerConn);

        peerConn->startNew(singlePeer->ipAddress, singlePeer->port);

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

        acceptor.open(tcp::v4());
        acceptor.set_option(tcp::socket::reuse_address(true));
        acceptor.bind(tcp::endpoint(tcp::v4(), networkPort));

        acceptor.async_accept(
            [&](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    auto peerConn =
                            std::make_shared<Peer>(
                                &torrentList, clientID, acc_io_context,
                                std::move(socket), networkPort);

                    //connect signals to slots
                    peerConn->sig_blockRequested->connect(
                                boost::bind(
                                    &WorkingTorrents::handleBlockRequested,
                                    this, _1, _2));

                    peerConn->sig_blockCancelled->connect(
                                boost::bind(
                                    &WorkingTorrents::handleBlockCancelled,
                                    this, _1, _2));

                    peerConn->sig_blockReceived->connect(
                                boost::bind(
                                    &WorkingTorrents::handleBlockReceived,
                                    this, _1, _2));

                    peerConn->sig_disconnected->connect(
                                boost::bind(
                                    &WorkingTorrents::handlePeerDisconnected,
                                    this, _1));

                    peerConn->sig_stateChanged->connect(
                                boost::bind(
                                    &WorkingTorrents::handlePeerStateChanged,
                                    this, _1));

                    //lock mutex before adding to map
                    std::lock_guard<std::mutex> guard(mtx_map);
                    //add to class member map so it can be accessed outside thread
                    peerConnMap.emplace(
                                torrent->hashesData.urlEncodedInfoHash,
                                        peerConn);
                }

                //start listening for new peer connections for same torrent
                acceptNewConnection(torrent);
            });

        acc_io_context.run();
    });
}

//disconnect all peers with info hash equal to
//argument torrent's infohash
void WorkingTorrents::disableTorrentConnection(Torrent* torrent)
{
    if (torrent->statusData.currentState ==
            TorrentStatus::currentStatus::started)
    {
        torrent->statusData.currentState =
                TorrentStatus::currentStatus::stopped;
    }

    auto range = peerConnMap.equal_range(
                torrent->hashesData.urlEncodedInfoHash);

    for (auto it = range.first; it != range.second; ++it)
    {
        it->second->disconnect();

        //erase since it is easier to recreate peer object than reset
        //existing one
        peerConnMap.erase(it);
    }
}

void WorkingTorrents::handlePieceVerified(int piece)
{

}

void WorkingTorrents::handleBlockRequested(Peer* peer, Peer::dataRequest newDataRequest)
{
    std::lock_guard<std::mutex> outgoingGuard(mtx_outgoing);

    outgoingBlocks.push_back(newDataRequest);

    processUploads();
}

void WorkingTorrents::handleBlockCancelled(Peer* peer, Peer::dataRequest newDataRequest)
{
    //block scope for lock_guard so that processUploads() can be called
    //separately elsewhere while retaining its own lock
    {
        std::lock_guard<std::mutex> outgoingGuard(mtx_outgoing);

        //flag block for cancelling (processed later in processUploads())
        for (auto& block : outgoingBlocks)
        {
            if (block != newDataRequest)
            {
                continue;
            }

            block.isCancelled = true;
        }
    }

    processUploads();
}

void WorkingTorrents::handleBlockReceived(Peer* peer, Peer::dataPackage newPackage)
{

}

void WorkingTorrents::handlePeerDisconnected(std::shared_ptr<Peer> senderPeer)
{
    //manually disconnect slots from signals just in case
    senderPeer->sig_blockRequested->disconnect_all_slots();
    senderPeer->sig_blockCancelled->disconnect_all_slots();
    senderPeer->sig_blockReceived->disconnect_all_slots();
    senderPeer->sig_disconnected->disconnect_all_slots();
    senderPeer->sig_stateChanged->disconnect_all_slots();

    auto infoHash = senderPeer->torrent->hashesData.urlEncodedInfoHash;

    //remove from peer map
    auto range = peerConnMap.equal_range(
                senderPeer->torrent->hashesData.urlEncodedInfoHash);

    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second->peerHost == senderPeer->peerHost &&
                it->second->peerPort == senderPeer->peerPort)
        {
            peerConnMap.erase(it);
        }
    }
}

void WorkingTorrents::handlePeerStateChanged(Peer* senderPeer)
{

}

void WorkingTorrents::processPeers()
{
    //locking mutex because this method can be run on multiple threads
    std::lock_guard<std::mutex> processGuard(mtx_process);

    //move map to vector and sort by descending pieces required available
    std::vector<std::shared_ptr<Peer>> sortedPeers;
    for (auto it = peerConnMap.begin(); it != peerConnMap.end(); ++it)
    {
        sortedPeers.push_back(it->second);
    }

    std::sort(sortedPeers.begin(), sortedPeers.end(),
              [&](
              const std::shared_ptr<Peer>& p1,
              const std::shared_ptr<Peer>& p2)
    {
        return p1->piecesRequiredAvailable() > p2->piecesRequiredAvailable();
    });

    //processing
    for (auto& peer : sortedPeers)
    {
        //if nullptr, continue

        //peer connection timed out
        if (std::chrono::high_resolution_clock::now() >
                peer->lastActive + peerTimeout)
        {
            peer->disconnect();
            continue;
        }

        //peer not fully set up yet
        if (!peer->isHandshakeSent || ! peer->isHandshakeReceived)
        {
            continue;
        }

        //send interested message if we want to request data
        if (peer->torrent->statusData.isCompleted())
        {
            peer->sendNotInterested();
        }
        else
        {
            peer->sendInterested();
        }

        //torrent completed so can disconnect
        if (peer->isCompleted() && peer->torrent->statusData.isCompleted())
        {
            peer->disconnect();
            continue;
        }

        //periodic message to keep peer connection alive
        //method has a check to prevent spamming
        peer->sendKeepAlive();

        //allow leeching
        auto infoHash = peer->torrent->hashesData.urlEncodedInfoHash;
        if (peer->torrent->statusData.isStarted() &&
                leechersMap.count(infoHash) < maxLeechersPerTorrent)
        {
            if (peer->IsInterestedReceived && peer->isChokeSent)
            {
                peer->sendUnchoke();
                leechersMap.emplace(peer->torrent->hashesData.urlEncodedInfoHash,
                                   peer);
            }
        }

        //if peer is unchoked, add them to seeders
        if (!peer->torrent->statusData.isCompleted() &&
                seedersMap.count(infoHash) < maxSeedersPerTorrent)
        {
            if (!peer->IsChokeReceived)
            {
                seedersMap.emplace(peer->torrent->hashesData.urlEncodedInfoHash,
                                   peer);
            }
        }
    }
}

void WorkingTorrents::processUploads()
{
    std::lock_guard<std::mutex> outgoingGuard(mtx_outgoing);

    Peer::dataRequest tempBlock;

    while (!uploadThrottle.isThrottled() && !outgoingBlocks.empty())
    {
        tempBlock = outgoingBlocks.front();
        outgoingBlocks.pop_front();

        if (tempBlock.isCancelled)
        {
            continue;
        }

        //check piece exists locally
        if (!tempBlock.sourcePeer->torrent->
                statusData.isPieceVerified[tempBlock.piece])
        {
            continue;
        }

        //get data from file and send
        std::vector<byte> data = readBlock(*tempBlock.sourcePeer->torrent,
                                           tempBlock.piece,
                                           tempBlock.offset,
                                           tempBlock.dataSize);

        if (data.empty())
        {
            continue;
        }

        //send to peer
        tempBlock.sourcePeer->sendPiece(tempBlock.piece, tempBlock.offset,
                                        data);

        //add to throttle bucket
        uploadThrottle.add(tempBlock.dataSize);

        //update torrent info
        tempBlock.sourcePeer->torrent->statusData.uploaded += tempBlock.dataSize;
    }
}

}




