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
    : defaultSettings{80, 6689, 6682, 8388600, 1024000, 40, 40},
      trackerTimer{std::make_unique<TrackerTimer>(
                       clientID, defaultSettings.httpPort,
                       defaultSettings.udpPort, defaultSettings.tcpPort)},
      downloadThrottle{defaultSettings.maxDLSpeed, std::chrono::seconds{1}},
      uploadThrottle{defaultSettings.maxULSpeed, std::chrono::seconds{1}},
      dataReceivedTime{std::chrono::high_resolution_clock::time_point::min()},
      dataPreviousTotal{0},
      rand{}, rng{rand()}, peerTimeout{std::chrono::seconds{30}},
      threadPoolSize{15},
      work{boost::asio::make_work_guard(io_context)}, acceptor_{io_context},
      t_processDL{}, t_processUL{}, t_processPeers{}, t_checkComplete{},
      masterProcessCondition{true}, isProcessing{false}
{
    //Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    acceptor_.open(tcp::v4());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(tcp::endpoint(tcp::v4(), defaultSettings.tcpPort));
    acceptor_.listen();

    // Create a pool of threads to share io_context and run
    //executor_work_guard will keep it running when tasks are not queued
      for (std::size_t i = 0; i < threadPoolSize; ++i)
      {
          auto thread = std::make_shared<std::thread>(
              boost::bind(&boost::asio::io_context::run, &io_context));

          threadPool.push_back(thread);
      }

      //initiate processing threads
      initProcessing();
}

WorkingTorrents::~WorkingTorrents()
{
    // Wait for all threads in the pool to exit.
    for (std::size_t i = 0; i < threadPool.size(); ++i)
    {
        if (threadPool.at(i)->joinable())
        {
            threadPool.at(i)->join();
        }
    }

    masterProcessCondition = false;
    isProcessing = false;
    cv.notify_all();

    if (t_processPeers.joinable())
    {
        t_processPeers.join();
    }

    if (t_processDL.joinable())
    {
        t_processDL.join();
    }

    if (t_processUL.joinable())
    {
        t_processUL.join();
    }
}

void WorkingTorrents::initProcessing()
{
    t_processPeers = std::thread([&]()
            {
                loguru::set_thread_name("Process Peers");
                LOG_F(INFO, "Initiated Peers-Processing Thread.");

                while (masterProcessCondition)
                {
                    std::unique_lock<std::mutex> lck(mtx_condition);
                    cv.wait(lck, [&] {return isProcessing; });

                    std::unique_lock<std::mutex> torListGuard(mtx_torrentList);

                    for (auto torrent : torrentList)
                    {
                        processPeers(torrent.get());
                    }

                    torListGuard.unlock();
                }
            });

    t_processDL = std::thread([&]()
        {
            loguru::set_thread_name("Process Downloads 1");
            LOG_F(INFO, "Initiated Downloads-Processing Thread.");

            while (masterProcessCondition)
            {
                std::unique_lock<std::mutex> lck(mtx_condition);
                cv.wait(lck, [&] {return isProcessing; });

                processDownloads();
            }
        });

    t_processUL = std::thread([&]()
        {
            loguru::set_thread_name("Process Uploads");
            LOG_F(INFO, "Initiated Uploads-Processing Thread.");

            while (masterProcessCondition)
            {
                std::unique_lock<std::mutex> lck(mtx_condition);
                cv.wait(lck, [&] {return isProcessing; });

                processUploads();
                //std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
}

std::string WorkingTorrents::isDuplicateTorrent(Torrent* modifiedTorrent)
{
    std::lock_guard<std::mutex> torListGuard(mtx_torrentList);

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
    std::unique_lock<std::mutex> torListGuard(mtx_torrentList);

    torrentList.push_back(std::make_shared<Torrent>(*modifiedTorrent));

    //connect signals and slots
    torrentList.back()->sig_addPeer->connect(
                boost::bind(&WorkingTorrents::addPeer,
                            this, _1, _2));

    torrentList.back()->sig_pieceVerified->connect(
                boost::bind(&WorkingTorrents::handlePieceVerified,
                            this, _1, _2));

    torListGuard.unlock();

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

    //verify torrent in case its files already exist (partly or fully)
    //and mark as complete if all pieces are verified
    modifiedTorrent->statusData.isVerifying = true;

    for (size_t i = 0; i < modifiedTorrent->piecesData.pieces.size();
         ++i)
    {
        verify(*modifiedTorrent, i);
    }

    modifiedTorrent->statusData.isVerifying = false;

    if (modifiedTorrent->statusData.isCompleted())
    {
        modifiedTorrent->statusData.currentState =
                    TorrentStatus::currentStatus::completed;
    }
}

void WorkingTorrents::removeTorrent(int position)
{
    std::lock_guard<std::mutex> torListGuard(mtx_torrentList);

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

    //remove infohash associated with removed torrent from the
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
    for (auto it = range.first; it != range.second;)
    {
        it = peerConnMap.erase(it);
    }

    range = seedersMap.equal_range(
                    torrentList.at(position)->hashesData.urlEncodedInfoHash);
    for (auto it = range.first; it != range.second;)
    {
        it = seedersMap.erase(it);
    }

    range = leechersMap.equal_range(
                    torrentList.at(position)->hashesData.urlEncodedInfoHash);
    for (auto it = range.first; it != range.second;)
    {
        it = leechersMap.erase(it);
    }

    //remove torrent info from vectors
    torrentList.erase(torrentList.begin() + position);
    addedOnList.erase(addedOnList.begin() + position);

    LOG_F(INFO, "Removed Torrent '%s' from client!",
          logName.c_str());
}

void WorkingTorrents::start(int position)
{
    std::unique_lock<std::mutex> statusGuard(mtx_status);

    //start seeding if torrent is completed
    if (torrentList.at(position)->statusData.currentState ==
            TorrentStatus::currentStatus::completed)
    {
        statusGuard.unlock();

        startSeeding(torrentList.at(position).get());

        return;
    }
    //if torrent stopped, start it and process accordingly
    if (torrentList.at(position)->statusData.currentState ==
            TorrentStatus::currentStatus::stopped)
    {
        torrentList.at(position)->statusData.currentState =
                TorrentStatus::currentStatus::started;

        statusGuard.unlock();

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
                    torrentList.at(position)->statusData.currentState,
                    clientID, defaultSettings.httpPort, defaultSettings.udpPort,
                    torrentList.at(position)->hashesData.urlEncodedInfoHash,
                    torrentList.at(position)->hashesData.infoHash,
                    torrentList.at(position)->statusData.uploaded,
                    torrentList.at(position)->statusData.downloaded(),
                    torrentList.at(position)->statusData.remaining());

                    LOG_F(INFO, "Processed tracker %s",
                          tracker.trackerAddress.c_str());
                });

                count++;

                //every 5 threads, join threads and clear thread vector to
                //limit number of concurrent threads
                if (count == 4 ||
                        tracker.trackerAddress == torrentList.at(position)->
                        generalData.trackerList.back().trackerAddress)
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
                 torrentList.at(position)->generalData.trackerList)
            {
                if (std::chrono::high_resolution_clock::now() <
                        tracker.lastPeerRequest + std::chrono::minutes{5})
                {
                    continue;
                }

                threadVector.emplace_back([&]()
                {
                    tracker.update(
                    torrentList.at(position)->statusData.currentState,
                    clientID, defaultSettings.httpPort, defaultSettings.udpPort,
                    torrentList.at(position)->hashesData.urlEncodedInfoHash,
                    torrentList.at(position)->hashesData.infoHash,
                    torrentList.at(position)->statusData.uploaded,
                    torrentList.at(position)->statusData.downloaded(),
                    torrentList.at(position)->statusData.remaining());

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
            //torrentList.at(position)->generalData.getPeerList();
            torrentList.at(position)->handlePeerListUpdated();
        }
        else
        {
            auto infoHash = torrentList.at(position)->
                    hashesData.urlEncodedInfoHash;

            std::unique_lock<std::mutex> mapGuard(mtx_map);

            //check if peers already in map (e.g. due to tracker update signal)
            if (peerConnMap.find(infoHash) != peerConnMap.end())
            {
                auto range = peerConnMap.equal_range(infoHash);

                //push map values into vector so they can be searched
                std::vector<std::shared_ptr<Peer>> mapPeerRange;

                for (auto it = range.first; it != range.second; ++it)
                {
                    mapPeerRange.push_back(it->second);
                }

                mapGuard.unlock();

                for (auto singlePeer :
                     torrentList.at(position)->generalData.uniquePeerList)
                {
                    //create new Peer object if not in peerConnMap
                    //else resume connection
                    auto itr_find =
                            std::find_if(mapPeerRange.begin(), mapPeerRange.end(),
                                         [&singlePeer]
                                         (const std::shared_ptr<Peer>& p)
                    {
                        return p->peerHost == singlePeer.ipAddress;
                    });

                    if (itr_find == mapPeerRange.end())
                    {
                        addPeer(singlePeer,
                                torrentList.at(position));
                    }
                    else
                    {
                        resumePeer(*itr_find);
                    }
                }
            }
        }
    }

    startProcessing();
}

void WorkingTorrents::stop(int position)
{
    std::lock_guard<std::mutex> torListGuard(mtx_torrentList);

    disableTorrentConnection(torrentList.at(position).get());

    pauseProcessing();
}

//only allow seeding of one torrent at a time
void WorkingTorrents::startSeeding(Torrent* torrent)
{
    torrent->statusData.isSeeding = true;
    acceptNewConnection(torrent);
}

//peers for downloading
void WorkingTorrents::addPeer(peer singlePeer, std::shared_ptr<Torrent> torrent)
{
    auto peerConn = std::make_shared<Peer>(torrent, clientID, io_context,
                                       defaultSettings.tcpPort);

    peerConn->peerHost = singlePeer.ipAddress;
    peerConn->peerPort = singlePeer.port;

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
    std::unique_lock<std::mutex> mapGuard(mtx_map);

    //add to class member map so it can be accessed outside thread
    peerConnMap.emplace(torrent->hashesData.urlEncodedInfoHash,
                        peerConn);

    mapGuard.unlock();

    auto presolver = std::make_shared<tcp::resolver>(io_context);

    //resolve peer and start connect
    presolver->async_resolve(
                tcp::resolver::query(singlePeer.ipAddress, singlePeer.port),
                [presolver, &peerConn]
                (const boost::system::error_code& ec,
                const tcp::resolver::results_type results)
                {
                    peerConn->connectToPeer(ec, results);
                });


    LOG_F(INFO, "Successfully added new peer!");
}

void WorkingTorrents::resumePeer(std::shared_ptr<Peer> peer)
{
    LOG_F(INFO, "(%s:%s) Attempted to resume peer connection...",
          peer->peerHost.c_str(), peer->peerPort.c_str());

    peer->setSocketOptions(defaultSettings.tcpPort);

    //reset transfer checks
    peer->isHandshakeReceived = false;
    peer->isHandshakeSent = false;
    peer->isChokeSent = true;
    peer->isChokeReceived = true;
    peer->isInterestedSent = false;
    peer->isInterestedReceived = false;
    peer->isBitfieldSent = false;
    peer->isDisconnected = false;

    //re-connect signals to slots
    peer->sig_blockRequested->connect(
                boost::bind(
                    &WorkingTorrents::handleBlockRequested,
                    this, _1));

    peer->sig_blockCancelled->connect(
                boost::bind(
                    &WorkingTorrents::handleBlockCancelled,
                    this, _1));

    peer->sig_blockReceived->connect(
                boost::bind(
                    &WorkingTorrents::handleBlockReceived,
                    this, _1));

    peer->sig_disconnected->connect(
                boost::bind(
                    &WorkingTorrents::handlePeerDisconnected,
                    this, _1));

    peer->sig_stateChanged->connect(
                boost::bind(
                    &WorkingTorrents::handlePeerStateChanged,
                    this, _1));

    auto presolver = std::make_shared<tcp::resolver>(io_context);

    //resolve peer and restart connection
    peer->restartResolve(presolver);
}

void WorkingTorrents::acceptNewConnection(Torrent* torrent)
{
    auto peerConn = std::make_shared<Peer>(&torrentList, clientID, io_context,
                                       defaultSettings.tcpPort);

    if (acceptor_.is_open())
    {
        acceptor_.async_accept(peerConn->socket(),
              boost::bind(&WorkingTorrents::handle_accept, this,
                boost::asio::placeholders::error, peerConn, torrent));
    }
}

void WorkingTorrents::handle_accept(const boost::system::error_code& ec,
                                    std::shared_ptr<Peer> peerConn, Torrent* torrent)
{
  if (!ec)
  {
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
      std::unique_lock<std::mutex> mapGuard(mtx_map);

      //add to class member map so it can be accessed outside thread
      peerConnMap.emplace(
                  torrent->hashesData.urlEncodedInfoHash,
                          peerConn);

      mapGuard.unlock();

      //start reading
      peerConn->readFromAcceptedPeer();

      LOG_F(INFO, "Successfully accepted new connection from peer!");
  }

  acceptNewConnection(torrent);
}

//disconnect all peers with info hash equal to
//argument torrent's infohash
void WorkingTorrents::disableTorrentConnection(Torrent* torrent)
{
    std::unique_lock<std::mutex> statusGuard(mtx_status);

    if (torrent->statusData.currentState ==
            TorrentStatus::currentStatus::started)
    {
        torrent->statusData.currentState =
                TorrentStatus::currentStatus::stopped;

        statusGuard.unlock();

        //change seeding status
        if (torrent->statusData.isSeeding)
        {
            torrent->statusData.isSeeding = false;
        }

        std::lock_guard<std::mutex> mapGuard(mtx_map);

        auto range = peerConnMap.equal_range(
                    torrent->hashesData.urlEncodedInfoHash);

        for (auto it = range.first; it != range.second; ++it)
        {
            it->second->disconnect();
        }
    }
}

void WorkingTorrents::handlePieceVerified(Torrent* torrent, int piece)
{
//    LOG_F(INFO, "Handling verified piece...");

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
    LOG_F(INFO, "Handling requested block...");

    std::lock_guard<std::mutex> outgoingGuard(mtx_outgoing);

    outgoingBlocks.push_back(request);
}

void WorkingTorrents::handleBlockCancelled(Peer::dataRequest request)
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

void WorkingTorrents::handleBlockReceived(Peer::dataPackage package)
{
    //update interval data variable
    package.sourcePeer->torrent->statusData.dataIntervalTotal +=
            package.data.size();

    //crude download speed calc
    calcDownloadSpeed(package);

    std::lock_guard<std::mutex> incomingGuard(mtx_incoming);

    //add to deque for processing
    incomingBlocks.push_back(package);

    //reset block requested
    package.sourcePeer->isBlockRequested.at(package.piece).at(package.block)
            = false;

    std::unique_lock<std::mutex> mapGuard(mtx_map);
    auto range = peerConnMap.equal_range(
                package.sourcePeer->torrent->
                hashesData.urlEncodedInfoHash);
    mapGuard.unlock();

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
}

void WorkingTorrents::handlePeerDisconnected(Peer* senderPeer)
{
    //manually disconnect slots from signals just in case
    senderPeer->sig_blockRequested->disconnect_all_slots();
    senderPeer->sig_blockCancelled->disconnect_all_slots();
    senderPeer->sig_blockReceived->disconnect_all_slots();
    senderPeer->sig_disconnected->disconnect_all_slots();
    senderPeer->sig_stateChanged->disconnect_all_slots();

    std::lock_guard<std::mutex> mapGuard(mtx_map);

    //store info so it can be used after peer has been deleted
    auto tempHost = senderPeer->peerHost;
    auto tempPort = senderPeer->peerPort;
    auto infoHash = senderPeer->torrent->hashesData.urlEncodedInfoHash;

    //remove from seeders/leechers maps
    //(don't remove from peerConnMap so object is kept alive)
    auto range = seedersMap.equal_range(
                    senderPeer->torrent->hashesData.urlEncodedInfoHash);

    for (auto it = range.first; it != range.second;)
    {
        if (it->second->peerHost == senderPeer->peerHost &&
                it->second->peerPort == senderPeer->peerPort)
        {
            it = seedersMap.erase(it);
        }
        else
        {
            ++it;
        }
    }

    range = leechersMap.equal_range(
                    senderPeer->torrent->hashesData.urlEncodedInfoHash);

    for (auto it = range.first; it != range.second;)
    {
        if (it->second->peerHost == senderPeer->peerHost &&
                it->second->peerPort == senderPeer->peerPort)
        {
            it = leechersMap.erase(it);
        }
        else
        {
            ++it;
        }
    }

    LOG_F(INFO, "(%s:%s) Successfully disconnected from peer.",
          tempHost.c_str(), tempPort.c_str());
}

void WorkingTorrents::handlePeerStateChanged(Peer* senderPeer)
{
    processPeers(senderPeer->torrent.get());
}

void WorkingTorrents::processPeers(Torrent* torrent)
{
//    LOG_F(INFO, "Processing peers for torrent %s.",
//          torrent->generalData.fileName.c_str());

    //locking mutex because this method can be run on multiple threads
    std::lock_guard<std::mutex> processGuard(mtx_process);

    auto infoHash = torrent->hashesData.urlEncodedInfoHash;

    //move peers of specific torrent from map to vector
    //and sort by (descending) pieces required available
    auto sortedPeers = sortPeers(torrent);

    //processing
    for (auto& peer : sortedPeers)
    {
        if (peer->isDisconnected)
        {
            //try reconnecting after some time has passed
            if (peer->disconnectTime + std::chrono::seconds{30} <
                    std::chrono::high_resolution_clock::now())
            {
                resumePeer(peer);
            }

            continue;
        }

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
        if (!peer->isHandshakeSent || !peer->isHandshakeReceived)
        {
            continue;
        }

        //send interested message if we want to request data
        if (torrent->statusData.isCompleted())
        {
            peer->sendNotInterested();
        }
        else
        {
            peer->sendInterested();
        }

        //torrent completed so disconnect peer,
        //update torrent status and start seeding
        if (peer->isCompleted() && torrent->statusData.isCompleted())
        {
            peer->disconnect();

            std::unique_lock<std::mutex> statusGuard(mtx_status);


            torrent->statusData.currentState =
                    TorrentStatus::currentStatus::completed;

            statusGuard.unlock();

            startSeeding(torrent);

            continue;
        }

        //periodic message to keep peer connection alive
        //method has a check to prevent spamming
        peer->sendKeepAlive();

        std::lock_guard<std::mutex> seedersGuard(mtx_seeders);

        //allow leeching
        auto infoHash = torrent->hashesData.urlEncodedInfoHash;
        if (torrent->statusData.isStarted() &&
                leechersMap.count(infoHash) < defaultSettings.maxSeeders)
        {
            if (peer->isInterestedReceived && peer->isChokeSent)
            {
                peer->sendUnchoke();
                leechersMap.emplace(torrent->hashesData.urlEncodedInfoHash,
                                   peer);

                LOG_F(INFO, "Added peer %s:%s to leechers.",
                      peer->peerHost.c_str(), peer->peerPort.c_str());
            }
        }

        //if peer is unchoked, add them to seeders
        if (!torrent->statusData.isCompleted() &&
                seedersMap.count(infoHash) < defaultSettings.maxSeeders)
        {
            if (!peer->isChokeReceived)
            {
                seedersMap.emplace(torrent->hashesData.urlEncodedInfoHash,
                                   peer);

                LOG_F(INFO, "Added peer %s:%s to seeders.",
                      peer->peerHost.c_str(), peer->peerPort.c_str());
            }
        }
    }

//    LOG_F(INFO, "Completed processing peers for torrent %s.",
//          torrent->generalData.fileName.c_str());
}

void WorkingTorrents::processUploads()
{
//    LOG_F(INFO, "Processing uploads...");
    Peer::dataRequest tempBlock;

    std::lock_guard<std::mutex> outgoingGuard(mtx_outgoing);

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
    //order torrents by descending ranking
    auto rankedTorrents = getRankedTorrents();

    //process each torrent
    for (auto torrent : rankedTorrents)
    {
        //skip the rest if torrent downloading is done
        if (torrent->statusData.isCompleted())
        {
            continue;
        }

        //rank pieces to be requested using piece progress and rarity
        std::vector<int> rankedPieces =
                getRankedPieces(torrent);

        for (auto piece : rankedPieces)
        {
            //skip if already have piece
            if (torrent->statusData.isPieceVerified.at(piece))
            {
//                LOG_F(INFO, "Already have piece %d.", piece);

                continue;
            }

            auto rankedSeeders = getRankedSeeders(torrent);

            for (auto seeder : rankedSeeders)
            {
                //skip if seeder doesn't have the piece
                if (!seeder->isPieceDownloaded.at(piece))
                {
//                    LOG_F(INFO, "Seeder doesn't have piece %d.", piece);

                    continue;
                }

                for (int block = 0;
                     block < torrent->piecesData.getBlockCount(piece);
                     ++block)
                {
                    //skip if already acquired
                    if (torrent->
                            statusData.isBlockAcquired.at(piece).at(block))
                    {
//                        LOG_F(INFO, "Already aquired block %d", block);

                        continue;
                    }

                    //skip if throttled
                    if (downloadThrottle.isThrottled())
                    {
                        continue;
                    }

                    //send data request to peer
                    int blockSize = torrent->piecesData.getBlockSize(
                                piece, block);

//                    LOG_F(INFO, "Requesting piece %d, block %d...", piece,
//                          block);

                    seeder->sendDataRequest(
                                piece,
                                block * torrent->piecesData.blockSize,
                                blockSize);

                    //add expected data size to throttle bucket
                    //(i.e. throttle requests)
                    downloadThrottle.add(blockSize);

                    //update info
                    seeder->isBlockRequested.at(piece).at(block) = true;
                }
            }
        }
    }

//    LOG_F(INFO, "Finished processing download batch.");
}

//sort torrents in order of descending ranking
std::vector<Torrent*> WorkingTorrents::getRankedTorrents()
{
    std::vector<Torrent*> rankedTorrents;

    std::unique_lock<std::mutex> torListGuard(mtx_torrentList);

    std::unique_lock<std::mutex> statusGuard(mtx_status);

    for (auto torrent : torrentList)
    {
        if (torrent->statusData.currentState ==
                TorrentStatus::currentStatus::started)
        {
            rankedTorrents.push_back(torrent.get());
        }
    }

    statusGuard.unlock();

    torListGuard.unlock();

    //sort torrents by ranking
    std::sort(rankedTorrents.begin(), rankedTorrents.end(),
              [&](const Torrent* r1,
              const Torrent* r2)
    {
        return r1->clientRank > r2->clientRank;
    });

    return rankedTorrents;
}

std::vector<Peer*> WorkingTorrents::getRankedSeeders(Torrent* torrent)
{
    //shuffle seeders
    std::vector<Peer*> rankedSeeders;
    rankedSeeders.reserve(seedersMap.size());

    std::lock_guard<std::mutex> seedersGuard(mtx_seeders);

    for (auto peer : seedersMap)
    {
        if (peer.second->torrent->hashesData.urlEncodedInfoHash ==
                torrent->hashesData.urlEncodedInfoHash)
        {
            rankedSeeders.push_back(peer.second.get());
        }
    }

    std::shuffle(rankedSeeders.begin(), rankedSeeders.end(), rng);

    return rankedSeeders;
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

    for (size_t i = 0; i < indexScores.size(); ++i)
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
    std::lock_guard<std::mutex> mapGuard(mtx_map);

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

void WorkingTorrents::startProcessing()
{
    using status = TorrentStatus::currentStatus;

    //if at least one torrent is active, allow processing threads to run
    //(if not already running)
    if (!isProcessing)
    {
        //no need to lock mtx_torrentList since calling function already locks
        std::unique_lock<std::mutex> statusGuard(mtx_status);

        auto findActive = std::find_if(torrentList.begin(), torrentList.end(),
                     [&](const std::shared_ptr<Torrent>& t)
        {
            if (t->statusData.currentState == status::started ||
                    (t->statusData.currentState == status::completed &&
                     t->statusData.isSeeding))
            {
                return true;
            }

        });

        statusGuard.unlock();

        if (findActive != torrentList.end())
        {
            isProcessing = true;
            cv.notify_all();
        }
    }
}

void WorkingTorrents::pauseProcessing()
{
    using status = TorrentStatus::currentStatus;

    //if no torrents are active, stop processing threads
    //(if already running)
    if (isProcessing)
    {
        //no need to lock mtx_torrentList since calling function already locks
        std::unique_lock<std::mutex> statusGuard(mtx_status);

        auto findActive = std::find_if(torrentList.begin(), torrentList.end(),
                     [&](const std::shared_ptr<Torrent>& t)
        {
            if (t->statusData.currentState == status::started ||
                    (t->statusData.currentState == status::completed &&
                     t->statusData.isSeeding))
            {
                return true;
            }
        });

        statusGuard.unlock();

        if (findActive == torrentList.end())
        {
            isProcessing = true;
            cv.notify_all();
        }
    }
}

peerMap::iterator WorkingTorrents::searchValPeerMap(peerMap* map,
                                                     std::string host)
{
    auto find_itr = std::find_if(map->begin(), map->end(), [&]
                 (const std::unordered_multimap<std::string,
                 std::shared_ptr<Peer>>::value_type& s)
    {
        return s.second->peerHost == host;
    });

    return find_itr;
}

std::vector<std::shared_ptr<Peer>> WorkingTorrents::sortPeers(Torrent* torrent)
{
    std::vector<std::shared_ptr<Peer>> sortedPeers;

    auto infoHash = torrent->hashesData.urlEncodedInfoHash;

    for (auto it = peerConnMap.begin(); it != peerConnMap.end(); ++it)
    {
        if (it->second->torrent->hashesData.urlEncodedInfoHash == infoHash)
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

    return sortedPeers;

}

void WorkingTorrents::calcDownloadSpeed(const Peer::dataPackage& package)
{
    if (std::chrono::high_resolution_clock::now() >=
            package.sourcePeer->torrent->statusData.lastReceivedTime +
            std::chrono::seconds{1})
    {
        auto timeDelta = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::high_resolution_clock::now() -
                    package.sourcePeer->torrent->statusData.lastReceivedTime);

        package.sourcePeer->torrent->statusData.downloadSpeed =
        static_cast<int>(package.sourcePeer->torrent->
                         statusData.dataIntervalTotal /
                static_cast<int>(timeDelta.count()));

        //update tracking variables
        package.sourcePeer->torrent->statusData.lastReceivedTime =
                std::chrono::high_resolution_clock::now();
        package.sourcePeer->torrent->statusData.dataIntervalTotal = 0;
    }
}


}
