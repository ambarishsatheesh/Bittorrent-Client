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
      rand{}, rng{rand()}, peerTimeout{std::chrono::seconds{30}}
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

    torrentList.back()->sig_pieceVerified->connect(
                boost::bind(&WorkingTorrents::handlePieceVerified,
                            this, _1, _2));

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

    //remove relevant peers from all peer maps using infohash as key
    std::lock_guard<std::mutex> guard(mtx_map);

    auto range = peerConnMap.equal_range(
                torrentList.at(position)->hashesData.urlEncodedInfoHash);
    for (auto it = range.first; it != range.second; ++it)
    {
        peerConnMap.erase(it);
    }

    range = seedersMap.equal_range(
                    torrentList.at(position)->hashesData.urlEncodedInfoHash);
    for (auto it = range.first; it != range.second; ++it)
    {
        seedersMap.erase(it);
    }

    range = leechersMap.equal_range(
                    torrentList.at(position)->hashesData.urlEncodedInfoHash);
    for (auto it = range.first; it != range.second; ++it)
    {
        leechersMap.erase(it);
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

                //push map values into vector so they can be searched
                std::vector<std::string> mapHostRange;

                for (auto it = range.first; it != range.second; ++it)
                {
                    mapHostRange.push_back(it->second->peerHost);
                }

                //create new Peer object if not in peerConnMap
                for (auto singlePeer :
                     torrentList.at(position)->generalData.uniquePeerList)
                {
                    if (std::find(mapHostRange.begin(), mapHostRange.end(),
                                  singlePeer.ipAddress) == mapHostRange.end())
                    {
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
                                this, _1));

        peerConn->sig_blockCancelled->connect(
                    boost::bind(&WorkingTorrents::handleBlockCancelled,
                                this, _1));

        peerConn->sig_blockReceived->connect(
                    boost::bind(&WorkingTorrents::handleBlockReceived,
                                this, _1));

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

        //allow reuse of port across multiple sockets
        acceptor.open(tcp::v4());
        acceptor.set_option(tcp::socket::reuse_address(true));
        acceptor.bind(tcp::endpoint(tcp::v4(), networkPort));

        acceptor.async_accept(
            [&](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    //create Peer object and move socket to it
                    auto peerConn =
                            std::make_shared<Peer>(
                                &torrentList, clientID, acc_io_context,
                                std::move(socket), networkPort);

                    //connect signals to slots
                    peerConn->sig_blockRequested->connect(
                                boost::bind(
                                    &WorkingTorrents::handleBlockRequested,
                                    this, _1));

                    peerConn->sig_blockCancelled->connect(
                                boost::bind(
                                    &WorkingTorrents::handleBlockCancelled,
                                    this, _1));

                    peerConn->sig_blockReceived->connect(
                                boost::bind(
                                    &WorkingTorrents::handleBlockReceived,
                                    this, _1));

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


    //disconnect and erase from maps
    std::lock_guard<std::mutex> guard(mtx_map);

    auto range = peerConnMap.equal_range(
                torrent->hashesData.urlEncodedInfoHash);

    for (auto it = range.first; it != range.second; ++it)
    {
        it->second->disconnect();

        //erase since it is easier to recreate peer object than reset
        //existing one
        peerConnMap.erase(it);
    }

    range = seedersMap.equal_range(
                    torrent->hashesData.urlEncodedInfoHash);
    for (auto it = range.first; it != range.second; ++it)
    {
        seedersMap.erase(it);
    }

    range = leechersMap.equal_range(
                    torrent->hashesData.urlEncodedInfoHash);
    for (auto it = range.first; it != range.second; ++it)
    {
        leechersMap.erase(it);
    }

}

void WorkingTorrents::handlePieceVerified(Torrent* torrent, int piece)
{
    processPeers(torrent);

    std::lock_guard<std::mutex> guard(mtx_map);

    auto range = peerConnMap.equal_range(
                torrent->hashesData.urlEncodedInfoHash);

    for (auto it = range.first; it != range.second; ++it)
    {
        //peer not fully set up yet
        if (!it->second->isHandshakeReceived || !it->second->isHandshakeSent)
        {
            continue;
        }

        it->second->sendHave(piece);
    }
}

void WorkingTorrents::handleBlockRequested(Peer::dataRequest request)
{
    {
        std::lock_guard<std::mutex> outgoingGuard(mtx_outgoing);

        outgoingBlocks.push_back(request);
    }

    processUploads();
}

void WorkingTorrents::handleBlockCancelled(Peer::dataRequest request)
{
    //block scope for lock_guard so that processUploads() can be called
    //separately elsewhere while retaining its own lock
    {
        std::lock_guard<std::mutex> outgoingGuard(mtx_outgoing);

        //flag block for cancelling (processed later in processUploads())
        for (auto& block : outgoingBlocks)
        {
            if (block != request)
            {
                continue;
            }

            block.isCancelled = true;
        }
    }

    processUploads();
}

void WorkingTorrents::handleBlockReceived(Peer::dataPackage package)
{
    {
        std::lock_guard<std::mutex> incomingGuard(mtx_incoming);

        //add to deque for processing
        incomingBlocks.push_back(package);

        //reset block requested
        package.sourcePeer->isBlockRequested.at(package.piece).at(package.block)
                = false;

        auto range = peerConnMap.equal_range(
                    package.sourcePeer->torrent->
                    hashesData.urlEncodedInfoHash);

        for (auto it = range.first; it != range.second; ++it)
        {
            //ignore peer if block is not requested
            //(includes peer from argument due to reset above)
            if (!it->second->
                    isBlockRequested.at(package.piece).at(package.block))
            {
                continue;
            }

            it->second->sendCancel(package.piece,
                                   package.block *
                                   package.sourcePeer->torrent->
                                   piecesData.blockSize,
                                   package.sourcePeer->torrent->
                                   piecesData.blockSize);

            it->second->isBlockRequested.at(package.piece).at(package.block)
                    = false;
        }

    }

    processDownloads();
}

void WorkingTorrents::handlePeerDisconnected(Peer* senderPeer)
{
    //manually disconnect slots from signals just in case
    senderPeer->sig_blockRequested->disconnect_all_slots();
    senderPeer->sig_blockCancelled->disconnect_all_slots();
    senderPeer->sig_blockReceived->disconnect_all_slots();
    senderPeer->sig_disconnected->disconnect_all_slots();
    senderPeer->sig_stateChanged->disconnect_all_slots();

    auto infoHash = senderPeer->torrent->hashesData.urlEncodedInfoHash;

    std::lock_guard<std::mutex> guard(mtx_map);

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

    range = seedersMap.equal_range(
                    senderPeer->torrent->hashesData.urlEncodedInfoHash);

    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second->peerHost == senderPeer->peerHost &&
                it->second->peerPort == senderPeer->peerPort)
        {
            seedersMap.erase(it);
        }
    }

    range = leechersMap.equal_range(
                    senderPeer->torrent->hashesData.urlEncodedInfoHash);

    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second->peerHost == senderPeer->peerHost &&
                it->second->peerPort == senderPeer->peerPort)
        {
            leechersMap.erase(it);
        }
    }
}

void WorkingTorrents::handlePeerStateChanged(Peer* senderPeer)
{
    processPeers(senderPeer->torrent.get());
}

void WorkingTorrents::processPeers(Torrent* torrent)
{
    //locking mutex because this method can be run on multiple threads
    std::lock_guard<std::mutex> processGuard(mtx_process);

    //move peers of specific torrent from map to vector
    //and sort by (descending) pieces required available
    std::vector<std::shared_ptr<Peer>> sortedPeers;
    for (auto it = peerConnMap.begin(); it != peerConnMap.end(); ++it)
    {
        if (it->second->torrent->hashesData.urlEncodedInfoHash ==
                torrent->hashesData.urlEncodedInfoHash)
        {
            sortedPeers.push_back(it->second);
        }
    }

    std::sort(sortedPeers.begin(), sortedPeers.end(),
              [&](
              const std::shared_ptr<Peer>& p1,
              const std::shared_ptr<Peer>& p2)
    {
        return p1->piecesRequiredAvailable() > p2->piecesRequiredAvailable();
    });

    std::lock_guard<std::mutex> guard(mtx_map);

    //processing
    for (auto& peer : sortedPeers)
    {
        //if nullptr, continue
        if (!torrent)
        {
            continue;
        }

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
        auto infoHash = torrent->hashesData.urlEncodedInfoHash;
        if (torrent->statusData.isStarted() &&
                leechersMap.count(infoHash) < maxLeechersPerTorrent)
        {
            if (peer->IsInterestedReceived && peer->isChokeSent)
            {
                peer->sendUnchoke();
                leechersMap.emplace(torrent->hashesData.urlEncodedInfoHash,
                                   peer);
            }
        }

        //if peer is unchoked, add them to seeders
        if (!torrent->statusData.isCompleted() &&
                seedersMap.count(infoHash) < maxSeedersPerTorrent)
        {
            if (!peer->IsChokeReceived)
            {
                seedersMap.emplace(torrent->hashesData.urlEncodedInfoHash,
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

void WorkingTorrents::processDownloads()
{
    std::lock_guard<std::mutex> incomingGuard(mtx_incoming);

    Peer::dataPackage tempBlock;

    //write received data to disk only if block has not been acquired already
    while (!incomingBlocks.empty())
    {
        tempBlock = incomingBlocks.front();
        incomingBlocks.pop_front();

        if (!tempBlock.sourcePeer->torrent->
                statusData.isBlockAcquired
                .at(tempBlock.piece).at(tempBlock.block))
        {
            writeBlock(*tempBlock.sourcePeer->torrent, tempBlock.piece,
                       tempBlock.block, tempBlock.data);
        }
    }

    //return if torrent downloading is done
    if (tempBlock.sourcePeer->torrent->statusData.isCompleted())
    {
        return;
    }

}

std::vector<Peer*> WorkingTorrents::getRankedSeeders()
{
    //shuffle seeders
    std::vector<Peer*> rankedSeeders;
    rankedSeeders.reserve(seedersMap.size());

    for (auto& peer : seedersMap)
    {
        rankedSeeders.push_back(peer.second.get());
    }

    std::shuffle(rankedSeeders.begin(), rankedSeeders.end(), rng);
}

std::vector<int> WorkingTorrents::getRankedPieces(Torrent* torrent)
{
    //sort pieces by their respective scores
    struct indexScores
    {
        int index;
        float score;
    };

    std::vector<indexScores> indexScores(torrent->piecesData.pieceCount);

    for (int i = 0; i < indexScores.size(); ++i)
    {
        indexScores.at(i).index = i;
        indexScores.at(i).score = getPieceScore(torrent, i);
    }

    std::sort(indexScores.begin(), indexScores.end(),
              [&](const struct indexScores& a, const struct indexScores& b)
    {
        return a.score > b.score;
    });

    std::vector<int> resIndexes;
    resIndexes.reserve(indexScores.size());

    for (auto singleStruct : indexScores)
    {
        resIndexes.push_back(singleStruct.index);
    }

    return resIndexes;
}

float WorkingTorrents::getPieceScore(Torrent* torrent, int piece)
{
    float progress = getPieceProgress(torrent, piece);
    float rarity = getPieceRarity(torrent, piece);

    //if all blocks are acquired, minimise progress and (hence score)
    if (progress == 1.0)
    {
        progress = 0.0;
    }

    //use random float to handle case where progress and rarity values for
    //different peers are equal
    std::uniform_real_distribution<float> dist(0, 1);

    return progress + rarity + dist(rng);
}

float WorkingTorrents::getPieceProgress(Torrent* torrent, int piece)
{
    float progressSum = std::accumulate(
                torrent->statusData.isBlockAcquired.at(piece).begin(),
                torrent->statusData.isBlockAcquired.at(piece).end(), 0,
            [](float sum, const bool& b) -> float
    {
        return sum + (b ? 1.0 : 0.0);
    });

    return progressSum / torrent->statusData.isBlockAcquired.at(piece).size();
}

float WorkingTorrents::getPieceRarity(Torrent* torrent, int piece)
{
    if (peerConnMap.empty())
    {
        return 0.0;
    }

    auto infoHash = torrent->hashesData.urlEncodedInfoHash;

    //get summed rarity of peers who have required piece
    float raritySum = std::accumulate(
                peerConnMap.begin(), peerConnMap.end(), 0,
                    [&]
                (float sum,
                const std::unordered_multimap<std::string,
                std::shared_ptr<Peer>>::value_type& p) -> float
    {
        //only look at peers associated with relevant torrent
        if (p.second->torrent->hashesData.urlEncodedInfoHash == infoHash)
        {
            //add 1 when peer doesnt have piece to increase rarity value
            return sum + (p.second->isPieceDownloaded.at(piece) ? 0.0 : 1.0);
        }
        else
        {
            return sum;
        }
    });

    //average rarity
    return raritySum / peerConnMap.count(infoHash);
}

}




