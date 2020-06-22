#ifndef TIMER_H
#define TIMER_H

#include "Torrent.h"

#include <vector>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <set>

namespace Bittorrent {

using pair = std::pair<trackerObj*, Torrent*>;

//set comparator to order by ascending peer request interval
struct compare
{
    bool operator()(const pair& t1, const pair& t2) const
    {
        if (t1.first->peerRequestInterval == t2.first->peerRequestInterval)
        {
            return t1.second->generalData.fileName < t2.second->generalData.fileName;
        }
        return t1.first->peerRequestInterval < t2.first->peerRequestInterval;
    }
};

using pSet = std::multiset<pair, compare>;


class TrackerTimer
{
public:
    TrackerTimer(std::vector<byte> clientID);
    ~TrackerTimer();

    void start(pSet* trackerSet);
    void stop();
    bool isRunning();

private:
    std::mutex mtx;
    std::condition_variable cv{};
    std::thread wait_thread;
    std::vector<byte> clientID;
    std::atomic_bool stopThread;

    void wait_then_call(pSet* trackerSet);
};

}


#endif // TIMER_H
