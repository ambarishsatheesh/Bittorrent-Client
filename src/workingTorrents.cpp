#include "WorkingTorrents.h"
#include "Decoder.h"
#include "loguru.h"


#include <QDateTime>
#include <QtConcurrent>
#include <memory>

namespace Bittorrent
{
using namespace torrentManipulation;

WorkingTorrents::WorkingTorrents()
    : trackerTimer{std::make_unique<TrackerTimer>(clientID)}
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

    //remove torrent info from list
    torrentList.erase(torrentList.begin() + position);
    addedOnList.erase(addedOnList.begin() + position);

    LOG_F(INFO, "Removed Torrent '%s' from client!",
          logName.c_str());
}

void WorkingTorrents::start(int position)
{
    if (torrentList.at(position)->statusData.currentState !=
            TorrentStatus::currentStatus::started)
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



        //begin remaining process
        run();
    }
}

void WorkingTorrents::stop(int position)
{
    if (torrentList.at(position)->statusData.currentState ==
            TorrentStatus::currentStatus::started)
    {
        torrentList.at(position)->statusData.currentState =
                TorrentStatus::currentStatus::stopped;
    }
}

void WorkingTorrents::run()
{

}


}


