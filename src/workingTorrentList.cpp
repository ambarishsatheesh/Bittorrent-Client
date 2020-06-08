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
        valueDictionary decodedTorrent =
                boost::get<valueDictionary>(Decoder::decode(buffer));
        Torrent loadedTorrent = toTorrentObj(
                    fileName.c_str(), decodedTorrent);

        auto loadedTorrent_ptr = std::make_shared<Torrent>(loadedTorrent);
        torrentList.push_back(loadedTorrent_ptr);

        //get current time as appropriately formatted string

        addedOnList.push_back(
                    QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm"));

        LOG_F(INFO, "Added Torrent %s to client!", loadedTorrent.generalData.fileName.c_str());
    }

    void workingTorrentList::removeTorrent(int rowNum)
    {
        torrentList.erase(torrentList.begin() + (rowNum-1));
        addedOnList.erase(addedOnList.begin() + (rowNum-1));
    }

}


