#include "workingTorrentList.h"
#include "Decoder.h"
#include "TorrentManipulation.h"
#include "loguru.h"

#include <QDateTime>

namespace Bittorrent
{
    using namespace torrentManipulation;

    workingTorrentList::workingTorrentList()
    {
    }

    void workingTorrentList::addNewTorrent(std::string fileName,
                                           std::string buffer)
    {
        LOG_F(INFO, "buffer: %s", buffer.c_str());
        valueDictionary decodedTorrent =
                boost::get<valueDictionary>(Decoder::decode(buffer));
        Torrent loadedTorrent = toTorrentObj(
                    fileName.c_str(), decodedTorrent);

        auto loadedTorrent_ptr = std::make_shared<Torrent>(loadedTorrent);
        torrentList.push_back(loadedTorrent_ptr);

        //get current time as appropriately formatted string
        addedOnList.push_back(
                    QDateTime::currentDateTime().
                    toString("yyyy/MM/dd HH:mm"));

        //store map of unique trackers and how many torrents use them
        for (auto trackers : loadedTorrent.generalData.trackerList)
        {
            QString fullTrackerAdd =
                    QString::fromStdString(trackers.trackerAddress);

            auto mainHost = fullTrackerAdd.split('.').last().split('/').first();

            //store only main domain and increment if already exists
            infoTrackerMap[mainHost]++;

            LOG_F(INFO, "added host: %s, count: %d",
                  mainHost.toStdString().c_str(),
                  infoTrackerMap[mainHost]);
        }

        LOG_F(INFO, "Added Torrent %s to client!", loadedTorrent.generalData.fileName.c_str());
    }

    void workingTorrentList::removeTorrent(int position)
    {
        torrentList.erase(torrentList.begin() + position);
        addedOnList.erase(addedOnList.begin() + position);
    }

}


