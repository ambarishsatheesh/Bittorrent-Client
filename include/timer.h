#ifndef TIMER_H
#define TIMER_H

#include "Torrent.h"

#include <vector>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace Bittorrent {

using ptr_vec = std::shared_ptr<std::vector<std::shared_ptr<Torrent>>>;

class TrackerTimer
{
public:
    TrackerTimer(size_t time, ptr_vec torrentVec);
    ~TrackerTimer();

private:
    void wait_then_call();
    std::mutex mtx;
    std::condition_variable cv{};
    std::thread wait_thread;
    ptr_vec ptr_torrentVec;
};

}


#endif // TIMER_H
