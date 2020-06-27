#ifndef WORKINGTORRENTLIST_H
#define WORKINGTORRENTLIST_H

#include "Peer.h"
#include "timer.h"
#include "TorrentManipulation.h"

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

    std::string isDuplicateTorrent(Torrent* modifiedtorrent);
    void addNewTorrent(Torrent* modifiedtorrent);
    void removeTorrent(int position);

    void start(int position);
    void stop(int position);
    void run();

    //slots
    void addPeer(peer* singlePeer, Torrent* torrent);
    void handlePieceVerified(int index);

    WorkingTorrents();
};

}


#endif // WORKINGTORRENTLIST_H
