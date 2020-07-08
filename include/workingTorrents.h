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
using peerMap = std::unordered_multimap<std::string, std::shared_ptr<Peer>>;

class WorkingTorrents
{
public:
    //public so MainWindow class can access
    std::mutex mtx_ranking;

    //network transfer parameters
    struct settings
    {
        int httpPort;
        int udpPort;
        int tcpPort;

        int maxDLSpeed;
        int maxULSpeed;

        int maxSeeders;     //per torrent
        int maxLeechers;    //per torrent

        settings(int httpPort, int udpPort, int tcpPort,
                           int maxDLSpeed, int maxULSpeed,
                           int maxSeeders, int maxLeechers)
            : httpPort{httpPort}, udpPort{udpPort}, tcpPort{tcpPort},
              maxDLSpeed{maxDLSpeed}, maxULSpeed{maxULSpeed},
              maxSeeders{maxSeeders}, maxLeechers{maxLeechers}
        {}
    } defaultSettings;

    //time the torrent was added (string format)
    std::vector<QString> addedOnList;

    std::vector<byte> clientID;

    pSet trackerUpdateSet;

    std::unique_ptr<TrackerTimer> trackerTimer;

    std::vector<std::shared_ptr<Torrent>> torrentList;

    //map of peer connections (torrent infohash as key)
    peerMap peerConnMap;
    peerMap seedersMap;
    peerMap leechersMap;

    //unique trackers
    QMap<QString, int> infoTrackerMap;
    QMap<QString, std::set<QString>> trackerTorrentMap;

    //std::vector<std::shared_ptr<Peer>> sortedPeers;

    //torrent processing
    std::string isDuplicateTorrent(Torrent* modifiedtorrent);
    void addNewTorrent(Torrent* modifiedtorrent);
    void removeTorrent(int position);

    //torrent functionality
    void start(int position);
    void stop(int position);
    void startSeeding(Torrent* torrent);

    //slots
    void addPeer(peer singlePeer, std::shared_ptr<Torrent> torrent);
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
    std::chrono::high_resolution_clock::time_point dataReceivedTime;
    long long dataPreviousTotal;

    //ranking
    std::random_device rand;
    std::default_random_engine rng;
    std::vector<Torrent*> getRankedTorrents();
    std::vector<Peer*> getRankedSeeders(Torrent* torrent);
    std::vector<int> getRankedPieces(Torrent* torrent);
    float getPieceScore(Torrent* torrent, int piece);
    float getPieceProgress(Torrent* torrent, int piece);
    float getPieceRarity(Torrent* torrent, int piece);

    //non slot peer-related methods
    void disableTorrentConnection(Torrent* torrent);
    void acceptNewConnection(Torrent *torrent);

    //public utility
    void verifyTorrent(Torrent* torrent);

    WorkingTorrents();
    ~WorkingTorrents();

private:
    std::condition_variable cv;
    std::mutex mtx_condition;
    std::mutex mtx_torrentList;
    std::mutex mtx_status;
    std::mutex mtx_map;
    std::mutex mtx_seeders;
    std::mutex mtx_process;
    std::mutex mtx_outgoing;
    std::mutex mtx_incoming;
    std::mutex mtx_torList;

    std::chrono::duration<int> peerTimeout;
    std::size_t threadPoolSize;

    //need deque for both FIFO & iteration
    std::deque<Peer::dataRequest> outgoingBlocks;
    std::deque<Peer::dataPackage> incomingBlocks;

    //asio
    boost::asio::io_context io_context;
    //keep io_context running even when there is no work
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
    tcp::acceptor acceptor_;

    void handle_accept(const boost::system::error_code& ec, std::shared_ptr<Peer> peerConn,
                       Torrent* torrent);
    void resumePeer(std::shared_ptr<Peer> peer);

    //threads
    std::vector<std::shared_ptr<std::thread>> threadPool;
    std::thread t_processDL;
    std::thread t_processUL;
    std::thread t_processPeers;
    std::thread t_checkComplete;
    void initProcessing();
    void startProcessing();
    void pauseProcessing();
    bool masterProcessCondition;
    bool isProcessing;

    //utility
    peerMap::iterator searchValPeerMap(peerMap* map, std::string host);
    std::vector<std::shared_ptr<Peer>> sortPeers(Torrent* torrent);
    void calcDownloadSpeed(Torrent* torrent);
    void updateTrackers(Torrent* torrent);
};

}


#endif // WORKINGTORRENTLIST_H
