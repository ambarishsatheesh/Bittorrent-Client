#ifndef WORKINGTORRENTLIST_H
#define WORKINGTORRENTLIST_H

#include "Peer.h"
#include "timer.h"
#include "TorrentManipulation.h"

#include <vector>
#include <QString>
#include <QMap>
#include <unordered_map>
#include <mutex>

namespace Bittorrent
{

class WorkingTorrents
{
public:
    //hardcoded parameters
    static constexpr int maxSeedersPerTorrent = 5;
    static constexpr int maxLeechersPerTorrent = 5;
    static constexpr int maxDownloadBytesPerSecond = 4194300;
    static constexpr int maxUploadBytesPerSecond = 512000;

    std::mutex mtx_map;
    std::mutex mtx_process;

    //time the torrent was added (string format)
    std::vector<QString> addedOnList;

    std::vector<byte> clientID;

    pSet trackerUpdateSet;

    std::unique_ptr<TrackerTimer> trackerTimer;

    std::vector<std::shared_ptr<Torrent>> torrentList;
    std::vector<std::shared_ptr<Torrent>> runningTorrents;

    //map of peer connections (torrent infohash as key)
    std::unordered_multimap<std::string, std::shared_ptr<Peer>> peerConnMap;
    std::unordered_multimap<std::string, std::shared_ptr<Peer>> seedersMap;
    std::unordered_multimap<std::string, std::shared_ptr<Peer>> leechersMap;

    //sorted peers for processing in order of pieces available
    std::vector<std::shared_ptr<Peer>> sortedPeers;

    //unique trackers
    QMap<QString, int> infoTrackerMap;
    QMap<QString, std::set<QString>> trackerTorrentMap;

    //torrent processing
    std::string isDuplicateTorrent(Torrent* modifiedtorrent);
    void addNewTorrent(Torrent* modifiedtorrent);
    void removeTorrent(int position);

    //torrent functionality
    void start(int position);
    void stop(int position);
    void run();
    int seedingCount;
    void startSeeding(int position);

    //slots
    void addPeer(peer* singlePeer, Torrent* torrent);
    void handlePieceVerified(int index);
    void handleBlockRequested(Peer* peer, dataRequest newDataRequest);
    void handleBlockCancelled(Peer* peer, dataRequest newDataRequest);
    void handleBlockReceived(Peer* peer, dataPackage newPackage);
    void handlePeerDisconnected(std::shared_ptr<Peer> senderPeer);
    void handlePeerStateChanged(Peer* peer);

    //processing peers/data
    void processPeers();

    //non slot peer-related methods
    void disableTorrentConnection(Torrent* torrent);
    void acceptNewConnection(Torrent *torrent);

    WorkingTorrents();

private:
    std::chrono::duration<int> peerTimeout;
};

}


#endif // WORKINGTORRENTLIST_H
