#include "timer.h"

namespace Bittorrent {


TrackerTimer::TrackerTimer(size_t time, ptr_vec torrentVec)
    : wait_thread{ }, ptr_torrentVec{std::move(torrentVec)}
{
     wait_thread = std::thread{&TrackerTimer::wait_then_call, this};
}

TrackerTimer::~TrackerTimer()
{
    if (wait_thread.joinable())
    {
        wait_thread.join();
    }
}

void TrackerTimer::wait_then_call()
{
    std::chrono::seconds time;
    std::unique_lock<std::mutex> lck{ mtx };
    for (auto tracker : *ptr_torrentVec)
    {
        cv.wait_for(lck, time);
    }
}

}
