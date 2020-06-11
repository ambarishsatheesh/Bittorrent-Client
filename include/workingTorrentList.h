#ifndef WORKINGTORRENTLIST_H
#define WORKINGTORRENTLIST_H

#endif // WORKINGTORRENTLIST_H

#include "Torrent.h"

#include <vector>
#include <QString>
#include <QMap>

namespace Bittorrent
{
    class workingTorrentList
    {
    public:
        //time the torrent was added (string format)
        std::vector<QString> addedOnList;

        std::vector<std::shared_ptr<Torrent>> torrentList;

        //unique trackers
        QMap<QString, int> infoTrackerMap;

        void addNewTorrent(std::string fileName, std::string buffer);
        void removeTorrent(int position);

        workingTorrentList();

    };
}
