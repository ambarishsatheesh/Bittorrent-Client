#ifndef WORKINGTORRENTLIST_H
#define WORKINGTORRENTLIST_H

#endif // WORKINGTORRENTLIST_H

#include "Torrent.h"
#include "timer.h"

#include <vector>
#include <QString>
#include <QMap>

namespace Bittorrent
{

class WorkingTorrents
{
public:
    //time the torrent was added (string format)
    std::vector<QString> addedOnList;

    std::vector<byte> clientID;

    pSet trackerUpdateSet;

    std::unique_ptr<TrackerTimer> trackerTimer;

    std::vector<std::shared_ptr<Torrent>> torrentList;
    std::vector<std::shared_ptr<Torrent>> runningTorrents;

    //unique trackers
    QMap<QString, int> infoTrackerMap;
    QMap<QString, std::set<QString>> trackerTorrentMap;

    std::string isDuplicateTorrent(const std::string& fileName,
                            const std::string& buffer);
    void addNewTorrent(const std::string& fileName, const std::string& buffer);
    void removeTorrent(int position);

    void start(int position);
    void stop(int position);
    void run();

    WorkingTorrents();
};

}
