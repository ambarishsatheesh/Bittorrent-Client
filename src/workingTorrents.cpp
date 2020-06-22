#include "WorkingTorrents.h"
#include "Decoder.h"
#include "TorrentManipulation.h"
#include "loguru.h"


#include <QDateTime>
#include <QtConcurrent>
#include <memory>

namespace Bittorrent
{
    using namespace torrentManipulation;

    WorkingTorrents::WorkingTorrents()
    {
    }

    std::string WorkingTorrents::isDuplicateTorrent(const std::string& fileName,
                                                const std::string& buffer)
    {
        valueDictionary decodedTorrent =
                boost::get<valueDictionary>(Decoder::decode(buffer));
        Torrent loadedTorrent = toTorrentObj(
                    fileName.c_str(), decodedTorrent);

        //check if torrent already exists in list
        for (auto torrent : torrentList)
        {
            if (loadedTorrent.hashesData.hexStringInfoHash ==
                    torrent->hashesData.hexStringInfoHash)
            {
                return torrent->generalData.fileName;
            }
        }

        return "";
    }


    void WorkingTorrents::addNewTorrent(const std::string& fileName,
                                          const std::string& buffer)
    {
        valueDictionary decodedTorrent =
                boost::get<valueDictionary>(Decoder::decode(buffer));
        Torrent loadedTorrent = toTorrentObj(
                    fileName.c_str(), decodedTorrent);

        torrentList.push_back(std::make_shared<Torrent>(loadedTorrent));

        //get current time as appropriately formatted string
        addedOnList.push_back(
                    QDateTime::currentDateTime().
                    toString("yyyy/MM/dd HH:mm"));

        //store map of unique trackers and how many torrents use them
        for (auto trackers : loadedTorrent.generalData.trackerList)
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
            trackerTorrentMap[QString::fromStdString(mainHost)].insert(
                        QString::fromStdString(
                            loadedTorrent.hashesData.hexStringInfoHash));
        }

        LOG_F(INFO, "Added Torrent %s to client!",
              loadedTorrent.generalData.fileName.c_str());
    }

    void WorkingTorrents::removeTorrent(int position)
    {
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

        //remove torrent info from list
        torrentList.erase(torrentList.begin() + position);
        addedOnList.erase(addedOnList.begin() + position);
    }

    void WorkingTorrents::start(int position)
    {
        if (torrentList.at(position)->statusData.currentState !=
                TorrentStatus::currentStatus::started)
        {
            torrentList.at(position)->statusData.currentState =
                    TorrentStatus::currentStatus::started;

            runningTorrents.push_back(torrentList.at(position));

            std::vector<std::thread> threadVector;

            LOG_F(INFO, "PTR ADDRESS: %p", std::addressof(runningTorrents.back()->generalData.trackerList));

            //run initial tracker updates in separate threads
            //if tracker list has more than 5 torrents,
            //process 5 trackers at a time
            if (runningTorrents.back()->generalData.trackerList.size() > 5)
            {
                int count = 0;
                for (auto& tracker :
                     runningTorrents.back()->generalData.trackerList)
                {
                    threadVector.emplace_back([&]()
                    {
                        tracker.update(
                        runningTorrents.back().get()->statusData.currentState,
                        clientID,
                        0,
                        runningTorrents.back().get()->
                        hashesData.urlEncodedInfoHash,
                        runningTorrents.back().get()->
                        hashesData.infoHash,
                        runningTorrents.back().get()->statusData.uploaded(),
                        runningTorrents.back().get()->statusData.downloaded(),
                        runningTorrents.back().get()->statusData.remaining());

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
                    threadVector.emplace_back([&]()
                    {
                        tracker.update(
                        runningTorrents.back().get()->statusData.currentState,
                        clientID,
                        0,
                        runningTorrents.back().get()->
                        hashesData.urlEncodedInfoHash,
                        runningTorrents.back().get()->
                        hashesData.infoHash,
                        runningTorrents.back().get()->statusData.uploaded(),
                        runningTorrents.back().get()->statusData.downloaded(),
                        runningTorrents.back().get()->statusData.remaining());

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

            for (auto& tracker : runningTorrents.back()->generalData.trackerList)
            {
                trackerUpdateSet.emplace(std::make_pair(
                                             &tracker,
                                             runningTorrents.back().get()));
            }

            LOG_F(INFO, "Processed trackers for torrent %s!",
                  torrentList.at(position)->generalData.fileName.c_str());
        }
    }

    void WorkingTorrents::stop(int position)
    {
        torrentList.at(position)->statusData.currentState =
                TorrentStatus::currentStatus::stopped;

    }

    void WorkingTorrents::run()
    {

    }




}


