#ifndef WORKINGTORRENTLIST_H
#define WORKINGTORRENTLIST_H

#include "Peer.h"
#include "timer.h"
#include "TorrentManipulation.h"
#include "throttle.h"

#include <vector>
#include <QString>
#include <QMap>
#include <unordered_map>
#include <mutex>
#include <deque>
#include <random>

namespace Bittorrent
{

class WorkingTorrents
{
public:
    //network transfer parameters
    int networkPort;
    int maxDownloadBytesPerSecond = 4194300;
    int maxUploadBytesPerSecond = 512000;
    int maxSeedersPerTorrent = 5;
    int maxLeechersPerTorrent = 5;

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
    void startSeeding(int position);

    //slots
    void addPeer(peer* singlePeer, Torrent* torrent);
    void handlePieceVerified(Torrent* torrent, int index);
    void handleBlockRequested(Peer::dataRequest request);
    void handleBlockCancelled(Peer::dataRequest request);
    void handleBlockReceived(Peer::dataPackage package);
    void handlePeerDisconnected(Peer* senderPeer);
    void handlePeerStateChanged(Peer* peer);

    //processing peers/data
    Throttle downloadThrottle;
    Throttle uploadThrottle;
    void processPeers(Torrent* torrent);
    void processUploads();
    void processDownloads();

    //peer ranking
    std::random_device rand;
    std::default_random_engine rng;
    std::vector<Peer*> getRankedSeeders();
    std::vector<int> getRankedPieces(Torrent* torrent);
    float getPieceScore(Torrent* torrent, int piece);
    float getPieceProgress(Torrent* torrent, int piece);
    float getPieceRarity(Torrent* torrent, int piece);

    //non slot peer-related methods
    void disableTorrentConnection(Torrent* torrent);
    void acceptNewConnection(Torrent *torrent);

    WorkingTorrents();

private:
    std::mutex mtx_map;
    std::mutex mtx_seeders;
    std::mutex mtx_process;
    std::mutex mtx_outgoing;
    std::mutex mtx_incoming;

    std::chrono::duration<int> peerTimeout;

    //need deque for both FIFO & iteration
    std::deque<Peer::dataRequest> outgoingBlocks;
    std::deque<Peer::dataPackage> incomingBlocks;
};

}


#endif // WORKINGTORRENTLIST_H
